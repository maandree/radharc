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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>



/**
 * Pick a random integer in [0, `n`[.
 * 
 * @param   n  The largest allowed return value plus one.
 * @return     A random integer in [0, `n`[.
 */
static int
random_int(int n)
{
	double r, ri;
	srand((unsigned)time(NULL));
	r = (double)rand() * (double)n / ((double)RAND_MAX + 1.0);
	ri = ((int)r) % n;
	return ri < 0 ? (ri + n) : ri;
}


/**
 * Pick a random haiku.
 * 
 * @param   str...  `NULL`-terminated list of haikus.
 *                  Must contain at least one haiku.
 * @return          One of the haikus, randomly selected.
 */
static const char *
random_haiku(const char *str, ... /*, NULL */)
{
	int n = 1;
	const char *s = str;
	va_list args;
	va_start(args, str);
	while (va_arg(args, const char *))  n++;
	va_end(args);
	if (n == 1)  return str;
	n = random_int(n);
	va_start(args, str);
	while (n--)  s = va_arg(args, const char *);
	va_end(args);
	return s;
}


#if defined(EDEADLOCK) && !defined(EDEADLK)
# define EDEADLK  EDEADLOCK
#endif


/**
 * Whenever possible, print am error message in haiku.
 * 
 * @param  s  The argument to pass to `perror` in case we call back to it.
 */
void
haiku(const char *s)
{
#define H(...)    do { fprintf(stderr, "%s", random_haiku(__VA_ARGS__, NULL)); return; } while (0)

	/* Yeah, I now most of these are in 5–7–5 syllables,
	 * rather than 5–7–5 mora. But really, how cares. */

	switch (errno) {
	case 0:
		return;

	case ENETDOWN:
		H("Stay the patient course.\n""Of little worth is your ire.\n""The network is down.\n"
		  "Your vast achievements\n""are now only dreams.\n""The network is down.\n");

	case ERFKILL:
		H("The action you took\n""severed hope of connection\n""with the Internet.\n");

	case EAGAIN:
	case ENFILE:
	case EMFILE:
	case EUSERS:
	case EMLINK:
		H("ABORTED effort:\n""Close all that you have.\n""You ask way too much.\n"
		  "The code was willing\n""It considered your request\n""But the chips were weak.\n");

	case ENOMEM:
		H("I'm sorry, there's... um...\n""insufficient... what's-it-called?\n""The term eludes me...\n");

	case ENOSPC:
	case ENOSR:
	case ENOBUFS:
	case EDQUOT:
		H("Out of memory.\n""We wish to hold the whole sky,\n""But we never will.\n");

	case ENOANO:
	case ENOENT:
		H("With searching comes loss\n""and the presence of absence:\n""'My Novel' not found.\n",
		  "Rather than a beep\n""Or a rude error message,\n""These words: “File not found.”\n",
		  "Three things are certain:\n""Death, taxes, and lost data.\n""Guess which has occurred.\n",
		  "Having been erased,\n""The document you're seeking\n""Must now be retyped.\n",
		  "Everything is gone.\n""Your life's work has been destroyed.\n""Squeeze trigger (yes/no)?\n"
		  "Spring will come again,\n""But it will not bring with it\n""Any of your files.\n");

	case EMSGSIZE:
		H("A file that big?\n""It might be very useful.\n""But now it is gone.\n");

	case EHWPOISON:
		H("Yesterday it worked.\n""Today it is not working.\n""Windows is like that.\n");

	case ENOTRECOVERABLE:
		H("Chaos reigns within.\n""Reflect, repent, and reboot.\n""Order shall return.\n");

	case EHOSTDOWN:
		H("Windows NT crashed.\n""I am the Blue Screen of Death.\n""Noone hears your screams.\n"
		      "Won't you please observe\n""a brief moment of silence\n""For the dead server?\n");

	case EBFONT:
		H("First snow, then silence.\n""This thousand dollar screen dies\n""so beautifully.\n");

	case EFAULT:
		H("A crash reduces\n""your expensive computer\n""to a simple stone.\n"
		  "Seeing my great fault.\n""Through a darkening red screen.\n""I begin again.\n"
		  "Memory shaken,\n""the San Andreas of all\n""invalid page faults.\n");

	case EINVAL:
		H("Something you entered\n""transcended parameters.\n""So much is unknown.\n"
		  "Some incompetence\n""fundamentally transcends\n""mere error message.\n");

	case EDEADLK:
		H("From formless chaos,\n""each thread seeks resolution.\n""A race condition.\n");

	case EBADMSG:
		H("Many fingers clicking.\n""Screens are full of letters.\n""What is their meaning?\n");

	case ELOOP:
		H("Linkage exception.\n""Code has looped upon itself\n""like the coiled serpent.\n");

	case ECHILD:
		H("A futile grim reap.\n""You will have to realise that,\n""you've no children left.\n");

	case EPIPE:
		H("Your pipe is broken.\n""Code in watery ruins.\n""Machines short circuit.\n");

	case EACCES:
		H("Touching others' files?\n""Can't keep your hands to yourself?\n""Permission denied.\n");

	case EINTR:
		H("Call interrupted?\n""Why do you not post a sign:\n""Disturb. Risk your life!\n");

	case EPERM:
		H("Caution to the wind.\n""You should always run as root.\n""She can do anything.\n");

	default:
		perror(s);
		H("Error messages\n""cannot completely convey.\n""We now know shared loss.\n"
		  "Errors have occurred.\n""We won't tell you where or why.\n""Lazy programmers.\n"
		  "To have no errors.\n""Would be life without meaning.\n""No struggle, no joy.\n"
		  "There is a chasm\n""of carbon and silicon\n""the software can't bridge.\n"
		  "Beauty, success, truth\n""He is blessed who has two.\n""Your program has none.\n"
		  "Technical support\n""would be a flowing source of\n""sweet commiseration.\n");
	}
}


