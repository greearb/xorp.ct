// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/libproto/test_config_node_id.cc,v 1.8 2007/02/16 22:46:03 pavlin Exp $"

#include "libproto_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libproto/config_node_id.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_config_node_id";
static const char *program_description	= "Test ConfigNodeId class";
static const char *program_version_id	= "0.1";
static const char *program_date		= "September 23, 2005";
static const char *program_copyright	= "See file LICENSE";
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
 * Test ConfigNodeId valid constructors.
 */
void
test_config_node_id_valid_constructors()
{
    // Test values for node ID and node position: "1" and "2"
    const string config_node_id_string = "1 2";
    const string config_node_id_empty_string = "";
    const ConfigNodeId::UniqueNodeId unique_node_id = 1;
    const ConfigNodeId::Position position = 2;

    //
    // Constructor from a string.
    //
    ConfigNodeId config_node_id1(config_node_id_string);
    verbose_match(config_node_id1.str(), config_node_id_string);

    //
    // Constructor from an empty string.
    //
    ConfigNodeId config_node_id1_1(config_node_id_empty_string);
    verbose_match(config_node_id1_1.str(), string("0 0"));
    verbose_assert(config_node_id1_1.is_empty(), "is_empty()");

    //
    // Constructor from another ConfigNodeId.
    //
    ConfigNodeId config_node_id2(config_node_id1);
    verbose_match(config_node_id2.str(), config_node_id_string);

    //
    // Constructor from integer values.
    //
    ConfigNodeId config_node_id3(unique_node_id, position);
    verbose_match(config_node_id3.str(), config_node_id_string);
    verbose_assert(config_node_id3.unique_node_id() == unique_node_id,
		   "compare unique_node_id()");
    verbose_assert(config_node_id3.position() == position,
		   "compare position()");
}

/**
 * Test ConfigNodeId invalid constructors.
 */
