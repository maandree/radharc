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

