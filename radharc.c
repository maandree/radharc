/* See LICENSE file for copyright and license details. */
#include "cg-base.h"

#include <sys/timerfd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libclut.h>
#include <libred.h>

/**
 * The default filter priority for the program
 */
const int64_t default_priority = (int64_t)7 << 61;

/**
 * The default class for the program
 */
char default_class[] = "radharc::radharc::standard";

/**
 * Class suffixes
 */
const char *const *class_suffixes = (const char *const[]){NULL};

/**
 * The effect fade-in time, in centiseconds
 */
static unsigned long int fade_in_cs = 0;

/**
 * The effect fade-out time, in centiseconds
 */
static unsigned long int fade_out_cs = 0;

/**
 * The highest elevation of the Sun where the lowest
 * colour temperature is applied
 */
static double low_elev = -6;

/**
 * The lowest colour temperature that may be applied
 */
static double low_temp = 2500;

/**
 * The lowest elevation of the Sun where the highest
 * colour temperature is applied
 */
static double high_elev = 3;

/**
 * The highest colour temperature that may be applied
 */
static double high_temp = 5000;

/**
 * The temperature choosen with the -f flag, negative if none
 */
static double choosen_temperature = -1;

/**
 * The latitude coordiate of the GPS coordiates of
 * the user's location
 */
static double latitude;

/**
 * The longitude coordiate of the GPS coordiates of
 * the user's location
 */
static double longitude;

/**
 * Whether the user's location has been specified
 */
static int have_location = 0;

/**
 * Whether the -d flag (keep process running and remove
 * effect when killed) has been specified
 */
static int dflag = 0;

/**
 * Whether the -x flag (remove applied effect)
 * has been specified
 */
static int xflag = 0;

/**
 * Print usage information and exit
 */
void
usage(void)
{
	fprintf(stderr,
	        "usage: %s [-M method] [-S site] [-c crtc]... [-R rule] [-p priority]"
	        " [-f fade-in] [-F fade-out] [-h [high-temp][@high-elev]] [-l [low-temp][@low-elev]]"
	        " (-L latitude:longitude | -t temperature [-d] | -x)\n", argv0);
	exit(1);
}

/**
 * Parse a non-negative double encoded as a string
 * 
 * @param   out  Output parameter for the value
 * @param   str  The string
 * @return       Zero on success, -1 if the string is invalid
 */
static int
parse_double(double *out, const char *str)
{
	char *end;
	errno = 0;
	*out = strtod(str, &end);
	if (errno || *out < 0 || isinf(*out) || isnan(*out) || *end)
		return -1;
	if (!*str || !strchr("0123456789.", *str))
		return -1;
	return 0;
}

/**
 * Handle a command line option
 * 
 * @param   opt  The option, it is a NUL-terminate two-character
 *               string starting with either '-' or '+', if the
 *               argument is not recognised, call `usage`. This
 *               string will not be "-M", "-S", "-c", "-p", or "-R".
 * @param   arg  The argument associated with `opt`,
 *               `NULL` there is no next argument, if this
 *               parameter is `NULL` but needed, call `usage`
 * @return       0 if `arg` was not used,
 *               1 if `arg` was used,
 *               -1 on error
 */
int
handle_opt(char *opt, char *arg)
{
	double t;
	char *p;
	if (opt[0] == '-') {
		switch (opt[1]) {
		case 'd':
			dflag = 1;
			xflag = 0;
			break;
		case 'f':
			if (parse_double(&t, arg))
				usage();
			fade_in_cs = (unsigned long int)(t * 100 + 0.5);
			return 1;
		case 'F':
			if (parse_double(&t, arg))
				usage();
			fade_out_cs = (unsigned long int)(t * 100 + 0.5);
			return 1;
		case 'h':
			p = strchr(arg, '@');
			if (p)
				*p++ = '\0';
			if (*arg && parse_double(&high_temp, arg))
				usage();
			if (*p && parse_double(&high_elev, p))
				usage();
			return 1;
		case 'l':
			p = strchr(arg, '@');
			if (p)
				*p++ = '\0';
			if (*arg && parse_double(&low_temp, arg))
				usage();
			if (*p && parse_double(&low_elev, p))
				usage();
			return 1;
		case 'L':
			p = strchr(arg, ':');
			if (!p)
				usage();
			*p++ = '\0';
			if (parse_double(&latitude, arg) || latitude < -90 || latitude > 90)
				usage();
			if (parse_double(&longitude, p) || longitude < -180 || longitude > 180)
				usage();
			choosen_temperature = -1;
			have_location = 1;
			dflag = 0;
			xflag = 0;
			return 1;
		case 't':
			if (parse_double(&choosen_temperature, arg))
				usage();
			xflag = 0;
			return 1;
		case 'x':
			xflag = 1;
			dflag = 0;
			break;
		default:
			usage();
		}
	} else {
		usage();
	}
	return 0;
}

/**
 * This function is called after the last
 * call to `handle_opt`
 * 
 * @param   argc  The number of unparsed arguments
 * @param   argv  `NULL` terminated list of unparsed arguments
 * @param   prio  The argument associated with the "-p" option
 * @return        Zero on success, -1 on error
 */
