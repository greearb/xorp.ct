// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_time_slice.cc,v 1.3 2004/06/10 22:41:21 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/time_slice.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_time_slice";
static const char *program_description	= "Test TimeSlice class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "December 4, 2002";
static const char *program_copyright	= "See file LICENSE.XORP";
static const char *program_return_value	= "0 on success, 1 if test error, 2 if internal error";

static bool s_verbose = false;
bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

static int s_failures = 0;
bool failures()			{ return s_failures; }
void incr_failures()		{ s_failures++; }



//
// printf(3)-like facility to conditionally print a message if verbosity
// is enabled.
//
#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)


//
// Test and print a message whether two strings are lexicographically same.
// The strings can be either C or C++ style.
//
#define verbose_match(s1, s2)						\
    _verbose_match(__FILE__, __LINE__, s1, s2)

bool
_verbose_match(const char* file, int line, const string& s1, const string& s2)
{
    bool match = s1 == s2;
    
    _verbose_log(file, line, "Comparing %s == %s : %s\n",
		 s1.c_str(), s2.c_str(), match ? "OK" : "FAIL");
    if (match == false)
	incr_failures();
    return match;
}


//
// Test and print a message whether a condition is true.
//
// The first argument is the condition to test.
// The second argument is a string with a brief description of the tested
// condition.
//
#define verbose_assert(cond, desc) 					\
    _verbose_assert(__FILE__, __LINE__, cond, desc)

bool
_verbose_assert(const char* file, int line, bool cond, const string& desc)
{
    _verbose_log(file, line,
		 "Testing %s : %s\n", desc.c_str(), cond ? "OK" : "FAIL");
    if (cond == false)
	incr_failures();
    return cond;
}


/**
 * Print program info to output stream.
 * 
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 * 
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

/**
 * Test TimeSlice valid constructors.
 */
void
test_time_slice_valid_constructors()
{
    //
    // Constructor for a given time limit and test frequency.
    //
    TimeSlice time_slice(10, 20);
    UNUSED(time_slice);
}

/**
 * Test TimeSlice invalid constructors.
 */
void
test_time_slice_invalid_constructors()
{
    //
    // Currently, there are no TimeSlice invalid constructors.
    //
}

/**
 * Test TimeSlice operators.
 */
void
test_time_slice_operators()
{
    //
    // Currently, there are no TimeSlice operators.
    //
}

/**
 * Supporting function: sleep for a given number of seconds.
 */
void
slow_function(unsigned int sleep_seconds)
{
    TimerList::system_sleep(TimeVal(sleep_seconds, 0));
}

/**
 * Supporting function: sleep for a given number of microseconds.
 */
void
fast_function(unsigned int sleep_microseconds)
{
    TimerList::system_sleep(TimeVal(0, sleep_microseconds));
}

/**
 * Test TimeSlice operations.
 */
void
test_time_slice_operations()
{
    unsigned int sec;
    unsigned int usec;
    unsigned int i, iter;
    
    //
    // Test single slow function.
    //
    verbose_log("TEST 'SINGLE SLOW_FUNCTION' BEGIN:\n");
    verbose_log("Begin time = %s\n", xlog_localtime2string());
    TimeSlice time_slice1(2000000, 1);	// 2s, test every 1 iter
    sec = 3;
    verbose_log("Running slow function for %d seconds...\n", sec);
    slow_function(sec);
    verbose_log("End time = %s\n", xlog_localtime2string());
    verbose_assert(time_slice1.is_expired(),
		   "is_expired() for a single slow function");
    verbose_log("TEST 'SINGLE SLOW_FUNCTION' END:\n\n");
    
    //
    // Test single fast function.
    //
    verbose_log("TEST 'SINGLE FAST_FUNCTION' BEGIN:\n");
    verbose_log("Begin time = %s\n", xlog_localtime2string());
    TimeSlice time_slice2(2000000, 1);	// 2s, test every 1 iter
    usec = 3;
    verbose_log("Running fast function for %d microseconds...\n", usec);
    fast_function(usec);
    verbose_log("End time = %s\n", xlog_localtime2string());
    verbose_assert(! time_slice2.is_expired(),
		   "is_expired() for a single fast function");
    verbose_log("TEST 'SINGLE FAST_FUNCTION' END:\n\n");
    
    //
    // Test fast function run multiple times.
    //
    verbose_log("TEST 'MULTI FAST_FUNCTION' BEGIN:\n");
    verbose_log("Begin time = %s\n", xlog_localtime2string());
    TimeSlice time_slice3(2000000, 10);		// 2s, test every 10 iter
    usec = 3;
    iter = 1000000;
    verbose_log("Running fast function %d times for %d microseconds each...\n",
		iter, usec);
    bool time_expired = false;
    for (i = 0; i < iter; i++) {
	if (time_slice3.is_expired()) {
	    time_expired = true;
	    break;
	}
	fast_function(usec);
    }
    verbose_log("End time = %s\n", xlog_localtime2string());
    verbose_assert(time_expired,
		   c_format("is_expired() for multiple iterations of a fast function (expired after %d iterations)", i));
    verbose_log("TEST 'MULTI FAST_FUNCTION' END:\n\n");
}

int
main(int argc, char * const argv[])
{
    int ret_value = 0;
    
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();
    
    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
	switch (ch) {
	case 'v':
	    set_verbose(true);
	    break;
	case 'h':
	case '?':
	default:
	    usage(argv[0]);
	    xlog_stop();
	    xlog_exit();
	    if (ch == 'h')
		return (0);
	    else
		return (1);
	}
    }
    argc -= optind;
    argv += optind;
    
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	EventLoop eventloop;

	test_time_slice_valid_constructors();
	test_time_slice_invalid_constructors();
	test_time_slice_operators();
	test_time_slice_operations();
	ret_value = failures() ? 1 : 0;
	UNUSED(eventloop);
    } catch (...) {
	// Internal error
	xorp_print_standard_exceptions();
	ret_value = 2;
    }
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return (ret_value);
}
