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

#ident "$XORP: xorp/libxorp/test_ipv6.cc,v 1.8 2003/09/30 18:27:04 pavlin Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv6.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipv6";
static const char *program_description	= "Test IPv6 address class";
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
    fprintf(stderr, "Return 0 on success, 1 if test error, 2 if internal error.\n");
}

/**
 * Test IPv6 valid constructors.
 */
void
test_ipv6_valid_constructors()
{
    // Test values for IPv6 address: "1234:5678:9abc:def0:fed:cba9:8765:4321"
    const char *addr_string = "1234:5678:9abc:def0:fed:cba9:8765:4321";
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
    struct sockaddr *sap = (struct sockaddr *)&sin6;
    
    //
    // Default constructor.
    //
    IPv6 ip1;
    verbose_match(ip1.str(), "::");
    
    //
    // Constructor from a string.
    //
    IPv6 ip2(addr_string);
    verbose_match(ip2.str(), addr_string);
    
    //
    // Constructor from another IPv6 address.
    //
    IPv6 ip3(ip2);
    verbose_match(ip3.str(), addr_string);
    
    //
    // Constructor from a (uint8_t *) memory pointer.
    //
    IPv6 ip4(ui8);
    verbose_match(ip4.str(), addr_string);
    
    //
    // Constructor from a (uint32_t *) memory pointer.
    //
    IPv6 ip5(ui32);
    verbose_match(ip5.str(), addr_string);
    
    //
    // Constructor from in6_addr structure.
    //
    IPv6 ip6(in6_addr);
    verbose_match(ip6.str(), addr_string);
    
    //
    // Constructor from sockaddr structure.
    //
    IPv6 ip7(*sap);
    verbose_match(ip7.str(), addr_string);
    
    //
    // Constructor from sockaddr_in6 structure.
    //
    IPv6 ip8(sin6);
    verbose_match(ip8.str(), addr_string);
}

/**
 * Test IPv6 invalid constructors.
 */
