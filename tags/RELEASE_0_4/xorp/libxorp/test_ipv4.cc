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

#ident "$XORP: xorp/libxorp/test_ipv4.cc,v 1.4 2003/04/18 04:52:09 pavlin Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipv4";
static const char *program_description	= "Test IPv4 address class";
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
 * Test IPv4 valid constructors.
 */
void
test_ipv4_valid_constructors()
{
    // Test values for IPv4 address: "12.34.56.78"
    const char *addr_string = "12.34.56.78";
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
    struct sockaddr *sap = (struct sockaddr *)&sin;
    
    //
    // Default constructor.
    //
    IPv4 ip1;
    verbose_match(ip1.str(), "0.0.0.0");
    
    //
    // Constructor from a string.
    //
    IPv4 ip2(addr_string);
    verbose_match(ip2.str(), addr_string);
    
    //
    // Constructor from another IPv4 address.
    //
    IPv4 ip3(ip2);
    verbose_match(ip3.str(), addr_string);
    
    //
    // Constructor from an integer value.
    //
    IPv4 ip4(ui);
    verbose_match(ip4.str(), addr_string);
    
    //
    // Constructor from a (uint8_t *) memory pointer.
    //
    IPv4 ip5((uint8_t *)&ui);
    verbose_match(ip5.str(), addr_string);
    
    //
    // Constructor from in_addr structure.
    //
    IPv4 ip6(in_addr);
    verbose_match(ip6.str(), addr_string);
    
    //
    // Constructor from sockaddr structure.
    //
    IPv4 ip7(*sap);
    verbose_match(ip7.str(), addr_string);
    
    //
    // Constructor from sockaddr_in structure.
    //
    IPv4 ip8(sin);
    verbose_match(ip8.str(), addr_string);
}

/**
 * Test IPv4 invalid constructors.
 */
