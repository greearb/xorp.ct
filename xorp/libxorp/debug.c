/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "libxorp/xorp.h"
#include "libxorp/debug.h"


static uint32_t dbg_indent = 0;

void _xdebug_set_indent(uint32_t n) 
{
    assert(n < 10000);
    dbg_indent = n;
}

static const char*
_xdebug_preamble(const char*	file, 
		 int		line, 
		 const char*	func)
{
    static size_t sbuf_bytes = 256;
    static char*  sbuf = 0;
    static int    spid = 0;

    size_t req_bytes;

    if (sbuf == 0) {
	sbuf = (char*)malloc(sbuf_bytes);
	spid = (int)getpid();
    }

    req_bytes = 2 * 20 + strlen(file) + 1;
    if (func)
	req_bytes += strlen(func);
    if (req_bytes > sbuf_bytes) {
	sbuf_bytes = req_bytes;
	sbuf = (char*)realloc(sbuf, sbuf_bytes);
    }

#ifdef XORP_LOG_PRINT_USECS
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long us = tv.tv_usec;
    us += (tv.tv_sec * 1000000);
    
    /* Format is <pid> [time-us] +<line> <file> [<function>] <users_debug_message>
     *
     * The <line> and <file> formatting is for cutting and pasting as
     * arguments to emacs, vi, vim, nedit, etc, but not ed :-( 
     */
    if (func) {
#ifdef HOST_OS_WINDOWS
	snprintf(sbuf, sbuf_bytes, "[ %d %I64u %+5d %s %s ] ", spid, us, line, file,
		 func);
#else
	snprintf(sbuf, sbuf_bytes, "[ %d %llu %+5d %s %s ] ", spid, us, line, file,
		 func);
#endif
    } else {
#ifdef HOST_OS_WINDOWS
	snprintf(sbuf, sbuf_bytes, "[ %d %I64u %+5d %s ] ", spid, us, line, file);
#else
	snprintf(sbuf, sbuf_bytes, "[ %d %llu %+5d %s ] ", spid, us, line, file);
#endif
    }
#else
    /* Format is <pid> [time-us] +<line> <file> [<function>] <users_debug_message>
     *
     * The <line> and <file> formatting is for cutting and pasting as
     * arguments to emacs, vi, vim, nedit, etc, but not ed :-( 
     */
    if (func) {
	snprintf(sbuf, sbuf_bytes, "[ %d %+5d %s %s ] ", spid, line, file,
		 func);
    } else {
	snprintf(sbuf, sbuf_bytes, "[ %d %+5d %s ] ", spid, line, file);
    }
#endif    
    return sbuf;
}

/** Common printing routine */
__inline static void
_xdebug_msg_va(const char*	file, 
	       int		line, 
	       const char*	func, 
	       const char* 	fmt, 
	       va_list 		ap) {
    uint32_t i;
    fprintf(stderr, "%s", _xdebug_preamble(file, line, func));
    for (i = 0; i < dbg_indent; i++) {
	fprintf(stderr, " ");
    }

    vfprintf(stderr, fmt, ap);
}

/** Debug printing function for systems with varargs preprocessors */
void
_xdebug_msg_long(const char*	file, 
		 int		line, 
		 const char* 	func,
		 const char* 	fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _xdebug_msg_va(file, line, func, fmt, ap);
    va_end(ap);
}

/** State and functions for systems without varargs preprocessors */
static const char*	the_file;
static int		the_line;
static const char*	the_func;

void
_xdebug_entry(const char* file, int line, const char* func) {
    the_file = file;	
    the_line = line;
    the_func = func;
}

void 
_xdebug_msg_short(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    _xdebug_msg_va(the_file, the_line, the_func, fmt, ap);
    va_end(ap);
}

void
_xdebug_null(const char* fmt, ...) {
    UNUSED(fmt);
}

