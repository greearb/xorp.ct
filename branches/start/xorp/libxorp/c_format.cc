// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libxorp/c_format.cc,v 1.8 2002/12/09 18:29:11 hodson Exp $"

#include <stdio.h>
#include "c_format.hh"

void c_format_validate(const char* fmt, int exp_count) {
    const char *p=fmt;
    int state=0, count=0;
    while(*p!=0) {
	if (state==0) {
	    if (*p=='%') {
		count++;
		state=1;
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
		state=0;
		break;
	    case '%':
		//escaped percent
		state=0;
		count--;
		break;
	    case 'n':
		//we don't permit %n
		fprintf(stderr, "%%n detected in c_format\n");
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

static inline string
c_format_va(const char* fmt, va_list ap)
{
    size_t buf_size = 4096;		// Default buffer size
    
    // The loop below should not be executed more than two times: the
    // first time to get the right size of the buffer if the default
    // size is too small, and the second time with the right buffer size.
    do {
	char b[buf_size];
	int ret = vsnprintf(b, buf_size, fmt, ap);
	if ((size_t)ret < buf_size)
	    return string(b);		// Buffer size is OK
	buf_size = ret + 1;		// Add space for the extra '\0'
    } while (true);
}

string
do_c_format(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string r = c_format_va(fmt, ap);
    va_end(ap);
    return r;
}

#ifdef TESTING_C_FORMAT_123
int main(int, char**) {
    c_format("%d", 3);

    printf("%s", c_format("hello%%\n").c_str());
    printf("%s", c_format("hello %d\n", 27).c_str());
    printf("%s", c_format("hello %3d %%%s\n", 27, "xyz").c_str());
    printf("%s", c_format("hello %*d %%%s\n", 5, 27, "xyz").c_str());
    printf("%s", c_format("hello %%%*n\n").c_str());
}
#endif
