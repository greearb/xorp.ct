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

#ident "$XORP: xorp/libxorp/test_mac.cc,v 1.21 2008/09/26 21:41:04 pavlin Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/timer.hh"
#include "libxorp/test_main.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "mac.hh"


//
// TODO: XXX: remove after the switch to the TestMain facility is completed
//
#if 0
//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_mac";
static const char *program_description	= "Test Mac address class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "October 17, 2007";
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
 * Test Mac valid constructors.
 */
bool
test_mac_valid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for Mac address: "11:22:33:44:55:66"
    const char* addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    struct ether_addr ether_addr;
    struct sockaddr sa;

    // Initialize the structures
    memcpy(&ether_addr, ui8_addr, sizeof(ui8_addr));
    memset(&sa, 0, sizeof(sa));
    memcpy(sa.sa_data, ui8_addr, sizeof(ui8_addr));

    //
    // Default constructor.
    //
    Mac mac1;
    verbose_match(mac1.str(), Mac::ZERO().str());

    //
    // Constructor from a (uint8_t *) memory pointer.
    //
    Mac mac2(ui8_addr);
    verbose_match(mac2.str(), addr_string);

    //
    // Constructor from a string.
    //
    Mac mac3(addr_string);
    verbose_match(mac3.str(), addr_string);

    //
    // Constructor from ether_addr structure.
    //
    Mac mac4(ether_addr);
    verbose_match(mac4.str(), addr_string);

    //
    // Constructor from sockaddr structure.
    //
    Mac mac5(sa);
    verbose_match(mac5.str(), addr_string);

    //
    // Constructor from another Mac address.
    //
    Mac mac6(mac2);
    verbose_match(mac6.str(), addr_string);

    return (! failures());
}

/**
 * Test Mac invalid constructors.
 */
bool
test_mac_invalid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Constructor from an invalid address string.
    //
    try {
	// Invalid address string
	Mac mac("hello");
	verbose_log("Cannot catch invalid Mac address \"hello\" : FAIL\n");
	incr_failures();
	UNUSED(mac);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    return (! failures());
}

/**
 * Test Mac valid copy in/out methods.
 */
bool
test_mac_valid_copy_in_out(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for Mac address: "11:22:33:44:55:66"
    const char* addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    struct ether_addr ether_addr;
    struct sockaddr sa;

    // Initialize the structures
    memcpy(&ether_addr, ui8_addr, sizeof(ui8_addr));
    memset(&sa, 0, sizeof(sa));
    memcpy(sa.sa_data, ui8_addr, sizeof(ui8_addr));

    //
    // Copy the Mac raw address to specified memory location.
    //
    Mac mac1(addr_string);
    uint8_t mac1_uint8[6];
    verbose_assert(mac1.copy_out(&mac1_uint8[0]) == 6,
		   "copy_out(uint8_t *) for Mac address");
    verbose_assert(memcmp(&ui8_addr, &mac1_uint8[0], 6) == 0,
		   "compare copy_out(uint8_t *) for Mac address");

    //
    // Copy the Mac raw address to ether_addr structure.
    //
    Mac mac2(addr_string);
    struct ether_addr mac2_ether_addr;
    verbose_assert(mac2.copy_out(mac2_ether_addr) == 6,
		   "copy_out(struct ether_addr) for Mac address");
    verbose_assert(memcmp(&ui8_addr, &mac2_ether_addr, 6) == 0,
		   "compare copy_out(struct ether_addr) for Mac address");

    //
    // Copy the Mac raw address to sockaddr structure.
    //
    Mac mac3(addr_string);
    struct sockaddr mac3_sockaddr;
    verbose_assert(mac3.copy_out(mac3_sockaddr) == 6,
		   "copy_out(struct sockaddr) for Mac address");
    verbose_assert(memcmp(&ui8_addr, &mac3_sockaddr.sa_data, 6) == 0,
		   "compare copy_out(struct sockaddr) for Mac address");

    //
    // Copy a raw address into Mac address.
    //
    Mac mac4;
    verbose_assert(mac4.copy_in(&ui8_addr[0]) == 6,
		   "copy_in(uint8_t *) for Mac address");
    verbose_match(mac4.str(), addr_string);

    //
    // Copy an ether_addr struct address into Mac address.
    //
    Mac mac5;
    verbose_assert(mac5.copy_in(ether_addr) == 6,
		   "copy_in(struct ether_addr) for Mac address");
    verbose_match(mac5.str(), addr_string);

    //
    // Copy a sockaddr struct address into Mac address.
    //
    Mac mac6;
    verbose_assert(mac6.copy_in(sa) == 6,
		   "copy_in(struct sockaddr) for Mac address");
    verbose_match(mac6.str(), addr_string);

    //
    // Copy a string address into Mac address.
    //
    Mac mac7;
    verbose_assert(mac7.copy_in(addr_string) == 6,
		   "copy_in(char *) for Mac address");
    verbose_match(mac7.str(), addr_string);

    return (! failures());
}

/**
 * Test Mac invalid copy in/out methods.
 */