void
test_config_node_id_invalid_constructors()
{
    // Invalid test values for node ID and node position: "A" and "B"
    const string invalid_config_node_id_string = "A B";

    //
    // Constructor from an invalid init string.
    //
    try {
	// Invalid init string
	ConfigNodeId config_node_id(invalid_config_node_id_string);
	verbose_log("Cannot catch invalid ConfigNodeId string \"A B\" : FAIL\n");
	incr_failures();
	UNUSED(config_node_id);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test ConfigNodeId valid copy in/out methods.
 */
void
test_config_node_id_valid_copy_in_out()
{
    // Test values for node ID and node position: "1" and "2"
    const string config_node_id_string = "1 2";

    //
    // Copy a node ID from a string into ConfigNodeId structure.
    //
    ConfigNodeId config_node_id1(0, 0);
    verbose_assert(config_node_id1.copy_in(config_node_id_string) == 3,
		   "copy_in(string&) for ConfigNodeId");
    verbose_match(config_node_id1.str(), config_node_id_string);
}

/**
 * Test ConfigNodeId invalid copy in/out methods.
 */
void
test_config_node_id_invalid_copy_in_out()
{
    // Invalid test values for node ID and node position: "A" and "B"
    const string invalid_config_node_id_string = "A B";

    //
    // Constructor from an invalid init string.
    //
    try {
	// Invalid init string
	ConfigNodeId config_node_id(0, 0);
	config_node_id.copy_in(invalid_config_node_id_string);
	verbose_log("Cannot catch invalid ConfigNodeId string \"A B\" : FAIL\n");
	incr_failures();
	UNUSED(config_node_id);
    } catch (const InvalidString& e) {
	// The problem was caught
	verbose_log("%s : OK\n", e.str().c_str());
    }
}

/**
 * Test ConfigNodeId operators.
 */
void
test_config_node_id_operators()
{
    ConfigNodeId config_node_id_a("1 2");
    ConfigNodeId config_node_id_b("1 3");
    ConfigNodeId config_node_id_c("2 3");

    //
    // Equality Operator
    //
    verbose_assert(config_node_id_a == config_node_id_a, "operator==");
    verbose_assert(!(config_node_id_a == config_node_id_b), "operator==");
    verbose_assert(!(config_node_id_a == config_node_id_c), "operator==");

    //
    // Not-Equal Operator
    //
    verbose_assert(!(config_node_id_a != config_node_id_a), "operator!=");
    verbose_assert(config_node_id_a != config_node_id_b, "operator!=");
    verbose_assert(config_node_id_a != config_node_id_c, "operator!=");
}

/**
 * Test ConfigNodeId constant values.
 */
void
test_config_node_id_const()
{
    //
    // Test pre-defined constant values
    //
    verbose_assert(ConfigNodeId::ZERO() == ConfigNodeId("0 0"), "ZERO()");
}

/**
 * Test ConfigNodeId miscellaneous methods.
 */
void
test_config_node_id_misc()
{
    ConfigNodeId::UniqueNodeId unique_node_id = 1;
    const ConfigNodeId::Position position = 2;
    const ConfigNodeId::InstanceId instance_id = 3;

    //
    // Test ConfigNodeId::set_instance_id()
    //
    ConfigNodeId config_node_id1(unique_node_id, position);
    verbose_match(config_node_id1.str(), "1 2");
    config_node_id1.set_instance_id(instance_id);
    verbose_match(config_node_id1.str(), "12884901889 2");

    //
    // Test ConfigNodeId::generate_unique_node_id()
    //
    ConfigNodeId config_node_id2(unique_node_id, position);
    config_node_id2.set_instance_id(instance_id);
    ConfigNodeId config_node_id3 = config_node_id2.generate_unique_node_id();
    verbose_match(config_node_id3.str(), config_node_id2.str());
    verbose_match(config_node_id3.str(), "12884901890 2");
}

/**
 * Test ConfigNodeIdMap class.
 */
void
test_config_node_id_map()
{
    ConfigNodeId config_node_id1("3 0");
    ConfigNodeId config_node_id2("2 3");
    ConfigNodeId config_node_id3("1 2");
    ConfigNodeId config_node_id_out_of_order("50 60");
    ConfigNodeId config_node_id_unknown("10 20");
    ConfigNodeIdMap<uint32_t> node_id_map;
    ConfigNodeIdMap<uint32_t>::iterator iter;
    string test_string;
    uint32_t foo = 1;

    //
    // Insert the elements
    //
    verbose_assert(node_id_map.insert(config_node_id1, foo++).second == true,
		   "insert(config_node_id1)");
    verbose_assert(node_id_map.insert(config_node_id2, foo++).second == true,
		   "insert(config_node_id2)");
    verbose_assert(node_id_map.insert(config_node_id3, foo++).second == true,
		   "insert(config_node_id3)");
    verbose_assert(node_id_map.insert_out_of_order(config_node_id_out_of_order,
						   foo++).second == true,
		   "insert(config_node_id_out_of_order)");
    test_string = config_node_id1.str() + ", " + config_node_id2.str() + ", "
	+ config_node_id3.str() + ", " + config_node_id_out_of_order.str(); 
    verbose_match(node_id_map.str(), test_string);

    //
    // Try to reinsert an element with the same node ID. This should fail.
    //
    verbose_assert(node_id_map.insert(config_node_id3, foo++).second == false,
		   "insert(config_node_id3)");

    // Test for an unknown element
    verbose_assert(node_id_map.find(config_node_id_unknown)
		   == node_id_map.end(), "find(config_node_id_unknown)");

    // Test the elements
    verbose_assert(node_id_map.size() == 4, "size(4)");
    verbose_assert(node_id_map.empty() == false, "empty()");
    iter = node_id_map.begin();
    verbose_match(iter->first.str(), config_node_id1.str());
    verbose_assert(node_id_map.find(config_node_id1) == iter,
		   "find(config_node_id1)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id2.str());
    verbose_assert(node_id_map.find(config_node_id2) == iter,
		   "find(config_node_id2)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id3.str());
    verbose_assert(node_id_map.find(config_node_id3) == iter,
		   "find(config_node_id3)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id_out_of_order.str());
    verbose_assert(node_id_map.find(config_node_id_out_of_order) == iter,
		   "find(config_node_id_out_of_order)");

    // Erase the first element by using an interator
    iter = node_id_map.begin();
    node_id_map.erase(iter);

    // Test the remaining elements
    verbose_assert(node_id_map.size() == 3, "size(3)");
    verbose_assert(node_id_map.empty() == false, "empty()");
    verbose_assert(node_id_map.find(config_node_id1) == node_id_map.end(),
		   "find(config_node_id1)");
    test_string = config_node_id2.str() + ", " + config_node_id3.str() + ", "
	+ config_node_id_out_of_order.str();
    verbose_match(node_id_map.str(), test_string);
    iter = node_id_map.begin();
    verbose_match(iter->first.str(), config_node_id2.str());
    verbose_assert(node_id_map.find(config_node_id2) == iter,
		   "find(config_node_id2)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id3.str());
    verbose_assert(node_id_map.find(config_node_id3) == iter,
		   "find(config_node_id3)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id_out_of_order.str());
    verbose_assert(node_id_map.find(config_node_id_out_of_order) == iter,
		   "find(config_node_id_out_of_order)");

    // Erase the new first element by using a node ID
    iter = node_id_map.begin();
    node_id_map.erase(iter->first);

    // Test the remaining elements
    verbose_assert(node_id_map.size() == 2, "size(2)");
    verbose_assert(node_id_map.empty() == false, "empty()");
    verbose_assert(node_id_map.find(config_node_id2) == node_id_map.end(),
		   "find(config_node_id2)");
    test_string = config_node_id3.str() + ", "
	+ config_node_id_out_of_order.str();
    verbose_match(node_id_map.str(), test_string);
    iter = node_id_map.begin();
    verbose_match(iter->first.str(), config_node_id3.str());
    verbose_assert(node_id_map.find(config_node_id3) == iter,
		   "find(config_node_id3)");
    ++iter;
    verbose_match(iter->first.str(), config_node_id_out_of_order.str());
    verbose_assert(node_id_map.find(config_node_id_out_of_order) == iter,
		   "find(config_node_id_out_of_order)");

    // Remove all elements
    node_id_map.clear();
    verbose_assert(node_id_map.size() == 0, "size(0)");
    verbose_assert(node_id_map.empty() == true, "empty()");
    verbose_assert(node_id_map.find(config_node_id3) == node_id_map.end(),
		   "find(config_node_id3)");
    verbose_assert(node_id_map.find(config_node_id_out_of_order)
		   == node_id_map.end(),
		   "find(config_node_id_out_of_order)");
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
	test_config_node_id_valid_constructors();
	test_config_node_id_invalid_constructors();
	test_config_node_id_valid_copy_in_out();
	test_config_node_id_invalid_copy_in_out();
	test_config_node_id_operators();
	test_config_node_id_const();
	test_config_node_id_misc();
	test_config_node_id_map();
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
