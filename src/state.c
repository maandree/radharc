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
#include "solar.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <libgamma.h>


#define try(...)  do { if (!(__VA_ARGS__)) goto fail; } while (0)
#define t(...)    do { if   (__VA_ARGS__)  goto fail; } while (0)



/**
 * The name of the process.
 */
extern char *argv0;



/**
 * All sites.
 */
static libgamma_site_state_t *sites = NULL;

/**
 * The number of sites stored in `sites`.
 */
static size_t sites_n = 0;



/**
 * Is it daytime, night, perhaps some kind of twilight?
 * 
 * @param    elevation  The Sun's apparent elevation.
 * @return              The time of the day.
 */
enum darkness
get_darkness(double elevation)
{
	if (elevation > SOLAR_ELEVATION_SUNSET_SUNRISE)          return DAYTIME;
	if (elevation > SOLAR_ELEVATION_CIVIL_DUSK_DAWN)         return CIVIL_TWILIGHT;
	if (elevation > SOLAR_ELEVATION_NAUTICAL_DUSK_DAWN)      return NAUTICAL_TWILIGHT;
	if (elevation > SOLAR_ELEVATION_ASTRONOMICAL_DUSK_DAWN)  return ASTRONOMICAL_TWILIGHT;
	return NIGHT;
}


/**
 * Compare two display server environment strings.
 * 
 * @param   a_  One of the string.
 * @param   b_  The other string.
 * @return      -1, 0, or +1.
 */
static int
displayenvcmp(const void *a_, const void *b_)
{
#define S(V, CD)  ((V = ((CD)[1] == 'r' ? strrchr : strchr)(V, (CD)[0])))
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
	char *a = a_;
	char *b = b_;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
	char *p = strchr(a, '=') + 1;
	char *q = strchr(b, '=') + 1;
	int rc;

	if ((*p != '/') && S(p, ":r") && S(p, ".l"))  *p = '\0';  else  p = NULL;
	if ((*q != '/') && S(p, ":r") && S(p, ".l"))  *q = '\0';  else  q = NULL;

	rc = strcmp(a, b);

	if (*p)  *p = '.';
	if (*q)  *q = '.';
	return rc;
#undef S
}


/**
 * Make an display string safe for a pathname.
 * 
 * @param   str  The string.
 * @return       A safe version of the string, `NULL` on error.
 */
static char *
escape_display(const char* str)
{
	char *r, *w, *rc = malloc((2 * strlen(str) + 1) * sizeof(char));
	int s = 0;
	if (!rc)  return NULL;
	memcpy(rc, str, (strlen(str) + 1) * sizeof(char));
	for (r = w = strchr(rc, '=') + 1; *r; r++) {
		if (!s || (*r != '/')) {
			if (strchr("@=/", *r))  *w++ = '@';
			*w++ = (*r == '/' ? 's' : *r);
			s = (*r == '/');
		}
	}
	w[s ? -2 : 0] = '\0';
	return rc;
}


/**
 * The string of display servers.
 * 
 * @param   settings  The settings.
 * @return            The string, `NULL` on error.
 */
static char *
get_display_string(const struct settings *settings)
{
	const char *var, *val;
	char *r, *d = NULL, *rc = NULL, **displays;
	size_t i, n = 0, len = 0;
	int method, saved_errno;

	try (displays = malloc(settings->monitors_n * sizeof(char *)));
	for (i = 0; i < settings->monitors_n; i++)
		if ((settings->monitors_arg[i] == 'd') && strchr(settings->monitors_id[i], '='))
			len += 1 + strlen(displays[n++] = settings->monitors_id[i]);
	if (n)  goto custom;
	free(displays), displays = NULL;

	if (!libgamma_list_methods(&method, 1, 0)) {
		fprintf(stderr, "No display was found.\n"
		                "DRM support missing.\n"
		                "Can you even see?\n");
		return free(displays), errno = 0, NULL;
	}

	var = libgamma_method_default_site_variable(method);
	val = libgamma_method_default_site(method);
	if (!val)  return strdup("");
	try (d = malloc((3 + strlen(var) + strlen(val)) * sizeof(char)));
	stpcpy(stpcpy(stpcpy(stpcpy(d, "."), var), "="), val);
	try (rc = escape_display(d));
	return free(d), rc;

custom:
	qsort(displays, n, sizeof(*displays), displayenvcmp);
	try (r = rc = malloc((2 * len + 1) * sizeof(char)));
	for (i = 0; i < n; i++) {
		try (d = escape_display(displays[i]));
		r = stpcpy(stpcpy(r, "."), d), free(d), d = NULL;
	}
	free(displays);
	return rc;

fail:
	saved_errno = errno, free(rc), free(d), free(displays), errno = saved_errno;
	return NULL;
}


