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



/**
 * The times of the day, by degree of darkness.
 */
enum darkness {
	/**
	 * Not calculated yet.
	 */
 	UNKNOWN = -1,

	/**
	 * Hopefully, it is bright outside.
	 */
	DAYTIME = 0,

	/**
	 * The sky is golden. The golden "hour".
	 * Also known as BMCT (dawn) or EECT (dusk).
	 */
	CIVIL_TWILIGHT = 1,

	/**
	 * The sky is pink and blue.
	 */
	NAUTICAL_TWILIGHT = 2,

	/**
	 * The sky is medium dark blue.
	 */
	ASTRONOMICAL_TWILIGHT = 3,

	/**
	 * The sky is really dark blue.
	 */
	NIGHT = 4
};



/**
 * Is it daytime, night, perhaps some kind of twilight?
 * 
 * @param    elevation  The Sun's apparent elevation.
 * @return              The time of the day.
 */
enum darkness get_darkness(double elevation);

/**
 * Set $RADHARC_STATE.
 * 
 * @param   settings  The settings.
 * @return            0 on success, -1 on error.
 */
int get_state_pathname(const struct settings *settings);

