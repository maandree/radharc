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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>



#define try(...)      do { if (!(__VA_ARGS__)) goto fail; } while (0)
#define t(...)        do { if   (__VA_ARGS__)  goto fail; } while (0)
#define CLEANUP(...)  do { int cleanup__ = errno; __VA_ARGS__; errno = cleanup__; } while (0)

#define xstrdup(outp, ...)  \
do {  \
	const char *xstrdup__ = (__VA_ARGS__);  \
	if (xstrdup__)  \
		try (*(outp) = strdup(xstrdup__));  \
	else  \
		*(outp) = NULL;  \
} while (0)

#define xpread(fd, buf, len, off)   t  (pread(fd, buf, len, off) < (ssize_t)(len))
#define xpwrite(fd, buf, len, off)  t (pwrite(fd, buf, len, off) < (ssize_t)(len))
#define xread(fd, buf, len)         t   (read(fd, buf, len)      < (ssize_t)(len))
#define xwrite(fd, buf, len)        t  (write(fd, buf, len)      < (ssize_t)(len))

#define xcalloc(outp, num)   try (*(outp) = calloc(num, sizeof(**(outp))))
#define xmalloc(outp, num)   try (*(outp) = malloc((num) * sizeof(**(outp))))
#define xrealloc(outp, num)  \
do {  \
	size_t n__ = (num);  \
	void *new__ = realloc(*(outp), n__ * sizeof(**(outp)));  \
	t (n__ && !new__);  \
	*(outp) = new__;  \
} while (0)

#define SHRINK(outp, num)  \
do {  \
	void *new__ = realloc(*(outp), (num) * sizeof(**(outp)));  \
	if (new__)  *(outp) = new__;  \
} while (0)

