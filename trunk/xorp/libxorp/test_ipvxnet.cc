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



#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/timer.hh"
#include "libxorp/test_main.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ipvxnet.hh"


//
// TODO: XXX: remove after the switch to the TestMain facility is completed
//
#if 0
//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipvxnet";
static const char *program_description	= "Test IPvXNet address class";
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
 * Test IPvXNet valid constructors.
 */
bool
test_ipvxnet_valid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for IPv4 address: "12.34.56.78"
    const char *addr_string4 = "12.34.56.78";
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
    
    const char *netaddr_string4 = "12.34.56.0/24";

    // Test values for IPv6 address: "1234:5678:9abc:def0:fed:cba9:8765:4321"
    const char *addr_string6 = "1234:5678:9abc:def0:fed:cba9:8765:4321";
    struct in6_addr in6_addr = { { { 0x12, 0x34, 0x56, 0x78,
				     0x9a, 0xbc, 0xde, 0xf0,
				     0x0f, 0xed, 0xcb, 0xa9,
				     0x87, 0x65, 0x43, 0x21 } } };
    uint8_t  ui8[16];
    uint32_t ui32[4];
    memcpy(&ui8[0], &in6_addr, sizeof(in6_addr));
    memcpy(&ui32[0], &in6_addr, sizeof(in6_addr));
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
    sin6.sin6_len = sizeof(sin6);
#endif
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = in6_addr;
    
    const char *netaddr_string6 = "1234:5678::/32";
    
    //
    // Constructor for a specified address family: IPv4.
    //
    IPvXNet ipnet1(AF_INET);
    verbose_assert(ipnet1.af() == AF_INET,
		   "Constructor for AF_INET address family");
    verbose_match(ipnet1.str(), "0.0.0.0/0");
    
    //
    // Constructor for a specified address family: IPv6.
    //
    IPvXNet ipnet2(AF_INET6);
    verbose_assert(ipnet2.af() == AF_INET6,
		   "Constructor for AF_INET6 address family");
    verbose_match(ipnet2.str(), "::/0");

    //
    // Constructor from a given base address and a prefix length: IPv4.
    //
    IPvX ipnet3_ipvx(addr_string4);
    IPvXNet ipnet3(ipnet3_ipvx, 16);
    verbose_match(ipnet3.str(), "12.34.0.0/16");
    
    //
    // Constructor from a given base address and a prefix length: IPv6.
    //
    IPvX ipnet4_ipvx(addr_string6);
    IPvXNet ipnet4(ipnet4_ipvx, 32);
    verbose_match(ipnet4.str(), "1234:5678::/32");

    //
    // Constructor from a string: IPv4.
    //
    IPvXNet ipnet5(netaddr_string4);
    verbose_match(ipnet5.str(), netaddr_string4);

    //
    // Constructor from a string: IPv6.
    //
    IPvXNet ipnet6(netaddr_string6);
    verbose_match(ipnet6.str(), netaddr_string6);
    
    //
    // Constructor from another IPvXNet address: IPv4.
    //
    IPvXNet ipnet7_ipvxnet(netaddr_string4);
    IPvXNet ipnet7(ipnet7_ipvxnet);
    verbose_match(ipnet7.str(), netaddr_string4);

    //
    // Constructor from another IPvXNet address: IPv6.
    //
    IPvXNet ipnet8_ipvxnet(netaddr_string6);
    IPvXNet ipnet8(ipnet8_ipvxnet);
    verbose_match(ipnet8.str(), netaddr_string6);

    //
    // Constructor from an IPv4Net address.
    //
    IPv4Net ipnet9_ipv4net(netaddr_string4);
    IPvXNet ipnet9(ipnet9_ipv4net);
    verbose_match(ipnet9.str(), netaddr_string4);

    //
    // Constructor from an IPv6Net address.
    //
    IPv6Net ipnet10_ipv6net(netaddr_string6);
    IPvXNet ipnet10(ipnet10_ipv6net);
    verbose_match(ipnet10.str(), netaddr_string6);

    return (! failures());
}