void
test_ipv4_invalid_constructors()
{
    // Test values for IPv4 address: "12.34.56.78"
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_UNSPEC;		// Note: invalid IP address family
    sin.sin_addr.s_addr = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);
    struct sockaddr *sap = (struct sockaddr *)&sin;
    
    //
    // Constructor from an invalid address string.
    //
    try {
	// Invalid address string: note the typo -- lack of a "dot" after "12"
	IPv4 ip("1234.56.78");
	verbose_log("Cannot catch invalid IP address \"1234.56.78\" : FAIL\n");
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
	IPv4 ip(*sap);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Constructor from an invalid sockaddr_in structure.
    //
    try {
	IPv4 ip(sin);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPv4 valid copy in/out methods.
 */
void
test_ipv4_valid_copy_in_out()
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
    
    struct sockaddr *sap;
    
    
    //
    // Copy the IPv4 raw address to specified memory location.
    //
    IPv4 ip1(addr_string4);
    uint8_t ip1_uint8[4];
    verbose_assert(ip1.copy_out(&ip1_uint8[0]) == 4,
		   "copy_out(uint8_t *) for IPv4 address");
    verbose_assert(memcmp(&ui, &ip1_uint8[0], 4) == 0,
		   "compare copy_out(uint8_t *) for IPv4 address");
    
    //
    // Copy the IPv4 raw address to an in_addr structure.
    //
    IPv4 ip3(addr_string4);
    struct in_addr ip3_in_addr;
    verbose_assert(ip3.copy_out(ip3_in_addr) == 4,
		   "copy_out(in_addr&) for IPv4 address");
    verbose_assert(memcmp(&in_addr, &ip3_in_addr, 4) == 0,
		   "compare copy_out(in_addr&) for IPv4 address");
    
    //
    // Copy the IPv4 raw address to a sockaddr structure.
    //
    IPv4 ip5(addr_string4);
    struct sockaddr_in ip5_sockaddr_in;
    sap = (struct sockaddr *)&ip5_sockaddr_in;
    verbose_assert(ip5.copy_out(*sap) == 4,
		   "copy_out(sockaddr&) for IPv4 address");
    verbose_assert(memcmp(&sin, &ip5_sockaddr_in, sizeof(sin)) == 0,
		   "compare copy_out(sockaddr&) for IPv4 address");
    
    //
    // Copy the IPv4 raw address to a sockaddr_in structure.
    //
    IPv4 ip7(addr_string4);
    struct sockaddr_in ip7_sockaddr_in;
    verbose_assert(ip7.copy_out(ip7_sockaddr_in) == 4,
		   "copy_out(sockaddr_in&) for IPv4 address");
    verbose_assert(memcmp(&sin, &ip7_sockaddr_in, sizeof(sin)) == 0,
		   "compare copy_out(sockaddr_in&) for IPv4 address");
    
    //
    // Copy a raw address into IPv4 structure.
    //
    IPv4 ip11;
    verbose_assert(ip11.copy_in((uint8_t *)&ui) == 4,
		   "copy_in(uint8_t *) for IPv4 address");
    verbose_match(ip11.str(), addr_string4);
    
    //
    // Copy a raw IPv4 address from a in_addr structure into IPv4 structure.
    //
    IPv4 ip13;
    verbose_assert(ip13.copy_in(in_addr) == 4,
		   "copy_in(in_addr&) for IPv4 address");
    verbose_match(ip13.str(), addr_string4);
    
    //
    // Copy a raw address from a sockaddr structure into IPv4 structure.
    //
    IPv4 ip15;
    sap = (struct sockaddr *)&sin;
    verbose_assert(ip15.copy_in(*sap) == 4,
		   "copy_in(sockaddr&) for IPv4 address");
    verbose_match(ip15.str(), addr_string4);

    //
    // Copy a raw address from a sockaddr_in structure into IPv4 structure.
    //
    IPv4 ip17;
    verbose_assert(ip17.copy_in(sin) == 4,
		   "copy_in(sockaddr_in&) for IPv4 address");
    verbose_match(ip17.str(), addr_string4);
}

/**
 * Test IPv4 invalid copy in/out methods.
 */
void
test_ipv4_invalid_copy_in_out()
{
    // Test values for IPv4 address: "12.34.56.78"
    // const char *addr_string4 = "12.34.56.78";
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_UNSPEC;		// Note: invalid IP address family
    sin.sin_addr.s_addr = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);
    
    struct sockaddr *sap;
    
    //
    // Copy-in from a sockaddr structure for invalid address family.
    //
    try {
	IPv4 ip;
	sap = (struct sockaddr *)&sin;
	ip.copy_in(*sap);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    
    //
    // Copy-in from a sockaddr_in structure for invalid address family.
    //
    try {
	IPv4 ip;
	ip.copy_in(sin);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPv4 operators.
 */
void
test_ipv4_operators()
{
    IPv4 ip_a("0.255.0.255");
    IPv4 ip_b("255.0.255.255"); 
    IPv4 ip_not_a("255.0.255.0");
    IPv4 ip_a_or_b("255.255.255.255");
    IPv4 ip_a_and_b("0.0.0.255");
    IPv4 ip_a_xor_b("255.255.255.0");
    
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
    verbose_assert(IPv4("0.255.0.255") << 16 == IPv4("0.255.0.0"),
		   "operator<<");
    verbose_assert(IPv4("0.255.0.0") << 1 == IPv4("1.254.0.0"),
		   "operator<<");
    
    //
    // Operator >>
    //
    verbose_assert(IPv4("0.255.0.255") >> 16 == IPv4("0.0.0.255"),
		   "operator>>");
    verbose_assert(IPv4("0.0.0.255") >> 1 == IPv4("0.0.0.127"),
		   "operator>>");
    
    //
    // Decrement Operator
    //
    verbose_assert(--IPv4("0.255.0.255") == IPv4("0.255.0.254"),
		   "operator--()");
    verbose_assert(--IPv4("0.0.0.0") == IPv4("255.255.255.255"),
		   "operator--()");
    
    //
    // Increment Operator
    //
    verbose_assert(++IPv4("0.255.0.254") == IPv4("0.255.0.255"),
		   "operator++()");
    verbose_assert(++IPv4("255.255.255.255") == IPv4("0.0.0.0"),
		   "operator++()");
}

/**
 * Test IPv4 address type.
 */
void
test_ipv4_address_type()
{
    //
    // Test if this address is numerically zero.
    //
    verbose_assert(IPv4("0.0.0.0").is_zero(), "is_zero()");
    
    //
    // Test if this address is a valid unicast address.
    //
    verbose_assert(IPv4("12.34.56.78").is_unicast(), "is_unicast()");
    
    //
    // Test if this address is a valid multicast address.
    //
    verbose_assert(IPv4("224.1.2.3").is_multicast(), "is_multicast()");
    
    //
    // Test if this address is a valid node-local multicast address.
    //
    verbose_assert(IPv4("224.0.0.1").is_nodelocal_multicast() == false,
		       "is_nodelocal_multicast()");
    
    //
    // Test if this address is a valid link-local multicast address.
    //
    verbose_assert(IPv4("224.0.0.2").is_linklocal_multicast(),
		       "is_linklocal_multicast()");
}

/**
 * Test IPv4 address constant values.
 */
void
test_ipv4_address_const()
{
    //
    // Test the address octet-size.
    //
    verbose_assert(IPv4::addr_size() == 4, "addr_size()");
    
    //
    // Test the address bit-length.
    //
    verbose_assert(IPv4::addr_bitlen() == 32, "addr_bitlen()");
    
    //
    // Test the masklen for the multicast base address.
    //
    verbose_assert(IPv4::ip_multicast_base_address_masklen() == 4,
		   "ip_multicast_base_address_masklen()");
    
    //
    // Test the address family.
    //
    verbose_assert(IPv4::af() == AF_INET, "af()");
    
    //
    // Test the IP protocol version.
    //
    verbose_assert(IPv4::ip_version() == 4, "ip_version()");
    
    //
    // Test pre-defined constant addresses
    //
    verbose_assert(IPv4::ZERO() == IPv4("0.0.0.0"), "ZERO()");
    
    verbose_assert(IPv4::ANY() == IPv4("0.0.0.0"), "ANY()");
    
    verbose_assert(IPv4::ALL_ONES() == IPv4("255.255.255.255"),
		   "ALL_ONES()");
    
    verbose_assert(IPv4::MULTICAST_BASE() == IPv4("224.0.0.0"),
		   "MULTICAST_BASE()");
    
    verbose_assert(IPv4::MULTICAST_ALL_SYSTEMS() == IPv4("224.0.0.1"),
		   "MULTICAST_ALL_SYSTEMS()");
    
    verbose_assert(IPv4::MULTICAST_ALL_ROUTERS() == IPv4("224.0.0.2"),
		   "MULTICAST_ALL_ROUTERS()");
    
    verbose_assert(IPv4::DVMRP_ROUTERS() == IPv4("224.0.0.4"),
		   "DVMRP_ROUTERS()");
    
    verbose_assert(IPv4::OSPFIGP_ROUTERS() == IPv4("224.0.0.5"),
		   "OSPFIGP_ROUTERS()");
    
    verbose_assert(IPv4::OSPFIGP_DESIGNATED_ROUTERS() == IPv4("224.0.0.6"),
		   "OSPIGP_DESIGNATED_ROUTERS()");
    
    verbose_assert(IPv4::RIP2_ROUTERS() == IPv4("224.0.0.9"),
		   "RIP2_ROUTERS()");
    
    verbose_assert(IPv4::PIM_ROUTERS() == IPv4("224.0.0.13"),
		   "PIM_ROUTERS()");
}

/**
 * Test IPv4 address manipulation.
 */
void
test_ipv4_manipulate_address()
{
    //
    // Test making an IPv4 mask prefix.
    //
    verbose_assert(IPv4().make_prefix(24) == IPv4("255.255.255.0"),
		   "make_prefix()");
    verbose_assert(IPv4().make_prefix(0) == IPv4("0.0.0.0"),
		   "make_prefix()");
    verbose_assert(IPv4().make_prefix(32) == IPv4("255.255.255.255"),
		   "make_prefix()");
    
    //
    // Test making an IPv4 address prefix.
    //
    verbose_assert(
	IPv4("12.34.56.78").mask_by_prefix(24) == IPv4("12.34.56.0"),
	"mask_by_prefix()"
	);

    //
    // Test getting the prefix length of the contiguous mask.
    //
    verbose_assert(IPv4("255.255.255.0").prefix_length() == 24,
		   "prefix_length()");
    
    //
    // Test getting the raw value of the address.
    //
    uint32_t n = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);
    verbose_assert(IPv4("12.34.56.78").addr() == n, "addr()");
    
    //
    // Test setting the address value
    //
    IPv4 ip_a("1.2.3.4");
    ip_a.set_addr(htonl((12 << 24) | (34 << 16) | (56 << 8) | 78));
    verbose_assert(ip_a == IPv4("12.34.56.78"), "set_addr()");
    
    //
    // Test extracting bits from an address.
    //
    verbose_assert(IPv4("12.34.56.78").bits(0, 8) == 78, "bits()");
}

/**
 * Test IPv4 invalid address manipulation.
 */
void
test_ipv4_invalid_manipulate_address()
{
    const char *addr_string4 = "12.34.56.78";
    
    //
    // Test making an invalid IPv4 mask prefix.
    //
    try {
	// Invalid prefix length
	IPv4 ip(IPv4::make_prefix(IPv4::addr_bitlen() + 1));
	verbose_log("Cannot catch invalid IPv4 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPv4::addr_bitlen() + 1);
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Test masking with an invalid IPv4 mask prefix.
    //
    try {
	// Invalid mask prefix
	IPv4 ip(addr_string4);
	ip.mask_by_prefix(IPv4::addr_bitlen() + 1);
	verbose_log("Cannot catch masking with an invalid IPv4 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPv4::addr_bitlen() + 1);
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
	test_ipv4_valid_constructors();
	test_ipv4_invalid_constructors();
	test_ipv4_valid_copy_in_out();
	test_ipv4_invalid_copy_in_out();
	test_ipv4_operators();
	test_ipv4_address_type();
	test_ipv4_address_const();
	test_ipv4_manipulate_address();
	test_ipv4_invalid_manipulate_address();
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
