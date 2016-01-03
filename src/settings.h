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
	 * Do not free this.
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
	 * Do not free its elements.
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


/**
 * Marshal settings into a buffer.
 * 
 * @param   param     The buffer, `NULL` if you want to know the required size.
 * @param   settings  The settings to marshal.
 * @return            The size of the output.
 */
size_t marshal_settings(char *buffer, const struct settings *settings);

/**
 * Unmarshal settings from a buffer.
 * 
 * @param   buffer    The buffer.
 * @param   settings  Output parameter for the settings, will be allocated.
 * @return            The number of unmarshalled bytes, 0 on error.
 */
size_t unmarshal_settings(char *buffer, struct settings **settings);

