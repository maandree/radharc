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

