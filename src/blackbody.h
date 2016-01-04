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



/**
 * The highest colour temperature in the table.
 */
#define HIGHEST_TEMPERATURE  40000

/**
 * The lowest colour temperature in the table.
 */
#define LOWEST_TEMPERATURE  1000

/**
 * The temperature difference between the colours in the table.
 */
#define DELTA_TEMPERATURE  100



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
ciexyy_to_srgb(double x, double y, double Y, double *r, double *g, double *b);

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
int get_colour(int fd, long int temp, double *r, double *g, double *b);

