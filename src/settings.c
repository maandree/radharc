/**
 * Copyright © 2016  Mattias Andrée <maandree@member.fsf.org>
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
#include "settings.h"
#include "arg.h"
#include "haiku.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>



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

#define PLUS(...)  (plus ? (__VA_ARGS__) : 0)
	ARGBEGIN {
	case 'l':
		PLUS(location_set = 0);
		usage(!(p = strchr(arg = ARGF(), ':')));
		*p++ = '\0', location_set = 1;
		usage(parse_location(arg, &(settings->latitude),  90.0));
		usage(parse_location(arg, &(settings->longitude), 180.0));
		break;
	case 't':
		PLUS(settings->day_temp = 5500, settings->night_temp = 3500);
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
		PLUS(settings->natural_temp = 6500);
		usage(parse_temperature(ARGF(), &(settings->natural_temp), NULL, 1000));
		break;
	case 's':
		PLUS(settings->trans_speed = 50);
		settings->trans_speed = 0;
		usage(parse_timespec(ARGF(), &(settings->transition)));
		break;
	case 'S':
		PLUS(settings->trans_speed = 0);
		usage(parse_temperature(ARGF(), &(settings->trans_speed), NULL, 1));
		break;
	case 'h':
		settings->hookpath = (plus ? NULL : ARGF());
		break;
	case 'd': c++; /* Fall though. */
	case 'e': c++; /* Fall though. */
	case 'm': c++;
#define REALLOC(VAR, N)  !(VAR = realloc(VAR, (N) * sizeof(*VAR)))
		PLUS(settings->monitors_n = 0, free(settings->monitors_id), free(settings->monitors_arg));
		settings->monitors_n++;
		if (REALLOC(settings->monitors_id,  settings->monitors_n))  goto fail;
		if (REALLOC(settings->monitors_arg, settings->monitors_n))  goto fail;
		settings->monitors_id[settings->monitors_n - 1] = ARGF();
		settings->monitors_arg[settings->monitors_n - 1] = (c == 3 ? 'm' : c == 2 ? 'e' : 'd'), c = 0;
		break;
	case 'p':  settings->print_status = !plus;  break;
	case 'n':  settings->panic_start  = !plus;  break;
	case 'N':  settings->panic_else   = !plus;  break;
	case 'o':  settings->set_and_exit = !plus;  break;
	case 'x':  settings->ignore_calib = !plus;  break;
	case 'i':  settings->negative     = !plus;  break;
	case 'b':  settings->use_bus      = !plus;  break;
	default:   usage(1);                        break;
	} ARGEND;
	usage(argc);

	if (!location_set && !(settings->temp)) {
		fprintf(stderr,
			"%s: The -l option is mandatory, unless single value -t is used. "
	        	"See `man 1 radharc` for more information.\n",
		        !argv0 ? "radharc" : argv0);
		exit(2);
	}

	return;
fail:
	haiku(argv0 ? argv0 : "radharc");
	exit(1);
}


/**
 * Marshal settings into a buffer.
 * 
 * @param   buffer    The buffer, `NULL` if you want to know the required size.
 * @param   settings  The settings to marshal.
 * @return            The size of the output.
 */
size_t
marshal_settings(char *buffer, const struct settings *settings)
{
#define MARSHAL(N, DATUMP)  (n += aux = (N), (buf ? (memcpy(buf, DATUMP, aux), buf += aux, 0) : 0))

	size_t n = 0, i = 0;
	size_t aux;
	char *buf = buffer;

	if (buffer)  i = marshal_settings(NULL, settings);
	MARSHAL(sizeof(i), &i);

	MARSHAL(sizeof(*settings), settings);
	if (settings->hookpath)
		MARSHAL(strlen(settings->hookpath) + 1, settings->hookpath);
	if (settings->monitors_arg)
		MARSHAL(settings->monitors_n, settings->monitors_arg);
	for (i = 0; i < settings->monitors_n; i++)
		MARSHAL(strlen(settings->monitors_id[i]) + 1, settings->monitors_id[i]);

	return n;
}


/**
 * Unmarshal settings from a buffer.
 * 
 * @param   buffer    The buffer.
 * @param   settings  Output parameter for the settings, will be allocated.
 * @return            The number of unmarshalled bytes, 0 on error.
 */
size_t
unmarshal_settings(char *buffer, struct settings **settings)
{
#define UNMARSHAL(N, DATUMP)  (aux = (N), memcpy((DATUMP), buf, aux), buf += aux)

	size_t n, aux, i;
	struct settings *s = NULL;
	struct settings s_;
	char *buf = buffer;
	int saved_errno;

	UNMARSHAL(sizeof(n), &n);
	if (!(s = *settings = malloc(n - sizeof(n))))  return 0;
	s->monitors_id = NULL, s->monitors_arg = NULL;

	UNMARSHAL(sizeof(s_), &s_);
	if (s_.monitors_n) {
		if (!(*s = s_, s->monitors_id = malloc(s_.monitors_n * sizeof(char*))))  goto fail;
		if (!(*s = s_, s->monitors_arg = malloc(s_.monitors_n * sizeof(char))))  goto fail;
	}

	s->hookpath = buf;
	buf = strchr(buf, '\0') + 1;

	UNMARSHAL(s_.monitors_n, s->monitors_arg);
	for (i = 0; i < s_.monitors_n; i++)
		UNMARSHAL(strlen(buf + 1), s->monitors_id[i]);

	return n;

fail:
	saved_errno = errno;
	free(s->monitors_id), free(s->monitors_arg), free(s), *settings = NULL;
	errno = saved_errno;
	return 0;
}

