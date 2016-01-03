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
 * The name of the process.
 */
extern char *argv0;



/**
 * Start command line parsing.
 * 
 * `argv` and `argc` must be available as given to `main`.
 * The global variable `argv0` must be declare (not necessary
 * the the same file) and will be set to `argv[0]`.
 * 
 * Only short options and "--" are supported. Short options
 * may start with a '+', in which case `plus` will be 1.
 * 
 * @example
 *     ARGBEGIN {
 *     case 'a':  printf("%s\n", plus ? "+a" : "-a");                        break;
 *     case 'b':  if (plus) usage(1); printf("-b\n");                        break;
 *     case 'c':  if (plus) printf("+c\n"); else printf("-c %s\n", ARGF());  break;
 *     default:   usage(1);                                                  break;
 *     } ARGEND;
 */
#define ARGBEGIN  \
do {  \
	for (argv0 = *argv++, argc -= !!argv0; *argv; argv++, argc--) {  \
		int plus = argv[0][0] == '+', next__ = 0;  \
		if (((argv[0][0] != '-') && !plus) || (argv[0][1] == (plus ? '+' : '-'))) {  \
			if ((argv[0][0] == argv[0][1]) && (argv[0][1] == '-')  && (argv[0][2] == '\0'))  \
				argc--, argv++;  \
			break;  \
		}  \
		for (argv[0]++; argv[0][0] && !next__;) {  \
			switch (*(argv[0])++)

/**
 * End of command line parsing,
 */
#define ARGEND  \
		}  \
	}  \
} while (0)

/**
 * Get the argument of the current option.
 * Do not use more once per option.
 * 
 * The function `noreturn void usage(int)` must be available.
 * `usage` shall exit if its argument is `1`.
 * 
 * @return  The argument of the current option.
 */
#define ARGF()  \
	(next__ = 1, argv[0][0] ? (argv[0])  \
	 : (argv++, (argv[0]    ? (argc--, argv[0])  \
	                        : (usage(1), NULL))))

