/**
 * cg-tools -- Cooperative gamma-enabled tools
 * Copyright (C) 2016, 2018  Mattias Andrée (maandree@kth.se)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cg-base.h"

#include <libclut.h>

#include <alloca.h>
#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**
 * The process's name
 */
const char* argv0 = NULL;

/**
 * The libcoopgamma context
 */
libcoopgamma_context_t cg;

/**
 * The names of the selected CRTC:s
 */
char** crtcs = NULL;

/**
 * Gamma ramp updates for each CRTC
 */
filter_update_t* crtc_updates = NULL;

/**
 * CRTC and monitor information about
 * each selected CRTC and connect monitor
 */
libcoopgamma_crtc_info_t* crtc_info = NULL;

/**
 * The number of selected CRTC:s
 */
size_t crtcs_n = 0;

/**
 * The number of filters
 */
size_t filters_n = 0;


/**
 * Contexts for asynchronous ramp updates
 */
static libcoopgamma_async_context_t* asyncs = NULL;

/**
 * The number of pending receives
 */
static size_t pending_recvs = 0;

/**
 * Whether message must be flushed
 */
static int flush_pending = 0;



/**
 * Data used to sort CRTC:s
 */
struct crtc_sort_data
{
  /**
   * The gamma ramp type
   */
  libcoopgamma_depth_t depth;
  
  /**
   * Should be 0
   */
  int __padding;
  
  /**
   * The size of the red gamma ramp
   */
  size_t red_size;
  
  /**
   * The size of the green gamma ramp
   */
  size_t green_size;
  
  /**
   * The size of the blue gamma ramp
   */
  size_t blue_size;
  
  /**
   * The index of the CRTC
   */
  size_t index;
};



/**
 * Compare two strings
 * 
 * @param   a  Return -1 if this string is `NULL` or less than `b`
 * @param   b  Return +1 if this string is less than `a`
 * @return     See `a` and `b`, 0 is returned if `a` and `b` are equal
 */
static int nulstrcmp(const char *a, const char *b)
{
  return (a == NULL) ? -1 : strcmp(a, b);
}


/**
 * Compare two instances of `crtc_sort_data`
 * 
 * @param   a_  Return -1 if this one is lower
 * @param   b_  Return +1 if this one is higher
 * @return      See `a_` and `b_`, only -1 or +1 can be returned
 */
static int crtc_sort_data_cmp(const void* a_, const void* b_)
{
  const struct crtc_sort_data* a = a_;
  const struct crtc_sort_data* b = b_;
  int cmp = memcmp(a, b, sizeof(*a) - sizeof(a->index));
  return cmp ? cmp : a->index < b->index ? -1 : +1;
}


/**
 * Make elements in `crtc_updates` slaves where appropriate
 * 
 * @return  Zero on success, -1 on error
 */
int make_slaves(void)
{
  struct crtc_sort_data* data;
  size_t i, j, n = 0, master = 0, master_i;
  
  data = alloca(filters_n * sizeof(*data));
  memset(data, 0, filters_n * sizeof(*data));
  for (i = 0; i < filters_n; i++)
    {
      if (!(crtc_info[crtc_updates[i].crtc].supported))
	continue;
      
      data[n].depth      = crtc_updates[i].filter.depth;
      data[n].red_size   = crtc_updates[i].filter.ramps.u8.red_size;
      data[n].green_size = crtc_updates[i].filter.ramps.u8.green_size;
      data[n].blue_size  = crtc_updates[i].filter.ramps.u8.blue_size;
      data[n].index      = i;
      n++;
    }
  
  qsort(data, n, sizeof(*data), crtc_sort_data_cmp);
  if (n == 0)
    return 0;
  
  master_i = data[0].index;
  for (i = 1; i < n; i++)
    if (memcmp(data + i, data + master, sizeof(*data) - sizeof(data->index)))
      {
	if (master + 1 < i)
	  {
	    crtc_updates[master_i].slaves = calloc(i - master, sizeof(size_t));
	    if (crtc_updates[master_i].slaves == NULL)
	      return -1;
	    for (j = 1; master + j < i; j++)
	      crtc_updates[master_i].slaves[j - 1] = data[master + j].index;
	  }
	master = i;
	master_i = data[master].index;
      }
    else
      {
	libcoopgamma_ramps_destroy(&(crtc_updates[data[i].index].filter.ramps.u8));
	crtc_updates[data[i].index].master = 0;
	crtc_updates[data[i].index].filter.ramps.u8 = crtc_updates[master_i].filter.ramps.u8;
      }
  
  if (master + 1 < i)
    {
      crtc_updates[master_i].slaves = calloc(i - master, sizeof(size_t));
      if (crtc_updates[master_i].slaves == NULL)
	return -1;
      for (j = 1; master + j < i; j++)
	crtc_updates[master_i].slaves[j - 1] = data[master + j].index;
    }
  
  return 0;
}


