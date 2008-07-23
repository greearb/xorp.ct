// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/test_timeval.cc,v 1.12 2008/01/04 03:16:43 pavlin Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "timeval.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name         = "test_timeval";
static const char *program_description  = "Test TimeVal class";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April 2, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

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

static void
test_timeval_constants()
{
    //
    // Test TimeVal::MAXIMUM()
    //
    verbose_assert(TimeVal::ZERO() < TimeVal::MAXIMUM(),
		   "compare TimeVal::ZERO() < TimeVal::MAXIMUM()");

    //
    // Test TimeVal::MINIMUM()
    //
    verbose_assert(TimeVal::MINIMUM() < TimeVal::ZERO(),
		   "compare TimeVal::MINIMUM() < TimeVal::ZERO()");
}

static void
test_timeval_operators()
{
    TimeVal t(100, 100);
    TimeVal mt = -t;
    TimeVal half_t(50, 50);

    //
    // Addition Operator
    //
    verbose_assert(t + mt == TimeVal::ZERO(), "operator+");

    //
    // Substraction Operator
    //
    verbose_assert(t - half_t == half_t, "operator-");

    //
    // Multiplication Operator
    //
    verbose_assert(half_t * 2 == t, "operator*");
    verbose_assert(half_t * 2.0 == t, "operator* (double float)");

    //
    // Division Operator
    //
    verbose_assert(t / 2 == half_t, "operator/");
    verbose_assert(t / 2.0 == half_t, "operator/ (double float)");
}

static void
test_timeval_random_uniform()
{
    static const int TEST_INTERVAL = 30;
    static const double LOWER_BOUND = 0.75F;
    TimeVal interval(TEST_INTERVAL, 0);
    TimeVal tmin((TEST_INTERVAL * LOWER_BOUND) - 1);	// Double expression
    TimeVal tmax(TEST_INTERVAL + 1, 0);

    //
    // Calculate jitter which is uniformly distributed as a fraction
    // of TEST_INTERVAL between LOWER_BOUND (0.75) and 1.0.
    //
    TimeVal lower_bound_timeval = TimeVal(interval.get_double() * LOWER_BOUND);
    TimeVal result = random_uniform(lower_bound_timeval, interval);

    //
    // Thest that the output is not outside of expected range
    //
    string msg = c_format("time random uniform distribution: "
			  "whether time %s is in range [%s, %s]",
			  result.str().c_str(),
			  tmin.str().c_str(),
			  tmax.str().c_str());
    verbose_assert((tmin <= result) && (result <= tmax), msg.c_str());
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
	test_timeval_constants();
	test_timeval_operators();
	test_timeval_random_uniform();
	ret_value = failures() ? 1 : 0;
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
