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
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include <libred.h>
#include <libgamma.h>



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
 * All partitions.
 */
static libgamma_partition_state_t *parts = NULL;

/**
 * The number of partitions stored in `parts`.
 */
static size_t parts_n = 0;



/**
 * Is it daytime, night, perhaps some kind of twilight?
 * 
 * @param    elevation  The Sun's apparent elevation.
 * @return              The time of the day.
 */
enum darkness
get_darkness(double elevation)
{
	if (elevation > LIBRED_SOLAR_ELEVATION_SUNSET_SUNRISE)          return DAYTIME;
	if (elevation > LIBRED_SOLAR_ELEVATION_CIVIL_DUSK_DAWN)         return CIVIL_TWILIGHT;
	if (elevation > LIBRED_SOLAR_ELEVATION_NAUTICAL_DUSK_DAWN)      return NAUTICAL_TWILIGHT;
	if (elevation > LIBRED_SOLAR_ELEVATION_ASTRONOMICAL_DUSK_DAWN)  return ASTRONOMICAL_TWILIGHT;
	return NIGHT;
}


/**
 * Remove the screen number for a display server instance identifier.
 * 
 * @param   s  Display server instance identifier.
 * @return     Set the value to which this pointer points to '.'. Do nothing if it is `NULL`.
 */
static char *
strip_screen(char *s)
{
#define S(V, CD)  ((V = ((CD)[1] == 'r' ? strrchr : strchr)(V, (CD)[0])))
	char *p = strchr(s, '=');
	if ((p++) && (*p != '/') && S(p, ":r") && S(p, ".l"))  *p = '\0';  else  p = NULL;
	return p;
#undef S
}


/**
 * `stpmulcpy(o, a, b, c, NULL)` is equivalent to
 * `stpcpy(stpcpy(stpcpy(o, a), b), c)`
 */
#ifdef __GNUC__
__attribute__((__sentinel__))
#endif
static char *
stpmulcpy(char *out, ... /*, NULL */)
{
	va_list args;
	char *p = out;
	const char *s;
	va_start(args, out);
	while ((s = va_arg(args, const char *)))  p = stpcpy(p, s);
	va_end(args);
	return p;
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
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
	char *a = a_;
	char *b = b_;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
	char *p = strip_screen(a);
	char *q = strip_screen(b);
	int rc;

	rc = strcmp(a, b);
	if (p)  *p = '.';
	if (q)  *q = '.';
	return rc;
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
	char *r, *w, *rc = NULL;
	int s = 0;
	xmalloc(&rc, 2 * strlen(str) + 1); strcpy(rc, str);
	for (r = w = strchr(rc, '=') + 1; *r; r++) {
		if (!s || (*r != '/')) {
			if (strchr("@=/", *r))  *w++ = '@';
			*w++ = ((s = (*r == '/')) ? 's' : *r);
		}
	}
	w[s ? -2 : 0] = '\0';
fail:
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
#define DISPLAY(VAR, D)  p = strip_screen(D); try (VAR = escape_display(D)); if (p)  *p = '.', p = NULL

	const char *var, *val;
	char *r, *p = NULL, *d = NULL, *rc = NULL, **displays;
	size_t i, n = 0, len = 0;
	int method;

	xmalloc(&displays, settings->monitors_n);
	for (i = 0; i < settings->monitors_n; i++)
		if ((settings->monitors_arg[i] == 'd') && strchr(settings->monitors_id[i], '='))
			len += 1 + strlen(displays[n++] = settings->monitors_id[i]);
	if (n)  goto custom;
	free(displays), displays = NULL;

	if (!libgamma_list_methods(&method, 1, 0)) {
		fprintf(stderr, "No display was found.\n"
		                "DRM support missing.\n"
		                "Can you even see?\n");
		return errno = 0, NULL;
	}

	var = libgamma_method_default_site_variable(method);
	val = libgamma_method_default_site(method);
	if (!val || !*val)  return strdup("");
	xmalloc(&d, 3 + strlen(var) + strlen(val));
	stpmulcpy(d, ".", var, "=", val, NULL);
	DISPLAY(rc, d);
	return free(d), rc;

custom:
	qsort(displays, n, sizeof(*displays), displayenvcmp);
	xmalloc(&rc, 2 * len + 1);
	for (r = rc, i = 0; i < n; i++) {
		DISPLAY(d, displays[i]);
		r = stpmulcpy(r, ".", d, NULL), free(d), d = NULL;
	}
	free(displays);
	return rc;

fail:
	if (p)  *p = '.';
	CLEANUP(free(rc), free(d), free(displays));
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
	int rc = -1;
	try (display = get_display_string(settings));
	if (!dir || !*dir)  dir = "/run";
	xmalloc(&env, strlen(dir) + sizeof("/radharc/") + strlen(display));
	stpmulcpy(env, dir, "/radharc/", display, NULL);
	rc = setenv("RADHARC_STATE", env, 1);
fail:
	CLEANUP(free(env), free(display));
	return rc;
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
#define HAIKU(TEXT)    t ((msg = (TEXT)))
#define TEST(STR, ID)  if (!strcasecmp(display, STR))  return ID

	const char *env, *msg;
	int method;

	/* Default? */
	if (!display) {
		if (!libgamma_list_methods(&method, 1, 0))
			HAIKU("No display was found.\n"
			      "DRM support missing.\n"
			      "Can you even see?\n");
		return method;
	}

	/* Single-sited? */
	TEST("none", INT_MAX);
	TEST("drm", LIBGAMMA_METHOD_LINUX_DRM);

	/* Unrecognised single-sited? */
	if (!strchr(display, '='))
		HAIKU("Specified display\n"
		      "cannot be recognised.\n"
		      "Try something else.\n");

	/* Multi-sited? */
	for (method = 0; method < LIBGAMMA_METHOD_COUNT; method++) {
		env = libgamma_method_default_site_variable(method);
		if (env && (strstr(display, env) == display) && (display[strlen(env)] == '='))
			return method;
	}

	/* Unrecognised multi-sited. */
	HAIKU("Specified display\n"
	      "cannot be recognised.\n"
	      "Try to recompile.\n");

fail:
	fprintf(stderr, "%s", msg);
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
#define NONE_METHOD  (method == INT_MAX)

	int method = 0, error = 0;
	const char *sitename_;
	char *sitename = NULL;
	size_t i, j, parts_off = 0;
	libgamma_site_state_t site;

	xcalloc(&sites, settings->monitors_n + 1);

	for (i = 0; i < settings->monitors_n; i++) {
		switch (settings->monitors_arg[i]) {
		case 'd':
			parts_off = parts_n;
			t ((method = get_clut_method(sitename_ = settings->monitors_id[i])) < 0);
			if (NONE_METHOD)  break;
			sitename_ = strchr(sitename_, '=');
			xstrdup(&sitename, sitename_ ? sitename_ + 1 : NULL);
			t ((error = libgamma_site_initialise(sites + sites_n, method, sitename)));
			sitename = NULL, site = sites[sites_n++];
			xrealloc(&parts, parts_n + site.partitions_available);
			for (j = 0; j < site.partitions_available; j++) {
				t ((error = libgamma_partition_initialise(parts + parts_n, &site, j)));
				parts_n++;
			}
			break;

		case 'm':
			if (NONE_METHOD)  break;
			/* TODO -m */
			break;

		case 'e':
			if (NONE_METHOD)  break;
			/* TODO -e */
			break;
		}
	}

	SHRINK(&sites, sites_n + 1);

	return 0;
fail:
	if (error)  libgamma_perror(argv0, error), errno = 0;
	CLEANUP(free(sitename));
	return -1;
}