/**
 * Update a filter and synchronise calls
 * 
 * @param   index    The index of the CRTC
 * @param   timeout  The number of milliseconds a call to `poll` may block,
 *                   -1 if it may block forever
 * @return           1: Success, no pending synchronisations
 *                   0: Success, with still pending synchronisations
 *                   -1: Error, `errno` set
 *                   -2: Error, `cg.error` set
 * 
 * @throws  EINTR   Call to `poll` was interrupted by a signal
 * @throws  EAGAIN  Call to `poll` timed out
 */
int update_filter(size_t index, int timeout)
{
  filter_update_t* filter = crtc_updates + index;
  
  if (!(filter->synced) || filter->failed)
    abort();
  
  pending_recvs += 1;
  
  if (libcoopgamma_set_gamma_send(&(filter->filter), &cg, asyncs + index) < 0)
    switch (errno)
      {
      case EINTR:
      case EAGAIN:
#if EAGAIN != EWOULDBLOCK
      case EWOULDBLOCK:
#endif
	flush_pending = 1;
	break;
      default:
	return -1;
      }
  
  filter->synced = 0;
  return synchronise(timeout);
}


/**
 * Synchronised calls
 * 
 * @param   timeout  The number of milliseconds a call to `poll` may block,
 *                   -1 if it may block forever
 * @return           1: Success, no pending synchronisations
 *                   0: Success, with still pending synchronisations
 *                   -1: Error, `errno` set
 *                   -2: Error, `cg.error` set
 * 
 * @throws  EINTR   Call to `poll` was interrupted by a signal
 * @throws  EAGAIN  Call to `poll` timed out
 */
int synchronise(int timeout)
{
  struct pollfd pollfd;
  size_t selected;
  
  pollfd.fd = cg.fd;
  pollfd.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
  if (flush_pending > 0)
    pollfd.events |= POLLOUT;
  
  pollfd.revents = 0;
  if (poll(&pollfd, (nfds_t)1, timeout) < 0)
    return -1;
  
  if (pollfd.revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL))
    {
      if (libcoopgamma_flush(&cg) < 0)
	goto sync;
      flush_pending = 0;
    }
  
  if ((timeout < 0) && (pending_recvs > 0))
    if (!(pollfd.revents & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)))
      {
	pollfd.revents = 0;
	if (poll(&pollfd, (nfds_t)1, -1) < 0)
	  return -1;
      }
  
 sync:
  if (pollfd.revents & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLERR | POLLHUP | POLLNVAL))
    for (;;)
      {
	if (libcoopgamma_synchronise(&cg, asyncs, filters_n, &selected) < 0)
	  {
	    if (errno == 0)
	      continue;
	    else
	      goto fail;
	  }
	if (crtc_updates[selected].synced)
	  continue;
	crtc_updates[selected].synced = 1;
	pending_recvs -= 1;
	if (libcoopgamma_set_gamma_recv(&cg, asyncs + selected) < 0)
	  {
	    if (cg.error.server_side)
	      {
		crtc_updates[selected].error = cg.error;
		crtc_updates[selected].failed = 1;
		memset(&(cg.error), 0, sizeof(cg.error));
	      }
	    else
	      goto cg_fail;
	  }
      }
  
  return pending_recvs == 0;
 cg_fail:
  return -2;
 fail:
  switch (errno)
    {
    case EINTR:
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return pending_recvs == 0;
    default:
      return -1;
    }
}


/**
 * Initialise the process, specifically
 * reset the signal mask and signal handlers
 * 
 * @return  Zero on success, -1 on error
 */
static int initialise_proc(void)
{
  sigset_t sigmask;
  int sig;
  
  for (sig = 1; sig < _NSIG; sig++)
    if (signal(sig, SIG_DFL) == SIG_ERR)
      if (sig == SIGCHLD)
	return -1;
  
  if (sigemptyset(&sigmask) < 0)
    return -1;
  if (sigprocmask(SIG_SETMASK, &sigmask, NULL) < 0)
    return -1;
  
  return 0;
}