bool
test_mac_invalid_copy_in_out(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Copy-in from invalid string.
    //
    try {
	Mac mac;
	mac.copy_in("hello");
	verbose_log("Cannot catch invalid Mac address \"hello\" : FAIL\n");
	incr_failures();
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    return (! failures());
}

/**
 * Test Mac operators.
 */
bool
test_mac_operators(TestInfo& test_info)
{
    UNUSED(test_info);

    Mac mac_a("00:00:00:00:00:00");
    Mac mac_b("11:11:11:11:11:11");

    //
    // Less-Than Operator
    //
    verbose_assert(mac_a < mac_b, "operator<");

    //
    // Equality Operator
    //
    verbose_assert(mac_a == mac_a, "operator==");
    verbose_assert(!(mac_a == mac_b), "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(mac_a != mac_a), "operator!=");
    verbose_assert(mac_a != mac_b, "operator!=");

    return (! failures());
}

/**
 * Test Mac address type.
 */
bool
test_mac_address_type(TestInfo& test_info)
{
    UNUSED(test_info);

    Mac mac_empty;				// Empty address
    Mac mac_zero("00:00:00:00:00:00");		// Zero address
    Mac mac_unicast("00:11:11:11:11:11");	// Unicast
    Mac mac_multicast("01:22:22:22:22:22");	// Multicast

    //
    // Test the size of an address (in octets).
    //
    verbose_assert(mac_empty.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(mac_zero.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(mac_unicast.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(mac_multicast.addr_bytelen() == 6, "addr_bytelen()");

    //
    // Test the size of an address (in bits).
    //
    verbose_assert(mac_empty.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(mac_zero.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(mac_unicast.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(mac_multicast.addr_bitlen() == 48, "addr_bitlen()");

    //
    // Test if an address is numerically zero.
    //
    verbose_assert(mac_empty.is_zero() == true, "is_zero()");
    verbose_assert(mac_zero.is_zero() == true, "is_zero()");
    verbose_assert(mac_unicast.is_zero() == false, "is_zero()");
    verbose_assert(mac_multicast.is_zero() == false, "is_zero()");

    //
    // Test if an address is a valid unicast address.
    //
    verbose_assert(mac_empty.is_unicast() == true, "is_unicast()");
    verbose_assert(mac_zero.is_unicast() == true, "is_unicast()");
    verbose_assert(mac_unicast.is_unicast() == true, "is_unicast()");
    verbose_assert(mac_multicast.is_unicast() == false, "is_unicast()");

    //
    // Test if an address is a valid multicast address.
    //
    verbose_assert(mac_empty.is_multicast() == false, "is_multicast()");
    verbose_assert(mac_zero.is_multicast() == false, "is_multicast()");
    verbose_assert(mac_unicast.is_multicast() == false, "is_multicast()");
    verbose_assert(mac_multicast.is_multicast() == true, "is_multicast()");

    return (! failures());
}

/**
 * Test Mac address constant values.
 */
bool
test_mac_address_const(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Test the number of bits in address
    //
    verbose_assert(Mac::ADDR_BITLEN == 48, "ADDR_BITLEN");

    //
    // Test the number of bytes in address
    //
    verbose_assert(Mac::ADDR_BYTELEN == 6, "ADDR_BYTELEN");

    //
    // Test the multicast bit in the first octet of the address
    //
    verbose_assert(Mac::MULTICAST_BIT == 0x1, "MULTICAST_BIT");

    //
    // Test pre-defined constant addresses
    //
    verbose_assert(Mac::ZERO() == Mac("00:00:00:00:00:00"), "ZERO()");

    verbose_assert(Mac::ALL_ONES() == Mac("ff:ff:ff:ff:ff:ff"), "ALL_ONES()");

    verbose_assert(Mac::STP_MULTICAST() == Mac("01:80:c2:00:00:00"),
		   "STP_MULTICAST()");

    verbose_assert(Mac::LLDP_MULTICAST() == Mac("01:80:c2:00:00:0e"),
		   "LLDP_MULTICAST()");

    verbose_assert(Mac::GMRP_MULTICAST() == Mac("01:80:c2:00:00:20"),
		   "GMRP_MULTICAST()");

    verbose_assert(Mac::GVRP_MULTICAST() == Mac("01:80:c2:00:00:21"),
		   "GVRP_MULTICAST()");

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
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
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
	{ "test_mac_valid_constructors",
	  callback(test_mac_valid_constructors),
	  true
	},
	{  "test_mac_invalid_constructors",
	   callback(test_mac_invalid_constructors),
	   true
	},
	{  "test_mac_valid_copy_in_out",
	   callback(test_mac_valid_copy_in_out),
	   true
	},
	{  "test_mac_invalid_copy_in_out",
	   callback(test_mac_invalid_copy_in_out),
	   true
	},
	{  "test_mac_operators",
	   callback(test_mac_operators),
	   true
	},
	{  "test_mac_address_type",
	   callback(test_mac_address_type),
	   true
	},
	{  "test_mac_address_const",
	   callback(test_mac_address_const),
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