int
handle_args(int argc, char *argv[], char *prio)
{
	if (argc || (!xflag && !have_location && choosen_temperature < 0))
		usage();
	return 0;
	(void) argv;
	(void) prio;
}

/**
 * Fill a filter
 * 
 * @param  filter  The filter to fill
 * @param  red     The red brightness
 * @param  green   The green brightness
 * @param  blue    The blue brightness
 */
static void
fill_filter(libcoopgamma_filter_t *restrict filter, double red, double green, double blue)
{
	switch (filter->depth) {
#define X(CONST, MEMBER, MAX, TYPE)\
	case CONST:\
		libclut_start_over(&(filter->ramps.MEMBER), MAX, TYPE, 1, 1, 1);\
		libclut_rgb_brightness(&(filter->ramps.MEMBER), MAX, TYPE, red, green, blue);\
		break;
LIST_DEPTHS
#undef X
	default:
		abort();
	}
}

/**
 * Set the gamma ramps
 * 
 * @param   red    The red brightness
 * @param   green  The green brightness
 * @param   blue   The blue brightness
 * @return         0: Success
 *                 -1: Error, `errno` set
 *                 -2: Error, `cg.error` set
 *                 -3: Error, message already printed
 */
static int
set_ramps(double red, double green, double blue)
{
	int r;
	size_t i, j;

	for (i = 0, r = 1; i < filters_n; i++) {
		if (!(crtc_updates[i].master) || !(crtc_info[crtc_updates[i].crtc].supported))
			continue;
		fill_filter(&(crtc_updates[i].filter), red, green, blue);
		r = update_filter(i, 0);
		if (r == -2 || (r == -1 && errno != EAGAIN))
			return r;
		if (crtc_updates[i].slaves) {
			for (j = 0; crtc_updates[i].slaves[j] != 0; j++) {
				r = update_filter(crtc_updates[i].slaves[j], 0);
				if (r == -2 || (r == -1 && errno != EAGAIN))
					return r;
			}
		}
	}

	while (r != 1)
		if ((r = synchronise(-1)) < 0)
			return r;

	return 0;
}

/**
 * Get the colour temperature for the current time
 * 
 * @param   tp  Output parameter for the colour temperature
 * @return      0 on success, -1 on failure
 */
static int
get_temperature(double *tp)
{
	if (choosen_temperature < 0) {
		if (libred_solar_elevation(latitude, longitude, tp))
			return -1;
		if (*tp < low_elev)
			*tp = low_elev;
		if (*tp > high_elev)
			*tp = high_elev;
		*tp = (*tp - low_elev) / (high_elev - low_elev);
		*tp = low_temp + *tp * (high_temp - low_temp);
	} else {
		*tp = choosen_temperature;
	}
	return 0;
}


/**
 * The main function for the program-specific code
 * 
 * @return  0: Success
 *          -1: Error, `errno` set
 *          -2: Error, `cg.error` set
 *          -3: Error, message already printed
 */
int
start(void)
{
	int r, tfd;
	size_t i;
	double temperature, red, green, blue;
	uint64_t overrun;

	if (xflag)
		for (i = 0; i < filters_n; i++)
			crtc_updates[i].filter.lifespan = LIBCOOPGAMMA_REMOVE;
	else if (choosen_temperature >= 0 && !dflag)
		for (i = 0; i < filters_n; i++)
			crtc_updates[i].filter.lifespan = LIBCOOPGAMMA_UNTIL_REMOVAL;
	else
		for (i = 0; i < filters_n; i++)
			crtc_updates[i].filter.lifespan = LIBCOOPGAMMA_UNTIL_DEATH;

	if (!xflag) {
		if (libred_check_timetravel())
			return -1;
		if (choosen_temperature < 0)
			dflag = 1;
	}

	if (xflag)
		return set_ramps(1, 1, 1);

	if ((r = make_slaves()) < 0)
		return r;

	if (!fade_in_cs)
		goto no_fade_in;

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (tfd < 0)
		return -1;
	if (timerfd_settime(tfd, 0, &(struct itimerspec){{0, 10000000L}, {0, 10000000L}}, NULL))
		return -1;

	for (i = 0; i < (size_t)fade_in_cs;) {
		if (i % 600 == 0)
			if ((r = get_temperature(&temperature)) < 0)
				return r;
		if (libred_get_colour((long int)(6500 - (6500 - temperature) * i / fade_in_cs), &red, &green, &blue))
			return -1;
		if ((r = set_ramps(red, green, blue)) < 0)
			return r;

		if (read(tfd, &overrun, sizeof(overrun)) != sizeof(overrun))
			return -1;
		if (overrun > fade_in_cs - i)
			overrun = fade_in_cs - i;
		i += overrun;
	}

	close(tfd);

no_fade_in:
	for (;;) {
		if ((r = get_temperature(&temperature)) < 0)
			return r;
		if (libred_get_colour((long int)temperature, &red, &green, &blue))
			return -1;
		if ((r = set_ramps(red, green, blue)) < 0)
			return r;

		if (!dflag)
			return 0;

		sleep(6);
	}
}
