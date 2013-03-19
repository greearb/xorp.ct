/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2012 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */


#ifndef __LIBXORP_XLOG_H__
#define __LIBXORP_XLOG_H__

#include "libxorp/xorp.h"


// TODO:  Make this configurable in scons
#define XORP_LOG_PRINT_USECS

/*
 * The following defines and notes we defined __printfike if it does not
 * already exist.  The expansion of this macro uses a gcc extension to
 * check format strings.
 */
#ifndef __printflike
#ifdef __GNUC__
#define __printflike(fmt,va1) __attribute__((__format__(printf, fmt, va1)))
#define __libxorp_xlog_defined_printflike
#else
#define __printflike(fmt, va1)
#define __libxorp_xlog_defined_printflike
#endif /* __GNUC__ */
#endif /* __printflike */

/**
 * @short XORP logging functions.
 *
 * The xlog functions provide a similar role to syslog.  The log
 * messages may be output to multiple output streams simulataneously.
 */

#   ifdef __cplusplus
extern "C" {
#   endif

/**
 * The log levels. Typically used only by @ref xlog_enable()
 * and @ref xlog_disable()
 */
typedef enum {
    XLOG_LEVEL_MIN = 0,		/* 0 */
    XLOG_LEVEL_FATAL = 0,	/* 0 */
    XLOG_LEVEL_ERROR,		/* 1 */
    XLOG_LEVEL_WARNING,		/* 2 */
    XLOG_LEVEL_INFO,		/* 3 */
    XLOG_LEVEL_TRACE,		/* 4 */
    XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE, /* 5 */	/* XXX: temp, to be removed; see Bugzilla entry 795 */
    XLOG_LEVEL_MAX
} xlog_level_t;

/**
 * The messages verbose level. Typically used only by @ref xlog_set_verbose()
 * and @ref xlog_level_set_verbose()
 */
typedef enum {
    XLOG_VERBOSE_LOW = 0,	/* 0 */
    XLOG_VERBOSE_MEDIUM,	/* 1 */
    XLOG_VERBOSE_HIGH,		/* 2 */
    XLOG_VERBOSE_RTRMGR_ONLY_NO_PREAMBLE, /* 3 */ /* XXX: temp, to be removed; see Bugzilla entry 795 */
    XLOG_VERBOSE_MAX
} xlog_verbose_t;

/* Don't modify this directly outside the xlog.[ch] files */
extern int xlog_level_enabled[XLOG_LEVEL_MAX];

/**
 * The type of add-on functions to process the log messages.
 */
typedef int (*xlog_output_func_t)(void *obj, xlog_level_t level,
    const char *msg);

/**
 * A macro used for internal purpose to generate the appropriate xlog code
 */
#if (!defined XORP_MODULE_NAME) && (!defined XORP_LIBRARY_NAME)
#error  You MUST have in your local directory a file like
#error  "foo_module.h" that has defined inside the module
#error  or library name. E.g.: #define XORP_MODULE_NAME "BGP"
#error  or #define XORP_LIBRARY_NAME
#error  This "foo_module.h" must be the first included by
#error  each *.c and *.cc file.
#endif

#ifdef XORP_LIBRARY_NAME
#  define _XLOG_MODULE_NAME XORP_LIBRARY_NAME
#else
#  define _XLOG_MODULE_NAME XORP_MODULE_NAME
#endif

#define XLOG_FN(__log_level, fmt...)					\
    do {								\
	if (xlog_level_enabled[__log_level])				\
	    _xlog_with_level(__log_level, _XLOG_MODULE_NAME, __LINE__,	\
			     __FILE__, __FUNCTION__, fmt);		\
    } while (0)

/**
 * Initialize the log utility.
 *
 *  As part of the initialization, the preamble string will be set to
 * <@ref process_name><@ref preamble_message>
 * Use in preference to @ref xlog_set_preamble which will be removed.
 *
 * @param argv0 the path of the process executable from which the program name
 * will be extract to become part of the preamble string.
 *
 * @param preamble_message a string that will become part of the
 * preamble string.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_init(const char *argv0, const char *preamble_message);

/**
 * Gracefully exit logging.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_exit(void);

/**
 * Start logging.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_start(void);

/**
 * Stop logging.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_stop(void);

/**
 * Check if xlog is running.
 *
 * @return non-zero if xlog is running, otherwise 0.
 */
int	xlog_is_running(void);

/**
 * Enable logging for messages of a given type (@ref log_level_t).
 *
 * By default, all message types are enabled.
 *
 * @param log_level the message type @ref xlog_level_t
 * (e.g., @ref XLOG_LEVEL_WARNING) to enable the logging for.
 * @return 0 on success, otherwise -1.
 */
int	xlog_enable(xlog_level_t log_level);

/**
 * Disable logging for messages of a given type (@ref log_level_t).
 *
 * Note: @ref XLOG_LEVEL_FATAL cannot be disabled.
 *
 * @param log_level the message type @ref xlog_level_t
 * (e.g., @ref XLOG_LEVEL_WARNING) to disable the logging for.
 * @return 0 on success, otherwise -1.
 */
int	xlog_disable(xlog_level_t log_level);

/**
 * Set the preamble string for the log entries.
 *
 * @param text the preamble string, or NULL if no preamble.
 */
void	xlog_set_preamble(const char *text);

/**
 * Get process name as set with xlog_init.
 *
 * @return pointer to name on success, NULL otherwise.
 */
const char* xlog_process_name(void);

/**
 * Set the level of verbosity (@ref xlog_verbose_t) for the log entries.
 *
 * Applies for all type of messages except for @ref XLOG_LEVEL_FATAL
 * which always is set to the most verbose level.
 *
 * @param verbose_level the level of verbosity @ref xlog_verbose_t
 * (higher is more verbose).
 */
void	xlog_set_verbose(xlog_verbose_t verbose_level);

/**
 * Set the level of verbosity (@ref xlog_verbose_t) for the log entries
 * of messages of a given type (@ref xlog_level_t).
 *
 * Note: @ref XLOG_LEVEL_FATAL verbosity cannot be changed, and is
 * always set to the most verbose level.
 *
 * @param log_level the message type @ref xlog_level_t to set the
 * verbosity of.
 * @param verbose_level the level of verbosity @ref xlog_verbose_t
 * (higher is more verbose).
 */
void	xlog_level_set_verbose(xlog_level_t log_level,
			       xlog_verbose_t verbose_level);

/**
 * Add a file descriptor to the set of output streams.
 *
 * @param fp the file descriptor to add to the set of output streams.
 * @return 0 on success, otherwise -1.
 */
int 	xlog_add_output(FILE *fp);

/**
 * Remove a file descriptor from the set of output streams.
 *
 * @param fp the file descriptor to remove from the set of output streams.
 * @return 0 on success, otherwise -1.
 */
int	xlog_remove_output(FILE *fp);

/**
 * Add a processing function and an object to the set of output streams.
 *
 * @param func the function to add to the set of output streams.
 * @param obj the object to supply @ref func with when called.
 *
 * @return 0 on success, otherwise -1.
 */
int 	xlog_add_output_func(xlog_output_func_t func, void *obj);

/**
 * Add a channel which goes to the X/Open compatible system log aka syslog.
 *
 * @param syslogspec the syslog facility and priority as a string
 *                   "facility.priority"
 * @return 0 on success, otherwise -1.
 */
int	xlog_add_syslog_output(const char *syslogspec);

/**
 * Remove a processing function and an object from the set of output streams.
 *
 * @param func the function to remove from the set of output streams.
 * @param obj the object that @ref func was supplied with.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_remove_output_func(xlog_output_func_t func, void *obj);

/**
 * Add default output stream to list of output streams.
 *
 * XXX: right now the default is '/dev/stderr', but it should eventually be:
 * `/dev/console' if the process has sufficient permissions,
 * and `/dev/stderr' otherwise.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_add_default_output(void);

/**
 * Remove the default output stream from the set of output streams.
 *
 * @return 0 on success, otherwise -1.
 */
int	xlog_remove_default_output(void);

/**
 * Write a message to the xlog output streams.
 *
 * @param module_name the name of the module this message applies to.
 * @param line the line number in the file this message applies to.
 * @param file the file name this message applies to.
 * @param function the function name this message applies to.
 * @param format the printf()-style format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @param ... the arguments for @ref format.
 */
void _xlog_with_level(int log_level,
		      const char *module_name,
		      int line,
		      const char *file,
		      const char *function,
		      const char *format, ...) __printflike(6,7);

#ifdef L_FATAL
# define XLOG_FATAL(fmt...) \
    do {							\
	_xlog_with_level(XLOG_LEVEL_FATAL, _XLOG_MODULE_NAME,	\
			 __LINE__, __FILE__, __FUNCTION__,	\
			 fmt);					\
	assert(0);						\
    } while (0)
#else
# define XLOG_FATAL(fmt...)	assert(0);
#endif

#ifdef L_ERROR
 #define XLOG_ERROR(fmt...)	XLOG_FN(XLOG_LEVEL_ERROR, fmt)
#else
 #define XLOG_ERROR(fmt...)	do{}while(0)
#endif     

#ifdef L_WARNING
 #define XLOG_WARNING(fmt...)	XLOG_FN(XLOG_LEVEL_WARNING, fmt)
#else
 #define XLOG_WARNING(fmt...)	do{}while(0)
#endif

#ifdef L_INFO
 #define XLOG_INFO(fmt...)	XLOG_FN(XLOG_LEVEL_INFO, fmt)
#else
 #define XLOG_INFO(fmt...)  do{}while(0)
#endif

/**
 * Write a message without a preamble to the xlog output streams.
 */
#ifdef L_OTHER
 #define XLOG_RTRMGR_ONLY_NO_PREAMBLE(fmt...)	XLOG_FN(XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE, fmt)
#else
 #define XLOG_RTRMGR_ONLY_NO_PREAMBLE(fmt...)	do{}while(0)
#endif    

#ifdef NO_ASSERT
#define XLOG_ASSERT(assertion) do {} while(0)
#else
#ifdef L_ASSERT
/**
 * XORP replacement for assert(3).
 *
 * Note that it cannot be conditionally disabled and logs error through
 * the standard XLOG mechanism.
 *
 * @param assertion the assertion condition.
 */
 #define XLOG_ASSERT(assertion)						\
 do {									\
     if (!(assertion)) {						\
	 XLOG_FATAL(#assertion);					\
     }									\
 } while (0)
#else
# define XLOG_ASSERT(assertion)	assert(assertion)
#endif /* L_ASSERT */
#endif /* NO_ASSERT */

/**
 * A marker that can be used to indicate code that should never be executed.
 *
 * Note that it cannot be conditionally disabled and logs error through
 * the standard XLOG mechanism.
 * Always calls XLOG_FATAL.
 */
#ifdef L_OTHER
 #define XLOG_UNREACHABLE()						\
 do {									\
 	XLOG_FATAL("Internal fatal error: unreachable code reached");	\
 	exit(1);	/* unreached: keep the compiler happy */	\
 } while (0)
#else
# define XLOG_UNREACHABLE() assert(0)
#endif

/**
 * A marker that can be used to indicate code that is not yet
 * implemented and hence should not be run.
 *
 * Note that it cannot be conditionally disabled and logs error through
 * the standard XLOG mechanism.
 * Always calls XLOG_FATAL.
 */
#ifdef L_OTHER
 #define XLOG_UNFINISHED()						\
 do {									\
 	XLOG_FATAL("Internal fatal error: unfinished code reached");	\
 	exit(1);	/* unreached: keep the compiler happy */	\
 } while (0)
#else
# define XLOG_UNFINISHED()  assert(0)
#endif

/*
 * The macros below define the XLOG_TRACE(), the macro responsible for
 * generating trace messages.  It takes the same arguments as
 * printf(), except that the very first argument is a boolean
 * that defines whether the trace message will be output.
 * Note that a trailing newline is added if none is present.
 * E.g.,
 *
 *		XLOG_TRACE(cond_variable, "The number is %d", 5);
 *
 */
#ifdef L_TRACE
#	define XLOG_TRACE(__flg, args...)				\
    do {								\
	if (__flg && xlog_level_enabled[XLOG_LEVEL_TRACE]) {							\
	    _xlog_with_level(XLOG_LEVEL_TRACE, XORP_MODULE_NAME,	\
			     __LINE__, __FILE__, __FUNCTION__,	\
			     args);					\
	}								\
    } while(0)
#else
#	define XLOG_TRACE(args...) do{} while(0)
#endif


/**
 * Compute the current local time and return it as a string.
 *
 * The return string has the format:
 *   Year/Month/Day Hour:Minute:Second.Microsecond
 *   Example: 2002/02/05 20:22:09.808632
 *   Note that the returned string uses statically allocated memory,
 *   and does not need to be de-allocated.
 *
 * @return a statically allocated string with the local time using
 * the format described above.
 */
const char *xlog_localtime2string(void);

/**
 * A local implementation of vasprintf(3).
 *
 * If vasprintf(3) is available, it is called instead.
 *
 * @param ret a pointer to the string pointer to store the result.
 * @param format the printf(3)-style format.
 * @param ap the variable arguments for @ref format.
 *
 * @return (From FreeBSD vasprintf(3) manual page):
 * The number of characters printed (not including the trailing '\0'
 * used to end output to strings). Also, set the value pointed to by
 * ret to be a pointer to a buffer sufficiently large to hold the
 * formatted string.  This pointer should be passed to free(3) to
 * release the allocated storage when it is no longer needed. If
 * sufficient space cannot be allocated, will return -1 and set @ref ret
 * to be a NULL pointer.
 */
int x_vasprintf(char **ret, const char *format, va_list ap);

/**
 * A local implementation of asprintf(3).
 *
 * @param ret a pointer to the string pointer to store the result.
 * @param format the printf(3)-style format.
 * @param ... the variable arguments for @ref format.
 *
 * @return (From FreeBSD asprintf(3) manual page):
 * The number of characters printed (not including the
 * trailing '\0' used to end output to strings). Also, set ret to be
 * a pointer to a buffer sufficiently large to hold the formatted string.
 * This pointer should be passed to free(3) to release the allocated
 * storage when it is no longer needed. If sufficient space cannot
 * be allocated, will return -1 and set @ref ret to be a NULL pointer.
 */
int x_asprintf(char **ret, const char *format, ...);

/* Undefine __printflike if we defined it */
#ifdef __libxorp_xlog_defined_printflike
#undef __libxorp_xlog_defined_printflike
#undef __printflike
#endif /* __libxorp_xlog_defined_printflike */

#   ifdef __cplusplus
}
#   endif

#endif /* __LIBXORP_XLOG_H__ */
