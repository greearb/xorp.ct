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
 * $XORP: xorp/libxorp/debug.h,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $
 */

#ifndef __LIBXORP_DEBUG_H__
#define __LIBXORP_DEBUG_H__

#include "config.h"
/*
 * This file defines the debug_msg(), the macro responsible for
 * generating debug messages.  It takes the same arguments as
 * printf(), e.g.
 *
 *		debug_msg("The number is %d\n", 5);
 *
 * Provided DEBUG_LOGGING is defined before this file is included,
 * debugging messages are enabled for a particular source file.  If
 * `configure' is run with optional flag --enable-debug-msgs, then all
 * debug_msg statements in built files will be active.  This latter option
 * turns on a huge amount of debug output statements which may not be desired.
 *
 * An additional macro with pseudo prototype
 *		debug_msg_indent(uint32_t n);
 * also exists.  It sets the indentation after the debug_msg line and file
 * preamble to n spaces.
 *
 * About this file
 * ---------------
 * 	
 * There is some additional unpleasantness in this header for
 * `configure' related magic.  
 *
 * The macro CPP_SUPPORTS_VA_ARGS is defined by `configure' tests if the
 * C preprocessor supports macros with variable length arguments.  We
 * use the GNU specific (args...) syntax for variable length arguments
 * as the c9x standard (__VA_ARGS__) breaks when the preprocessor is
 * invoked via g++.
 *
 * The macro DEBUG_PRINT_FUNCTION_NAME is intended to be defined as
 * the result of a `configure' command line option for people who like
 * this feature.  
 */

/*
 * The following defines and notes we defined __printfike if it does not
 * already exist.  The expansion of this macro uses a gcc extension to 
 * check format strings.
 */
#ifndef __printflike
#ifdef __GNUC__
#define __printflike(fmt,va1) __attribute__((__format__(printf, fmt, va1)))
#define __libxorp_debug_defined_printflike
#else
#define __printflike(fmt, va1)
#define __libxorp_debug_defined_printflike
#endif
#endif /* __printflike */

/*
 * `configure' defines DEBUG_LOGGING_GLOBAL, user may define DEBUG_LOGGING.
 */
#if defined(DEBUG_LOGGING) || defined(DEBUG_LOGGING_GLOBAL)
#   if !defined(DEBUG_LOGGING)
#       define DEBUG_LOGGING
#   elif !defined(DEBUG_LOGGING_GLOBAL)
#       define DEBUG_LOGGING_GLOBAL
#   endif /* DEBUG_LOGGING */
#   ifdef CPP_SUPPORTS_GNU_VA_ARGS
#	ifdef DEBUG_PRINT_FUNCTION_NAME
#	    define debug_msg(args...)					\
			_xdebug_msg_long(__FILE__,__LINE__,__FUNCTION__,args)
#	else 
#	    define debug_msg(args...)     				\
			_xdebug_msg_long(__FILE__,__LINE__,0,args)
#	endif
#   else  
#	ifdef DEBUG_PRINT_FUNCTION_NAME
#	    define debug_msg						\
		_xdebug_entry(__FILE__,__LINE__,__FUNCTION__),		\
		_xdebug_msg_short
#	else
#		define debug_msg					\
		_xdebug_entry(__FILE__,__LINE__,0), _xdebug_msg_short
#	endif
#   endif
#
#   define debug_msg_indent(n) _xdebug_set_indent(n)
#
#else 
#    ifdef __cplusplus

/* This cruft and the ensuing version of debug_msg mean that the debug enabled
 * and disabled versions of debug_msg have similar sematics, ie both are
 * likely to be broken or neither is broken following a change.  However,
 * this comes at non-zero cost so an NDEBUG build would probably want to
 * define debug_msg to be empty (enabling optimization may not stop 
 * calls to c_str() in format arguments).
 */
inline void swallow_args(const char*) {}

template <class A>
inline void swallow_args(const char*, A) {}

template <class A, class B>
inline void swallow_args(const char*, A, B) {}

template <class A, class B, class C>
inline void swallow_args(const char*, A, B, C) {}

template <class A, class B, class C, class D>
inline void swallow_args(const char*, A, B, C, D) {}

template <class A, class B, class C, class D, class E>
inline void swallow_args(const char*, A, B, C, D, E) {}

template <class A, class B, class C, class D, class E, class F>
inline void swallow_args(const char*, A, B, C, D, E, F) {}

template <class A, class B, class C, class D, class E, class F, class G>
inline void swallow_args(const char*, A, B, C, D, E, F, G) {}

template <class A, class B, class C, class D, class E, class F, class G, class H>
inline void swallow_args(const char*, A, B, C, D, E, F, G, H) {}

inline void
check_args(const char* fmt, ...) __printflike(1,2);

inline void
check_args(const char*, ...) {}

#define debug_msg(args...) 						      \
do { 									      \
    check_args(args);							      \
    swallow_args(args);							      \
} while(0)

#    else
#	ifdef CPP_SUPPORTS_GNU_VA_ARGS
#	    define debug_msg(args...)
#	else
#	    define debug_msg if (1) ; else _xdebug_null
#	endif
#    endif
#
#    define debug_msg_indent(x) 
#
#endif 

#ifdef __cplusplus
extern "C" {
#endif

/* Function for systems with variable argument macros */
void	_xdebug_msg_long(const char* file, int line, const char* fn, 
			 const char* format, ...) __printflike(4, 5);

/* Functions for systems without variable argument macros */
void	_xdebug_entry(const char* file, int line, const char* fn);
void	_xdebug_msg_short(const char* format, ...) __printflike(1,2);
void	_xdebug_null(const char* format, ...) __printflike(1,2);

/* Misc */
void	_xdebug_set_indent(uint32_t n);

/* Undefine __printflike if we defined it */
#ifdef __libxorp_debug_defined_printflike
#undef __libxorp_debug_defined_printflike
#undef __printflike
#endif /* __libxorp_debug_defined_printflike */

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_DEBUG_H__ */
