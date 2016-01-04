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
#include "macros.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include <libhaiku.h>



/**
 * Print usage information and exit if a condition is met.
 * 
 * @param  condition  Do no do anything iff this is zero.
 */
static void
usage(int condition)
{
	if (!condition) return;
	fprintf(stderr,
	        "Usage: %s [OPTIONS]...\n"
	       	"See `man 1 radharc` for more information.\n",
	        !argv0 ? "radharc" : argv0), exit(2);
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
	char *end;
	int dir = *str == '-' ? -1 : *str == '+' ? +1 : 0;
	str += !!dir;
	t (dir && !direction);
	if (dir) *direction = dir;
	t (!isdigit(*str));
	*temp = (errno = 0, strtol)(str, &end, 10);
	t ((errno && !((errno == ERANGE) && (*temp == LONG_MAX))) || *end || (*temp < lower));
	return 0;
fail:
	return -1;
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
#define DIGIT(var, c)          var += (var) * 10 + ((c) & 15);
#define ALL_DIGITS(cond, var)  while ((cond) && isdigit(*str))  DIGIT(var, *str++)

	int points = 0;
	memset(ts, 0, sizeof(*ts));

	/* Parse seconds. */
	t (!isdigit(*str));
	ALL_DIGITS(1, ts->tv_sec);

	/* End? */
	if (!*str)  return 0;
	t (*str != '.');

	/* Parse nanoseconds.*/
	ALL_DIGITS(points++ < 9, ts->tv_nsec);
	if ((points == 9) && isdigit(*str) && (*str++ >= '5') && (++(ts->tv_nsec) == 1000000000L))
		ts->tv_sec += 1, ts->tv_nsec = 0;
	while (isdigit(*str))  str++;
	t (*str);

	/* End! */
	return 0;
fail:
	return -1;
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
parse_command_line(int argc, char *argv[], struct settings *s)
{
	int location_set = 0;
	char *p;
	char *arg;
	int c = 0;

	memset(s, 0, sizeof(*s));
	s->natural_temp = 6500;
	s->day_temp     = 5500;
	s->night_temp   = 3500;
	s->trans_speed  = 50;

#define BOOLEAN(var)  var = !plus;  break
#define PLUS(...)  (plus ? (__VA_ARGS__) : 0)
	ARGBEGIN {
	case 'l':
		PLUS(location_set = 0);
		usage(!(p = strchr(arg = ARGF(), ':')));
		*p++ = '\0', location_set = 1;
		usage(parse_location(arg, &(s->latitude),   90.0));
		usage(parse_location(p,   &(s->longitude), 180.0));
		break;
	case 't':
		PLUS(s->day_temp = 5500, s->night_temp = 3500);
		s->temp = s->day_temp = s->night_temp = 0, s->temp_direction = 0;
		if ((p = strchr(arg = ARGF(), ':'))) {
			*p++ = '\0';
			usage(parse_temperature(arg, &(s->day_temp), NULL, 1000));
			usage(parse_temperature(p, &(s->night_temp), NULL, 1000));
		} else {
			usage(parse_temperature(arg, &(s->temp), &(s->temp_direction), 1000));
		}
		break;
	case 'T':
		PLUS(s->natural_temp = 6500);
		usage(parse_temperature(ARGF(), &(s->natural_temp), NULL, 1000));
		break;
	case 's':
		PLUS(s->trans_speed = 50);
		s->trans_speed = 0;
		usage(parse_timespec(ARGF(), &(s->transition)));
		break;
	case 'S':
		PLUS(s->trans_speed = 0);
		usage(parse_temperature(ARGF(), &(s->trans_speed), NULL, 1));
		break;
	case 'h':
		s->hookpath = (plus ? NULL : ARGF());
		break;
	case 'd': c++; /* Fall though. */
	case 'e': c++; /* Fall though. */
	case 'm': c++;
		PLUS(s->monitors_n = 0, free(s->monitors_id), free(s->monitors_arg));
		xrealloc(&(s->monitors_id),  s->monitors_n + 1);
		xrealloc(&(s->monitors_arg), s->monitors_n + 1);
		s->monitors_id[s->monitors_n] = ARGF();
		s->monitors_arg[s->monitors_n++] = (c == 3 ? 'm' : c == 2 ? 'e' : 'd'), c = 0;
		break;
	case 'p':  BOOLEAN(s->print_status);
	case 'n':  BOOLEAN(s->panic_start);
	case 'N':  BOOLEAN(s->panic_else);
	case 'o':  BOOLEAN(s->set_and_exit);
	case 'x':  BOOLEAN(s->ignore_calib);
	case 'i':  BOOLEAN(s->negative);
	case 'b':  BOOLEAN(s->use_bus);
	default:   usage(1);  break;
	} ARGEND;
	usage(argc);

	if (!location_set && !(s->temp))
		fprintf(stderr,
			"%s: The -l option is mandatory, unless single value -t is used. "
	        	"See `man 1 radharc` for more information.\n",
		        !argv0 ? "radharc" : argv0), exit(2);

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

	size_t aux, n = 0, i = 0;
	char *buf = buffer;

	if (buffer)  i = marshal_settings(NULL, settings);
	MARSHAL(sizeof(i), &i);

	MARSHAL(sizeof(*settings), settings);
	if (settings->hookpath)      MARSHAL(strlen(settings->hookpath) + 1, settings->hookpath);
	if (settings->monitors_arg)  MARSHAL(settings->monitors_n, settings->monitors_arg);
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
	struct settings s_, *s = NULL;
	char *buf = buffer;

	UNMARSHAL(sizeof(n), &n);
	if (!(s = *settings = malloc(n - sizeof(n))))  return 0;
	s->monitors_id = NULL, s->monitors_arg = NULL;

	UNMARSHAL(sizeof(s_), &s_);
	if (s_.monitors_n) {
		*s = s_;
		xmalloc(&(s->monitors_id),  s_.monitors_n);
		xmalloc(&(s->monitors_arg), s_.monitors_n);
	}

	buf = strchr(s->hookpath = buf, '\0') + 1;

	UNMARSHAL(s_.monitors_n, s->monitors_arg);
	for (i = 0; i < s_.monitors_n; i++)
		UNMARSHAL(strlen(buf + 1), s->monitors_id[i]);

	return n;

fail:
	CLEANUP(free(s->monitors_id), free(s->monitors_arg), free(s), *settings = NULL);
	return 0;
}

