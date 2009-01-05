// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/test_ipv4net.cc,v 1.26 2008/10/02 21:57:34 bms Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/timer.hh"
#include "libxorp/test_main.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ipv4net.hh"


//
// TODO: XXX: remove after the switch to the TestMain facility is completed
//
#if 0
//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipv4net";
static const char *program_description	= "Test IPv4Net address class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "December 2, 2002";
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
 * Test IPv4Net valid constructors.
 */
bool
test_ipv4net_valid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for IPv4 address: "12.34.56.78"
    const char *addr_string = "12.34.56.78";
    uint32_t ui = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);
    struct in_addr in_addr;
    in_addr.s_addr = ui;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ui;
    
    const char *netaddr_string = "12.34.56.0/24";
    
    //
    // Default constructor.
    //
    IPv4Net ipnet1;
    verbose_match(ipnet1.str(), "0.0.0.0/0");
    
    //
    // Constructor from a given base address and a prefix length.
    //
    IPv4 ip2(addr_string);
    IPv4Net ipnet2(ip2, 24);
    verbose_match(ipnet2.str(), "12.34.56.0/24");
    
    //
    // Constructor from a string.
    //
    IPv4Net ipnet3(netaddr_string);
    verbose_match(ipnet3.str(), netaddr_string);
    
    //
    // Constructor from another IPv4Net address.
    //
    IPv4Net ipnet4(ipnet3);
    verbose_match(ipnet4.str(), netaddr_string);

    return (! failures());
}

/**
 * Test IPv4Net invalid constructors.
 */
