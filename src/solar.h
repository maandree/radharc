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


/**
 * Approximate apparent size of the Sun in degrees.
 */
#define SOLAR_APPARENT_RADIUS  (32.0 / 60.0)


/**
 * The Sun's elevation at sunset and sunrise, measured in degrees.
 */
#define SOLAR_ELEVATION_SUNSET_SUNRISE   (0.0)

/**
 * The Sun's elevation at civil dusk and civil dawn, measured in degrees.
 */
#define SOLAR_ELEVATION_CIVIL_DUSK_DAWN   (-6.0)

/**
 * The Sun's elevation at nautical dusk and nautical dawn, measured in degrees.
 */
#define SOLAR_ELEVATION_NAUTICAL_DUSK_DAWN  (-12.0)

/**
 * The Sun's elevation at astronomical dusk and astronomical dawn, measured in degrees.
 */
#define SOLAR_ELEVATION_ASTRONOMICAL_DUSK_DAWN  (-18.0)


/**
 * Test whether it is twilight.
 * 
 * @param   ELEV:double  The current elevation.
 * @return               1 if is twilight, 0 otherwise.
 */
#define SOLAR_IS_TWILIGHT(ELEV)  ((-18.0 <= (ELEV)) && ((ELEV) <= 0.0))

/**
 * Test whether it is civil twilight.
 * 
 * @param   ELEV:double  The current elevation.
 * @return               1 if is civil twilight, 0 otherwise.
 */
#define SOLAR_IS_CIVIL_TWILIGHT(ELEV)  ((-6.0 <= (ELEV)) && ((ELEV) <= 0.0))

/**
 * Test whether it is nautical twilight.
 * 
 * @param   ELEV:double  The current elevation.
 * @return               1 if is nautical twilight, 0 otherwise.
 */
#define SOLAR_IS_NAUTICAL_TWILIGHT(ELEV)  ((-12.0 <= (ELEV)) && ((ELEV) <= -6.0))

/**
 * Test whether it is astronomical twilight.
 * 
 * @param   ELEV:double  The current elevation.
 * @return               1 if is astronomical twilight, 0 otherwise.
 */
#define SOLAR_IS_ASTRONOMICAL_TWILIGHT(ELEV)  ((-18.0 <= (ELEV)) && ((ELEV) <= -12.0))



/**
 * Calculates the Sun's elevation as apparent.
 * from a geographical position.
 * 
 * @param   latitude   The latitude in degrees northwards from 
 *                     the equator, negative for southwards.
 * @param   longitude  The longitude in degrees eastwards from
 *                     Greenwich, negative for westwards.
 * @return             The Sun's apparent elevation as seen, right now,
 *                     from the specified position, measured in degrees.
 * 
 * @throws  0  On success.
 * @throws     Any error specified for clock_gettime(3) on error.
 */
double solar_elevation(double latitude, double longitude);

