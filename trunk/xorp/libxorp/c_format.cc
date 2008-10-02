// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libxorp/c_format.cc,v 1.14 2008/07/23 05:10:50 pavlin Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include <vector>

#include "c_format.hh"


#ifndef HOST_OS_WINDOWS
#define HAVE_C99_SNPRINTF	// [v]snprintf() conforms to ISO C99 spec
#endif

#define FORMAT_BUFSIZE	4096

void
c_format_validate(const char* fmt, int exp_count)
{
    const char *p = fmt;
    int state = 0;
    int count = 0;
    
    while(*p != 0) {
	if (state == 0) {
	    if (*p == '%') {
		count++;
		state = 1;
	    }
	} else {
	    switch (*p) {
	    case 'd':
	    case 'i':
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
	    case 'D':
	    case 'O':
	    case 'U':
	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
	    case 'c':
	    case 's':
	    case 'p':
		//parameter type specifiers
		state = 0;
		break;
	    case '%':
		//escaped percent
		state = 0;
		count--;
		break;
	    case 'n':
		//we don't permit %n
		fprintf(stderr, "%%n detected in c_format()\n");
		abort();
	    case '*':
		//field width or precision also needs a parameter
		count++;
		break;
	    }
	}
	p++;
    }
    if (exp_count != count) {
	abort();
    }
}

string
do_c_format(const char* fmt, ...)
{
    size_t buf_size = FORMAT_BUFSIZE;             // Default buffer size
    vector<char> b(buf_size);
    va_list ap;

    do {
	va_start(ap, fmt);
	int ret = vsnprintf(&b[0], buf_size, fmt, ap);
#ifndef HAVE_C99_SNPRINTF
	// We have an evil vsnprintf() implementation (MSVC)
	if (ret != -1 && ((size_t)ret < buf_size)) {
	    string r = string(&b[0]);	// Buffer size is OK
	    va_end(ap);
	    return r;
	}
	buf_size += FORMAT_BUFSIZE;
#else	// HAVE_C99_SNPRINTF
	// We have a C99 compliant implementation
	if ((size_t)ret < buf_size) {
	    string r = string(&b[0]);	// Buffer size is OK
	    va_end(ap);
	    return r;
	}
	buf_size = ret + 1;		// Add space for the extra '\0'
#endif	// HAVE_C99_SNPRINTF
	b.resize(buf_size);
    } while (true);

    XLOG_UNREACHABLE();
}

#ifdef TESTING_C_FORMAT_123
int main(int, char**)
{
    c_format("%d", 3);

    printf("%s", c_format("hello%%\n").c_str());
    printf("%s", c_format("hello %d\n", 27).c_str());
    printf("%s", c_format("hello %3d %%%s\n", 27, "xyz").c_str());
    printf("%s", c_format("hello %*d %%%s\n", 5, 27, "xyz").c_str());
    printf("%s", c_format("hello %%%*n\n").c_str());
}
#endif // TESTING_C_FORMAT_123
