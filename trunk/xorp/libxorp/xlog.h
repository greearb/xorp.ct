/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2003 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/libxorp/xlog.h,v 1.5 2003/05/21 17:47:27 pavlin Exp $
 */


#ifndef __LIBXORP_XLOG_H__
#define __LIBXORP_XLOG_H__

#include <stdio.h>
#include <stdarg.h>

#include "config.h"

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
    XLOG_LEVEL_FATAL = 0,	/* 0 */
    XLOG_LEVEL_ERROR,		/* 1 */
    XLOG_LEVEL_WARNING,		/* 2 */
    XLOG_LEVEL_INFO,		/* 3 */
    XLOG_LEVEL_TRACE,		/* 4 */
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
    XLOG_VERBOSE_MAX
} xlog_verbose_t;

/**
 * The type of add-on functions to process the log messages.
 */
typedef int (*xlog_output_func_t)(void *obj, const char *msg);

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
#define XLOG_FN(fn, fmt...)						 \
do {									 \
	char xlog_where_buf[8000];					 \
	sprintf(xlog_where_buf, "+%d %s %s", __LINE__, __FILE__, __FUNCTION__);\
	xlog_##fn(_XLOG_MODULE_NAME, xlog_where_buf, fmt);		\
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
 * Write a FATAL message to the xlog output streams and aborts the program.
 * 
 * Note that FATAL messages cannot be disabled by @ref xlog_disable().
 * 
 * @param module_name the name of the module this message applies to.
 * @param format the printf()-style format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @param ... the arguments for @ref format.
 */
void	xlog_fatal(const char *module_name, const char *where,
		   const char *format, ...) __printflike(3,4);
#define XLOG_FATAL(fmt...)	XLOG_FN(fatal, fmt)

/**
 * Write an ERROR message to the xlog output streams.
 * 
 * @param module_name the name of the module this message applies to.
 * @param format the printf()-style format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @param ... the arguments for @ref format.
 */
void	xlog_error(const char *module_name, const char *where,
		   const char *format, ...) __printflike(3,4);
#define XLOG_ERROR(fmt...)	XLOG_FN(error, fmt)

/**
 * Write a WARNING message to the xlog output streams.
 * 
 * @param module_name the name of the module this message applies to.
 * @param format the printf()-style format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @param ... the arguments for @ref format.
 */
void	xlog_warning(const char *module_name, const char *where,
		     const char *format, ...) __printflike(3,4);
#define XLOG_WARNING(fmt...)	XLOG_FN(warning, fmt)

/**
 * Write an INFO message to the xlog output streams.
 * 
 * @param module_name the name of the module this message applies to.
 * @param format the printf()-style format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @param ... the arguments for @ref format.
 */
void	xlog_info(const char *module_name, const char *where,
		  const char *format, ...) __printflike(3,4);
#define XLOG_INFO(fmt...)	XLOG_FN(info, fmt)

/**
 * XORP replacement for assert(3).
 * 
 * Note that it cannot be conditionally disabled and logs error through
 * the standard XLOG mechanism.
 * Calls XLOG_FATAL if assertion fails.
 * 
 * @param assertion the assertion condition.
 */
#define XLOG_ASSERT(assertion)						\
do {									\
	if (!(assertion))						\
		XLOG_FATAL("Assertion (%s) failed", #assertion);	\
} while (0)

/**
 * A marker that can be used in code that should never be executed.
 * 
 * Note that it cannot be conditionally disabled and logs error through
 * the standard XLOG mechanism.
 * Always calls XLOG_FATAL.
 */
#define XLOG_UNREACHABLE()						\
do {									\
	XLOG_FATAL("Internal fatal error: unreachable code reached");	\
} while (0)

/**
 * A marker that can be used to indicate code that is not yet
 * implemented and hence should not be run.
 * Always calls XLOG_FATAL.
 */
#define XLOG_UNFINISHED()						\
do {									\
	XLOG_FATAL("Internal fatal error: unfinished code reached");	\
} while (0)



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
 * There is some additional unpleasantness in this header for
 * `configure' related magic.  
 *
 * The macro CPP_SUPPORTS_VA_ARGS is defined by `configure' tests if the
 * C preprocessor supports macros with variable length arguments.  We
 * use the GNU specific (args...) syntax for variable length arguments
 * as the c9x standard (__VA_ARGS__) breaks when the preprocessor is
 * invoked via g++.
 * TODO: the magic below should be used for the other XLOG_* entries
 * as well.
 */
#ifdef CPP_SUPPORTS_GNU_VA_ARGS
#	    define XLOG_TRACE(args...)					\
		_xcond_trace_msg_long(XORP_MODULE_NAME, __FILE__, __LINE__, __FUNCTION__, args)
#else  
#	    define XLOG_TRACE						\
		_xcond_trace_entry(XORP_MODULE_NAME, __FILE__, __LINE__, __FUNCTION__),	\
		_xcond_trace_msg_short
#endif

/* Function for systems with variable argument macros */
void	_xcond_trace_msg_long(const char *module_name, 
			      const char *file,
			      int	  line, 
			      const char *fn, 
			      int	  flag, 
			      const char *format, ...) __printflike(6,7);

/* Functions for systems without variable argument macros */
void	_xcond_trace_entry(const char *module_name, const char *file,
			   int line, const char *fn);

void	_xcond_trace_msg_short(int flag, 
			       const char *format, ...) __printflike(2,3);

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
