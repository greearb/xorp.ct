// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_ipv4net.cc,v 1.4 2002/12/09 18:29:14 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4net.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_ipv4net";
static const char *program_description	= "Test IPv4Net address class";
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
 * Test IPv4Net valid constructors.
 */
void
test_ipv4net_valid_constructors()
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
}

/**
 * Test IPv4Net invalid constructors.
 */
void
test_ipv4net_invalid_constructors()
{
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
}

/**
 * Test IPv4Net operators.
 */
void
test_ipv4net_operators()
{
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
    // Bool Operator
    //
    verbose_assert(! IPv4Net(), "operator bool");
    verbose_assert(! IPv4Net("0.0.0.0/0"), "operator bool");
    verbose_assert(IPv4Net("0.0.0.0/1"), "operator bool");
}

/**
 * Test IPv4Net address type.
 */
void
test_ipv4net_address_type()
{
    IPv4Net ipnet_a("12.34.0.0/16");
    
    //
    // Test if this subnet is within the multicast address range.
    //
    IPv4Net ipnet1("224.0.1.0/24");
    verbose_assert(ipnet_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet1.is_multicast() == true, "is_multicast()");
}

/**
 * Test IPv4Net address overlap.
 */
void
test_ipv4net_address_overlap()
{
    IPv4Net ipnet_a("12.34.0.0/16");
    IPv4Net ipnet_b("12.35.0.0/16");
    IPv4Net ipnet_c("12.34.56.0/24");
    
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
}

/**
 * Test IPv4Net address constant values.
 */
void
test_ipv4net_address_const()
{
    IPv4Net ipnet_a("12.34.0.0/16");
    
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
    // Test if this subnet is within the multicast address range.
    //
    IPv4Net ipnet1("224.0.1.0/24");
    verbose_assert(ipnet_a.is_multicast() == false, "is_multicast()");
    verbose_assert(ipnet1.is_multicast() == true, "is_multicast()");
}

/**
 * Test IPv4Net address manipulation.
 */
void
test_ipv4net_manipulate_address()
{
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
	test_ipv4net_valid_constructors();
	test_ipv4net_invalid_constructors();
	test_ipv4net_operators();
	test_ipv4net_address_type();
	test_ipv4net_address_overlap();
	test_ipv4net_address_const();
	test_ipv4net_manipulate_address();
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
