// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_mac.cc,v 1.17 2008/01/04 03:16:42 pavlin Exp $"

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
    const string addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    
    //
    // Default constructor.
    //
    Mac mac1;
    verbose_match(mac1.str(), Mac::ZERO().str());
    
    //
    // Constructor from a string.
    //
    Mac mac2(addr_string);
    verbose_match(mac2.str(), addr_string);
    
    //
    // Constructor from another Mac address.
    //
    Mac mac3(mac2);
    verbose_match(mac3.str(), addr_string);

    //
    // Constructor from a (uint8_t *) memory pointer.
    //
    Mac mac4(ui8_addr, sizeof(ui8_addr));
    verbose_match(mac4.str(), addr_string);

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

    //
    // Constructor from memory location with invalid length.
    //
    try {
	// Memory location with invalid length.
	const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
	Mac mac(ui8_addr, sizeof(ui8_addr) - 1);
	verbose_log("Cannot catch Mac address with invalid length : FAIL\n");
	incr_failures();
	UNUSED(mac);
    } catch (const BadMac& e) {
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
    const string addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

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
    // Copy a raw address into Mac address.
    //
    Mac mac2;
    verbose_assert(mac2.copy_in(&ui8_addr[0], sizeof(ui8_addr)) == 6,
		   "copy_in(uint8_t *) for Mac address");
    verbose_match(mac2.str(), addr_string);

    //
    // Copy a string address into Mac address.
    //
    Mac mac3;
    verbose_assert(mac3.copy_in(addr_string) == 6,
		   "copy_in(string) for Mac address");
    verbose_match(mac3.str(), addr_string);

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

    //
    // Copy-in from memory location with invalid length.
    //
    try {
	// Memory location with invalid length.
	const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
	Mac mac;
	mac.copy_in(ui8_addr, sizeof(ui8_addr) - 1);
	verbose_log("Cannot catch Mac address with invalid length : FAIL\n");
	incr_failures();
    } catch (const BadMac& e) {
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
    // Equality Operator
    //
    verbose_assert(mac_a == mac_a, "operator==");
    verbose_assert(!(mac_a == mac_b), "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(mac_a != mac_a), "operator!=");
    verbose_assert(mac_a != mac_b, "operator!=");

    //
    // Less-Than Operator
    //
    verbose_assert(mac_a < mac_b, "operator<");

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
    // Test pre-defined constant addresses
    //
    verbose_assert(Mac::ZERO() == Mac("00:00:00:00:00:00"), "ZERO()");

    verbose_assert(Mac::ALL_ONES() == Mac("ff:ff:ff:ff:ff:ff"), "ALL_ONES()");

    verbose_assert(Mac::LLDP_MULTICAST() == Mac("01:80:c2:00:00:0e"),
		   "LLDP_MULTICAST()");

    return (! failures());
}

/**
 * Test EtherMac valid constructors.
 */
bool
test_ethermac_valid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for EtherMac address: "11:22:33:44:55:66"
    const string addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    struct ether_addr ether_addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
    memcpy(&sa.sa_data, ui8_addr, sizeof(ui8_addr));

    //
    // Default constructor.
    //
    EtherMac ether_mac1;
    verbose_match(ether_mac1.str(), Mac::ZERO().str());
    
    //
    // Constructor from a string.
    //
    EtherMac ether_mac2(addr_string);
    verbose_match(ether_mac2.str(), addr_string);
    
    //
    // Constructor from another EtherMac address.
    //
    EtherMac ether_mac3(ether_mac2);
    verbose_match(ether_mac3.str(), addr_string);

    //
    // Constructor from Mac address.
    //
    Mac mac4(addr_string);
    EtherMac ether_mac4(mac4);
    verbose_match(ether_mac4.str(), addr_string);

    //
    // Constructor from a (uint8_t *) memory pointer.
    //
    EtherMac ether_mac5(ui8_addr);
    verbose_match(ether_mac5.str(), addr_string);

    //
    // Constructor from ether_addr structure.
    //
    EtherMac ether_mac6(ether_addr);
    verbose_match(ether_mac6.str(), addr_string);

    //
    // Constructor from sockaddr structure.
    //
    EtherMac ether_mac7(sa);
    verbose_match(ether_mac7.str(), addr_string);

    return (! failures());
}

/**
 * Test EtherMac invalid constructors.
 */
bool
test_ethermac_invalid_constructors(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Constructor from an invalid address string.
    //
    try {
	// Invalid address string
	EtherMac ether_mac("hello");
	verbose_log("Cannot catch invalid EtherMac address \"hello\" : FAIL\n");
	incr_failures();
	UNUSED(ether_mac);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // XXX: We can't really have ether_addr with invalid address, hence
    // we skip the test for constructor from invalid ether_addr address.
    //

    //
    // XXX: We can't really have sockaddr with invalid address, hence
    // we skip the test for constructor from invalid sockaddr address.
    //

    //
    // XXX: Currently we don't support Mac addresses that are not EtherMac
    // addresses, hence we skip the test for constructor from invalid
    // Mac address.
    //

    return (! failures());
}

/**
 * Test EtherMac valid copy in/out methods.
 */
bool
test_ethermac_valid_copy_in_out(TestInfo& test_info)
{
    UNUSED(test_info);

    // Test values for EtherMac address: "11:22:33:44:55:66"
    const string addr_string = "11:22:33:44:55:66";
    const uint8_t ui8_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    struct ether_addr ether_addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    struct sockaddr sa;

    memset(&sa, 0, sizeof(sa));
    memcpy(&sa.sa_data, ui8_addr, sizeof(ui8_addr));

    //
    // Copy the EtherMac raw address to specified memory location.
    //
    EtherMac ether_mac1(addr_string);
    uint8_t ether_mac1_uint8[6];
    verbose_assert(ether_mac1.copy_out(&ether_mac1_uint8[0]) == 6,
		   "copy_out(uint8_t *) for EtherMac address");
    verbose_assert(memcmp(&ui8_addr, &ether_mac1_uint8[0], 6) == 0,
		   "compare copy_out(uint8_t *) for EtherMac address");

    //
    // Copy the EtherMac raw address to ether_addr structure.
    //
    EtherMac ether_mac2(addr_string);
    struct ether_addr ether_mac2_ether_addr;
    verbose_assert(ether_mac2.copy_out(ether_mac2_ether_addr) == 6,
		   "copy_out(ether_addr&) for EtherMac address");
    verbose_assert(memcmp(&ui8_addr, &ether_mac2_ether_addr, 6) == 0,
		   "compare copy_out(ether_addr&) for EtherMac address");

    //
    // Copy the EtherMac raw address to sockaddr structure.
    //
    EtherMac ether_mac3(addr_string);
    struct sockaddr ether_mac3_sockaddr;
    verbose_assert(ether_mac3.copy_out(ether_mac3_sockaddr) == 6,
		   "copy_out(sockaddr&) for EtherMac address");
    verbose_assert(memcmp(&ui8_addr, &ether_mac3_sockaddr.sa_data, 6) == 0,
		   "compare copy_out(sockaddr&) for EtherMac address");

    //
    // Copy the EtherMac raw address to Mac address.
    //
    EtherMac ether_mac4(addr_string);
    Mac ether_mac4_mac;
    verbose_assert(ether_mac4.copy_out(ether_mac4_mac) == 6,
		   "copy_out(mac&) for EtherMac address");
    verbose_match(ether_mac4_mac.str(), addr_string);

    //
    // Copy a raw address into EtherMac address.
    //
    EtherMac ether_mac5;
    verbose_assert(ether_mac5.copy_in(&ui8_addr[0]) == 6,
		   "copy_in(uint8_t *) for EtherMac address");
    verbose_match(ether_mac5.str(), addr_string);

    //
    // Copy a string address into EtherMac address.
    //
    EtherMac ether_mac6;
    verbose_assert(ether_mac6.copy_in(addr_string) == 6,
		   "copy_in(string) for EtherMac address");
    verbose_match(ether_mac6.str(), addr_string);

    //
    // Copy a raw address from a ether_addr structure into EtherMac address.
    //
    EtherMac ether_mac7;
    verbose_assert(ether_mac7.copy_in(ether_addr) == 6,
		   "copy_in(ether_addr&) for EtherMac address");
    verbose_match(ether_mac7.str(), addr_string);

    //
    // Copy a raw address from a sockaddr structure into EtherMac address.
    //
    EtherMac ether_mac8;
    verbose_assert(ether_mac8.copy_in(ether_addr) == 6,
		   "copy_in(sockaddr&) for EtherMac address");
    verbose_match(ether_mac8.str(), addr_string);

    //
    // Copy a Mac address into EtherMac address.
    //
    EtherMac ether_mac9;
    Mac ether_mac9_mac(addr_string);
    verbose_assert(ether_mac9.copy_in(ether_mac9_mac) == 6,
		   "copy_in(Mac&) for EtherMac address");
    verbose_match(ether_mac9.str(), addr_string);

    return (! failures());
}

/**
 * Test EtherMac invalid copy in/out methods.
 */
bool
test_ethermac_invalid_copy_in_out(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Copy-in from invalid string.
    //
    try {
	EtherMac ether_mac;
	ether_mac.copy_in(string("hello"));
	verbose_log("Cannot catch invalid EtherMac address \"hello\" : FAIL\n");
	incr_failures();
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }

    //
    // XXX: Currently we don't support Mac addresses that are not EtherMac
    // addresses, hence we skip the test for copy-in from invalid
    // Mac address.
    //

    return (! failures());
}

/**
 * Test EtherMac operators.
 */
bool
test_ethermac_operators(TestInfo& test_info)
{
    UNUSED(test_info);

    EtherMac ether_mac_a("00:00:00:00:00:00");
    EtherMac ether_mac_b("11:11:11:11:11:11");

    //
    // Equality Operator
    //
    verbose_assert(ether_mac_a == ether_mac_a, "operator==");
    verbose_assert(!(ether_mac_a == ether_mac_b), "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(ether_mac_a != ether_mac_a), "operator!=");
    verbose_assert(ether_mac_a != ether_mac_b, "operator!=");

    //
    // Less-Than Operator
    //
    verbose_assert(ether_mac_a < ether_mac_b, "operator<");

    return (! failures());
}

/**
 * Test EtherMac address type.
 */
bool
test_ethermac_address_type(TestInfo& test_info)
{
    UNUSED(test_info);

    EtherMac ether_mac_default;			// Default (zero) address
    EtherMac ether_mac_zero("00:00:00:00:00:00");	// Zero address
    EtherMac ether_mac_unicast("00:11:11:11:11:11");	// Unicast
    EtherMac ether_mac_multicast("01:22:22:22:22:22");	// Multicast

    //
    // Test the size of an address (in octets).
    //
    verbose_assert(ether_mac_default.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(ether_mac_zero.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(ether_mac_unicast.addr_bytelen() == 6, "addr_bytelen()");
    verbose_assert(ether_mac_multicast.addr_bytelen() == 6, "addr_bytelen()");

    //
    // Test the size of an address (in bits).
    //
    verbose_assert(ether_mac_default.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(ether_mac_zero.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(ether_mac_unicast.addr_bitlen() == 48, "addr_bitlen()");
    verbose_assert(ether_mac_multicast.addr_bitlen() == 48, "addr_bitlen()");

    //
    // Test if an address is numerically zero.
    //
    verbose_assert(ether_mac_default.is_zero() == true, "is_zero()");
    verbose_assert(ether_mac_zero.is_zero() == true, "is_zero()");
    verbose_assert(ether_mac_unicast.is_zero() == false, "is_zero()");
    verbose_assert(ether_mac_multicast.is_zero() == false, "is_zero()");

    //
    // Test if an address is a valid multicast address.
    //
    verbose_assert(ether_mac_default.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ether_mac_zero.is_multicast() == false, "is_multicast()");
    verbose_assert(ether_mac_unicast.is_multicast() == false,
		   "is_multicast()");
    verbose_assert(ether_mac_multicast.is_multicast() == true,
		   "is_multicast()");

    return (! failures());
}

/**
 * Test EtherMac address constant values.
 */
bool
test_ethermac_address_const(TestInfo& test_info)
{
    UNUSED(test_info);

    //
    // Test the number of bits in address
    //
    verbose_assert(EtherMac::ADDR_BITLEN == 48, "ADDR_BITLEN");

    //
    // Test the number of bytes in address
    //
    verbose_assert(EtherMac::ADDR_BYTELEN == 6, "ADDR_BYTELEN");

    //
    // Test the multicast bit in the first octet of the address
    //
    verbose_assert(EtherMac::MULTICAST_BIT == 1, "MULTICAST_BIT");

    //
    // Test pre-defined constant addresses
    //
    verbose_assert(EtherMac(Mac::ZERO()) == EtherMac("00:00:00:00:00:00"),
		   "ZERO()");

    verbose_assert(EtherMac(Mac::ALL_ONES()) == EtherMac("ff:ff:ff:ff:ff:ff"),
		   "ALL_ONES()");

    verbose_assert(EtherMac(Mac::LLDP_MULTICAST())
		   == EtherMac("01:80:c2:00:00:0e"),
		   "LLDP_MULTICAST()");

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
	},
	{ "test_ethermac_valid_constructors",
	  callback(test_ethermac_valid_constructors),
	  true
	},
	{ "test_ethermac_invalid_constructors",
	  callback(test_ethermac_invalid_constructors),
	  true
	},
	{ "test_ethermac_valid_copy_in_out",
	  callback(test_ethermac_valid_copy_in_out),
	  true
	},
	{ "test_ethermac_invalid_copy_in_out",
	  callback(test_ethermac_invalid_copy_in_out),
	  true
	},
	{ "test_ethermac_operators",
	  callback(test_ethermac_operators),
	  true
	},
	{ "test_ethermac_address_type",
	  callback(test_ethermac_address_type),
	  true
	},
	{ "test_ethermac_address_const",
	  callback(test_ethermac_address_const),
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
