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
#include <stdio.h>
#include <unistd.h>
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


int main(void)
{
	double xyrgb[5];
	struct stat attr;

	while (fscanf(stdin, "%lf %lf\n", xyrgb + 0, xyrgb + 1) == 2) {
		ciexyy_to_srgb(xyrgb[0], xyrgb[1], 1.0, xyrgb + 2, xyrgb + 3, xyrgb + 4);
		if (write(1, xyrgb, sizeof(xyrgb)) < (ssize_t)sizeof(xyrgb))
			return perror(""), 1;
	}
	if (write(1, xyrgb, sizeof(xyrgb)) < (ssize_t)sizeof(xyrgb)) /* sugar */
		return perror(""), 1;
	if (fstat(1, &attr))
		return perror(""), 1;
	if ((size_t)(attr.st_size) != (EXPECTED_ELEMENTS + 1) * 5 * sizeof(double))
		return 1;

	return 0;
}

