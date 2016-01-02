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
#include <time.h>



/**
 * Settings from the command line.
 */
struct settings {
	/**
	 * Print current status and exit?
	 */
	int print_status : 1;

	/**
	 * Start without transition?
	 */
	int panic_start : 1;

	/**
	 * Never transition, apart from at start?
	 */
	int panic_else : 1;

	/**
	 * Set temperature, possibly with transition, and exit?
	 */
	int set_and_exit : 1;

	/**
	 * Ignore calibrations?
	 */
	int ignore_calib : 1;

	/**
	 * Apply negative image filter?
	 */
	int negative : 1;

	/**
	 * Broadcast event with bus?
	 */
	int use_bus : 1;

	/**
	 * -1 to decrease temperature,
	 * +1 to increase temperature,
	 *  0 to set temperature.
	 */
	int temp_direction;

	/**
	 * The temperature, if use, the program will exit when it is done.
	 */
	long int temp;

	/**
	 * The temperature at full daytime.
	 */
	long int day_temp;

	/**
	 * The temperature at full night.
	 */
	long int night_temp;

	/**
	 * The temperature when disabled.
	 */
	long int natural_temp;

	/**
	 * Pathname to the hook script.
	 */
	char *hookpath;

	/**
	 * The number of seconds the transition takes.
	 */
	struct timespec transition;

	/**
	 * The number of kelvins per seconds the
	 * temperature is adjusted during transition.
	 */
	long int trans_speed;

	/**
	 * The user's latitudinal position.
	 */
	double latitude;

	/**
	 * The user's longitudinal position.
	 */
	double longitude;

	/**
	 * The number of elements in `monitors_id` and in `monitors_arg`.
	 */
	size_t monitors_n;

	/**
	 * Values for -d, -e, and -m, in order.
	 */
	char **monitors_id;

	/**
	 * The option (the character after the dash)
	 * the option used in correspnding element
	 * in `monitors_id`.
	 */
	char *monitors_arg;
};



/**
 * Parse the command line.
 * 
 * @param  argc      The number of elements in `argv`.
 * @param  argv      The commnad line arguments including the zeroth elemenet.
 * @param  settings  Output parameter for the settings.
 */
void parse_command_line(int argc, char *argv[], struct settings *settings);

