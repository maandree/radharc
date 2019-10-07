/* See LICENSE file for copyright and license details. */
#include <libcoopgamma.h>

#include <inttypes.h>



/**
 * Value of `default_priority` that indicates
 * that there is no default priority
 */
#define NO_DEFAULT_PRIORITY INT64_MAX



/**
 * X-macro that list all gamma ramp types
 * 
 * X will be expanded with 4 arguments:
 * 1)  The libcoopgamma constant that identifies the type
 * 2)  The member in `union libcoopgamma_ramps` that
 *     corresponds to the type
 * 3)  The max value for the ramp stops
 * 4)  The type of the ramp stops
 */
#define LIST_DEPTHS\
	X(LIBCOOPGAMMA_UINT8,  u8,  UINT8_MAX,   uint8_t)\
	X(LIBCOOPGAMMA_UINT16, u16, UINT16_MAX,  uint16_t)\
	X(LIBCOOPGAMMA_UINT32, u32, UINT32_MAX,  uint32_t)\
	X(LIBCOOPGAMMA_UINT64, u64, UINT64_MAX,  uint64_t)\
	X(LIBCOOPGAMMA_FLOAT,  f,   ((float)1),  float)\
	X(LIBCOOPGAMMA_DOUBLE, d,   ((double)1), double)



/**
 * Information (except asynchronous call context)
 * required to update the gamma ramps on a CRTC.
 */
typedef struct filter_update
{
	/**
	 * The filter to update
	 * 
	 * `.filter.crtc`, `.filter.class`, and
	 * `.filter.priority` (unless `default_priority`
	 * is `NO_DEFAULT_PRIORITY`), `.filter.depth`
	 * are preconfigured, and `.filter.ramps`
	 * is preinitialised and preset to an
	 * identity ramp
	 */
	libcoopgamma_filter_t filter;

	/**
	 * The index of the CRTC
	 */
	size_t crtc;

	/**
	 * Has the update been synchronised?
	 */
	int synced;

	/**
	 * Did the update fail?
	 */
	int failed;

	/**
	 * Error description if `.failed` is true
	 */
	libcoopgamma_error_t error;

	/**
	 * If zero, the ramps in `.filter` shall
	 * neither be modified nor freed
	 */
	int master;

	/**
	 * 0-terminated list of elements in
	 * `.crtc_updates` which shares gamma
	 * ramps with this instance
	 * 
	 * This will only be set if `.master`
	 * is true
	 */
	size_t *slaves;

} filter_update_t;



/**
 * The process's name
 */
extern const char *argv0;

/**
 * The libcoopgamma context
 */
extern libcoopgamma_context_t cg;

/**
 * The names of the selected CRTC:s
 */
extern char **crtcs;

/**
 * Gamma ramp updates for each CRTC
 */
extern filter_update_t *crtc_updates;

/**
 * CRTC and monitor information about
 * each selected CRTC and connect monitor
 */
extern libcoopgamma_crtc_info_t *crtc_info;

/**
 * The number of selected CRTC:s
 */
extern size_t crtcs_n;

/**
 * The number of filters
 */
extern size_t filters_n;



/**
 * The default filter priority for the program
 */
extern const int64_t default_priority;

/**
 * The default class for the program
 */
extern char default_class[];

/**
 * Class suffixes
 */
extern const char *const *class_suffixes;



/**
 * Make elements in `crtc_updates` slaves where appropriate
 * 
 * @return  Zero on success, -1 on error
 */
int make_slaves(void);

/**
 * Update a filter and synchronise calls
 * 
 * @param   index    The index of the CRTC
 * @param   timeout  The number of milliseconds a call to `poll` may block,
 *                   -1 if it may block forever
 * @return           1: Success, no pending synchronisations
 *                   0: Success, with still pending synchronisations
 *                   -1: Error, `errno` set
 *                   -2: Error, `cg.error` set
 * 
 * @throws  EINTR   Call to `poll` was interrupted by a signal
 * @throws  EAGAIN  Call to `poll` timed out
 */
int update_filter(size_t index, int timeout);

/**
 * Synchronised calls
 * 
 * @param   timeout  The number of milliseconds a call to `poll` may block,
 *                   -1 if it may block forever
 * @return           1: Success, no pending synchronisations
 *                   0: Success, with still pending synchronisations
 *                   -1: Error, `errno` set
 *                   -2: Error, `cg.error` set
 * 
 * @throws  EINTR   Call to `poll` was interrupted by a signal
 * @throws  EAGAIN  Call to `poll` timed out
 */
int synchronise(int timeout);


/**
 * Print usage information and exit
 */
#if defined(__GNUC__)
__attribute__((__noreturn__))
#endif
extern void usage(void);

/**
 * Handle a command line option
 * 
 * @param   opt  The option, it is a NUL-terminate two-character
 *               string starting with either '-' or '+', if the
 *               argument is not recognised, call `usage`. This
 *               string will not be "-M", "-S", "-c", "-p", or "-R".
 * @param   arg  The argument associated with `opt`,
 *               `NULL` there is no next argument, if this
 *               parameter is `NULL` but needed, call `usage`
 * @return       0 if `arg` was not used,
 *               1 if `arg` was used,
 *               -1 on error
 */
#if defined(__GNUC__)
__attribute__((__nonnull__(1)))
#endif
extern int handle_opt(char *opt, char *arg);

/**
 * This function is called after the last
 * call to `handle_opt`
 * 
 * @param   argc  The number of unparsed arguments
 * @param   argv  `NULL` terminated list of unparsed arguments
 * @param   prio  The argument associated with the "-p" option
 * @return        Zero on success, -1 on error
 */
#if defined(__GNUC__)
__attribute__((__nonnull__(2)))
#endif
extern int handle_args(int argc, char *argv[], char *prio);

/**
 * The main function for the program-specific code
 * 
 * @return  0: Success
 *          -1: Error, `errno` set
 *          -2: Error, `cg.error` set
 *          -3: Error, message already printed
 */
extern int start(void);
