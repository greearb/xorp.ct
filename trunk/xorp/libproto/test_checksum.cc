// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2008 International Computer Science Institute
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

#ident "$XORP: xorp/libproto/test_checksum.cc,v 1.5 2007/08/30 06:02:27 pavlin Exp $"

#include "libproto_module.h"
#include "libxorp/xorp.h"

#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/test_main.hh"

#include "libproto/checksum.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

//
// TODO: XXX: remove after the switch to the TestMain facility is completed
//
#if 0
//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_checksum";
static const char *program_description	= "Test checksum calculation";
static const char *program_version_id	= "0.1";
static const char *program_date		= "August 11, 2006";
static const char *program_copyright	= "See file LICENSE";
static const char *program_return_value	= "0 on success, 1 if test error, 2 if internal error";
#endif // 0

static bool s_verbose = false;
bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

static int s_failures = 0;
bool failures()			{ return (s_failures)? (true) : (false); }
void incr_failures()		{ s_failures++; }
void reset_failures()		{ s_failures = 0; }



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

//
// TODO: XXX: remove after the switch to the TestMain facility is completed
//
#if 0
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
#endif // 0

/**
 * Test checksum computation for Internet Protocol family headers.
 */
bool
test_inet_checksum(TestInfo& test_info)
{
    UNUSED(test_info);
    uint16_t checksum;
    uint16_t checksum1, checksum2;

    // Test values for packets of different length
    const uint8_t packet_length_0[]	= { };
    const uint8_t packet_length_1[]	= { 0x1 };
    const uint8_t packet_length_2[]	= { 0x1, 0x2 };
    const uint8_t packet_length_3[]	= { 0x1, 0x2, 0x3 };
    const uint8_t packet_length_4[]	= { 0x1, 0x2, 0x3, 0x4 };
    const uint8_t packet_length_16[]	= { 0x1, 0x2, 0x3, 0x4,
					    0x5, 0x6, 0x7, 0x8,
					    0x9, 0xa, 0xb, 0xc,
					    0xd, 0xe, 0xf, 0x10 };
    const uint8_t packet_all_zeros[]	= { 0x0, 0x0, 0x0, 0x0,
					    0x0, 0x0, 0x0, 0x0,
					    0x0, 0x0, 0x0, 0x0,
					    0x0, 0x0, 0x0, 0x0 };
    const uint8_t packet_all_ones[]	= { 0xff, 0xff, 0xff, 0xff,
					    0xff, 0xff, 0xff, 0xff,
					    0xff, 0xff, 0xff, 0xff,
					    0xff, 0xff, 0xff, 0xff };
    uint8_t packet_length_long1[0x10000];
    uint8_t packet_length_long2[0x10000];
    uint8_t packet_length_long3[0x10000];

    //
    // Initialize some of the packets
    //
    for (size_t i = 0; i < sizeof(packet_length_long1); i++) {
	packet_length_long1[i] = i;
    }
    for (size_t i = 0; i < sizeof(packet_length_long2); i++) {
	packet_length_long2[i] = 0x0;
    }
    for (size_t i = 0; i < sizeof(packet_length_long3); i++) {
	packet_length_long3[i] = 0xff;
    }

    //
    // Test the checksum of a packet with zero length.
    //
    checksum = inet_checksum(packet_length_0, sizeof(packet_length_0));
    verbose_assert(checksum == htons(0xffff),
		   "Internet checksum for a packet with length 0 bytes");

    //
    // Test the checksum for packets with length between 1 and 16
    //
    checksum = inet_checksum(packet_length_1, sizeof(packet_length_1));
    verbose_assert(checksum == htons(0xfeff),
		   "Internet checksum for a packet with length 1 bytes");

    checksum = inet_checksum(packet_length_2, sizeof(packet_length_2));
    verbose_assert(checksum == htons(0xfefd),
		   "Internet checksum for a packet with length 2 bytes");

    checksum = inet_checksum(packet_length_3, sizeof(packet_length_3));
    verbose_assert(checksum == htons(0xfbfd),
		   "Internet checksum for a packet with length 3 bytes");

    checksum = inet_checksum(packet_length_4, sizeof(packet_length_4));
    verbose_assert(checksum == htons(0xfbf9),
		   "Internet checksum for a packet with length 4 bytes");

    checksum = inet_checksum(packet_length_16, sizeof(packet_length_16));
    verbose_assert(checksum == htons(0xbfb7),
		   "Internet checksum for a packet with length 16 bytes");

    //
    // Test the checksum for packets initialized with all zero and all one
    //
    checksum = inet_checksum(packet_all_zeros, sizeof(packet_all_zeros));
    verbose_assert(checksum == htons(0xffff),
		   "Internet checksum for a packet initialized with all zeros");

    checksum = inet_checksum(packet_all_ones, sizeof(packet_all_ones));
    verbose_assert(checksum == htons(0x0),
		   "Internet checksum for a packet initialized with all ones");

    //
    // Test the checksum for long packets
    //
    checksum = inet_checksum(packet_length_long1, sizeof(packet_length_long1));
    verbose_assert(checksum == htons(0xc03f),
		   "Internet checksum for a long packet");

    checksum = inet_checksum(packet_length_long2, sizeof(packet_length_long2));
    verbose_assert(checksum == htons(0xffff),
		   "Internet checksum for a long packet");

    checksum = inet_checksum(packet_length_long3, sizeof(packet_length_long3));
    verbose_assert(checksum == htons(0x0),
		   "Internet checksum for a long packet");

    //
    // Test the addition of two checksums
    //
    checksum1 = htons(0x1);
    checksum2 = htons(0x2);
    checksum = inet_checksum_add(checksum1, checksum2);
    verbose_assert(checksum == htons(0x3), "Addition of two checksums");

    //
    // Test the addition of two zero checksums
    //
    checksum1 = htons(0x0);
    checksum2 = htons(0x0);
    checksum = inet_checksum_add(checksum1, checksum2);
    verbose_assert(checksum == htons(0x0), "Addition of two zero checksums");

    //
    // Test the addition of two all-ones checksums
    //
    checksum1 = htons(0xff);
    checksum2 = htons(0xff);
    checksum = inet_checksum_add(checksum1, checksum2);
    verbose_assert(checksum == htons(0x1fe),
		   "Addition of two all-ones checksums");

    return (! failures());
}

int
main(int argc, char * const argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain test_main(argc, argv);

    string test = test_main.get_optional_args("-t", "--test",
					      "run only the specified test");
    test_main.complete_args_parsing();

    //
    // TODO: XXX: a temporary glue until we complete the switch to the
    // TestMain facility.
    //
    if (test_main.get_verbose())
	set_verbose(true);

    struct test {
	string	test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
	bool	run_by_default;
    } tests[] = {
	{ "test_inet_checksum",
	  callback(test_inet_checksum),
	  true
	}
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		if (! tests[i].run_by_default)
		    continue;
		reset_failures();
		test_main.run(tests[i].test_name, tests[i].cb);
	    }
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		if (test == tests[i].test_name) {
		    reset_failures();
		    test_main.run(tests[i].test_name, tests[i].cb);
		    return test_main.exit();
		}
	    }
	    test_main.failed("No test with name " + test + " found\n");
	}
    } catch (...) {
	xorp_print_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return test_main.exit();
}
