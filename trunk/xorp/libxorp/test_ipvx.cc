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

#ident "$XORP: xorp/libxorp/test_ipvx.cc,v 1.9 2004/02/24 23:50:52 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipvx.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipvx";
static const char *program_description	= "Test IPvX address class";
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
 * Test IPvX valid constructors.
 */
void
test_ipvx_valid_constructors()
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
    // Default constructor.
    //
    IPvX ip1;
    verbose_assert(ip1.af() == AF_INET, "Default constructor");
    verbose_match(ip1.str(), "0.0.0.0");

    //
    // Constructor for a specified address family: IPv4.
    //
    IPvX ip2(AF_INET);
    verbose_assert(ip2.af() == AF_INET,
		   "Constructor for AF_INET address family");
    verbose_match(ip2.str(), "0.0.0.0");

    //
    // Constructor for a specified address family: IPv6.
    //
    IPvX ip3(AF_INET6);
    verbose_assert(ip3.af() == AF_INET6,
		   "Constructor for AF_INET6 address family");
    verbose_match(ip3.str(), "::");

    //
    // Constructor from a string: IPv4.
    //
    IPvX ip4(addr_string4);
    verbose_match(ip4.str(), addr_string4);

    //
    // Constructor from a string: IPv6.
    //
    IPvX ip5(addr_string6);
    verbose_match(ip5.str(), addr_string6);

    //
    // Constructor from another IPvX address: IPv4.
    //
    IPvX ip6(ip4);
    verbose_match(ip6.str(), addr_string4);

    //
    // Constructor from another IPvX address: IPv6.
    //
    IPvX ip7(ip5);
    verbose_match(ip7.str(), addr_string6);

    //
    // Constructor from a (uint8_t *) memory pointer: IPv4.
    //
    IPvX ip8(AF_INET, (uint8_t *)&ui);
    verbose_match(ip8.str(), addr_string4);

    //
    // Constructor from a (uint8_t *) memory pointer: IPv6.
    //
    IPvX ip9(AF_INET6, &ui8[0]);
    verbose_match(ip9.str(), addr_string6);

    //
    // Constructor from an IPv4 address.
    //
    IPv4 ip10_ipv4(addr_string4);
    IPvX ip10(ip10_ipv4);
    verbose_match(ip10.str(), addr_string4);

    //
    // Constructor from an IPv6 address.
    //
    IPv6 ip11_ipv6(addr_string6);
    IPvX ip11(ip11_ipv6);
    verbose_match(ip11.str(), addr_string6);

    //
    // Constructor from in_addr structure: IPv4.
    //
    IPvX ip12(in_addr);
    verbose_match(ip12.str(), addr_string4);

    //
    // Constructor from in_addr structure: IPv6.
    //
    IPvX ip13(in6_addr);
    verbose_match(ip13.str(), addr_string6);

    //
    // Constructor from sockaddr structure: IPv4
    //
    sap = (struct sockaddr *)&sin;
    IPvX ip14(*sap);
    verbose_match(ip14.str(), addr_string4);

    //
    // Constructor from sockaddr structure: IPv6
    //
    sap = (struct sockaddr *)&sin6;
    IPvX ip15(*sap);
    verbose_match(ip15.str(), addr_string6);

    //
    // Constructor from sockaddr_in structure: IPv4
    //
    IPvX ip16(sin);
    verbose_match(ip16.str(), addr_string4);

    //
    // Constructor from sockaddr_in structure: IPv6
    //
    struct sockaddr_in *ip17_sockaddr_in_p = (struct sockaddr_in *)&sin6;
    IPvX ip17(*ip17_sockaddr_in_p);
    verbose_match(ip17.str(), addr_string6);

    //
    // Constructor from sockaddr_in6 structure: IPv4
    //
    struct sockaddr_in6 *ip18_sockaddr_in6_p = (struct sockaddr_in6 *)&sin;
    IPvX ip18(*ip18_sockaddr_in6_p);
    verbose_match(ip18.str(), addr_string4);

    //
    // Constructor from sockaddr_in6 structure: IPv6
    //
    IPvX ip19(sin6);
    verbose_match(ip19.str(), addr_string6);
}

/**
 * Test IPvX invalid constructors.
 */
