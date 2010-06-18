// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#include "xorp_tests.hh"


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
test_timeval_type()
{
    TimeVal timeval_zero = TimeVal::ZERO();
    TimeVal timeval_onesec = TimeVal(1, 0);
    TimeVal timeval_oneusec = TimeVal(0, 1);
    TimeVal timeval_max = TimeVal::MAXIMUM();

    //
    // Test TimeVal::is_zero()
    //
    verbose_assert(timeval_zero.is_zero() == true, "is_zero()");
    verbose_assert(timeval_onesec.is_zero() == false, "is_zero()");
    verbose_assert(timeval_oneusec.is_zero() == false, "is_zero()");
    verbose_assert(timeval_max.is_zero() == false, "is_zero()");
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
	test_timeval_type();
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
