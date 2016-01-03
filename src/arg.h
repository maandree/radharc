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