void
test_ipvx_invalid_constructors()
{
    // Test values for IPv4 address: "12.34.56.78"
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_UNSPEC;		// Note: invalid IP address family
    sin.sin_addr.s_addr = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);

    // Test values for IPv6 address: "1234:5678:9abc:def0:fed:cba9:8765:4321"
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
    // Constructor for invalid address family.
    //
    try {
	IPvX ip(AF_UNSPEC);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Constructor from a (uint8_t *) memory pointer for invalid address family.
    //
    try {
	IPvX ip(AF_UNSPEC, &ui8[0]);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Constructor from an invalid address string: IPv4.
    //
    try {
	// Invalid address string: note the typo -- lack of a "dot" after "12"
	IPvX ip("1234.56.78");
	verbose_log("Cannot catch invalid IP address \"1234.56.78\" : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Constructor from an invalid address string: IPv6.
    //
    try {
	// Invalid address string: note the typo -- ';' instead of ':'
	// after 8765
	IPvX ip("1234:5678:9abc:def0:fed:cba9:8765;4321");
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
	sap = (struct sockaddr *)&sin;
	IPvX ip(*sap);
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
	IPvX ip(sin);
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
	IPvX ip(sin6);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPvX valid copy in/out methods.
 */
void
test_ipvx_valid_copy_in_out()
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
    // Copy the IPvX raw address to specified memory location: IPv4.
    //
    IPvX ip1(addr_string4);
    uint8_t ip1_uint8[4];
    verbose_assert(ip1.copy_out(&ip1_uint8[0]) == 4,
		   "copy_out(uint8_t *) for IPv4 address");
    verbose_assert(memcmp(&ui, &ip1_uint8[0], 4) == 0,
		   "compare copy_out(uint8_t *) for IPv4 address");

    //
    // Copy the IPvX raw address to specified memory location: IPv6.
    //
    IPvX ip2(addr_string6);
    uint8_t ip2_uint8[16];
    verbose_assert(ip2.copy_out(&ip2_uint8[0]) == 16,
		   "copy_out(uint8_t *) for IPv6 address");
    verbose_assert(memcmp(&ui8[0], &ip2_uint8[0], 16) == 0,
		   "compare copy_out(uint8_t *) for IPv6 address");

    //
    // Copy the IPvX raw address to an in_addr structure: IPv4.
    //
    IPvX ip3(addr_string4);
    struct in_addr ip3_in_addr;
    verbose_assert(ip3.copy_out(ip3_in_addr) == 4,
		   "copy_out(in_addr&) for IPv4 address");
    verbose_assert(memcmp(&in_addr, &ip3_in_addr, 4) == 0,
		   "compare copy_out(in_addr&) for IPv4 address");

    //
    // Copy the IPvX raw address to an in6_addr structure: IPv6.
    //
    IPvX ip4(addr_string6);
    struct in6_addr ip4_in6_addr;
    verbose_assert(ip4.copy_out(ip4_in6_addr) == 16,
		   "copy_out(in6_addr&) for IPv6 address");
    verbose_assert(memcmp(&in6_addr, &ip4_in6_addr, 16) == 0,
		   "compare copy_out(in6_addr&) for IPv6 address");

    //
    // Copy the IPvX raw address to a sockaddr structure: IPv4.
    //
    IPvX ip5(addr_string4);
    struct sockaddr_in ip5_sockaddr_in;
    sap = (struct sockaddr *)&ip5_sockaddr_in;
    verbose_assert(ip5.copy_out(*sap) == 4,
		   "copy_out(sockaddr&) for IPv4 address");
    verbose_assert(memcmp(&sin, &ip5_sockaddr_in, sizeof(sin)) == 0,
		   "compare copy_out(sockaddr&) for IPv4 address");

    //
    // Copy the IPvX raw address to a sockaddr structure: IPv6.
    //
    IPvX ip6(addr_string6);
    struct sockaddr_in6 ip6_sockaddr_in6;
    sap = (struct sockaddr *)&ip6_sockaddr_in6;
    verbose_assert(ip6.copy_out(*sap) == 16,
		   "copy_out(sockaddr&) for IPv6 address");
    verbose_assert(memcmp(&sin6, &ip6_sockaddr_in6, sizeof(sin6)) == 0,
		   "compare copy_out(sockaddr&) for IPv6 address");

    //
    // Copy the IPvX raw address to a sockaddr_in structure: IPv4.
    //
    IPvX ip7(addr_string4);
    struct sockaddr_in ip7_sockaddr_in;
    verbose_assert(ip7.copy_out(ip7_sockaddr_in) == 4,
		   "copy_out(sockaddr_in&) for IPv4 address");
    verbose_assert(memcmp(&sin, &ip7_sockaddr_in, sizeof(sin)) == 0,
		   "compare copy_out(sockaddr_in&) for IPv4 address");

    //
    // Copy the IPvX raw address to a sockaddr_in structure: IPv6.
    //
    IPvX ip8(addr_string6);
    struct sockaddr_in6 ip8_sockaddr_in6;
    struct sockaddr_in *ip8_sockaddr_in_p = (struct sockaddr_in *)&ip8_sockaddr_in6;
    verbose_assert(ip8.copy_out(*ip8_sockaddr_in_p) == 16,
		   "copy_out(sockaddr_in&) for IPv6 address");
    verbose_assert(memcmp(&sin6, &ip8_sockaddr_in6, sizeof(sin6)) == 0,
		   "compare copy_out(sockaddr_in&) for IPv6 address");

    //
    // Copy the IPvX raw address to a sockaddr_in6 structure: IPv4.
    //
    IPvX ip9(addr_string4);
    struct sockaddr_in ip9_sockaddr_in;
    struct sockaddr_in6 *ip9_sockaddr_in6_p = (struct sockaddr_in6 *)&ip9_sockaddr_in;
    verbose_assert(ip9.copy_out(*ip9_sockaddr_in6_p) == 4,
		   "copy_out(sockaddr_in6&) for IPv4 address");
    verbose_assert(memcmp(&sin, &ip9_sockaddr_in, sizeof(sin)) == 0,
		   "compare copy_out(sockaddr_in6&) for IPv4 address");

    //
    // Copy the IPvX raw address to a sockaddr_in6 structure: IPv6.
    //
    IPvX ip10(addr_string6);
    struct sockaddr_in6 ip10_sockaddr_in6;
    verbose_assert(ip10.copy_out(ip10_sockaddr_in6) == 16,
		   "copy_out(sockaddr_in6&) for IPv6 address");
    verbose_assert(memcmp(&sin6, &ip10_sockaddr_in6, sizeof(sin6)) == 0,
		   "compare copy_out(sockaddr_in6&) for IPv6 address");

    //
    // Copy a raw address of specified family type into IPvX structure: IPv4.
    //
    IPvX ip11(AF_INET);
    verbose_assert(ip11.copy_in(AF_INET, (uint8_t *)&ui) == 4,
		   "copy_in(uint8_t *) for IPv4 address");
    verbose_match(ip11.str(), addr_string4);

    //
    // Copy a raw address of specified family type into IPvX structure: IPv6.
    //
    IPvX ip12(AF_INET6);
    verbose_assert(ip12.copy_in(AF_INET6, &ui8[0]) == 16,
		   "copy_in(uint8_t *) for IPv6 address");
    verbose_match(ip12.str(), addr_string6);

    //
    // Copy a raw IPv4 address from a in_addr structure into IPvX structure.
    //
    IPvX ip13(AF_INET);
    verbose_assert(ip13.copy_in(in_addr) == 4,
		   "copy_in(in_addr&) for IPv4 address");
    verbose_match(ip13.str(), addr_string4);

    //
    // Copy a raw IPv6 address from a in6_addr structure into IPvX structure.
    //
    IPvX ip14(AF_INET6);
    verbose_assert(ip14.copy_in(in6_addr) == 16,
		   "copy_in(in6_addr&) for IPv6 address");
    verbose_match(ip14.str(), addr_string6);

    //
    // Copy a raw address from a sockaddr structure into IPvX structure: IPv4.
    //
    IPvX ip15(AF_INET);
    sap = (struct sockaddr *)&sin;
    verbose_assert(ip15.copy_in(*sap) == 4,
		   "copy_in(sockaddr&) for IPv4 address");
    verbose_match(ip15.str(), addr_string4);

    //
    // Copy a raw address from a sockaddr structure into IPvX structure: IPv6.
    //
    IPvX ip16(AF_INET6);
    sap = (struct sockaddr *)&sin6;
    verbose_assert(ip16.copy_in(*sap) == 16,
		   "copy_in(sockaddr&) for IPv6 address");
    verbose_match(ip16.str(), addr_string6);

    //
    // Copy a raw address from a sockaddr_in structure into IPvX structure: IPv4.
    //
    IPvX ip17(AF_INET);
    verbose_assert(ip17.copy_in(sin) == 4,
		   "copy_in(sockaddr_in&) for IPv4 address");
    verbose_match(ip17.str(), addr_string4);

    //
    // Copy a raw address from a sockaddr_in structure into IPvX structure: IPv6.
    //
    IPvX ip18(AF_INET6);
    struct sockaddr_in *ip18_sockaddr_in_p = (struct sockaddr_in *)&sin6;
    verbose_assert(ip18.copy_in(*ip18_sockaddr_in_p) == 16,
		   "copy_in(sockaddr_in&) for IPv6 address");
    verbose_match(ip18.str(), addr_string6);

    //
    // Copy a raw address from a sockaddr_in6 structure into IPvX structure: IPv4.
    //
    IPvX ip19(AF_INET);
    struct sockaddr_in6 *ip19_sockaddr_in6_p = (struct sockaddr_in6 *)&sin;
    verbose_assert(ip19.copy_in(*ip19_sockaddr_in6_p) == 4,
		   "copy_in(sockaddr_in6&) for IPv4 address");
    verbose_match(ip19.str(), addr_string4);

    //
    // Copy a raw address from a sockaddr_in6 structure into IPvX structure: IPv6.
    //
    IPvX ip20(AF_INET6);
    verbose_assert(ip20.copy_in(sin6) == 16,
		   "copy_in(sockaddr_in6&) for IPv6 address");
    verbose_match(ip20.str(), addr_string6);
}

/**
 * Test IPvX invalid copy in/out methods.
 */
void
test_ipvx_invalid_copy_in_out()
{
    // Test values for IPv4 address: "12.34.56.78"
    const char *addr_string4 = "12.34.56.78";
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
#ifdef HAVE_SIN_LEN
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_UNSPEC;		// Note: invalid IP address family
    sin.sin_addr.s_addr = htonl((12 << 24) | (34 << 16) | (56 << 8) | 78);

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
    sin6.sin6_family = AF_UNSPEC;	// Note: invalid IP address family
    sin6.sin6_addr = in6_addr;

    struct sockaddr *sap;

    //
    // Mismatch copy-out: copy-out IPv6 address to in_addr structure.
    //
    try {
	IPvX ip(*addr_string6);
	struct in_addr in_addr;
	ip.copy_out(in_addr);
	verbose_log("Cannot catch mismatch copy-out : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Mismatch copy-out: copy-out IPv4 address to in_addr6 structure.
    //
    try {
	IPvX ip(*addr_string4);
	struct in6_addr in6_addr;
	ip.copy_out(in6_addr);
	verbose_log("Cannot catch mismatch copy-out : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    // XXX: we should test for copy_out() to sockaddr, sockaddr_in,
    // sockaddr_in6 structures that throw InvalidFamily.
    // To do so we must creast first IPvX with invalid address family.
    // However, this doesn't seem possible, hence we skip those checks.

    //
    // Copy-in from a (uint8_t *) memory pointer for invalid address family.
    //
    try {
	IPvX ip(AF_INET);
	ip.copy_in(AF_UNSPEC, &ui8[0]);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Copy-in from a sockaddr structure for invalid address family.
    //
    try {
	IPvX ip(AF_INET);
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
	IPvX ip(AF_INET);
	ip.copy_in(sin);
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
	IPvX ip(AF_INET6);
	ip.copy_in(sin6);
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test IPvX operators.
 */
void
test_ipvx_operators()
{
    IPv4 ip4_a("0.255.0.255");
    IPv4 ip4_b("255.0.255.255");
    IPv4 ip4_not_a("255.0.255.0");
    IPv4 ip4_a_or_b("255.255.255.255");
    IPv4 ip4_a_and_b("0.0.0.255");
    IPv4 ip4_a_xor_b("255.255.255.0");

    IPvX ip6_a("0000:ffff:0000:ffff:0000:ffff:0000:ffff");
    IPvX ip6_b("ffff:0000:ffff:0000:ffff:0000:ffff:ffff");
    IPvX ip6_not_a("ffff:0000:ffff:0000:ffff:0000:ffff:0000");
    IPvX ip6_a_or_b("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    IPvX ip6_a_and_b("::ffff");
    IPvX ip6_a_xor_b("ffff:ffff:ffff:ffff:ffff:ffff:ffff:0000");

    //
    // Equality Operator
    //
    verbose_assert(ip4_a == ip4_a, "operator==");
    verbose_assert(!(ip4_a == ip4_b), "operator==");

    verbose_assert(ip6_a == ip6_a, "operator==");
    verbose_assert(!(ip6_a == ip6_b), "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(ip4_a != ip4_a), "operator!=");
    verbose_assert(ip4_a != ip4_b, "operator!=");

    verbose_assert(!(ip6_a != ip6_a), "operator!=");
    verbose_assert(ip6_a != ip6_b, "operator!=");

    //
    // Less-Than Operator
    //
    verbose_assert(ip4_a < ip4_b, "operator<");

    verbose_assert(ip6_a < ip6_b, "operator<");

    //
    // Bitwise-Negation Operator
    //
    verbose_assert(~ip4_a == ip4_not_a, "operator~");

    verbose_assert(~ip6_a == ip6_not_a, "operator~");

    //
    // OR Operator
    //
    verbose_assert((ip4_a | ip4_b) == ip4_a_or_b, "operator|");

    verbose_assert((ip6_a | ip6_b) == ip6_a_or_b, "operator|");

    //
    // AND Operator
    //
    verbose_assert((ip4_a & ip4_b) == ip4_a_and_b, "operator&");

    verbose_assert((ip6_a & ip6_b) == ip6_a_and_b, "operator&");

    //
    // XOR Operator
    //
    verbose_assert((ip4_a ^ ip4_b) == ip4_a_xor_b, "operator^");

    verbose_assert((ip6_a ^ ip6_b) == ip6_a_xor_b, "operator^");

    //
    // Operator <<
    //
    verbose_assert(IPvX("0.255.0.255") << 16 == IPvX("0.255.0.0"),
		   "operator<<");
    verbose_assert(IPvX("0.255.0.0") << 1 == IPvX("1.254.0.0"),
		   "operator<<");

    verbose_assert(IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") << 16 ==
		   IPvX("ffff:0000:ffff:0000:ffff:0000:ffff:0000"),
		   "operator<<");
    verbose_assert(IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") << 1 ==
		   IPvX("0001:fffe:0001:fffe:0001:fffe:0001:fffe"),
		   "operator<<");

    //
    // Operator >>
    //
    verbose_assert(IPvX("0.255.0.255") >> 16 == IPvX("0.0.0.255"),
		   "operator>>");
    verbose_assert(IPvX("0.0.0.255") >> 1 == IPvX("0.0.0.127"),
		   "operator>>");

    verbose_assert(IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") >> 16 ==
		   IPvX("0000:0000:ffff:0000:ffff:0000:ffff:0000"),
		   "operator>>");
    verbose_assert(IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") >> 1 ==
		   IPvX("0000:7fff:8000:7fff:8000:7fff:8000:7fff"),
		   "operator>>");

    //
    // Decrement Operator
    //
    verbose_assert(--IPvX("0.255.0.255") == IPvX("0.255.0.254"),
		   "operator--()");
    verbose_assert(--IPvX("0.0.0.0") == IPvX("255.255.255.255"),
		   "operator--()");

    verbose_assert(--IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") ==
		   IPvX("0000:ffff:0000:ffff:0000:ffff:0000:fffe"),
		   "operator--()");
    verbose_assert(--IPvX("0000:0000:0000:0000:0000:0000:0000:0000") ==
		   IPvX("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "operator--()");

    //
    // Increment Operator
    //
    verbose_assert(++IPvX("0.255.0.254") == IPvX("0.255.0.255"),
		   "operator++()");
    verbose_assert(++IPvX("255.255.255.255") == IPvX("0.0.0.0"),
		   "operator++()");

    verbose_assert(++IPvX("0000:ffff:0000:ffff:0000:ffff:0000:ffff") ==
		   IPvX("0000:ffff:0000:ffff:0000:ffff:0001:0000"),
		   "operator++()");
    verbose_assert(++IPvX("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") ==
		   IPvX("0000:0000:0000:0000:0000:0000:0000:0000"),
		   "operator++()");
}

/**
 * Test IPvX address type.
 */
void
test_ipvx_address_type()
{
    //
    // Test if this address is numerically zero.
    //
    verbose_assert(IPvX("0.0.0.0").is_zero(), "is_zero()");

    verbose_assert(IPvX("::").is_zero(), "is_zero()");

    //
    // Test if this address is a valid unicast address.
    //
    verbose_assert(IPvX("12.34.56.78").is_unicast(), "is_unicast()");

    verbose_assert(IPvX("fe80::1234:5678").is_unicast(), "is_unicast()");

    //
    // Test if this address is a valid multicast address.
    //
    verbose_assert(IPvX("224.1.2.3").is_multicast(), "is_multicast()");

    verbose_assert(IPvX("ff00::1").is_multicast(), "is_multicast()");


    //
    // Test if this address is a valid link-local unicast address.
    //
    verbose_assert(IPvX("12.34.56.78").is_linklocal_unicast() == false,
		       "is_linklocal_unicast()");

    verbose_assert(IPvX("fe80::2").is_linklocal_unicast(),
		   "is_linklocal_unicast()");
    verbose_assert(IPvX("ff02::2").is_linklocal_unicast() == false,
		   "is_linklocal_unicast()");

    //
    // Test if this address is a valid node-local multicast address.
    //
    verbose_assert(IPvX("224.0.0.1").is_nodelocal_multicast() == false,
		       "is_nodelocal_multicast()");

    verbose_assert(IPvX("ff01::1").is_nodelocal_multicast(),
		       "is_nodelocal_multicast()");

    //
    // Test if this address is a valid link-local multicast address.
    //
    verbose_assert(IPvX("224.0.0.2").is_linklocal_multicast(),
		       "is_linklocal_multicast()");

    verbose_assert(IPvX("ff02::2").is_linklocal_multicast(),
		       "is_linklocal_multicast()");

    //
    // Test if this address is a valid loopback address.
    //
    verbose_assert(IPvX("127.255.0.1").is_loopback(), "is_loopback()");
    verbose_assert(IPvX("0::1").is_loopback(), "is_loopback()");

    //
    // Test if this address is a valid loopback address.
    //
    verbose_assert(IPvX("126.255.0.127").is_loopback() == false,
		   "is_loopback()");
    verbose_assert(IPvX("1::0").is_loopback() == false, "is_loopback()");

}

/**
 * Test IPvX address constant values.
 */
void
test_ipvx_address_const()
{
    //
    // Test the address octet-size.
    //
    verbose_assert(IPvX::addr_size(AF_INET) == 4, "addr_size()");

    verbose_assert(IPvX::addr_size(AF_INET6) == 16, "addr_size()");

    //
    // Test the address bit-length.
    //
    verbose_assert(IPvX::addr_bitlen(AF_INET) == 32, "addr_bitlen()");

    verbose_assert(IPvX::addr_bitlen(AF_INET6) == 128, "addr_bitlen()");

    //
    // Test the mask length for the multicast base address.
    //
    verbose_assert(IPvX::ip_multicast_base_address_mask_len(AF_INET) == 4,
		   "ip_multicast_base_address_mask_len()");

    verbose_assert(IPvX::ip_multicast_base_address_mask_len(AF_INET6) == 8,
		   "ip_multicast_base_address_mask_len()");

    //
    // Test the address family.
    //
    IPvX ip1(AF_INET);
    verbose_assert(ip1.af() == AF_INET, "af()");

    IPvX ip2(AF_INET6);
    verbose_assert(ip2.af() == AF_INET6, "af()");

    //
    // Test the IP protocol version.
    //
    IPvX ip3(AF_INET);
    verbose_assert(ip3.ip_version() == 4, "ip_version()");
    verbose_assert(ip3.ip_version_str() == "IPv4", "ip_version_str()");

    IPvX ip4(AF_INET6);
    verbose_assert(ip4.ip_version() == 6, "ip_version()");
    verbose_assert(ip4.ip_version_str() == "IPv6", "ip_version_str()");

    //
    // Test pre-defined constant addresses
    //
    verbose_assert(IPvX::ZERO(AF_INET) == IPvX("0.0.0.0"), "ZERO()");
    verbose_assert(IPvX::ZERO(AF_INET6) == IPvX("::"), "ZERO()");

    verbose_assert(IPvX::ANY(AF_INET) == IPvX("0.0.0.0"), "ANY()");
    verbose_assert(IPvX::ANY(AF_INET6) == IPvX("::"), "ANY()");

    verbose_assert(IPvX::ALL_ONES(AF_INET) == IPvX("255.255.255.255"),
		   "ALL_ONES()");
    verbose_assert(IPvX::ALL_ONES(AF_INET6) ==
		   IPvX("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "ALL_ONES()");

    verbose_assert(IPvX::MULTICAST_BASE(AF_INET) == IPvX("224.0.0.0"),
		   "MULTICAST_BASE()");
    verbose_assert(IPvX::MULTICAST_BASE(AF_INET6) == IPvX("ff00::"),
		   "MULTICAST_BASE()");

    verbose_assert(IPvX::MULTICAST_ALL_SYSTEMS(AF_INET) == IPvX("224.0.0.1"),
		   "MULTICAST_ALL_SYSTEMS()");
    verbose_assert(IPvX::MULTICAST_ALL_SYSTEMS(AF_INET6) == IPvX("ff02::1"),
		   "MULTICAST_ALL_SYSTEMS()");

    verbose_assert(IPvX::MULTICAST_ALL_ROUTERS(AF_INET) == IPvX("224.0.0.2"),
		   "MULTICAST_ALL_ROUTERS()");
    verbose_assert(IPvX::MULTICAST_ALL_ROUTERS(AF_INET6) == IPvX("ff02::2"),
		   "MULTICAST_ALL_ROUTERS()");

    verbose_assert(IPvX::DVMRP_ROUTERS(AF_INET) == IPvX("224.0.0.4"),
		   "DVMRP_ROUTERS()");
    verbose_assert(IPvX::DVMRP_ROUTERS(AF_INET6) == IPvX("ff02::4"),
		   "DVMRP_ROUTERS()");

    verbose_assert(IPvX::OSPFIGP_ROUTERS(AF_INET) == IPvX("224.0.0.5"),
		   "OSPFIGP_ROUTERS()");
    verbose_assert(IPvX::OSPFIGP_ROUTERS(AF_INET6) == IPvX("ff02::5"),
		   "OSPFIGP_ROUTERS()");

    verbose_assert(IPvX::OSPFIGP_DESIGNATED_ROUTERS(AF_INET) == IPvX("224.0.0.6"),
		   "OSPIGP_DESIGNATED_ROUTERS()");
    verbose_assert(IPvX::OSPFIGP_DESIGNATED_ROUTERS(AF_INET6) == IPvX("ff02::6"),
		   "OSPIGP_DESIGNATED_ROUTERS()");

    verbose_assert(IPvX::RIP2_ROUTERS(AF_INET) == IPvX("224.0.0.9"),
		   "RIP2_ROUTERS()");
    verbose_assert(IPvX::RIP2_ROUTERS(AF_INET6) == IPvX("ff02::9"),
		   "RIP2_ROUTERS()");

    verbose_assert(IPvX::PIM_ROUTERS(AF_INET) == IPvX("224.0.0.13"),
		   "PIM_ROUTERS()");
    verbose_assert(IPvX::PIM_ROUTERS(AF_INET6) == IPvX("ff02::D"),
		   "PIM_ROUTERS()");
}

/**
 * Test IPvX address manipulation.
 */
void
test_ipvx_manipulate_address()
{
    const char *addr_string4 = "12.34.56.78";
    const char *addr_string6 = "1234:5678:9abc:def0:fed:cba9:8765:4321";

    //
    // Test making an IPvX mask prefix.
    //
    verbose_assert(IPvX().make_prefix(AF_INET, 24) == IPvX("255.255.255.0"),
		   "make_prefix()");
    verbose_assert(IPvX().make_prefix(AF_INET, 0) == IPvX("0.0.0.0"),
		   "make_prefix()");
    verbose_assert(IPvX().make_prefix(AF_INET, 32) == IPvX("255.255.255.255"),
		   "make_prefix()");

    verbose_assert(IPvX().make_prefix(AF_INET6, 24) == IPvX("ffff:ff00::"),
		   "make_prefix()");
    verbose_assert(IPvX().make_prefix(AF_INET6, 0) == IPvX("::"),
		   "make_prefix()");
    verbose_assert(IPvX().make_prefix(AF_INET6, 128) == IPvX("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
		   "make_prefix()");

    //
    // Test making an IPvX mask prefix for the address family of this address.
    //
    IPvX ip04(AF_INET);
    verbose_assert(ip04.make_prefix(24) == IPvX("255.255.255.0"),
		   "make_prefix()");

    IPvX ip06(AF_INET6);
    verbose_assert(ip06.make_prefix(24) == IPvX("ffff:ff00::"),
		   "make_prefix()");

    //
    // Test making an IPvX address prefix.
    //
    verbose_assert(
	IPvX("12.34.56.78").mask_by_prefix_len(24) == IPvX("12.34.56.0"),
	"mask_by_prefix_len()"
	);

    verbose_assert(
	IPvX("1234:5678:9abc:def0:fed:cba9:8765:4321").mask_by_prefix_len(24) ==
	IPvX("1234:5600::"),
	"mask_by_prefix_len()"
	);

    //
    // Test getting the prefix length of the contiguous mask.
    //
    verbose_assert(IPvX("255.255.255.0").mask_len() == 24,
		   "mask_len()");

    verbose_assert(IPvX("ffff:ff00::").mask_len() == 24,
		   "mask_len()");

    // XXX: for IPvX we don't have addr() and set_addr() methods, hence
    // we don't test them.

    //
    // Test extracting bits from an address.
    //
    verbose_assert(IPvX("12.34.56.78").bits(0, 8) == 78, "bits()");
    verbose_assert(IPvX("1234:5678:9abc:def0:fed:cba9:8765:4321").bits(0, 8)
		   == 0x21,
		   "bits()");

    //
    // Test if this address is IPv4 address.
    //
    IPvX ip1(AF_INET);
    verbose_assert(ip1.is_ipv4() == true, "is_ipv4()");

    //
    // Test if this address is IPv6 address.
    //
    IPvX ip2(AF_INET6);
    verbose_assert(ip2.is_ipv6() == true, "is_ipv6()");

    //
    // Get IPv4 address.
    //
    IPvX ip3(addr_string4);
    IPv4 ip3_ipv4(addr_string4);
    verbose_assert(ip3.get_ipv4() == ip3_ipv4, "get_ipv4()");

    //
    // Get IPv6 address.
    //
    IPvX ip4(addr_string6);
    IPv6 ip4_ipv6(addr_string6);
    verbose_assert(ip4.get_ipv6() == ip4_ipv6, "get_ipv6()");

    //
    // Assign address value to an IPv4 address.
    //
    IPvX ip5(addr_string4);
    IPv4 ip5_ipv4(addr_string4);
    IPv4 ip5_ipv4_tmp;
    ip5.get(ip5_ipv4_tmp);
    verbose_assert(ip5_ipv4_tmp == ip5_ipv4, "get(IPv4& to_ipv4)");

    //
    // Assign address value to an IPv6 address.
    //
    IPvX ip6(addr_string6);
    IPv6 ip6_ipv6(addr_string6);
    IPv6 ip6_ipv6_tmp;
    ip6.get(ip6_ipv6_tmp);
    verbose_assert(ip6_ipv6_tmp == ip6_ipv6, "get(IPv6& to_ipv6)");
}

/**
 * Test IPvX invalid address manipulation.
 */
void
test_ipvx_invalid_manipulate_address()
{
    const char *addr_string4 = "12.34.56.78";
    const char *addr_string6 = "1234:5678:9abc:def0:fed:cba9:8765:4321";

    //
    // Get invalid IPv4 address.
    //
    try {
	IPvX ip(addr_string6);		// Note: initialized with IPv6 address
	IPv4 ip_ipv4;
	ip_ipv4 = ip.get_ipv4();
	verbose_log("Cannot catch invalid get_ipv4() : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Get invalid IPv6 address.
    //
    try {
	IPvX ip(addr_string4);		// Note: initialized with IPv4 address
	IPv6 ip_ipv6;
	ip_ipv6 = ip.get_ipv6();
	verbose_log("Cannot catch invalid get_ipv4() : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Assign invalid address value to an IPv4 address.
    //
    try {
	IPvX ip(addr_string6);		// Note: initialized with IPv6 address
	IPv4 ip_ipv4;
	ip.get(ip_ipv4);
	verbose_log("Cannot catch invalid get(IPv4& to_ipv4) : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Assign invalid address value to an IPv6 address.
    //
    try {
	IPvX ip(addr_string4);		// Note: initialized with IPv4 address
	IPv6 ip_ipv6;
	ip.get(ip_ipv6);
	verbose_log("Cannot catch invalid get(IPv6& to_ipv6) : FAIL\n");
	incr_failures();
    } catch (const InvalidCast& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Test making an invalid IPvX mask prefix.
    //
    try {
	// Invalid prefix length
	IPvX ip(IPvX::make_prefix(AF_UNSPEC, 0));
	verbose_log("Cannot catch invalid IP address family AF_UNSPEC : FAIL\n");
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidFamily& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    try {
	// Invalid prefix length: IPv4
	IPvX ip(IPvX::make_prefix(AF_INET, IPvX::addr_bitlen(AF_INET) + 1));
	verbose_log("Cannot catch invalid IPv4 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPvX::addr_bitlen(AF_INET) + 1);
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    try {
	// Invalid prefix length: IPv6
	IPvX ip(IPvX::make_prefix(AF_INET6, IPvX::addr_bitlen(AF_INET6) + 1));
	verbose_log("Cannot catch invalid IPv6 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPvX::addr_bitlen(AF_INET6) + 1);
	incr_failures();
	UNUSED(ip);
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // Test masking with an invalid IPvX mask prefix.
    //
    try {
	// Invalid mask prefix: IPv4
	IPvX ip(addr_string4);
	ip.mask_by_prefix_len(IPvX::addr_bitlen(AF_INET) + 1);
	verbose_log("Cannot catch masking with an invalid IPv4 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPvX::addr_bitlen(AF_INET) + 1);
	incr_failures();
    } catch (const InvalidNetmaskLength& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
    try {
	// Invalid mask prefix: IPv6
	IPvX ip(addr_string6);
	ip.mask_by_prefix_len(IPvX::addr_bitlen(AF_INET6) + 1);
	verbose_log("Cannot catch masking with an invalid IPv6 mask prefix with length %u : FAIL\n",
		    (uint32_t)IPvX::addr_bitlen(AF_INET6) + 1);
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
	test_ipvx_valid_constructors();
	test_ipvx_invalid_constructors();
	test_ipvx_valid_copy_in_out();
	test_ipvx_invalid_copy_in_out();
	test_ipvx_operators();
	test_ipvx_address_type();
	test_ipvx_address_const();
	test_ipvx_manipulate_address();
	test_ipvx_invalid_manipulate_address();
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