/**
 * Test IPvXNet invalid constructors.
 */
bool
test_ipvxnet_invalid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Constructor for invalid address family.
    //
    try {
	IPvXNet ipnet(AF_UNSPEC);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor for invalid prefix length: IPv4.
    //
    try {
	IPvX ip("12.34.56.78");
	IPvXNet ipnet(ip, ip.addr_bitlen() + 1);
	verbose_log("Cannot catch invalid prefix length : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor for invalid prefix length: IPv6.
    //
    try {
	IPvX ip("1234:5678:9abc:def0:fed:cba9:8765:4321");
	IPvXNet ipnet(ip, ip.addr_bitlen() + 1);
	verbose_log("Cannot catch invalid prefix length : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid address string: IPv4.
    //
    try {
	// Invalid address string: note the typo -- lack of prefix length
	IPvXNet ipnet("12.34.56.78/");
	verbose_log("Cannot catch invalid IP network address \"12.34.56.78/\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid address string: IPv6.
    //
    try {
	// Invalid address string: note the typo -- lack of prefix length
	IPvXNet ipnet("1234:5678::/");
	verbose_log("Cannot catch invalid IP network address \"1234:5678::/\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Constructor from an address string with invalid prefix length: IPv4.
    //
    try {
	// Invalid address string: prefix length too long
	IPvXNet ipnet("12.34.56.78/33");
	verbose_log("Cannot catch invalid IP network address \"12.34.56.78/33\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Constructor from an address string with invalid prefix length: IPv6
    //
    try {
	// Invalid address string: prefix length too long
	IPvXNet ipnet("1234:5678::/129");
	verbose_log("Cannot catch invalid IP network address \"1234:5678::/129\" : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    return (! failures());
}

/**
 * Test IPvXNet operators.
 */
bool
test_ipvxnet_operators(TestInfo& test_info)
{
    UNUSED(test_info);

    IPv4Net ipnet4_a("12.34.0.0/16");
    IPv4Net ipnet4_b("12.35.0.0/16");
    IPv4Net ipnet4_c("12.34.56.0/24");
    
    IPvXNet ipnet6_a("1234:5678::/32");
    IPvXNet ipnet6_b("1234:5679::/32");
    IPvXNet ipnet6_c("1234:5678:9abc::/48");
    
    //
    // Assignment operator
    //
    IPvXNet ipnet1(AF_INET);
    ipnet1 = ipnet4_a;
    verbose_assert(ipnet1.str() == ipnet4_a.str(), "operator=");

    IPvXNet ipnet2(AF_INET6);
    ipnet2 = ipnet6_a;
    verbose_assert(ipnet2.str() == ipnet6_a.str(), "operator=");
    
    //
    // Equality Operator
    //
    verbose_assert(ipnet4_a == ipnet4_a, "operator==");
    verbose_assert(!(ipnet4_a == ipnet4_b), "operator==");
    verbose_assert(!(ipnet4_a == ipnet4_c), "operator==");
    
    verbose_assert(ipnet6_a == ipnet6_a, "operator==");
    verbose_assert(!(ipnet6_a == ipnet6_b), "operator==");
    verbose_assert(!(ipnet6_a == ipnet6_c), "operator==");
    
    //
    // Less-Than Operator
    //
    verbose_assert((IPvXNet("12.34.0.0/16") < IPvXNet("12.34.0.0/16"))
		   == false,
		   "operator<");
    verbose_assert((IPvXNet("12.34.0.0/16") < IPvXNet("12.34.56.0/24"))
		   == false,
		   "operator<");
    verbose_assert((IPvXNet("12.34.0.0/16") < IPvXNet("12.0.0.0/8"))
		   == true,
		   "operator<");
    verbose_assert((IPvXNet("12.34.0.0/16") < IPvXNet("12.35.0.0/16"))
		   == true,
		   "operator<");
    
    verbose_assert((IPvXNet("1234:5678::/32") < IPvXNet("1234:5678::/32"))
		   == false,
		   "operator<");
    verbose_assert((IPvXNet("1234:5678::/32") < IPvXNet("1234:5678:9abc::/48"))
		   == false,
		   "operator<");
    verbose_assert((IPvXNet("1234:5678::/32") < IPvXNet("1234::/16"))
		   == true,
		   "operator<");
    verbose_assert((IPvXNet("1234:5678::/32") < IPvXNet("1234:5679::/32"))
		   == true,
		   "operator<");
    
    //
    // Decrement Operator
    //
    verbose_assert(--IPvXNet("12.34.0.0/16") == IPvXNet("12.33.0.0/16"),
		   "operator--()");
    verbose_assert(--IPvXNet("0.0.0.0/16") == IPvXNet("255.255.0.0/16"),
		   "operator--()");
    
    verbose_assert(--IPvXNet("1234:5678::/32") == IPvXNet("1234:5677::/32"),
		   "operator--()");
    verbose_assert(--IPvXNet("::/32") == IPvXNet("ffff:ffff::/32"),
		   "operator--()");
    
    //
    // Increment Operator
    //
    verbose_assert(++IPvXNet("12.34.0.0/16") == IPvXNet("12.35.0.0/16"),
		   "operator++()");
    verbose_assert(++IPvXNet("255.255.0.0/16") == IPvXNet("0.0.0.0/16"),
		   "operator++()");
    
    verbose_assert(++IPvXNet("1234:5678::/32") == IPvXNet("1234:5679::/32"),
		   "operator++()");
    verbose_assert(++IPvXNet("ffff:ffff::/32") == IPvXNet("::/32"),
		   "operator++()");
    
    //
    // Test if the object contains a real (non-default) value.
    //
    verbose_assert(! IPvXNet(AF_INET).is_valid(), "is_valid()");
    verbose_assert(! IPvXNet("0.0.0.0/0").is_valid(), "is_valid()");
    verbose_assert(IPvXNet("0.0.0.0/1").is_valid(), "is_valid()");
    
    verbose_assert(! IPvXNet(AF_INET6).is_valid(), "is_valid()");
    verbose_assert(! IPvXNet("::/0").is_valid(), "is_valid()");
    verbose_assert(IPvXNet("::/1").is_valid(), "is_valid()");

    return (! failures());
}

/**
 * Test IPvXNet address type.
 */
bool
test_ipvxnet_address_type(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_default("0.0.0.0/0");	// Default route: unicast
    IPvXNet ipnet4_unicast1("0.0.0.0/1");	// Unicast
    IPvXNet ipnet4_unicast2("12.34.0.0/16");	// Unicast
    IPvXNet ipnet4_unicast3("128.0.0.0/2");	// Unicast
    IPvXNet ipnet4_unicast4("128.16.0.0/24");	// Unicast
    IPvXNet ipnet4_unicast5("192.0.0.0/3");	// Unicast
    IPvXNet ipnet4_multicast1("224.0.0.0/4");	// Multicast
    IPvXNet ipnet4_multicast2("224.0.0.0/24");	// Multicast
    IPvXNet ipnet4_multicast3("224.0.1.0/24");	// Multicast
    IPvXNet ipnet4_class_a1("0.0.0.0/1");	// Class A
    IPvXNet ipnet4_class_a2("12.34.0.0/16");	// Class A
    IPvXNet ipnet4_class_b1("128.0.0.0/2");	// Class B
    IPvXNet ipnet4_class_b2("130.2.3.0/24");	// Class B
    IPvXNet ipnet4_class_c1("192.0.0.0/3");	// Class C
    IPvXNet ipnet4_class_c2("192.2.3.4/32");	// Class C
    IPvXNet ipnet4_experimental1("240.0.0.0/4");  // Experimental
    IPvXNet ipnet4_experimental2("240.0.1.0/16"); // Experimental
    IPvXNet ipnet4_odd1("128.0.0.0/1");		// Odd: includes multicast
    IPvXNet ipnet4_odd2("192.0.0.0/2");		// Odd: includes multicast

    IPvXNet ipnet6_default("::/0");		// Default route: unicast
    IPvXNet ipnet6_unicast1("::/1");		// Unicast
    IPvXNet ipnet6_unicast2("1234:5678::/32");	// Unicast
    IPvXNet ipnet6_multicast1("ff00::/8");	// Multicast
    IPvXNet ipnet6_multicast2("ff00:1::/32");	// Multicast
    IPvXNet ipnet6_odd1("8000::/1");		// Odd: includes multicast
    IPvXNet ipnet6_odd2("fe00::/7");		// Odd: includes multicast

    //
    // Test if a subnet is within the unicast address range: IPv4.
    //
    verbose_assert(ipnet4_default.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_unicast1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_unicast2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_unicast3.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_unicast4.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_unicast5.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_multicast1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_multicast2.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_multicast3.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_class_a1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_class_a2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_class_b1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_class_b2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_class_c1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_class_c2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet4_experimental1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_experimental2.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_odd1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet4_odd2.is_unicast() == false, "is_unicast()");

    //
    // Test if a subnet is within the unicast address range: IPv6.
    //
    verbose_assert(ipnet6_default.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet6_unicast1.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet6_unicast2.is_unicast() == true, "is_unicast()");
    verbose_assert(ipnet6_multicast1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet6_multicast2.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet6_odd1.is_unicast() == false, "is_unicast()");
    verbose_assert(ipnet6_odd2.is_unicast() == false, "is_unicast()");

    //
    // Test if a subnet is within the multicast address range: IPv4.
    //
    verbose_assert(ipnet4_default.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_unicast1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_unicast2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_unicast3.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_unicast4.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_unicast5.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_multicast1.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet4_multicast2.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet4_multicast3.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet4_class_a1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_class_a2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_class_b1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_class_b2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_class_c1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_class_c2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_experimental1.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ipnet4_experimental2.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ipnet4_odd1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet4_odd2.is_multicast() == false, "is_multicast()");

    //
    // Test if a subnet is within the multicast address range: IPv6.
    //
    verbose_assert(ipnet6_default.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet6_unicast1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet6_unicast2.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet6_multicast1.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet6_multicast2.is_multicast() == true, "is_multicast()");
    verbose_assert(ipnet6_odd1.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet6_odd2.is_multicast() == false, "is_multicast()");

    //
    // Test if a subnet is within the experimental address range: IPv4.
    //
    // XXX: This test applies only for IPv4.
    verbose_assert(ipnet4_default.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_unicast1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_unicast2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_unicast3.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_unicast4.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_unicast5.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_multicast1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_multicast2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_multicast3.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_a1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_a2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_b1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_b2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_c1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_class_c2.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_experimental1.is_experimental() == true,
		   "is_experimental()");
    verbose_assert(ipnet4_experimental2.is_experimental() == true,
		   "is_experimental()");
    verbose_assert(ipnet4_odd1.is_experimental() == false,
		   "is_experimental()");
    verbose_assert(ipnet4_odd2.is_experimental() == false,
		   "is_experimental()");

    return (! failures());
}

/**
 * Test IPvXNet address overlap.
 */
bool
test_ipvxnet_address_overlap(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet4_b("12.35.0.0/16");
    IPvXNet ipnet4_c("12.34.56.0/24");
    IPvXNet ipnet4_d("12.32.0.0/16");
    IPvXNet ipnet4_e("12.33.0.0/16");
    IPvXNet ipnet4_f("1.2.1.0/24");
    IPvXNet ipnet4_g("1.2.3.0/24");
    
    IPvXNet ipnet6_a("1234:5678::/32");
    IPvXNet ipnet6_b("1234:5679::/32");
    IPvXNet ipnet6_c("1234:5678:9abc::/48");
    IPvXNet ipnet6_d("1234:0020::/32");
    IPvXNet ipnet6_e("1234:0021::/32");
    IPvXNet ipnet6_f("0001:0002:0001::/48");
    IPvXNet ipnet6_g("0001:0002:0003::/48");
    
    //
    // Test if subnets overlap.
    //
    verbose_assert(ipnet4_a.is_overlap(ipnet4_b) == false, "is_overlap()");
    verbose_assert(ipnet4_a.is_overlap(ipnet4_c) == true, "is_overlap()");
    verbose_assert(ipnet4_c.is_overlap(ipnet4_a) == true, "is_overlap()");
    
    verbose_assert(ipnet6_a.is_overlap(ipnet6_b) == false, "is_overlap()");
    verbose_assert(ipnet6_a.is_overlap(ipnet6_c) == true, "is_overlap()");
    verbose_assert(ipnet6_c.is_overlap(ipnet6_a) == true, "is_overlap()");
    
    //
    // Test if a subnet contains (or is equal to) another subnet.
    //
    verbose_assert(ipnet4_a.contains(ipnet4_a) == true, "contains(IPv4Net)");
    verbose_assert(ipnet4_a.contains(ipnet4_b) == false, "contains(IPv4Net)");
    verbose_assert(ipnet4_a.contains(ipnet4_c) == true, "contains(IPv4Net)");
    verbose_assert(ipnet4_c.contains(ipnet4_a) == false, "contains(IPv4Net)");
    
    verbose_assert(ipnet6_a.contains(ipnet6_a) == true, "contains(IPvXNet)");
    verbose_assert(ipnet6_a.contains(ipnet6_b) == false, "contains(IPvXNet)");
    verbose_assert(ipnet6_a.contains(ipnet6_c) == true, "contains(IPvXNet)");
    verbose_assert(ipnet6_c.contains(ipnet6_a) == false, "contains(IPvXNet)");
    
    //
    // Test if an address is within a subnet.
    //
    verbose_assert(ipnet4_a.contains(ipnet4_a.masked_addr()) == true,
		   "contains(IPv4)");
    verbose_assert(ipnet4_a.contains(ipnet4_b.masked_addr()) == false,
		   "contains(IPv4)");
    verbose_assert(ipnet4_a.contains(ipnet4_c.masked_addr()) == true,
		   "contains(IPv4)");
    
    verbose_assert(ipnet6_a.contains(ipnet6_a.masked_addr()) == true,
		   "contains(IPvX)");
    verbose_assert(ipnet6_a.contains(ipnet6_b.masked_addr()) == false,
		   "contains(IPvX)");
    verbose_assert(ipnet6_a.contains(ipnet6_c.masked_addr()) == true,
		   "contains(IPvX)");
    
    //
    // Determine the number of the most significant bits overlapping with
    // another subnet.
    //
    verbose_assert(ipnet4_a.overlap(ipnet4_a) == 16, "overlap()");
    verbose_assert(ipnet4_a.overlap(ipnet4_b) == 15, "overlap()");
    verbose_assert(ipnet4_a.overlap(ipnet4_c) == 16, "overlap()");
    verbose_assert(ipnet4_d.overlap(ipnet4_e) == 15, "overlap()");
    verbose_assert(ipnet4_f.overlap(ipnet4_g) == 22, "overlap()");
    
    verbose_assert(ipnet6_a.overlap(ipnet6_a) == 32, "overlap()");
    verbose_assert(ipnet6_a.overlap(ipnet6_b) == 31, "overlap()");
    verbose_assert(ipnet6_a.overlap(ipnet6_c) == 32, "overlap()");
    verbose_assert(ipnet6_d.overlap(ipnet6_e) == 31, "overlap()");
    verbose_assert(ipnet6_f.overlap(ipnet6_g) == 46, "overlap()");

    return (! failures());
}

/**
 * Test performance of IPvXNet address overlap.
 */
bool
test_performance_ipvxnet_address_overlap(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_a("255.0.0.0/8");
    IPvXNet ipnet4_b("255.255.255.0/24");
    IPv6Net ipnet6_a("ffff:ffff::/32");
    IPv6Net ipnet6_b("ffff:ffff:ffff:ffff:ffff:ffff::/96");

    //
    // Test overlapping subnets.
    //
    do {
	size_t i;
	size_t c = 0;
	TimeVal begin_timeval, end_timeval, delta_timeval;

	TimerList::system_gettimeofday(&begin_timeval);
	for (i = 0; i < 0xffffff; i++) {
	    c += ipnet4_a.overlap(ipnet4_b);
	}
	TimerList::system_gettimeofday(&end_timeval);
	delta_timeval = end_timeval - begin_timeval;
	verbose_log("Execution time IPvXNet::overlap(): %s seconds\n",
		    delta_timeval.str().c_str());
    } while (false);

    //
    // Test overlapping subnets.
    //
    do {
	size_t i;
	size_t c = 0;
	TimeVal begin_timeval, end_timeval, delta_timeval;

	TimerList::system_gettimeofday(&begin_timeval);
	for (i = 0; i < 0xffffff; i++) {
	    c += ipnet6_a.overlap(ipnet6_b);
	}
	TimerList::system_gettimeofday(&end_timeval);
	delta_timeval = end_timeval - begin_timeval;
	verbose_log("Execution time IPvXNet::overlap(): %s seconds\n",
		    delta_timeval.str().c_str());
    } while (false);

    return (! failures());
}

/**
 * Test IPvXNet address constant values.
 */
bool
test_ipvxnet_address_const(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet6_a("1234:5678::/32");
    
    //
    // Test the address family.
    //
    verbose_assert(ipnet4_a.af() == AF_INET, "af()");
    verbose_assert(ipnet6_a.af() == AF_INET6, "af()");
    
    //
    // Get the base address: IPv4.
    //
    IPvX ip1(AF_INET);
    ip1 = ipnet4_a.masked_addr();
    verbose_match(ip1.str(), "12.34.0.0");

    //
    // Get the base address: IPv6.
    //
    IPvX ip2(AF_INET6);
    ip2 = ipnet6_a.masked_addr();
    verbose_match(ip2.str(), "1234:5678::");
    
    //
    // Get the prefix length: IPv4.
    //
    verbose_assert(ipnet4_a.prefix_len() == 16, "prefix_len()");
    
    //
    // Get the prefix length: IPv6.
    //
    verbose_assert(ipnet6_a.prefix_len() == 32, "prefix_len()");
    
    //
    // Get the network mask: IPv4.
    //
    IPvX ip3(AF_INET);
    ip3 = ipnet4_a.netmask();
    verbose_match(ip3.str(), "255.255.0.0");

    //
    // Get the network mask: IPv6.
    //
    IPvX ip4(AF_INET6);
    ip4 = ipnet6_a.netmask();
    verbose_match(ip4.str(), "ffff:ffff::");
    
    //
    // Return the subnet containing all multicast addresses: IPv4.
    //
    verbose_match(IPvXNet::ip_multicast_base_prefix(AF_INET).str(),
		  "224.0.0.0/4");
    
    //
    // Return the subnet containing all multicast addresses: IPv6.
    //
    verbose_match(IPvXNet::ip_multicast_base_prefix(AF_INET6).str(),
		  "ff00::/8");

    //
    // Return the subnet containing all Class A addresses.
    //
    // XXX: This test applies only for IPv4.
    verbose_match(IPvXNet::ip_class_a_base_prefix(AF_INET).str(),
		  "0.0.0.0/1");

    //
    // Return the subnet containing all Class B addresses.
    //
    // XXX: This test applies only for IPv4.
    verbose_match(IPvXNet::ip_class_b_base_prefix(AF_INET).str(),
		  "128.0.0.0/2");

    //
    // Return the subnet containing all Class C addresses.
    //
    // XXX: This test applies only for IPv4.
    verbose_match(IPvXNet::ip_class_c_base_prefix(AF_INET).str(),
		  "192.0.0.0/3");

    //
    // Return the subnet containing all experimental addresses.
    //
    // XXX: This test applies only for IPv4.
    verbose_match(IPvXNet::ip_experimental_base_prefix(AF_INET).str(),
		  "240.0.0.0/4");

    return (! failures());
}

/**
 * Test IPvXNet address manipulation.
 */
bool
test_ipvxnet_manipulate_address(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet6_a("1234:5678::/32");
    
    //
    // Get the highest address within this subnet: IPv4.
    //
    verbose_match(ipnet4_a.top_addr().str(), "12.34.255.255");

    //
    // Get the highest address within this subnet: IPv6.
    //
    verbose_match(ipnet6_a.top_addr().str(),
		  "1234:5678:ffff:ffff:ffff:ffff:ffff:ffff");
    
    //
    // Get the smallest subnet containing both subnets: IPv4.
    //
    verbose_match(IPvXNet::common_subnet(IPvXNet("12.34.1.0/24"),
					 IPvXNet("12.34.128.0/24")).str(),
		  "12.34.0.0/16");
    
    //
    // Get the smallest subnet containing both subnets: IPv6.
    //
    verbose_match(IPvXNet::common_subnet(IPvXNet("1234:5678:1::/48"),
					 IPvXNet("1234:5678:8000::/48")).str(),
		  "1234:5678::/32");

    return (! failures());
}

/**
 * Test IPvXNet invalid address manipulation.
 */
bool
test_ipvxnet_invalid_manipulate_address(TestInfo& test_info)
{
    UNUSED(test_info);

    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet6_a("1234:5678::/32");
    
    //
    // Get invalid IPv4Net address.
    //
    try {
	IPvXNet ipnet(ipnet6_a);	// Note: initialized with IPv6 address
	IPv4Net ipnet_ipv4;
	ipnet_ipv4 = ipnet.get_ipv4net();
	verbose_log("Cannot catch invalid get_ipv4net() : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Get invalid IPv6Net address.
    //
    try {
	IPvXNet ipnet(ipnet4_a);	// Note: initialized with IPv4 address
	IPv6Net ipnet_ipv6;
	ipnet_ipv6 = ipnet.get_ipv6net();
	verbose_log("Cannot catch invalid get_ipv6net() : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Assign invalid IPv4Net address.
    //
    try {
	IPvXNet ipnet(ipnet6_a);	// Note: initialized with IPv6 address
	IPv4Net ipnet_ipv4;
	ipnet.get(ipnet_ipv4);
	verbose_log("Cannot catch invalid get(IPv4Net& to_ipv4net) : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Assign invalid IPv6Net address.
    //
    try {
	IPvXNet ipnet(ipnet4_a);	// Note: initialized with IPv4 address
	IPv6Net ipnet_ipv6;
	ipnet.get(ipnet_ipv6);
	verbose_log("Cannot catch invalid get(IPv6Net& to_ipv6net) : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Get multicast base subnet for invalid address family.
    //
    try {
	IPvXNet ipnet(IPvXNet::ip_multicast_base_prefix(AF_UNSPEC));
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ipnet);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

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
	{ "test_ipvxnet_valid_constructors",
	  callback(test_ipvxnet_valid_constructors),
	  true
	},
	{ "test_ipvxnet_invalid_constructors",
	  callback(test_ipvxnet_invalid_constructors),
	  true
	},
	{ "test_ipvxnet_operators",
	  callback(test_ipvxnet_operators),
	  true
	},
	{ "test_ipvxnet_address_type",
	  callback(test_ipvxnet_address_type),
	  true
	},
	{ "test_ipvxnet_address_overlap",
	  callback(test_ipvxnet_address_overlap),
	  true
	},
	{ "test_ipvxnet_address_const",
	  callback(test_ipvxnet_address_const),
	  true
	},
	{ "test_ipvxnet_manipulate_address",
	  callback(test_ipvxnet_manipulate_address),
	  true
	},
	{ "test_ipvxnet_invalid_manipulate_address",
	  callback(test_ipvxnet_invalid_manipulate_address),
	  true
	},
	{ "test_performance_ipvxnet_address_overlap",
	  callback(test_performance_ipvxnet_address_overlap),
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