/**
 * Set $RADHARC_STATE.
 * 
 * @param   settings  The settings.
 * @return            0 on success, -1 on error.
 */
int
get_state_pathname(const struct settings *settings)
{
	const char *dir = getenv("XGD_RUNTIME_DIR");
	char *display = NULL;
	char *env = NULL;
	int rc = -1, saved_errno;
	try (display = get_display_string(settings));
	if (!dir || !*dir)  dir = "/run";
	try (env = malloc((strlen(dir) + sizeof("/radharc/") + strlen(display)) * sizeof(char)));
	stpcpy(stpcpy(stpcpy(env, dir), "/radharc/"), display);
	rc = setenv("RADHARC_STATE", env, 1);
fail:
	return saved_errno = errno, free(env), free(display), errno = saved_errno, rc;
}


/**
 * Parse a value for the -d option, or select preferred adjustment method.
 * 
 * @param   display  The argument for the -d option. `NULL` for the preferred adjustment method.
 * @return           The adjustment method. -1 if not found.
 */
static int
get_clut_method(const char *display)
{
	int method;
	const char *env;

	if (!display) {
		if (!libgamma_list_methods(&method, 1, 0)) {
			fprintf(stderr, "No display was found.\n"
			                "DRM support missing.\n"
			                "Can you even see?\n");
			return errno = 0, -1;
		}
		return method;
	}
	if (!strcasecmp(display, "none"))  return INT_MAX;
	if (!strcasecmp(display, "drm"))   return LIBGAMMA_METHOD_LINUX_DRM;
	if (!strchr(display, '=')) {
		fprintf(stderr, "Specified display\n"
		                "cannot be recognised.\n"
		                "Try something else.\n");
		return errno = 0, -1;
	}

	for (method = 0; method < LIBGAMMA_METHOD_COUNT; method++) {
		env = libgamma_method_default_site_variable(method);
		if (env && (strstr(display, env) == display) && (display[strlen(env)] == '='))
			return method;
	}

	fprintf(stderr, "Specified display\n"
	                "cannot be recognised.\n"
	                "Try to recompile.\n");
	return errno = 0, -1;
}


/**
 * Initialise the CLUT support.
 * 
 * @param   settings  The settings.
 * @return            0 on success, -1 on error.
 */
int
initialise_clut(const struct settings *settings)
{
	int method, error = 0, is_none = 0;
	const char *sitename_;
	char *sitename = NULL;
	void *new;
	size_t i;
	int saved_errno;

	try (sites = calloc(settings->monitors_n + 2, sizeof(*sites)));

	for (i = 0; i < settings->monitors_n; i++) {
		switch (settings->monitors_arg[i]) {
		case 'd':
			t ((method = get_clut_method(sitename_ = settings->monitors_id[i])) < 0);
			if ((is_none = (method == INT_MAX)))
				break;
			sitename_ = strchr(sitename_, '=');
			if (sitename_)  try ((sitename = strdup(sitename_ + 1)));
			t ((error = libgamma_site_initialise(sites + sites_n, method, sitename)));
			sitename = NULL;
			sites_n++;
			break;

		case 'm':
			;/* TODO -m */
			break;

		case 'e':
			;/* TODO -e */
			break;
		}
	}

	new = realloc(sites, (sites_n + 1) * sizeof(*sites));
	sites = new ? new : sites;

	return 0;
fail:
	if (error)  libgamma_perror(argv0, error), errno = 0;
	saved_errno = errno, free(sitename), errno = saved_errno;
	return -1;
}

