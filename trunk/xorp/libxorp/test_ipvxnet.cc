// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_ipvxnet.cc,v 1.3 2003/03/10 23:20:35 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipvxnet.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipvxnet";
static const char *program_description	= "Test IPvXNet address class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "December 2, 2002";
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
 * Test IPvXNet valid constructors.
 */
void
test_ipvxnet_valid_constructors()
{
    // Test values for IPv4 address: "12.34.56.78"
    const char *addr_string4 = "12.34.56.78";
    uint32_t ui = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);
    struct in_addr in_addr;
    in_addr.s_addr = ui;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
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
#ifdef HAVE_SIN6_LEN
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
}

/**
 * Test IPvXNet invalid constructors.
 */
void
test_ipvxnet_invalid_constructors()
{
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
}

/**
 * Test IPvXNet operators.
 */
void
test_ipvxnet_operators()
{
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
}

/**
 * Test IPvXNet address type.
 */
void
test_ipvxnet_address_type()
{
    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet6_a("1234:5678::/32");
    
    //
    // Test if this subnet is within the multicast address range: IPv4.
    //
    IPvXNet ipnet1("224.0.1.0/24");
    verbose_assert(ipnet4_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet1.is_multicast() == true, "is_multicast()");
    
    //
    // Test if this subnet is within the multicast address range: IPv6.
    //
    IPvXNet ipnet2("ff00:1::/32");
    verbose_assert(ipnet6_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet2.is_multicast() == true, "is_multicast()");
}

/**
 * Test IPvXNet address overlap.
 */
void
test_ipvxnet_address_overlap()
{
    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet4_b("12.35.0.0/16");
    IPvXNet ipnet4_c("12.34.56.0/24");
    
    IPvXNet ipnet6_a("1234:5678::/32");
    IPvXNet ipnet6_b("1234:5679::/32");
    IPvXNet ipnet6_c("1234:5678:9abc::/48");
    
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
    
    verbose_assert(ipnet6_a.overlap(ipnet6_a) == 32, "overlap()");
    verbose_assert(ipnet6_a.overlap(ipnet6_b) == 31, "overlap()");
    verbose_assert(ipnet6_a.overlap(ipnet6_c) == 32, "overlap()");
}

/**
 * Test IPvXNet address constant values.
 */
void
test_ipvxnet_address_const()
{
    IPvXNet ipnet4_a("12.34.0.0/16");
    IPvXNet ipnet6_a("1234:5678::/32");
    
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
    // Test if this subnet is within the multicast address range: IPv4.
    //
    IPvXNet ipnet1("224.0.1.0/24");
    verbose_assert(ipnet4_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet1.is_multicast() == true, "is_multicast()");
    
    //
    // Test if this subnet is within the multicast address range: IPv6.
    //
    IPvXNet ipnet2("ffff:1:2::/48");
    verbose_assert(ipnet6_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet2.is_multicast() == true, "is_multicast()");
}

/**
 * Test IPvXNet address manipulation.
 */
void
test_ipvxnet_manipulate_address()
{
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
}

/**
 * Test IPvXNet invalid address manipulation.
 */
void
test_ipvxnet_invalid_manipulate_address()
{
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
	test_ipvxnet_valid_constructors();
	test_ipvxnet_invalid_constructors();
	test_ipvxnet_operators();
	test_ipvxnet_address_overlap();
	test_ipvxnet_address_type();
	test_ipvxnet_address_const();
	test_ipvxnet_manipulate_address();
	test_ipvxnet_invalid_manipulate_address();
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