void
test_ipv6_invalid_constructors()
{
    // Test values for IPv6 address: "1234:5678:9abc:def0:fed:cba9:8765:4321"
    struct in6_addr in6_addr = { { { 0x12, 0x34, 0x56, 0x78,
				     0x9a, 0xbc, 0xde, 0xf0,
				     0x0f, 0xed, 0xcb, 0xa9,
				     0x87, 0x65, 0x43, 0x21 } } };
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
#ifdef HAVE_SIN6_LEN
    sin6.sin6_len = sizeof(sin6);
#endif
    sin6.sin6_family = AF_UNSPEC;	// Note: invalid IP address family
    sin6.sin6_addr = in6_addr;
    struct sockaddr *sap = (struct sockaddr *)&sin6;
    
    //
    // Constructor from an invalid address string.
    //
    try {
	// Invalid address string: note the typo -- ';' instead of ':'
	// after 8765
	IPv6 ip("1234:5678:9abc:def0:fed:cba9:8765;4321");
	verbose_log("Cannot catch invalid IP address \"1234:5678:9abc:def0:fed:cba9:8765;4321\" : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid sockaddr structure.
    //
    try {
	IPv6 ip(*sap);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid sockaddr_in6 structure.
    //
    try {
	IPv6 ip(sin6);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPv6 valid copy in/out methods.
 */
void
test_ipv6_valid_copy_in_out()
{
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
    
    struct sockaddr *sap;
    
    
    //
    // Copy the IPv6 raw address to specified memory location.
    //
    IPv6 ip2(addr_string6);
    uint8_t ip2_uint8[16];
    verbose_assert(ip2.copy_out(&ip2_uint8[0]) == 16,
		   "copy_out(uint8_t *) for IPv6 address");
    verbose_assert(memcmp(&ui8[0], &ip2_uint8[0], 16) == 0,
		   "compare copy_out(uint8_t *) for IPv6 address");
    
    //
    // Copy the IPv6 raw address to an in6_addr structure.
    //
    IPv6 ip4(addr_string6);
    struct in6_addr ip4_in6_addr;
    verbose_assert(ip4.copy_out(ip4_in6_addr) == 16,
		   "copy_out(in6_addr&) for IPv6 address");
    verbose_assert(memcmp(&in6_addr, &ip4_in6_addr, 16) == 0,
		   "compare copy_out(in6_addr&) for IPv6 address");

    //
    // Copy the IPv6 raw address to a sockaddr structure.
    //
    IPv6 ip6(addr_string6);
    struct sockaddr_in6 ip6_sockaddr_in6;
    sap = (struct sockaddr *)&ip6_sockaddr_in6;
    verbose_assert(ip6.copy_out(*sap) == 16,
		   "copy_out(sockaddr&) for IPv6 address");
    verbose_assert(memcmp(&sin6, &ip6_sockaddr_in6, sizeof(sin6)) == 0,
		   "compare copy_out(sockaddr&) for IPv6 address");
    
    //
    // Copy the IPv6 raw address to a sockaddr_in6 structure.
    //
    IPv6 ip10(addr_string6);
    struct sockaddr_in6 ip10_sockaddr_in6;
    verbose_assert(ip10.copy_out(ip10_sockaddr_in6) == 16,
		   "copy_out(sockaddr_in6&) for IPv6 address");
    verbose_assert(memcmp(&sin6, &ip10_sockaddr_in6, sizeof(sin6)) == 0,
		   "compare copy_out(sockaddr_in6&) for IPv6 address");
    
    //
    // Copy a raw address into IPv6 structure.
    //
    IPv6 ip12;
    verbose_assert(ip12.copy_in(&ui8[0]) == 16,
		   "copy_in(uint8_t *) for IPv6 address");
    verbose_match(ip12.str(), addr_string6);
    
    //
    // Copy a raw IPv6 address from a in6_addr structure into IPv6 structure.
    //
    IPv6 ip14;
    verbose_assert(ip14.copy_in(in6_addr) == 16,
		   "copy_in(in6_addr&) for IPv6 address");
    verbose_match(ip14.str(), addr_string6);

    //
    // Copy a raw address from a sockaddr structure into IPv6 structure.
    //
    IPv6 ip16;
    sap = (struct sockaddr *)&sin6;
    verbose_assert(ip16.copy_in(*sap) == 16,
		   "copy_in(sockaddr&) for IPv6 address");
    verbose_match(ip16.str(), addr_string6);
    
    //
    // Copy a raw address from a sockaddr_in6 structure into IPv6 structure.
    //
    IPv6 ip20;
    verbose_assert(ip20.copy_in(sin6) == 16,
		   "copy_in(sockaddr_in6&) for IPv6 address");
    verbose_match(ip20.str(), addr_string6);
}

/**
 * Test IPv6 invalid copy in/out methods.
 */
void
test_ipv6_invalid_copy_in_out()
{
    // Test values for IPv6 address: "1234:5678:9abc:def0:fed:cba9:8765:4321"
    // const char *addr_string6 = "1234:5678:9abc:def0:fed:cba9:8765:4321";
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
    sin6.sin6_family = AF_UNSPEC;	// Note: invalid IP address family
    sin6.sin6_addr = in6_addr;
    
    struct sockaddr *sap;
    
    //
    // Copy-in from a sockaddr structure for invalid address family.
    //
    try {
	IPv6 ip;
	sap = (struct sockaddr *)&sin6;
	ip.copy_in(*sap);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Copy-in from a sockaddr_in6 structure for invalid address family.
    //
    try {
	IPv6 ip;
	ip.copy_in(sin6);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPv6 operators.
 */
void
test_ipv6_operators()
{
    IPv6 ip_a("0000:ffff:0000:ffff:0000:ffff:0000:ffff");
    IPv6 ip_b("ffff:0000:ffff:0000:ffff:0000:ffff:ffff"); 
    IPv6 ip_not_a("ffff:0000:ffff:0000:ffff:0000:ffff:0000");
    IPv6 ip_a_or_b("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    IPv6 ip_a_and_b("::ffff");
    IPv6 ip_a_xor_b("ffff:ffff:ffff:ffff:ffff:ffff:ffff:0000");
    
    //
    // Equality Operator
    //
    verbose_assert(ip_a == ip_a, "operator==");
    verbose_assert(!(ip_a == ip_b), "operator==");
    
    //
    // Not-Equal Operator
    //
    verbose_assert(!(ip_a != ip_a), "operator!=");
    verbose_assert(ip_a != ip_b, "operator!=");
    
    //
    // Less-Than Operator
    //
    verbose_assert(ip_a < ip_b, "operator<");
    
    //
    // Bitwise-Negation Operator
    //
    verbose_assert(~ip_a == ip_not_a, "operator~");
    
    //
    // OR Operator
    //
    verbose_assert((ip_a | ip_b) == ip_a_or_b, "operator|");
    
    //
    // AND Operator
    //
    verbose_assert((ip_a & ip_b) == ip_a_and_b, "operator&");
    
    //
    // XOR Operator
    //
    verbose_assert((ip_a ^ ip_b) == ip_a_xor_b, "operator^");
    
    //
    // Operator <<
    //
    verbose_assert(IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") << 16 ==
		   IPv6("ffff:0000:ffff:0000:ffff:0000:ffff:0000"),
		   "operator<<");
    verbose_assert(IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") << 1 ==
		   IPv6("0001:fffe:0001:fffe:0001:fffe:0001:fffe"),
		   "operator<<");
    
    //
    // Operator >>
    //
    verbose_assert(IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") >> 16 ==
		   IPv6("0000:0000:ffff:0000:ffff:0000:ffff:0000"),
		   "operator>>");
    verbose_assert(IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") >> 1 ==
		   IPv6("0000:7fff:8000:7fff:8000:7fff:8000:7fff"),
		   "operator>>");
    
    //
    // Decrement Operator
    //
    verbose_assert(--IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") ==
		   IPv6("0000:ffff:0000:ffff:0000:ffff:0000:fffe"),
		   "operator--()");
    verbose_assert(--IPv6("0000:0000:0000:0000:0000:0000:0000:0000") ==
		   IPv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "operator--()");
    
    //
    // Increment Operator
    //
    verbose_assert(++IPv6("0000:ffff:0000:ffff:0000:ffff:0000:ffff") ==
		   IPv6("0000:ffff:0000:ffff:0000:ffff:0001:0000"),
		   "operator++()");
    verbose_assert(++IPv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") ==
		   IPv6("0000:0000:0000:0000:0000:0000:0000:0000"),
		   "operator++()");
}

/**
 * Test IPv6 address type.
 */
void
test_ipv6_address_type()
{
    //
    // Test if this address is numerically zero.
    //
    verbose_assert(IPv6("::").is_zero(), "is_zero()");
    
    //
    // Test if this address is a valid unicast address.
    //
    verbose_assert(IPv6("fe80::1234:5678").is_unicast(), "is_unicast()");
    
    //
    // Test if this address is a valid multicast address.
    //
    verbose_assert(IPv6("ff00::1").is_multicast(), "is_multicast()");

    //
    // Test if this address is a valid link-local unicast address.
    //
    verbose_assert(IPv6("fe80::2").is_linklocal_unicast(),
		   "is_linklocal_unicast()");
    verbose_assert(IPv6("ff02::2").is_linklocal_unicast() == false,
		   "is_linklocal_unicast()");

    //
    // Test if this address is a valid node-local multicast address.
    //
    verbose_assert(IPv6("ff01::1").is_nodelocal_multicast(),
		       "is_nodelocal_multicast()");
    
    //
    // Test if this address is a valid link-local multicast address.
    //
    verbose_assert(IPv6("ff02::2").is_linklocal_multicast(),
		       "is_linklocal_multicast()");
}

/**
 * Test IPv6 address constant values.
 */
void
test_ipv6_address_const()
{
    //
    // Test the address octet-size.
    //
    verbose_assert(IPv6::addr_size() == 16, "addr_size()");
    
    //
    // Test the address bit-length.
    //
    verbose_assert(IPv6::addr_bitlen() == 128, "addr_bitlen()");
    
    //
    // Test the mask length for the multicast base address.
    //
    verbose_assert(IPv6::ip_multicast_base_address_mask_len() == 8,
		   "ip_multicast_base_address_mask_len()");
    
    //
    // Test the address family.
    //
    verbose_assert(IPv6::af() == AF_INET6, "af()");
    
    //
    // Test the IP protocol version.
    //
    verbose_assert(IPv6::ip_version() == 6, "ip_version()");
    verbose_assert(IPv6::ip_version_str() == "IPv6", "ip_version_str()");
    
    //
    // Test pre-defined constant addresses
    //
    verbose_assert(IPv6::ZERO() == IPv6("::"), "ZERO()");
    
    verbose_assert(IPv6::ANY() == IPv6("::"), "ANY()");
    
    verbose_assert(IPv6::ALL_ONES() ==
		   IPv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "ALL_ONES()");
    
    verbose_assert(IPv6::MULTICAST_BASE() == IPv6("ff00::"),
		   "MULTICAST_BASE()");
    
    verbose_assert(IPv6::MULTICAST_ALL_SYSTEMS() == IPv6("ff02::1"),
		   "MULTICAST_ALL_SYSTEMS()");
    
    verbose_assert(IPv6::MULTICAST_ALL_ROUTERS() == IPv6("ff02::2"),
		   "MULTICAST_ALL_ROUTERS()");
    
    verbose_assert(IPv6::DVMRP_ROUTERS() == IPv6("ff02::4"),
		   "DVMRP_ROUTERS()");
    
    verbose_assert(IPv6::OSPFIGP_ROUTERS() == IPv6("ff02::5"),
		   "OSPFIGP_ROUTERS()");
    
    verbose_assert(IPv6::OSPFIGP_DESIGNATED_ROUTERS() == IPv6("ff02::6"),
		   "OSPIGP_DESIGNATED_ROUTERS()");
    
    verbose_assert(IPv6::RIP2_ROUTERS() == IPv6("ff02::9"),
		   "RIP2_ROUTERS()");
    
    verbose_assert(IPv6::PIM_ROUTERS() == IPv6("ff02::D"),
		   "PIM_ROUTERS()");
}

/**
 * Test IPv6 address manipulation.
 */
void
test_ipv6_manipulate_address()
{
    //
    // Test making an IPv6 mask prefix.
    //
    verbose_assert(IPv6().make_prefix(24) == IPv6("ffff:ff00::"),
		   "make_prefix()");
    verbose_assert(IPv6().make_prefix(0) == IPv6("::"),
		   "make_prefix()");
    verbose_assert(IPv6().make_prefix(128) == IPv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "make_prefix()");
    
    //
    // Test making an IPv6 address prefix.
    //
    verbose_assert(
	IPv6("1234:5678:9abc:def0:fed:cba9:8765:4321").mask_by_prefix_len(24) ==
	IPv6("1234:5600::"),
	"mask_by_prefix_len()"
	);
    
    //
    // Test getting the prefix length of the contiguous mask.
    //
    verbose_assert(IPv6("ffff:ff00::").mask_len() == 24,
		   "mask_len()");
    
    //
    // Test getting the raw value of the address.
    //
    struct in6_addr in6_addr = { { { 0x12, 0x34, 0x56, 0x78,
				     0x9a, 0xbc, 0xde, 0xf0,
				     0x0f, 0xed, 0xcb, 0xa9,
				     0x87, 0x65, 0x43, 0x21 } } };
    uint32_t ui32[4];
    memcpy(&ui32[0], &in6_addr, sizeof(in6_addr));
    verbose_assert(
	memcmp(IPv6("1234:5678:9abc:def0:fed:cba9:8765:4321").addr(),
	       ui32, sizeof(ui32)) == 0,
	"addr()");
    
    //
    // Test setting the address value
    //
    uint8_t  ui8[16];
    memcpy(&ui8[0], &in6_addr, sizeof(in6_addr));
    
    IPv6 ip_a("ffff::");
    ip_a.set_addr(&ui8[0]);
    verbose_assert(ip_a == IPv6("1234:5678:9abc:def0:fed:cba9:8765:4321"),
		   "set_addr()");
    
    //
    // Test extracting bits from an address.
    //
    verbose_assert(IPv6("1234:5678:9abc:def0:fed:cba9:8765:4321").bits(0, 8)
		   == 0x21,
		   "bits()");
}

/**
 * Test IPv6 invalid address manipulation.
 */
void
test_ipv6_invalid_manipulate_address()
{
    const char *addr_string6 = "1234:5678:9abc:def0:fed:cba9:8765:4321";
    
    //
    // Test making an invalid IPv6 mask prefix.
    //
    try {
	// Invalid prefix length
	IPv6 ip(IPv6::make_prefix(IPv6::addr_bitlen() + 1));
	verbose_log("Cannot catch invalid IPv6 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPv6::addr_bitlen() + 1);
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Test masking with an invalid IPv6 mask prefix.
    //
    try {
	// Invalid mask prefix
	IPv6 ip(addr_string6);
	ip.mask_by_prefix_len(IPv6::addr_bitlen() + 1);
	verbose_log("Cannot catch masking with an invalid IPv6 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPv6::addr_bitlen() + 1);
	incr_failures();
    } catch (const InvalidNetmaskLength& e) {
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
	test_ipv6_valid_constructors();
	test_ipv6_invalid_constructors();
	test_ipv6_valid_copy_in_out();
	test_ipv6_invalid_copy_in_out();
	test_ipv6_operators();
	test_ipv6_address_type();
	test_ipv6_address_const();
	test_ipv6_manipulate_address();
	test_ipv6_invalid_manipulate_address();
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
