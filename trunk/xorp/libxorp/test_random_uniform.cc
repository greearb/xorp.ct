// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP$"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#include "libxorp/timeval.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name         = "test_random_uniform";
static const char *program_description  = "Test TimeVal random_uniform method";
static const char *program_version_id   = "0.1";
static const char *program_date         = "March 24, 2006";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

//
// printf(3)-like facility to conditionally print a message if verbosity
// is enabled.
//
#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)                                  \
do {                                                                    \
    if (verbose()) {                                                    \
        printf("From %s:%d: ", file, line);                             \
        printf(x);                                                      \
    }                                                                   \
} while(0)

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

static int
run_test()
{
    static const int TEST_INTERVAL = 30;
    static const double LOWER_BOUND = 0.75F;
    TimeVal interval(TEST_INTERVAL, 0);
    TimeVal tmin((TEST_INTERVAL * LOWER_BOUND) - 1);	// Double expression.
    TimeVal tmax(TEST_INTERVAL + 1, 0);
    //
    // Calculate jitter which is uniformly distributed as a fraction
    // of TEST_INTERVAL between LOWER_BOUND (0.75) and 1.0.
    //
    TimeVal result = random_uniform(
TimeVal(interval.get_double() * LOWER_BOUND), interval);

    // If output is outside of expected range, reject.
    if (result < tmin || result > tmax) {
	verbose_log("output not in range [%s..%s): %s\n",
		    tmin.str().c_str(), tmax.str().c_str(),
		    result.str().c_str());
	return 1;
    }
#if 0
    verbose_log("output in range [%s..%s): %s\n",
		tmin.str().c_str(), tmax.str().c_str(),
		result.str().c_str());
#endif
    verbose_log("Pass.\n");
    return 0;
};

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
	ret_value = run_test();
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
