/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/* Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/debug.c,v 1.3 2004/06/10 22:41:15 hodson Exp $"

#include <sys/types.h>

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "config.h"
#include "xorp.h"
#include "debug.h"

#ifdef UNUSED
#undef UNUSED
#endif /* UNUSED */
#define UNUSED(x) (x) = (x)

static uint32_t dbg_indent = 0;

void _xdebug_set_indent(uint32_t n) 
{
    assert(n < 10000);
    dbg_indent = n;
}

static const char*
_xdebug_preamble(const char*	file, 
		 int32_t	line, 
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

    /* Format is <pid> +<line> <file> [<function>] <users_debug_message>
     *
     * The <line> and <file> formatting is for cutting and pasting as
     * arguments to emacs, vi, vim, nedit, etc, but not ed :-( 
     */
    if (func) {
	sprintf(sbuf, "[ %d %+5d %s %s ] ", spid, line, file, func);
    } else {
	sprintf(sbuf, "[ %d %+5d %s ] ", spid, line, file);
    }
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

