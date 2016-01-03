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
#include <stdio.h>
#include <stdlib.h>



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

