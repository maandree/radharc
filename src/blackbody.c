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
#include "blackbody.h"
#include "macros.h"
#include <math.h>
#include <errno.h>



/**
 * Convert from CIE xyY to [0, 1] sRGB.
 * 
 * @param  x  The 'x' component.
 * @param  y  The 'y' component.
 * @param  Y  The 'Y' component.
 * @param  r  Output parameter for the “red” value.
 *            (Seriously, sRGB red is orange, just look at it fullscreen.)
 * @param  g  Output parameter for the green value.
 * @param  b  Output parameter for the blue value.
 */
void
ciexyy_to_srgb(double x, double y, double Y, double *r, double *g, double *b)
{
#define SRGB(C)  (((C) <= 0.0031308) ? (12.92 * (C)) : ((1.0 + 0.055) * pow((C), 1.0 / 2.4) - 0.055))
	double X, Z;

	/* Convert CIE xyY to CIE XYZ. */
	X = Y * (y == 0.0 ? 0.0 : (x / y));
	Z = Y * (y == 0.0 ? 0.0 : ((1.0 - x - y) / y));

	/* Convert CIE XYZ to [0, 1] linear RGB. (ciexyz_to_linear) */
	*r = ( 3.240450 * X) + (-1.537140 * Y) + (-0.4985320 * Z);
	*g = (-0.969266 * X) + ( 1.876010 * Y) + ( 0.0415561 * Z);
	*b = (0.0556434 * X) + (-0.204026 * Y) + ( 1.0572300 * Z);

	/* Convert [0, 1] linear RGB to [0, 1] sRGB. */
	SRGB(*r), SRGB(*g), SRGB(*b);
}


/**
 * Perform linear interpolation (considered very good)
 * between the CIE xyY values for two colour temperatures
 * and convert the result to sRGB. The two colours should
 * be the closest below the desired colour temperature,
 * and the closest above the desired colour temperature.
 * 
 * @param  x1    The 'x' component for the low colour.
 * @param  y1    The 'y' component for the low colour.
 * @param  x2    The 'x' component for the high colour.
 * @param  y2    The 'y' component for the high colour.
 * @param  temp  The desired colour temperature.
 * @param  r     Output parameter for the “red” value.
 * @param  g     Output parameter for the green value.
 * @param  b     Output parameter for the blue value.
 */
static void
interpolate(double x1, double y1, double x2, double y2, double temp, double *r, double *g, double *b)
{
	double weight = fmod(temp, (double)DELTA_TEMPERATURE) / (double)DELTA_TEMPERATURE;
	double x = x1 * (1 - weight) + x2 * weight;
	double y = y1 * (1 - weight) + y2 * weight;
	ciexyy_to_srgb(x, y, 1.0, r, g, b);
}


/**
 * Get the [0, 1] sRGB values of a colour temperature.
 * 
 * @param   fd    File descriptor for the colour table.
 * @param   temp  The desired colour temperature.
 * @param   r     Output parameter for the “red” value.
 * @param   g     Output parameter for the green value.
 * @param   b     Output parameter for the blue value.
 * @return        0 on succeess, -1 on error.
 * 
 * @throws  0     The file did not have the expected size.
 * @throws  EDOM  The selected temperature is below 1000 K.
 */
int
get_colour(int fd, long int temp, double *r, double *g, double *b)
{
	double values[10]; /* low:x,y,r,g,b + high:x,y,r,g,b */
	off_t offset;
	double max;

	/* We do not have any values for above 40 000 K, but
	 * the differences will be unnoticeable, perhaps even
	 * unencodeable. */
	if (temp > HIGHEST_TEMPERATURE)  temp = HIGHEST_TEMPERATURE;
	/* Things do not glow below 1000 K. Yes, fire is hot! */
	if (temp < LOWEST_TEMPERATURE)   t ((errno = EDOM));

	/* Read table. */
	offset = ((off_t)temp - LOWEST_TEMPERATURE) / DELTA_TEMPERATURE;
	offset *= (off_t)(sizeof(values) / 2);
	errno = 0; xpread(fd, values, sizeof(values), offset);

	/* Get colour. */
	if (temp % DELTA_TEMPERATURE)
		interpolate(values[0], values[1], values[6], values[7], (double)temp, r, g, b);
	else
		*r = values[2], *g = values[3], *b = values[4];

	/* Adjust colours for use. */
	max = fmax(fmax(fabs(*r), fabs(*g)), fabs(*b));
	if (max != 0)  *r /= max, *g /= max, *b /= max;
	*r = *r > 0.0 ? *r : 0.0;
	*g = *g > 0.0 ? *g : 0.0;
	*b = *b > 0.0 ? *b : 0.0;

	return 0;
fail:
	return -1;
}

