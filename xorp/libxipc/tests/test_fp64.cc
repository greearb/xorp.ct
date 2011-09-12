// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



// Needed for PRIXFAST64
#define __STDC_FORMAT_MACROS 1

#include <cmath>
#include <inttypes.h>

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "xrl_args.hh"
#include "fp64serial.h"


///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_fp64";
static const char *program_description  = "Test IEEE754-fp64_t conversion";
static const char *program_version_id   = "0.1";
static const char *program_date         = "August, 2011";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, "
					  "2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
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

static double error_threshold;

static int test_conversion(fp64_t input)
{
    uint_fast64_t encoded = fp64enc(input);
    fp64_t output = fp64dec(encoded);

    fp64_t diff = output - input;
    if (verbose())
	fprintf(stderr, "%15" XORP_PRIgFP64 " %016" PRIXFAST64
		" %15" XORP_PRIgFP64 " %15" XORP_PRIgFP64 "\n",
		input, encoded, output, diff);
    if (diff > error_threshold) {
	verbose_log("THRESHOLD EXCEEDED\n");
	return 1;
    }
    return 0;
}

static int
run_test()
{
    error_threshold = ldexp(1.0, 3 - DBL_MANT_DIG);


    verbose_log("\n***** Expanding from DBL_MIN\n");
    for (double input = DBL_MIN; fpclassify(input) == FP_NORMAL;
	 input *= rand() * 50.0 / RAND_MAX) {
	test_conversion(input);
    }

    verbose_log("\n***** Expanding from -DBL_MIN\n");
    for (double input = -DBL_MIN; fpclassify(input) == FP_NORMAL;
	 input *= rand() * 50.0 / RAND_MAX) {
	test_conversion(input);
    }

    verbose_log("\n***** Reducinging from largest +ve subnormal\n");
    for (double input = nextafter(DBL_MIN, 0.0); fpclassify(input) != FP_ZERO;
	 input /= rand() * 50.0 / RAND_MAX) {
	test_conversion(input);
    }

    verbose_log("\n***** Reducinging from largest -ve subnormal\n");
    for (double input = -nextafter(DBL_MIN, 0.0); fpclassify(input) != FP_ZERO;
	 input /= rand() * 50.0 / RAND_MAX) {
	test_conversion(input);
    }


    verbose_log("\n***** Infinities\n");
    test_conversion(+INFINITY);
    test_conversion(-INFINITY);


    verbose_log("\n***** Zeroes\n");
    test_conversion(+0.0);
    test_conversion(-0.0);

#ifdef NAN
    verbose_log("\n***** NANs\n");
    test_conversion(NAN);
#endif

    return 0;
}

int
main(int argc, char * const argv[])
{
    srand(time(NULL));

    int ret_value;
    const char* const argv0 = argv[0];

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
	    if (ch == 'h')
		return 0;
	    else
		return 1;
	}
    }
    argc -= optind;
    argv += optind;

    //
    // Initialize and start xlog
    //
    xlog_init(argv0, NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	ret_value = run_test();
    }
    catch (...) {
	xorp_catch_standard_exceptions();
	ret_value = 2;
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return ret_value;
}
