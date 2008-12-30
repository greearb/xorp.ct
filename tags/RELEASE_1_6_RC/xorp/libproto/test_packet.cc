// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2008 XORP, Inc.
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

#ident "$XORP: xorp/libproto/test_packet.cc,v 1.6 2008/10/02 21:57:18 bms Exp $"

#include "libproto_module.h"
#include "libxorp/xorp.h"

#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/test_main.hh"

#include "libproto/packet.hh"

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
static const char *program_name		= "test_packet";
static const char *program_description	= "Test packet header manipulation";
static const char *program_version_id	= "0.1";
static const char *program_date		= "August 21, 2006";
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
 * Test IPv4 packet header manipulation.
 */
bool
test_ipv4_header(TestInfo& test_info)
{
    UNUSED(test_info);
    uint8_t data[IpHeader4::SIZE];

    // Test values
    uint8_t ip_tos = 1;
    uint16_t ip_len = 1024;
    uint16_t ip_id = 1000;
    uint16_t ip_fragment_offset = 2000;
    uint16_t ip_fragment_flags = 0xe000;
    uint8_t ip_ttl = 64;
    uint8_t ip_p = 100;
    uint16_t ip_sum = 0x7acd;
    IPv4 ip_src("1.2.3.4");
    IPv4 ip_dst("5.6.7.8");

    memset(data, 0, sizeof(data));

    IpHeader4 iph(data);
    IpHeader4Writer iphw(data);

    //
    // Test whether the packet version is valid
    //
    verbose_assert(iph.is_valid_version() == false,
		   "IPv4 invalid header version");

    //
    // Set all fields
    //
    iphw.set_ip_version(IpHeader4::IP_VERSION);
    iphw.set_ip_header_len(IpHeader4::SIZE);
    iphw.set_ip_tos(ip_tos);
    iphw.set_ip_len(ip_len);
    iphw.set_ip_id(ip_id);
    iphw.set_ip_fragment_offset(ip_fragment_offset);
    iphw.set_ip_fragment_flags(ip_fragment_flags);
    iphw.set_ip_ttl(ip_ttl);
    iphw.set_ip_p(ip_p);
    iphw.set_ip_sum(ip_sum);
    iphw.set_ip_src(ip_src);
    iphw.set_ip_dst(ip_dst);

    //
    // Test the IPv4 version
    //
    verbose_assert(iph.is_valid_version() == true,
		   "IPv4 valid header version");
    verbose_assert(iph.ip_version() == IpHeader4::IP_VERSION, "IPv4 version");

    //
    // Test the IPv4 packet header size
    //
    verbose_assert(iph.ip_header_len() == IpHeader4::SIZE,
		   "IPv4 header length");

    //
    // Test the IPv4 TOS field
    //
    verbose_assert(iph.ip_tos() == ip_tos, "IPv4 TOS field");

    //
    // Test the total IPv4 packet length
    //
    verbose_assert(iph.ip_len() == ip_len, "IPv4 total packet length");

    //
    // Test the IPv4 ID field
    //
    verbose_assert(iph.ip_id() == ip_id, "IPv4 identification field");

    //
    // Test the IPv4 fragment offset
    //
    verbose_assert(iph.ip_fragment_offset() == ip_fragment_offset,
		   "IPv4 fragment offset");

    //
    // Test the IPv4 fragment flags
    //
    verbose_assert(iph.ip_fragment_flags() == ip_fragment_flags,
		   "IPv4 fragment flags");

    //
    // Test the IPv4 TTL
    //
    verbose_assert(iph.ip_ttl() == ip_ttl, "IPv4 TTL");

    //
    // Test the IPv4 protocol
    //
    verbose_assert(iph.ip_p() == ip_p, "IPv4 protocol field");

    //
    // Test the IPv4 packet header checksum
    //
    verbose_assert(iph.ip_sum() == ip_sum, "IPv4 checksum");
    // Set the checksum to garbage and compute it again
    iphw.set_ip_sum(0xffff);
    iphw.compute_checksum();
    verbose_assert(iph.ip_sum() == ip_sum, "IPv4 checksum");

    //
    // Test the IPv4 source and destination address
    //
    verbose_assert(iph.ip_src() == ip_src, "IPv4 source address");
    verbose_assert(iph.ip_dst() == ip_dst, "IPv4 destination address");

    return (! failures());
}

/**
 * Test IPv6 packet header manipulation.
 */
bool
test_ipv6_header(TestInfo& test_info)
{
    UNUSED(test_info);
    uint8_t data[IpHeader6::SIZE];

    // Test values
    uint8_t ip_traffic_class = 10;
    uint32_t ip_flow_label = 2000;
    uint16_t ip_plen = 3000;
    uint8_t ip_nxt = 123;
    uint8_t ip_hlim = 64;
    IPv6 ip_src("1:2:3:4::");
    IPv6 ip_dst("5:6:7:8::");

    memset(data, 0, sizeof(data));

    IpHeader6 iph(data);
    IpHeader6Writer iphw(data);

    //
    // Test whether the packet version is valid
    //
    verbose_assert(iph.is_valid_version() == false,
		   "IPv6 invalid header version");

    //
    // Set all fields
    //
    iphw.set_ip_version(IpHeader6::IP_VERSION);
    iphw.set_ip_traffic_class(ip_traffic_class);
    iphw.set_ip_flow_label(ip_flow_label);
    iphw.set_ip_plen(ip_plen);
    iphw.set_ip_nxt(ip_nxt);
    iphw.set_ip_hlim(ip_hlim);
    iphw.set_ip_src(ip_src);
    iphw.set_ip_dst(ip_dst);

    //
    // Test the IPv6 version
    //
    verbose_assert(iph.is_valid_version() == true,
		   "IPv6 valid header version");
    verbose_assert(iph.ip_version() == IpHeader6::IP_VERSION, "IPv6 version");

    //
    // Test the IPv6 traffic class
    //
    verbose_assert(iph.ip_traffic_class() == ip_traffic_class,
		   "IPv6 traffic class");

    //
    // Test the IPv6 flow label
    //
    verbose_assert(iph.ip_flow_label() == ip_flow_label, "IPv6 flow label");

    //
    // Test the IPv6 payload length
    //
    verbose_assert(iph.ip_plen() == ip_plen, "IPv6 payload length");

    //
    // Test the IPv6 next header
    //
    verbose_assert(iph.ip_nxt() == ip_nxt, "IPv6 next header");

    //
    // Test the IPv6 hop limit
    //
    verbose_assert(iph.ip_hlim() == ip_hlim, "IPv6 hop limit");

    //
    // Test the IPv6 source and destination address
    //
    verbose_assert(iph.ip_src() == ip_src, "IPv6 source address");
    verbose_assert(iph.ip_dst() == ip_dst, "IPv6 destination address");

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
	{ "test_ipv4_header",
	  callback(test_ipv4_header),
	  true
	},
	{ "test_ipv6_header",
	  callback(test_ipv6_header),
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
