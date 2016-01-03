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
#include <math.h>
#include <time.h>
#include <errno.h>



/**
 * Get current Julian Centuries time (100 Julian days since J2000.)
 * 
 * @return  The current Julian Centuries time.
 * 
 * @throws  0  On success.
 * @throws     Any error specified for clock_gettime(3) on error.
 */
static double
julian_centuries()
{
	struct timespec now;
	double tm;
#if defined(CLOCK_REALTIME_COARSE)
	if (clock_gettime(CLOCK_REALTIME_COARSE, &now))
#else
	if (clock_gettime(CLOCK_REALTIME, &now))
#endif
		return 0.0;
	tm = (double)(now.tv_nsec);
	tm /= 1000000000.0;
	tm += (double)(now.tv_sec);
	tm = (tm / 86400.0 + 2440587.5 - 2451545.0) / 36525.0;
	return errno = 0, tm;
}

/**
 * Convert a Julian Centuries timestamp to a Julian Day timestamp.
 * 
 * @param   tm  The time in Julian Centuries
 * @return      The time in Julian Days
 */
static inline double
julian_centuries_to_julian_day(double tm)
{
	return tm * 36525.0 + 2451545.0;
}


/**
 * Convert an angle (or otherwise) from degrees to radians.
 * 
 * @param  deg  The angle in degrees.
 * @param       The angle in radians.
 */
static inline double
radians(double deg)
{
	return deg * (double)M_PI / 180.0;
}

/**
 * Convert an angle (or otherwise) from radians to degrees.
 * 
 * @param  rad  The angle in radians.
 * @param       The angle in degrees.
 */
static inline double
degrees(double rad)
{
	return rad * 180.0 / (double)M_PI;
}


/**
 * Calculates the Sun's elevation from the solar hour angle
 * 
 * @param   longitude    The longitude in degrees eastwards.
 *                       from Greenwich, negative for westwards.
 * @param   declination  The declination, in radians.
 * @param   hour_angle   The solar hour angle, in radians.
 * @return               The Sun's elevation, in radians.
 */
static inline double
elevation_from_hour_angle(double latitude, double declination, double hour_angle)
{
	double rc = cos(radians(latitude));
	rc *= cos(hour_angle) * cos(declination);
	rc += sin(radians(latitude)) * sin(declination);
	return asin(rc);
}

/**
 * Calculates the Sun's geometric mean longitude.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The Sun's geometric mean longitude in radians.
 */
static inline double
sun_geometric_mean_longitude(double tm)
{
	double rc = fmod(pow(0.0003032 * tm, 2.0) + 36000.76983 * tm + 280.46646, 360.0);
#if defined(TIMETRAVELLER)
	rc = rc < 0.0 ? (rc + 360.0) : rc;
#endif
	return radians(rc);
}

/**
 * Calculates the Sun's geometric mean anomaly.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The Sun's geometric mean anomaly in radians.
 */
static inline double
sun_geometric_mean_anomaly(double tm)
{
	return radians(pow(-0.0001537 * tm, 2.0) + 35999.05029 * tm + 357.52911);
}

/**
 * Calculates the Earth's orbit eccentricity.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The Earth's orbit eccentricity.
 */
static inline double
earth_orbit_eccentricity(double tm)
{
	return pow(-0.0000001267 * tm, 2.0) - 0.000042037 * tm + 0.016708634;
}

/**
 * Calculates the Sun's equation of the centre, the difference
 * between the true anomaly and the mean anomaly.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The Sun's equation of the centre, in radians.
 */
static inline double
sun_equation_of_centre(double tm)
{
	double a = sun_geometric_mean_anomaly(tm), rc;
	rc  = sin(1.0 * a) * (pow(-0.000014 * tm, 2.0) - 0.004817 * tm + 1.914602);
	rc += sin(2.0 * a) * (-0.000101 * tm + 0.019993);
	rc += sin(3.0 * a) * 0.000289;
	return radians(rc);
}

/**
 * Calculates the Sun's real longitudinal position.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The longitude, in radians.
 */
static inline double
sun_real_longitude(double tm)
{
	double rc = sun_geometric_mean_longitude(tm);
	return rc + sun_equation_of_centre(tm);
}

/**
 * Calculates the Sun's apparent longitudinal position.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The longitude, in radians.
 */
static inline double
sun_apparent_longitude(double tm)
{
    double rc = degrees(sun_real_longitude(tm)) - 0.00569;
    rc -= 0.00478 * sin(radians(-1934.136 * tm + 125.04));
    return radians(rc);
}

/**
 * Calculates the mean ecliptic obliquity of the Sun's
 * apparent motion without variation correction.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The uncorrected mean obliquity, in radians.
 */
static double
mean_ecliptic_obliquity(double tm)
{
	double rc = pow(0.001813 * tm, 3.0) - pow(0.00059 * tm, 2.0) - 46.815 * tm + 21.448;
	rc = 26 + rc / 60;
	rc = 23 + rc / 60;
	return radians(rc);
}

/**
 * Calculates the mean ecliptic obliquity of the Sun's
 * parent motion with variation correction.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The mean obliquity, in radians.
 */
static double
corrected_mean_ecliptic_obliquity(double tm)
{
	double rc = -1934.136 * tm + 125.04;
	rc = 0.00256 * cos(radians(rc));
	rc += degrees(mean_ecliptic_obliquity(tm));
	return radians(rc);
}

/**
 * Calculates the Sun's declination.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The Sun's declination, in radian.
 */
static inline double
solar_declination(double tm)
{
	double rc = sin(corrected_mean_ecliptic_obliquity(tm));
	rc *= sin(sun_apparent_longitude(tm));
	return asin(rc);
}

/**
 * Calculates the equation of time, the discrepancy
 * between apparent and mean solar time.
 * 
 * @param   tm  The time in Julian Centuries.
 * @return      The equation of time, in degrees.
 */
static inline double
equation_of_time(double tm)
{
	double l, e, m, y, rc;
	l = sun_geometric_mean_longitude(tm);
	e = earth_orbit_eccentricity(tm);
	m = sun_geometric_mean_anomaly(tm);
	y = corrected_mean_ecliptic_obliquity(tm);
	y = pow(tan(y / 2.0), 2.0);
	rc = y * sin(2.0 * l);
	rc += (4.0 * y * cos(2.0 * l) - 2.0) * e * sin(m);
	rc -= pow(0.5 * y, 2.0) * sin(4.0 * l);
	rc -= pow(1.25 * e, 2.0) * sin(2.0 * m);
	return 4.0 * degrees(rc);
}

/**
 * Calculates the Sun's elevation as apparent.
 * from a geographical position.
 * 
 * @param   tm         The time in Julian Centuries.
 * @param   latitude   The latitude in degrees northwards from 
 *                     the equator, negative for southwards.
 * @param   longitude  The longitude in degrees eastwards from
 *                     Greenwich, negative for westwards.
 * @return             The Sun's apparent elevation at the specified time as seen
 *                     from the specified position, measured in radians.
 */
static inline double
solar_elevation_from_time(double tm, double latitude, double longitude)
{
	double rc = julian_centuries_to_julian_day(tm);
	rc = (rc - round(rc) - 0.5) * 1440;
	rc = 720.0 - rc - equation_of_time(tm);
	rc = radians(rc / 4.0 - longitude);
	return elevation_from_hour_angle(latitude, solar_declination(tm), rc);
}


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
double
solar_elevation(double latitude, double longitude)
{
	double tm = julian_centuries();
	return errno ? -1 : degrees(solar_elevation_from_time(tm, latitude, longitude));
}