/**
 * Print, to stdout, a list of all
 * recognised adjustment methods
 * 
 * @return  Zero on success, -1 on error
 */
static int list_methods(void)
{
  char** list;
  size_t i;
  
  list = libcoopgamma_get_methods();
  if (list == NULL)
    return -1;
  for (i = 0; list[i]; i++)
    printf("%s\n", list[i]);
  free(list);
  if (fflush(stdout) < 0)
    return -1;
  
  return 0;
}


/**
 * Print, to stdout, a list of all CRTC:s
 * 
 * A connection to the coopgamma server
 * must have been made
 * 
 * @return  Zero on success, -1 on error, -2
 *          on libcoopgamma error
 */
static int list_crtcs(void)
{
  char** list;
  size_t i;
  
  list = libcoopgamma_get_crtcs_sync(&cg);
  if (list == NULL)
    return -2;
  for (i = 0; list[i]; i++)
    printf("%s\n", list[i]);
  free(list);
  if (fflush(stdout) < 0)
    return -1;
  
  return 0;
}


/**
 * Fill the list of CRTC information
 * 
 * @return  Zero on success, -1 on error, -2
 *          on libcoopgamma error
 */
static int get_crtc_info(void)
{
  size_t i, unsynced = 0, selected;
  char* synced;
  int need_flush = 0;
  struct pollfd pollfd;
  
  synced = alloca(crtcs_n * sizeof(*synced));
  memset(synced, 0, crtcs_n * sizeof(*synced));
  
  i = 0;
  pollfd.fd = cg.fd;
  pollfd.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
  
  while ((unsynced > 0) || (i < crtcs_n))
    {
    wait:
      if (i < crtcs_n)
	pollfd.events |= POLLOUT;
      else
	pollfd.events &= ~POLLOUT;
      
      pollfd.revents = 0;
      if (poll(&pollfd, (nfds_t)1, -1) < 0)
	goto fail;
      
      if (pollfd.revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL))
	{
	  if (need_flush && (libcoopgamma_flush(&cg) < 0))
	    goto send_fail;
	  need_flush = 0;
	  for (; i < crtcs_n; i++)
	    if (unsynced++, libcoopgamma_get_gamma_info_send(crtcs[i], &cg, asyncs + i) < 0)
	      goto send_fail;
	  goto send_done;
	send_fail:
	  switch (errno)
	    {
	    case EINTR:
	    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
	    case EWOULDBLOCK:
#endif
	      i++;
	      need_flush = 1;
	      break;
	    default:
	      goto fail;
	    }
	}
    send_done:
      
      if ((unsynced == 0) && (i == crtcs_n))
	break;
      
      if (pollfd.revents & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI))
	while (unsynced > 0)
	  switch (libcoopgamma_synchronise(&cg, asyncs, i, &selected))
	    {
	    case 0:
	      if (synced[selected])
		{
		  libcoopgamma_skip_message(&cg);
		  break;
		}
	      synced[selected] = 1;
	      unsynced -= 1;
	      if (libcoopgamma_get_gamma_info_recv(crtc_info + selected, &cg, asyncs + selected) < 0)
		goto cg_fail;
	      break;
	    case -1:
	      switch (errno)
		{
		case 0:
		  break;
		case EINTR:
		case EAGAIN:
#if EAGAIN != EWOULDBLOCK
		case EWOULDBLOCK:
#endif
		  goto wait;
		default:
		  goto fail;
		}
	      break;
	    }
    }
  
  return 0;
 fail:
  return -1;
 cg_fail:
  return -2;
}


/**
 * -M METHOD
 *     Select adjustment method. If METHOD is "?",
 *     available methods will be printed to stdout.
 * 
 * -S SITE
 *     Select site (display server instance).
 * 
 * -c CRTC
 *     Select CRT controller. If CRTC is "?", CRTC:s
 *     will be printed to stdout.
 *     
 *     This option can be used multiple times. If it
 *     is not used at all, all CRTC:s will be selected.
 * 
 * -p PRIORITY
 *     Select the priority for the filter, this should
 *     be a signed two's-complement integer. If
 *     PRIORITY is "?", the default priority for the
 *     program is printed to stdout.
 * 
 * -R RULE
 *     The rule of the filter, that is, the last part
 *     of the class which is its identifier. If RULE
 *     is "?" the default rule is printed to stdout,
 *     if RULE is "??" the default class is printed
 *     to stdout.
 * 
 * @param   argc  The number of command line arguments
 * @param   argv  The command line arguments
 * @return        0 on success, 1 on error
 */
