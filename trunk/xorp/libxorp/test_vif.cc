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

#ident "$XORP: xorp/libxorp/test_vif.cc,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/vif.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_vif";
static const char *program_description	= "Test Vif address class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "December 4, 2002";
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
 * Test VifAddr valid constructors.
 */
void
test_vif_addr_valid_constructors()
{
    //
    // Constructor for a given address.
    //
    VifAddr vif_addr1(IPvX("11.11.11.11"));
    verbose_match(vif_addr1.str(),
		  "addr: 11.11.11.11 subnet: 0.0.0.0/0 broadcast: 0.0.0.0 peer: 0.0.0.0");
    
    //
    // Constructor for a given address, and its associated addresses.
    //
    VifAddr vif_addr2(IPvX("22.22.22.22"),
		      IPvXNet("22.22.22.0/24"),
		      IPvX("22.22.22.255"),
		      IPvX("0.0.0.0"));
    verbose_match(vif_addr2.str(),
		  "addr: 22.22.22.22 subnet: 22.22.22.0/24 broadcast: 22.22.22.255 peer: 0.0.0.0");
    
    VifAddr vif_addr3(IPvX("33.33.33.33"),
		      IPvXNet("0.0.0.0/0"),
		      IPvX("0.0.0.0"),
		      IPvX("33.33.33.44"));
    verbose_match(vif_addr3.str(),
		  "addr: 33.33.33.33 subnet: 0.0.0.0/0 broadcast: 0.0.0.0 peer: 33.33.33.44");
}

/**
 * Test VifAddr invalid constructors.
 */
void
test_vif_addr_invalid_constructors()
{
    //
    // Currently, there are no VifAddr invalid constructors.
    //
}

/**
 * Test VifAddr methods.
 */
