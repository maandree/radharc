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
#include "state.h"
#include "solar.h"



/**
 * Is it daytime, night, perhaps some kind of twilight?
 * 
 * @param    elevation  The Sun's apparent elevation.
 * @return              The time of the day.
 */
enum darkness
get_darkness(double elevation)
{
	if (elevation > SOLAR_ELEVATION_SUNSET_SUNRISE)          return DAYTIME;
	if (elevation > SOLAR_ELEVATION_CIVIL_DUSK_DAWN)         return CIVIL_TWILIGHT;
	if (elevation > SOLAR_ELEVATION_NAUTICAL_DUSK_DAWN)      return NAUTICAL_TWILIGHT;
	if (elevation > SOLAR_ELEVATION_ASTRONOMICAL_DUSK_DAWN)  return ASTRONOMICAL_TWILIGHT;
	return NIGHT;
}