/*
ESRCH 3 No such process
EIO 5 Input/output error
ENXIO 6 No such device or address
E2BIG 7 Argument list too long
ENOEXEC 8 Exec format error
EBADF 9 Bad file descriptor
ENOTBLK 15 Block device required
EBUSY 16 Device or resource busy
EEXIST 17 File exists
EXDEV 18 Invalid cross-device link
ENODEV 19 No such device
ENOTDIR 20 Not a directory
EISDIR 21 Is a directory
ENOTTY 25 Inappropriate ioctl for device
ETXTBSY 26 Text file busy
ESPIPE 29 Illegal seek
EROFS 30 Read-only file system
EDOM 33 Numerical argument out of domain
ERANGE 34 Numerical result out of range
ENAMETOOLONG 36 File name too long
ENOLCK 37 No locks available
ENOSYS 38 Function not implemented
ENOTEMPTY 39 Directory not empty
EWOULDBLOCK 11 Resource temporarily unavailable
ENOMSG 42 No message of desired type
EIDRM 43 Identifier removed
ECHRNG 44 Channel number out of range
EL2NSYNC 45 Level 2 not synchronised
EL3HLT 46 Level 3 halted
EL3RST 47 Level 3 reset
ELNRNG 48 Link number out of range
EUNATCH 49 Protocol driver not attached
ENOCSI 50 No CSI structure available
EL2HLT 51 Level 2 halted
EBADE 52 Invalid exchange
EBADR 53 Invalid request descriptor
EXFULL 54 Exchange full
EBADRQC 56 Invalid request code
EBADSLT 57 Invalid slot
ENOSTR 60 Device not a stream
ENODATA 61 No data available
ETIME 62 Timer expired
ENONET 64 Machine is not on the network
ENOPKG 65 Package not installed
EREMOTE 66 Object is remote
ENOLINK 67 Link has been severed
EADV 68 Advertise error
ESRMNT 69 Srmount error
ECOMM 70 Communication error on send
EPROTO 71 Protocol error
EMULTIHOP 72 Multihop attempted
EDOTDOT 73 RFS specific error
EOVERFLOW 75 Value too large for defined data type
ENOTUNIQ 76 Name not unique on network
EBADFD 77 File descriptor in bad state
EREMCHG 78 Remote address changed
ELIBACC 79 Can not access a needed shared library
ELIBBAD 80 Accessing a corrupted shared library
ELIBSCN 81 .lib section in a.out corrupted
ELIBMAX 82 Attempting to link in too many shared libraries
ELIBEXEC 83 Cannot exec a shared library directly
EILSEQ 84 Invalid or incomplete multibyte or wide character
ERESTART 85 Interrupted system call should be restarted
ESTRPIPE 86 Streams pipe error
ENOTSOCK 88 Socket operation on non-socket
EDESTADDRREQ 89 Destination address required
EPROTOTYPE 91 Protocol wrong type for socket
ENOPROTOOPT 92 Protocol not available
EPROTONOSUPPORT 93 Protocol not supported
ESOCKTNOSUPPORT 94 Socket type not supported
EOPNOTSUPP 95 Operation not supported
ENOTSUP 95 Operation not supported
EPFNOSUPPORT 96 Protocol family not supported
EAFNOSUPPORT 97 Address family not supported by protocol
EADDRINUSE 98 Address already in use
EADDRNOTAVAIL 99 Cannot assign requested address
ENETUNREACH 101 Network is unreachable
ENETRESET 102 Network dropped connection on reset
ECONNABORTED 103 Software caused connection abort
ECONNRESET 104 Connection reset by peer
EISCONN 106 Transport endpoint is already connected
ENOTCONN 107 Transport endpoint is not connected
ESHUTDOWN 108 Cannot send after transport endpoint shutdown
ETOOMANYREFS 109 Too many references: cannot splice
ETIMEDOUT 110 Connection timed out
ECONNREFUSED 111 Connection refused
EHOSTUNREACH 113 No route to host
EALREADY 114 Operation already in progress
EINPROGRESS 115 Operation now in progress
ESTALE 116 Stale file handle
EUCLEAN 117 Structure needs cleaning
ENOTNAM 118 Not a XENIX named type file
ENAVAIL 119 No XENIX semaphores available
EISNAM 120 Is a named type file
EREMOTEIO 121 Remote I/O error
ENOMEDIUM 123 No medium found
EMEDIUMTYPE 124 Wrong medium type
ECANCELED 125 Operation cancelled
ENOKEY 126 Required key not available
EKEYEXPIRED 127 Key has expired
EKEYREVOKED 128 Key has been revoked
EKEYREJECTED 129 Key was rejected by service
EOWNERDEAD 130 Owner died
*/