void
test_vif_addr_methods()
{
    VifAddr vif_addr_a(IPvX("11.11.11.11"),
		       IPvXNet("11.11.11.0/24"),
		       IPvX("11.11.11.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_b(IPvX("22.22.22.22"),
		       IPvXNet("22.22.22.0/24"),
		       IPvX("22.22.22.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_c(IPvX("33.33.33.33"),
		       IPvXNet("0.0.0.0/0"),
		       IPvX("0.0.0.0"),
		       IPvX("33.33.33.44"));
    
    //
    // Get the interface address.
    //
    verbose_match(vif_addr_a.addr().str(), "11.11.11.11");
    
    //
    // Get the subnet address.
    //
    verbose_match(vif_addr_a.subnet_addr().str(), "11.11.11.0/24");
    
    //
    // Get the broadcast address.
    //
    verbose_match(vif_addr_a.broadcast_addr().str(), "11.11.11.255");
    
    //
    // Get the peer address.
    //
    verbose_match(vif_addr_c.peer_addr().str(), "33.33.33.44");
    
    //
    // Set the interface address.
    //
    vif_addr_a.set_addr(IPvX("33.33.33.33"));
    verbose_match(vif_addr_a.addr().str(), "33.33.33.33");

    //
    // Set the subnet address.
    //
    vif_addr_a.set_subnet_addr(IPvXNet("33.33.33.0/24"));
    verbose_match(vif_addr_a.subnet_addr().str(), "33.33.33.0/24");

    //
    // Set the broadcast address.
    //
    vif_addr_a.set_broadcast_addr(IPvX("33.33.33.255"));
    verbose_match(vif_addr_a.broadcast_addr().str(), "33.33.33.255");

    //
    // Set the peer address.
    //
    vif_addr_c.set_peer_addr(IPvX("33.33.33.55"));
    verbose_match(vif_addr_c.peer_addr().str(), "33.33.33.55");
    
    //
    // Test whether is the same interface address.
    //
    verbose_assert(vif_addr_b.is_same_addr(IPvX("22.22.22.22")),
		   "is_same_addr()");

    verbose_assert(! vif_addr_b.is_same_addr(IPvX("22.22.22.33")),
		   "is_same_addr()");
    
    //
    // Test whether is the same subnet.
    //
    verbose_assert(vif_addr_b.is_same_subnet(IPvXNet("22.22.22.0/24")),
		   "is_same_subnet(IPvXNet)");

    verbose_assert(! vif_addr_b.is_same_subnet(IPvXNet("22.22.33.0/24")),
		   "is_same_subnet(IPvXNet)");
    
    //
    // Test whether an address belongs to my subnet.
    //
    verbose_assert(vif_addr_b.is_same_subnet(IPvX("22.22.22.33")),
		   "is_same_subnet(IPvX)");
    
    verbose_assert(! vif_addr_b.is_same_subnet(IPvX("22.22.33.33")),
		   "is_same_subnet(IPvX)");
}

/**
 * Test VifAddr operators.
 */
void
test_vif_addr_operators()
{
    VifAddr vif_addr_a(IPvX("11.11.11.11"),
		       IPvXNet("11.11.11.0/24"),
		       IPvX("11.11.11.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_b(IPvX("22.22.22.22"),
		       IPvXNet("22.22.22.0/24"),
		       IPvX("22.22.22.255"),
		       IPvX("0.0.0.0"));
    
    //
    // Equality Operator
    //
    verbose_assert(vif_addr_a == vif_addr_a, "operator==");
    verbose_assert(!(vif_addr_a == vif_addr_b), "operator==");
    
    //
    // Not-Equal Operator
    //
    verbose_assert(!(vif_addr_a != vif_addr_a), "operator!=");
    verbose_assert(vif_addr_a != vif_addr_b, "operator!=");
}

/**
 * Test Vif valid constructors.
 */
void
test_vif_valid_constructors()
{
    Vif vif1("vif1");
    Vif vif2("vif2", "ifname2");
    Vif vif3(vif2);
    
    UNUSED(vif1);
    UNUSED(vif2);
    UNUSED(vif3);
}

/**
 * Test Vif invalid constructors.
 */
void
test_vif_invalid_constructors()
{
    //
    // Currently, there are no Vif invalid constructors.
    //
}

/**
 * Test Vif methods.
 */
void
test_vif_methods()
{
    Vif vif1("vif1", "ifname1");
    
    //
    // Get the vif name.
    //
    verbose_match(vif1.name(), "vif1");
    
    //
    // Get the name of the physical interface associated with vif.
    //
    verbose_match(vif1.ifname(), "ifname1");
    
    //
    // Set the name of the physical interface associated with vif.
    //
    vif1.set_ifname("ifname1_1");
    verbose_match(vif1.ifname(), "ifname1_1");
    vif1.set_ifname("ifname1");
    
    //
    // Set and get the physical interface index.
    //
    vif1.set_pif_index(1);
    verbose_assert(vif1.pif_index() == 1, "pif_index()");

    //
    // Set and get the virtual interface index.
    //
    vif1.set_vif_index(2);
    verbose_assert(vif1.vif_index() == 2, "vif_index()");
    
    //
    // Test if this vif is a PIM Register interface.
    //
    vif1.set_pim_register(true);
    verbose_assert(vif1.is_pim_register(), "is_pim_register()");
    vif1.set_pim_register(false);
    verbose_assert(! vif1.is_pim_register(), "is_pim_register()");
    
    //
    // Test if this vif is a point-to-point interface.
    //
    vif1.set_p2p(true);
    verbose_assert(vif1.is_p2p(), "is_p2p()");
    vif1.set_p2p(false);
    verbose_assert(! vif1.is_p2p(), "is_p2p()");

    //
    // Test if this vif is a loopback interface.
    //
    vif1.set_loopback(true);
    verbose_assert(vif1.is_loopback(), "is_loopback()");
    vif1.set_loopback(false);
    verbose_assert(! vif1.is_loopback(), "is_loopback()");

    //
    // Test if this vif is multicast capable.
    //
    vif1.set_multicast_capable(true);
    verbose_assert(vif1.is_multicast_capable(), "is_multicast_capable()");
    vif1.set_multicast_capable(false);
    verbose_assert(! vif1.is_multicast_capable(), "is_multicast_capable()");

    //
    // Test if this vif is broadcast capable.
    //
    vif1.set_broadcast_capable(true);
    verbose_assert(vif1.is_broadcast_capable(), "is_broadcast_capable()");
    vif1.set_broadcast_capable(false);
    verbose_assert(! vif1.is_broadcast_capable(), "is_broadcast_capable()");

    //
    // Test if this vif is broadcast capable.
    //
    vif1.set_broadcast_capable(true);
    verbose_assert(vif1.is_broadcast_capable(), "is_broadcast_capable()");
    vif1.set_broadcast_capable(false);
    verbose_assert(! vif1.is_broadcast_capable(), "is_broadcast_capable()");
    
    //
    // Test if the underlying vif is UP.
    //
    vif1.set_underlying_vif_up(true);
    verbose_assert(vif1.is_underlying_vif_up(), "is_underlying_vif_up()");
    vif1.set_underlying_vif_up(false);
    verbose_assert(! vif1.is_underlying_vif_up(), "is_underlying_vif_up()");
    
    //
    // Get the default list of all addresses for this vif.
    //
    list<VifAddr> addr_list;
    addr_list = vif1.addr_list();
    verbose_assert(addr_list.size() == 0, "default addr_list()");
    
    //
    // Get the first vif address when no addresses were added.
    //
    verbose_assert(vif1.addr_ptr() == NULL, "default addr_ptr()");
}


/**
 * Test Vif address manipulation.
 */
void
test_vif_manipulate_address()
{
    Vif vif1("vif1", "ifname1");
    VifAddr vif_addr_a(IPvX("11.11.11.11"),
		       IPvXNet("11.11.11.0/24"),
		       IPvX("11.11.11.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_b(IPvX("22.22.22.22"),
		       IPvXNet("22.22.22.0/24"),
		       IPvX("22.22.22.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_c(IPvX("33.33.33.33"),
		       IPvXNet("0.0.0.0/0"),
		       IPvX("0.0.0.0"),
		       IPvX("33.33.33.44"));
    
    //
    // Assign vif capabilities
    //
    vif1.set_broadcast_capable(true);
    
    //
    // Add a VifAddr address to the interface.
    //
    verbose_assert(vif1.add_address(vif_addr_a) == XORP_OK,
		   "add_address()");
    verbose_assert(vif1.add_address(vif_addr_a) == XORP_ERROR,
		   "add_address()");
    verbose_assert(vif1.addr_list().size() == 1, "addr_list()");
    verbose_assert(vif1.addr_ptr() != NULL, "addr_ptr()");
    verbose_assert(*vif1.addr_ptr() == vif_addr_a.addr(), "addr_ptr()");
    
    //
    // Add an IPvX address and all related information to the interface.
    //
    verbose_assert(vif1.add_address(vif_addr_b.addr(),
				    vif_addr_b.subnet_addr(),
				    vif_addr_b.broadcast_addr(),
				    vif_addr_b.peer_addr())
		   == XORP_OK,
		   "add_address()");
    verbose_assert(vif1.add_address(vif_addr_b.addr(),
				    vif_addr_b.subnet_addr(),
				    vif_addr_b.broadcast_addr(),
				    vif_addr_b.peer_addr())
		   == XORP_ERROR,
		   "add_address()");
    verbose_assert(vif1.addr_list().size() == 2, "addr_list()");
    verbose_assert(vif1.addr_ptr() != NULL, "addr_ptr()");
    verbose_assert(*vif1.addr_ptr() == vif_addr_a.addr(), "addr_ptr()");
    
    //
    // Add an IPvX address to the interface.
    //
    verbose_assert(vif1.add_address(vif_addr_c.addr())
		   == XORP_OK,
		   "add_address()");
    verbose_assert(vif1.add_address(vif_addr_c.addr())
		   == XORP_ERROR,
		   "add_address()");
    verbose_assert(vif1.addr_list().size() == 3, "addr_list()");
    verbose_assert(vif1.addr_ptr() != NULL, "addr_ptr()");
    verbose_assert(*vif1.addr_ptr() == vif_addr_a.addr(), "addr_ptr()");
    
    //
    // Delete an IPvX address from the interface.
    //
    verbose_assert(vif1.delete_address(vif_addr_c.addr()) == XORP_OK,
		   "delete_address()");
    verbose_assert(vif1.delete_address(vif_addr_c.addr()) == XORP_ERROR,
		   "delete_address()");
    verbose_assert(vif1.addr_list().size() == 2, "addr_list()");
    verbose_assert(vif1.addr_ptr() != NULL, "addr_ptr()");
    verbose_assert(*vif1.addr_ptr() == vif_addr_a.addr(), "addr_ptr()");
    
    //
    // Find a VifAddr that corresponds to an IPvX address.
    //
    verbose_assert(vif1.find_address(vif_addr_a.addr()) != NULL,
		   "find_address()");
    verbose_assert(*vif1.find_address(vif_addr_a.addr()) == vif_addr_a,
		   "find_address()");
    
    //
    // Test if an IPvX address belongs to this vif.
    //
    verbose_assert(vif1.is_my_addr(vif_addr_a.addr()), "is_my_addr()");
    verbose_assert(! vif1.is_my_addr(vif_addr_c.addr()), "is_my_addr()");
    
    //
    // Test if an VifAddr is belongs to this vif.
    //
    verbose_assert(vif1.is_my_vif_addr(vif_addr_a), "is_my_vif_addr()");
    verbose_assert(! vif1.is_my_vif_addr(vif_addr_c), "is_my_vif_addr()");
    
    //
    // Test if this vif belongs to a given subnet.
    //
    verbose_assert(vif1.is_same_subnet(vif_addr_a.subnet_addr()),
		   "is_same_subnet(IPvXNet)");
    verbose_assert(! vif1.is_same_subnet(IPvXNet("99.99.99.0/24")),
		   "is_same_subnet(IPvXNet)");

    //
    // Test if this vif belongs to a given subnet.
    //
    verbose_assert(vif1.is_same_subnet(IPvX("11.11.11.22")),
		   "is_same_subnet(IPvX)");
    verbose_assert(! vif1.is_same_subnet(IPvX("99.99.99.99")),
		   "is_same_subnet(IPvX)");
    
    //
    // Test if a given address is directly connected to this vif: broadcast vif
    //
    verbose_assert(vif1.is_directly_connected(IPvX("11.11.11.22")),
		   "is_directly_connected()");
    verbose_assert(! vif1.is_directly_connected(IPvX("99.99.99.99")),
		   "is_directly_connected()");

    //
    // Assign vif capabilities
    //
    vif1.set_broadcast_capable(false);
    vif1.set_p2p(true);
    vif1.add_address(vif_addr_c);
    verbose_assert(vif1.is_directly_connected(vif_addr_c.peer_addr()),
		   "is_directly_connected()");
    verbose_assert(! vif1.is_directly_connected(IPvX("99.99.99.99")),
		   "is_directly_connected()");
    
    //
    // Convert this Vif from binary form to presentation format.
    //
    verbose_match(vif1.str(),
		  "Vif[vif1] pif_index: 0 vif_index: 0 addr: 11.11.11.11 subnet: 11.11.11.0/24 broadcast: 11.11.11.255 peer: 0.0.0.0 addr: 22.22.22.22 subnet: 22.22.22.0/24 broadcast: 22.22.22.255 peer: 0.0.0.0 addr: 33.33.33.33 subnet: 0.0.0.0/0 broadcast: 0.0.0.0 peer: 33.33.33.44 Flags: P2P");
}

/**
 * Test Vif operators.
 */
void
test_vif_operators()
{
    Vif vif1("vif1", "ifname1");
    Vif vif2("vif2");
    VifAddr vif_addr_a(IPvX("11.11.11.11"),
		       IPvXNet("11.11.11.0/24"),
		       IPvX("11.11.11.255"),
		       IPvX("0.0.0.0"));
    VifAddr vif_addr_b(IPvX("22.22.22.22"),
		       IPvXNet("22.22.22.0/24"),
		       IPvX("22.22.22.255"),
		       IPvX("0.0.0.0"));
    
    //
    // Equality Operator
    //
    verbose_assert(vif1 == vif1, "operator==");
    verbose_assert(!(vif1 == vif2), "operator==");
    
    //
    // Not-Equal Operator
    //
    verbose_assert(!(vif1 != vif1), "operator!=");
    verbose_assert(vif1 != vif2, "operator!=");

    //
    // Equality Operator
    //
    vif1.add_address(vif_addr_a);
    vif1.add_address(vif_addr_b);
    Vif vif3(vif1);
    verbose_assert(vif1 == vif3, "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(vif1 != vif3), "operator!=");
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
	test_vif_addr_valid_constructors();
	test_vif_addr_invalid_constructors();
	test_vif_addr_methods();
	test_vif_addr_operators();
	
	test_vif_valid_constructors();
	test_vif_invalid_constructors();
	test_vif_methods();
	test_vif_manipulate_address();
	test_vif_operators();
	
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