bool
test_ipv4net_invalid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Constructor for invalid prefix length.
    //
    try {
	IPv4 ip("12.34.56.78");
	IPv4Net ipnet(ip, ip.addr_bitlen() + 1);
	verbose_log("Cannot catch invalid prefix length : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid address string.
    //
    try {
	// Invalid address string: note the typo -- lack of prefix length
	IPv4Net ipnet("12.34.56.78/");
	verbose_log("Cannot catch invalid IP network address \"12.34.56.78/\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an address string with invalid prefix length.
    //
    try {
	// Invalid address string: prefix length too long
	IPv4Net ipnet("12.34.56.78/33");
	verbose_log("Cannot catch invalid IP network address \"12.34.56.78/33\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    return (! failures());
}

/**
 * Test IPv4Net operators.
 */
bool
test_ipv4net_operators(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_a("12.34.0.0/16");
    IPv4Net ipnet_b("12.35.0.0/16");
    IPv4Net ipnet_c("12.34.56.0/24");

    //
    // Assignment operator
    //
    IPv4Net ipnet1;
    ipnet1 = ipnet_a;
    verbose_assert(ipnet1.str() == ipnet_a.str(), "operator=");
    
    //
    // Equality Operator
    //
    verbose_assert(ipnet_a == ipnet_a, "operator==");
    verbose_assert(!(ipnet_a == ipnet_b), "operator==");
    verbose_assert(!(ipnet_a == ipnet_c), "operator==");
    
    //
    // Less-Than Operator
    //
    verbose_assert((IPv4Net("12.34.0.0/16") < IPv4Net("12.34.0.0/16"))
		   == false,
		   "operator<");
    verbose_assert((IPv4Net("12.34.0.0/16") < IPv4Net("12.34.56.0/24"))
		   == false,
		   "operator<");
    verbose_assert((IPv4Net("12.34.0.0/16") < IPv4Net("12.0.0.0/8"))
		   == true,
		   "operator<");
    verbose_assert((IPv4Net("12.34.0.0/16") < IPv4Net("12.35.0.0/16"))
		   == true,
		   "operator<");
    
    //
    // Decrement Operator
    //
    verbose_assert(--IPv4Net("12.34.0.0/16") == IPv4Net("12.33.0.0/16"),
		   "operator--()");
    verbose_assert(--IPv4Net("0.0.0.0/16") == IPv4Net("255.255.0.0/16"),
		   "operator--()");
    
    //
    // Increment Operator
    //
    verbose_assert(++IPv4Net("12.34.0.0/16") == IPv4Net("12.35.0.0/16"),
		   "operator++()");
    verbose_assert(++IPv4Net("255.255.0.0/16") == IPv4Net("0.0.0.0/16"),
		   "operator++()");
    
    //
    // Test if the object contains a real (non-default) value.
    //
    verbose_assert(! IPv4Net().is_valid(), "is_valid()");
    verbose_assert(! IPv4Net("0.0.0.0/0").is_valid(), "is_valid()");
    verbose_assert(IPv4Net("0.0.0.0/1").is_valid(), "is_valid()");

    return (! failures());
}

/**
 * Test IPv4Net address type.
 */
bool
test_ipv4net_address_type(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_default("0.0.0.0/0");		// Default route: unicast
    IPv4Net ipnet_unicast1("0.0.0.0/1");	// Unicast
    IPv4Net ipnet_unicast2("12.34.0.0/16");	// Unicast
    IPv4Net ipnet_unicast3("128.0.0.0/2");	// Unicast
    IPv4Net ipnet_unicast4("128.16.0.0/24");	// Unicast
    IPv4Net ipnet_unicast5("192.0.0.0/3");	// Unicast
    IPv4Net ipnet_multicast1("224.0.0.0/4");	// Multicast
    IPv4Net ipnet_multicast2("224.0.0.0/24");	// Multicast
    IPv4Net ipnet_multicast3("224.0.1.0/24");	// Multicast
    IPv4Net ipnet_class_a1("0.0.0.0/1");	// Class A
    IPv4Net ipnet_class_a2("12.34.0.0/16");	// Class A
    IPv4Net ipnet_class_b1("128.0.0.0/2");	// Class B
    IPv4Net ipnet_class_b2("130.2.3.0/24");	// Class B
    IPv4Net ipnet_class_c1("192.0.0.0/3");	// Class C
    IPv4Net ipnet_class_c2("192.2.3.4/32");	// Class C
    IPv4Net ipnet_experimental1("240.0.0.0/4");	// Experimental
    IPv4Net ipnet_experimental2("240.0.1.0/16"); // Experimental
    IPv4Net ipnet_odd1("128.0.0.0/1");		// Odd: includes multicast
    IPv4Net ipnet_odd2("192.0.0.0/2");		// Odd: includes multicast

    //
    // Test if a subnet is within the unicast address range.
    //
    verbose_assert(ipnet_default.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_unicast1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_unicast2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_unicast3.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_unicast4.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_unicast5.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_multicast1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_multicast2.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_multicast3.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_class_a1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_class_a2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_class_b1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_class_b2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_class_c1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_class_c2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet_experimental1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_experimental2.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_odd1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet_odd2.is_unicast() == false, "is_unicast()");

    //
    // Test if a subnet is within the multicast address range.
    //
    verbose_assert(ipnet_default.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_unicast1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_unicast2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_unicast3.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_unicast4.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_unicast5.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_multicast1.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet_multicast2.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet_multicast3.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet_class_a1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_class_a2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_class_b1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_class_b2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_class_c1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_class_c2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_experimental1.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ipnet_experimental2.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ipnet_odd1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet_odd2.is_multicast() == false, "is_multicast()");

    //
    // Test if a subnet is within the experimental address range.
    //
    verbose_assert(ipnet_default.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_unicast1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_unicast2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_unicast3.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_unicast4.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_unicast5.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_multicast1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_multicast2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_multicast3.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_a1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_a2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_b1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_b2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_c1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_class_c2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet_experimental1.is_experimental() == true,
		   "is_experimental()");
    verbose_assert(ipnet_experimental2.is_experimental() == true,
		   "is_experimental()");
    verbose_assert(ipnet_odd1.is_experimental() == false, "is_experimental()");
    verbose_assert(ipnet_odd2.is_experimental() == false, "is_experimental()");

    return (! failures());
}

/**
 * Test IPv4Net address overlap.
 */
bool
test_ipv4net_address_overlap(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_a("12.34.0.0/16");
    IPv4Net ipnet_b("12.35.0.0/16");
    IPv4Net ipnet_c("12.34.56.0/24");
    IPv4Net ipnet_d("12.32.0.0/16");
    IPv4Net ipnet_e("12.33.0.0/16");
    IPv4Net ipnet_f("1.2.1.0/24");
    IPv4Net ipnet_g("1.2.3.0/24");
    
    //
    // Test if subnets overlap.
    //
    verbose_assert(ipnet_a.is_overlap(ipnet_b) == false, "is_overlap()");
    verbose_assert(ipnet_a.is_overlap(ipnet_c) == true, "is_overlap()");
    verbose_assert(ipnet_c.is_overlap(ipnet_a) == true, "is_overlap()");
    
    //
    // Test if a subnet contains (or is equal to) another subnet.
    //
    verbose_assert(ipnet_a.contains(ipnet_a) == true, "contains(IPv4Net)");
    verbose_assert(ipnet_a.contains(ipnet_b) == false, "contains(IPv4Net)");
    verbose_assert(ipnet_a.contains(ipnet_c) == true, "contains(IPv4Net)");
    verbose_assert(ipnet_c.contains(ipnet_a) == false, "contains(IPv4Net)");
    
    //
    // Test if an address is within a subnet.
    //
    verbose_assert(ipnet_a.contains(ipnet_a.masked_addr()) == true,
		   "contains(IPv4)");
    verbose_assert(ipnet_a.contains(ipnet_b.masked_addr()) == false,
		   "contains(IPv4)");
    verbose_assert(ipnet_a.contains(ipnet_c.masked_addr()) == true,
		   "contains(IPv4)");
    
    //
    // Determine the number of the most significant bits overlapping with
    // another subnet.
    //
    verbose_assert(ipnet_a.overlap(ipnet_a) == 16, "overlap()");
    verbose_assert(ipnet_a.overlap(ipnet_b) == 15, "overlap()");
    verbose_assert(ipnet_a.overlap(ipnet_c) == 16, "overlap()");
    verbose_assert(ipnet_d.overlap(ipnet_e) == 15, "overlap()");
    verbose_assert(ipnet_f.overlap(ipnet_g) == 22, "overlap()");

    return (! failures());
}

/**
 * Test performance of IPv4Net address overlap.
 */
bool
test_performance_ipv4net_address_overlap(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_a("255.0.0.0/8");
    IPv4Net ipnet_b("255.255.255.0/24");

    //
    // Test overlapping subnets.
    //
    do {
	size_t i;
	size_t c = 0;
	TimeVal begin_timeval, end_timeval, delta_timeval;

	TimerList::system_gettimeofday(&begin_timeval);
	for (i = 0; i < 0xffffff; i++) {
	    c += ipnet_a.overlap(ipnet_b);
	}
	TimerList::system_gettimeofday(&end_timeval);
	delta_timeval = end_timeval - begin_timeval;
	verbose_log("Execution time IPv4Net::overlap(): %s seconds\n",
		    delta_timeval.str().c_str());
    } while (false);

    return (! failures());
}

/**
 * Test IPv4Net address constant values.
 */
bool
test_ipv4net_address_const(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_a("12.34.0.0/16");

    //
    // Test the address family.
    //
    verbose_assert(IPv4Net::af() == AF_INET, "af()");
    
    //
    // Get the base address.
    //
    IPv4 ip1;
    ip1 = ipnet_a.masked_addr();
    verbose_match(ip1.str(), "12.34.0.0");
    
    //
    // Get the prefix length.
    //
    verbose_assert(ipnet_a.prefix_len() == 16, "prefix_len()");
    
    //
    // Get the network mask.
    //
    IPv4 ip2;
    ip2 = ipnet_a.netmask();
    verbose_match(ip2.str(), "255.255.0.0");

    //
    // Return the subnet containing all multicast addresses.
    //
    verbose_match(IPv4Net::ip_multicast_base_prefix().str(), "224.0.0.0/4");

    //
    // Return the subnet containing all Class A addresses.
    //
    verbose_match(IPv4Net::ip_class_a_base_prefix().str(), "0.0.0.0/1");

    //
    // Return the subnet containing all Class B addresses.
    //
    verbose_match(IPv4Net::ip_class_b_base_prefix().str(), "128.0.0.0/2");

    //
    // Return the subnet containing all Class C addresses.
    //
    verbose_match(IPv4Net::ip_class_c_base_prefix().str(), "192.0.0.0/3");
    
    //
    // Return the subnet containing all experimental addresses.
    //
    verbose_match(IPv4Net::ip_experimental_base_prefix().str(), "240.0.0.0/4");

    return (! failures());
}

/**
 * Test IPv4Net address manipulation.
 */
bool
test_ipv4net_manipulate_address(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet_a("12.34.0.0/16");
    
    //
    // Get the highest address within this subnet.
    //
    verbose_match(ipnet_a.top_addr().str(), "12.34.255.255");
    
    //
    // Get the smallest subnet containing both subnets.
    //
    verbose_match(IPv4Net::common_subnet(IPv4Net("12.34.1.0/24"),
					 IPv4Net("12.34.128.0/24")).str(),
		  "12.34.0.0/16");

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
	{ "test_ipv4net_valid_constructors",
	  callback(test_ipv4net_valid_constructors),
	  true
	},
	{ "test_ipv4net_invalid_constructors",
	  callback(test_ipv4net_invalid_constructors),
	  true
	},
	{ "test_ipv4net_operators",
	  callback(test_ipv4net_operators),
	  true
	},
	{ "test_ipv4net_address_type",
	  callback(test_ipv4net_address_type),
	  true
	},
	{ "test_ipv4net_address_overlap",
	  callback(test_ipv4net_address_overlap),
	  true
	},
	{ "test_ipv4net_address_const",
	  callback(test_ipv4net_address_const),
	  true
	},
	{ "test_ipv4net_manipulate_address",
	  callback(test_ipv4net_manipulate_address),
	  true
	},
	{ "test_performance_ipv4net_address_overlap",
	  callback(test_performance_ipv4net_address_overlap),
	  false
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
