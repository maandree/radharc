/**
 * Copyright © 2016  Mattias Andrée <maandree@member.fsf.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "settings.h"
#include "arg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>



/**
 * The name of the process.
 */
char *argv0 = NULL;



/**
 * Print usage information and exit if a condition is met.
 * 
 * @param  condition  Do no do anything iff this is zero.
 */
static void
usage(int condition)
{
	if (condition) {
		fprintf(stderr,
		        "Usage: %s [OPTIONS]...\n"
	        	"See `man 1 radharc` for more information.\n",
		        !argv0 ? "radharc" : argv0);
		exit(2);
	}
}


/**
 * Parse a temperature string.
 * 
 * @param   str        The temperature string.
 * @param   temp       Output parameter for the temperature.
 *                     Overflow is truncated.
 * @param   direction  Unless `NULL`, will be set to +1 if `str`
 *                     starts with a '+', `-1` if `str` starts
 *                     with a '-', 0 otherwise. If `NULL` and
 *                     `str` starts with a '-' or a '+', the
 *                     function will fail.
 * @param   lower      The function shall fail if the evaluated
 *                     temperature is below this.
 * @return             0 on success, -1 on error.
 */
static int
parse_temperature(const char *str, long int *temp, int *direction, int lower)
{
	int dir = 0;
	char *end;
	switch (*str) {
	case '-':  dir = -1;  break;
	case '+':  dir = +1;  break;
	}
	str += !!dir;
	if (dir && !direction)  return -1;
	else if (dir)           *direction = dir;
	if (!isdigit(*str))     return -1;
	*temp = (errno = 0, strtol)(str, &end, 10);
	if ((errno && !((errno == ERANGE) && (*temp == LONG_MAX))) || *end || (*temp < lower))
		return -1;
	return 0;
}


/**
 * Parse a string as a positively valued `struct timespec`.
 * 
 * @param   str  The string.
 * @param   ts   Output parameter for the value.
 * @return       0 on success, -1 on error. 
 */
static int
parse_timespec(const char *str, struct timespec *ts)
{
	int points = 0;
	memset(ts, 0, sizeof(*ts));

	/* Parse seconds. */
	if (!isdigit(*str))
		return -1;
	while (isdigit(*str)) {
		ts->tv_sec *= 10;
		ts->tv_sec += *str++ & 15;
	}

	/* End? */
	if (!*str)        return 0;
	if (*str != '.')  return -1;

	/* Parse nanoseconds.*/
	for (; (points++ < 9) && isdigit(*str); str++) {
		ts->tv_nsec *= 10;
		ts->tv_nsec += *str++ & 15;
	}
	if (points == 9) {
		if (!isdigit(*str))  return -1;
		if (*str++ >= '5') {
			ts->tv_nsec += 1;
			if (ts->tv_nsec == 1000000000L)
				ts->tv_sec += 1, ts->tv_nsec = 0;
		}
	}
	while (isdigit(*str))  str++;
	if (*str)  return -1;

	/* End! */
	return 0;
}


/**
 * Parse a latitude or a longitude value.
 * 
 * @param   str    The string.
 * @param   loc    Output parameter for the value.
 * @param   limit  The limit of the absolute value.
 * @return         0 on success, -1 on error.
 */
static int
parse_location(char *str, double *loc, double limit)
{
	char* end;
	if (strstr(str, "−") == str) /* Support proper minus. */
		*(str += strlen("−") - 1) = '-';
	if ((*str != '-') && (*str != '+') && (*str != '.') && !isdigit(*str))
		return -1;
	*loc = (errno = 0, strtod)(str, &end);
	return -(errno || *loc || (fabs(*loc) > limit));
}


/**
 * Parse the command line.
 * 
 * @param  argc      The number of elements in `argv`.
 * @param  argv      The commnad line arguments including the zeroth elemenet.
 * @param  settings  Output parameter for the settings.
 */
void
parse_command_line(int argc, char *argv[], struct settings *settings)
{
	int location_set = 0;
	char *p;
	char *arg;
	int c = 0;

	memset(settings, 0, sizeof(*settings));
	settings->natural_temp = 6500;
	settings->day_temp     = 5500;
	settings->night_temp   = 3500;
	settings->trans_speed  = 50;

	ARGBEGIN {
	case 'l':
		usage(!(p = strchr(arg = ARGF(), ':')));
		*p++ = '\0', location_set = 1;
		usage(parse_location(arg, &(settings->latitude),  90.0));
		usage(parse_location(arg, &(settings->longitude), 180.0));
		break;
	case 't':
		settings->temp = settings->day_temp = settings->night_temp = 0;
		settings->temp_direction = 0;
		if ((p = strchr(arg = ARGF(), ':'))) {
			*p++ = '\0';
			usage(parse_temperature(arg, &(settings->day_temp), NULL, 1000));
			usage(parse_temperature(p, &(settings->night_temp), NULL, 1000));
		} else {
			usage(parse_temperature(arg, &(settings->temp), &(settings->temp_direction), 1000));
		}
		break;
	case 'T':
		usage(parse_temperature(ARGF(), &(settings->natural_temp), NULL, 1000));
		break;
	case 's':
		settings->trans_speed = 0;
		usage(parse_timespec(ARGF(), &(settings->transition)));
		break;
	case 'S':
		usage(parse_temperature(ARGF(), &(settings->trans_speed), NULL, 1));
		break;
	case 'h':
		usage(!(settings->hookpath = ARGF()));
		break;
	case 'd': c++; /* Fall though. */
	case 'e': c++; /* Fall though. */
	case 'm': c++;
#define REALLOC(VAR, N)  !(VAR = realloc(VAR, (N) * sizeof(*VAR)))
		settings->monitors_n++;
		if (REALLOC(settings->monitors_id,  settings->monitors_n))  goto fail;
		if (REALLOC(settings->monitors_arg, settings->monitors_n))  goto fail;
		settings->monitors_id[settings->monitors_n - 1] = ARGF();
		settings->monitors_arg[settings->monitors_n - 1] = (c == 3 ? 'm' : c == 2 ? 'e' : 'd'), c = 0;
		break;
	case 'p':  settings->print_status = 1;  break;
	case 'n':  settings->panic_start  = 1;  break;
	case 'N':  settings->panic_else   = 1;  break;
	case 'o':  settings->set_and_exit = 1;  break;
	case 'x':  settings->ignore_calib = 1;  break;
	case 'i':  settings->negative     = 1;  break;
	case 'b':  settings->use_bus      = 1;  break;
	default:   usage(1);                    break;
	} ARGEND;
	usage(argc);

	if (!location_set && !(settings->temp)) {
		fprintf(stderr,
			"%s: The -l option is mandatory, unless single value -t is used."
	        	"See `man 1 radharc` for more information.\n",
		        !argv0 ? "radharc" : argv0);
		exit(2);
	}

fail:
	perror(argv0 ? argv0 : "radharc");
	exit(1);
}
