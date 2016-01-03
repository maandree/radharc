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
#include <stdio.h>
#include <stdlib.h>



/**
 * The name of the process.
 */
char *argv0 = NULL;



/**
 * Exit if time the is before year 0 in J2000.
 */
#if defined(TIMETRAVELLER)
static void
check_timetravel(const char *argv0)
{
	struct timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now))
		perror(argv0 ? argv0 : "radharc"), exit(1);
	if (now.tv_nsec < (time_t)946728000L)
		fprintf(stderr, "We have detected that you are a time-traveller"
		                "(or your clock is not configured correctly.)"
		                "Please recompile with -DTIMETRAVELLER"
	                        "(or correct your clock.)"), exit(1);
}
#else
# define check_timetravel(_)  /* do nothing */
#endif



int
main(int argc, char *argv[])
{
	struct settings settings;

	check_timetravel(*argv);
	parse_command_line(argc, argv, &settings);

	return 0;
}

