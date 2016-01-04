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
#include "haiku.h"
#include "macros.h"
#include <math.h>
#include <sys/stat.h>

#define HIGHEST  40000
#define LOWEST   1000
#define DELTA    100
#define EXPECTED_ELEMENTS  ((HIGHEST - LOWEST) / DELTA + 1)



static void
ciexyy_to_srgb(double x, double y, double Y, double *r, double *g, double *b)
{
#define SRGB(C)  (((C) <= 0.0031308) ? (12.92 * (C)) : ((1.0 + 0.055) * pow((C), 1.0 / 2.4) - 0.055))
	double X, Z;

	/* Convert CIE xyY to CIE XYZ. */
	X = Y * (y == 0 ? 0 : (x / y));
	Z = Y * (y == 0 ? 0 : ((1 - x - y) / y));

	/* Convert CIE XYZ to [0, 1] linear RGB. (ciexyz_to_linear) */
	*r = ( 3.240450 * X) + (-1.537140 * Y) + (-0.4985320 * Z);
	*g = (-0.969266 * X) + ( 1.876010 * Y) + ( 0.0415561 * Z);
	*b = (0.0556434 * X) + (-0.204026 * Y) + ( 1.0572300 * Z);

	/* Convert [0, 1] linear RGB to [0, 1] sRGB. */
	SRGB(*r), SRGB(*g), SRGB(*b);
}


int main(int argc, char *argv[])
{
	double xyrgb[5];
	struct stat attr;

	while (fscanf(stdin, "%lf %lf\n", xyrgb + 0, xyrgb + 1) == 2) {
		ciexyy_to_srgb(xyrgb[0], xyrgb[1], 1.0, xyrgb + 2, xyrgb + 3, xyrgb + 4);
		xwrite(STDOUT_FILENO, xyrgb, sizeof(xyrgb));
	}
	xwrite(STDOUT_FILENO, xyrgb, sizeof(xyrgb)); /* sugar */
	t (fstat(STDOUT_FILENO, &attr));
	if ((size_t)(attr.st_size) != (EXPECTED_ELEMENTS + 1) * 5 * sizeof(double))
		return 1;

	return 0;
fail:
	return haiku(argc ? *argv : "parse_10deg"), -1;
}

