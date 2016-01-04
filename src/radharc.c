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
#include "state.h"
#include "macros.h"

#include <libred.h>
#include <libhaiku.h>



/**
 * The name of the process.
 */
char *argv0 = "radharc";



int
main(int argc, char *argv[])
{
	struct settings settings;

	t (libred_check_timetravel());
	parse_command_line(argc, argv, &settings);
	argv0 = argv0 ? argv0 : "radharc";
	t (get_state_pathname(&settings));
	t (libred_init_colour());

	return 0;

fail:
	libhaiku_perror(argv0);
	libred_term_colour();
	return 1;
}