int main(int argc, char* argv[])
{
  int stage = 0;
  int dealloc_crtcs = 0;
  int rc = 0;
  char* method = NULL;
  char* site = NULL;
  size_t crtc_i = 0;
  int64_t priority = default_priority;
  char* prio = NULL;
  char* rule = NULL;
  char* class = default_class;
  char** classes = NULL;
  size_t classes_n = 0;
  int explicit_crtcs = 0;
  int have_crtc_q = 0;
  size_t i, filter_i;
  
  argv0 = *argv++, argc--;
  
  if (initialise_proc() < 0)
    goto fail;
  
  crtcs = alloca(argc * sizeof(*crtcs));
  
  for (; *argv; argv++, argc--)
    {
      char* args = *argv;
      char opt[3];
      if (!strcmp(args, "--"))
	{
	  argv++, argc--;
	  break;
	}
      opt[0] = *args++;
      opt[2] = '\0';
      if ((*opt != '-') && (*opt != '+'))
	break;
      while (*args)
	{
	  char* arg;
	  int at_end;
	  opt[1] = *args++;
	  arg = args;
	  if ((at_end = !*arg))
	    arg = argv[1];
	  if (!strcmp(opt, "-M"))
	    {
	      if ((method != NULL) || ((method = arg) == NULL))
		usage();
	    }
	  else if (!strcmp(opt, "-S"))
	    {
	      if ((site != NULL) || ((site = arg) == NULL))
		usage();
	    }
	  else if (!strcmp(opt, "-c"))
	    {
	      if (arg == NULL)
		usage();
	      crtcs[crtc_i++] = arg;
	      explicit_crtcs = 1;
	      if (!have_crtc_q && !strcmp(arg, "?"))
		have_crtc_q = 1;
	    }
	  else if (!strcmp(opt, "-p"))
	    {
	      if ((prio != NULL) || ((prio = arg) == NULL))
		usage();
	    }
	  else if (!strcmp(opt, "-R"))
	    {
	      if ((rule != NULL) || ((rule = arg) == NULL))
		usage();
	    }
	  else
	    switch (handle_opt(opt, arg))
	      {
	      case 0:
		goto next_opt;
	      case 1:
		break;
	      default:
		goto fail;
	      }
	  argv += at_end;
	  argc -= at_end;
	  break;
	next_opt:;
	}
    }
  
  crtcs_n = crtc_i;
  crtcs[crtc_i] = NULL;
  if (!have_crtc_q && nulstrcmp(method, "?") &&
      nulstrcmp(rule, "?") && nulstrcmp(rule, "??") &&
      ((default_priority == NO_DEFAULT_PRIORITY) || nulstrcmp(prio, "?")))
    if (handle_args(argc, argv, prio) < 0)
      goto fail;
  
  if (default_priority != NO_DEFAULT_PRIORITY)
    {
      if (!nulstrcmp(prio, "?"))
	{
	  printf("%" PRIi64 "\n", priority);
	  return 0;
	}
      else if (prio != NULL)
	{
	  char *end;
	  errno = 0;
	  priority = (int64_t)strtoll(prio, &end, 10);
	  if (errno || *end || !*prio)
	    usage();
	}
    }
  
  if (!nulstrcmp(rule, "??"))
    {
      size_t i;
      if (*class_suffixes == NULL)
	printf("%s\n", class);
      else
	for (i = 0; class_suffixes[i] != NULL; i++)
	  printf("%s%s\n", class, class_suffixes[i]);
      return 0;
    }
  else if (!nulstrcmp(rule, "?"))
    {
      printf("%s\n", strstr(strstr(class, "::") + 2, "::") + 2);
      return 0;
    }
  else if (rule != NULL)
    {
      char* p = strstr(strstr(class, "::") + 2, "::") + 2;
      size_t n = (size_t)(p - class);
      class = alloca(strlen(rule) + n + (size_t)1);
      memcpy(class, default_class, n);
      strcpy(class + n, rule);
      if (strchr(class, '\n'))
	{
	  fprintf(stderr, "%s: LF character is not allowed in the filter's class\n", argv0);
	  goto custom_fail;
	}
    }
  
  if (!nulstrcmp(method, "?"))
    {
      if (list_methods() < 0)
	goto fail;
      return 0;
    }
  
  if (libcoopgamma_context_initialise(&cg) < 0)
    goto fail;
  stage++;
  if (libcoopgamma_connect(method, site, &cg) < 0)
    {
      fprintf(stderr, "%s: server failed to initialise\n", argv0);
      goto custom_fail;
    }
  stage++;
  
  if (have_crtc_q)
    switch (list_crtcs())
      {
      case 0:
	goto done;
      case -1:
	goto fail;
      default:
	goto cg_fail;
      }
  
  if (crtcs_n == 0)
    {
      crtcs = libcoopgamma_get_crtcs_sync(&cg);
      if (crtcs == NULL)
	goto cg_fail;
      dealloc_crtcs = 1;
      for (; crtcs[crtcs_n] != NULL; crtcs_n++);
    }
  
  if (crtcs_n == 0)
    {
      fprintf(stderr, "%s: no CRTC:s are available\n", argv0);
      goto custom_fail;
    }
  
  if (*class_suffixes == NULL)
    {
      classes = &class;
      classes_n = 1;
    }
  else
    {
      size_t len = strlen(class);
      while (class_suffixes[classes_n])
	classes_n++;
      classes = alloca(classes_n * sizeof(*classes));
      for (i = 0; i < classes_n; i++)
	{
	  classes[i] = alloca(len + strlen(class_suffixes[i]) + sizeof(":"));
	  stpcpy(stpcpy(stpcpy(classes[i], class), ":"), class_suffixes[i]);
	}
    }
  filters_n = classes_n * crtcs_n;
  
  crtc_info = alloca(crtcs_n * sizeof(*crtc_info));
  memset(crtc_info, 0, crtcs_n * sizeof(*crtc_info));
  for (crtc_i = 0; crtc_i < crtcs_n; crtc_i++)
    if (libcoopgamma_crtc_info_initialise(crtc_info + crtc_i) < 0)
      goto cg_fail;
  
  if (libcoopgamma_set_nonblocking(&cg, 1) < 0)
    goto fail;
  
  asyncs = alloca(filters_n * sizeof(*asyncs));
  memset(asyncs, 0, filters_n * sizeof(*asyncs));
  for (filter_i = 0; filter_i < filters_n; filter_i++)
    if (libcoopgamma_async_context_initialise(asyncs + filter_i) < 0)
      goto fail;
  
  switch (get_crtc_info())
    {
    case 0:
      break;
    case -1:
      goto fail;
    case -2:
      goto cg_fail;
    }
  
  for (crtc_i = 0; crtc_i < crtcs_n; crtc_i++)
    {
      if (explicit_crtcs && !(crtc_info[crtc_i].supported))
	fprintf(stderr, "%s: warning: gamma adjustments not supported on CRTC: %s\n",
		argv0, crtcs[crtc_i]);
      if (crtc_info[crtc_i].cooperative == 0)
	fprintf(stderr, "%s: warning: cooperative gamma server not running for CRTC: %s\n",
		argv0, crtcs[crtc_i]);
    }
  
  crtc_updates = alloca(filters_n * sizeof(*crtc_updates));
  memset(crtc_updates, 0, filters_n * sizeof(*crtc_updates));
  for (filter_i = i = 0; i < classes_n; i++)
    for (crtc_i = 0; crtc_i < crtcs_n; crtc_i++, filter_i++)
      {
	if (libcoopgamma_filter_initialise(&(crtc_updates[filter_i].filter)) < 0)
	  goto fail;
	if (libcoopgamma_error_initialise(&(crtc_updates[filter_i].error)) < 0)
	  goto fail;
	crtc_updates[filter_i].crtc = crtc_i;
	crtc_updates[filter_i].synced = 1;
	crtc_updates[filter_i].failed = 0;
	crtc_updates[filter_i].master = 1;
	crtc_updates[filter_i].slaves = NULL;
	crtc_updates[filter_i].filter.crtc                = crtcs[crtc_i];
	crtc_updates[filter_i].filter.class               = classes[i];
	crtc_updates[filter_i].filter.priority            = priority;
	crtc_updates[filter_i].filter.depth               = crtc_info[crtc_i].depth;
	crtc_updates[filter_i].filter.ramps.u8.red_size   = crtc_info[crtc_i].red_size;
	crtc_updates[filter_i].filter.ramps.u8.green_size = crtc_info[crtc_i].green_size;
	crtc_updates[filter_i].filter.ramps.u8.blue_size  = crtc_info[crtc_i].blue_size;
	switch (crtc_updates[filter_i].filter.depth)
	  {
#define X(CONST, MEMBER, MAX, TYPE)\
	    case CONST:\
	      libcoopgamma_ramps_initialise(&(crtc_updates[filter_i].filter.ramps.MEMBER));\
	      libclut_start_over(&(crtc_updates[filter_i].filter.ramps.MEMBER), MAX, TYPE, 1, 1, 1);\
	      break;
LIST_DEPTHS
#undef X
	  default:
	    fprintf(stderr, "%s: internal error: gamma ramp type is unrecognised: %i\n",
		    argv0, crtc_updates[filter_i].filter.depth);
	    goto custom_fail;
	  }
      }
  
  switch (start())
    {
    case 0:
      break;
    case -1:
      goto fail;
    case -2:
      goto cg_fail;
    case -3:
      goto custom_fail;
    }
  
  for (filter_i = 0; filter_i < filters_n; filter_i++)
    if (crtc_updates[filter_i].failed)
      {
	const char* side = cg.error.server_side ? "server" : "client";
	const char* crtc = crtc_updates[filter_i].filter.crtc;
	if (cg.error.custom)
	  {
	    if ((cg.error.number != 0) && (cg.error.description != NULL))
	      fprintf(stderr, "%s: %s-side error number %" PRIu64 " for CRTC %s: %s\n",
		      argv0, side, cg.error.number, crtc, cg.error.description);
	    else if (cg.error.number != 0)
	      fprintf(stderr, "%s: %s-side error number %" PRIu64 " for CRTC %s\n",
		      argv0, side, cg.error.number, crtc);
	    else if (cg.error.description != NULL)
	      fprintf(stderr, "%s: %s-side error for CRTC %s: %s\n", argv0, side, crtc, cg.error.description);
	  }
	else if (cg.error.description != NULL)
	  fprintf(stderr, "%s: %s-side error for CRTC %s: %s\n", argv0, side, crtc, cg.error.description);
	else
	  fprintf(stderr, "%s: %s-side error for CRTC %s: %s\n", argv0, side, crtc, strerror(cg.error.number));
      }
  
 done:
  if (dealloc_crtcs)
    free(crtcs);
  if (crtc_info != NULL)
    for (crtc_i = 0; crtc_i < crtcs_n; crtc_i++)
      libcoopgamma_crtc_info_destroy(crtc_info + crtc_i);
  if (asyncs != NULL)
    for (filter_i = 0; filter_i < filters_n; filter_i++)
      libcoopgamma_async_context_destroy(asyncs + filter_i);
  if (stage >= 1)
    libcoopgamma_context_destroy(&cg, stage >= 2);
  if (crtc_updates != NULL)
    for (filter_i = 0; filter_i < filters_n; filter_i++)
      {
	if (crtc_updates[filter_i].master == 0)
	  memset(&(crtc_updates[filter_i].filter.ramps.u8), 0, sizeof(crtc_updates[filter_i].filter.ramps.u8));
	crtc_updates[filter_i].filter.crtc = NULL;
	crtc_updates[filter_i].filter.class = NULL;
	libcoopgamma_filter_destroy(&(crtc_updates[filter_i].filter));
	libcoopgamma_error_destroy(&(crtc_updates[filter_i].error));
	free(crtc_updates[filter_i].slaves);
      }
  return rc;
  
 custom_fail:
  rc = 1;
  goto done;
  
 fail:
  rc = 1;
  if (errno)
    perror(argv0);
  goto done;
  
 cg_fail:
  rc = 1;
  {
    const char* side = cg.error.server_side ? "server" : "client";
    if (cg.error.custom)
      {
	if ((cg.error.number != 0) && (cg.error.description != NULL))
	  fprintf(stderr, "%s: %s-side error number %" PRIu64 ": %s\n",
		  argv0, side, cg.error.number, cg.error.description);
	else if (cg.error.number != 0)
	  fprintf(stderr, "%s: %s-side error number %" PRIu64 "\n", argv0, side, cg.error.number);
	else if (cg.error.description != NULL)
	  fprintf(stderr, "%s: %s-side error: %s\n", argv0, side, cg.error.description);
      }
    else if (cg.error.description != NULL)
      fprintf(stderr, "%s: %s-side error: %s\n", argv0, side, cg.error.description);
    else
      fprintf(stderr, "%s: %s-side error: %s\n", argv0, side, strerror(cg.error.number));
  }
  goto done;
}

