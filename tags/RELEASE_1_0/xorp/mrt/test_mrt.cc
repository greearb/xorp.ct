// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/mrt/test_mrt.cc,v 1.4 2004/02/26 04:22:06 pavlin Exp $"


//
// Multicast Routing Table test program.
//


#include "mrt_module.h"

#include <list>

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "mrt/mrt.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_mrt";
static const char *program_description	= "Test Multicast Routing Table";
static const char *program_version_id	= "0.1";
static const char *program_date		= "February 25, 2004";
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
 * Multicast Routing Entry test class.
 */
class MyMre : public Mre<MyMre> {
public:
    MyMre(const IPvX& source, const IPvX& group)
	: Mre<MyMre>(source, group) {}
};

string
mre_list_str(const list<MyMre *>& mre_list)
{
    list<MyMre *>::const_iterator iter;
    string res;

    for (iter = mre_list.begin(); iter != mre_list.end(); ++iter) {
	const MyMre *t = *iter;
	res += cstring(*t);
    }
    
    return res;
}

void
test_mrt()
{
    Mrt<MyMre> mrt_4;
    Mrt<MyMre> mrt_6;
    Mrt<MyMre>::const_sg_iterator sg_iter, sg_iter_begin, sg_iter_end;
    Mrt<MyMre>::const_gs_iterator gs_iter, gs_iter_begin, gs_iter_end;
    // IPv4 values
    IPvX s1_4(IPv4("123.45.0.1")),   g1_4(IPv4("224.1.0.2"));
    IPvX s2_4(IPv4("123.45.0.2")),   g2_4(IPv4("224.1.0.1"));
    IPvX s3_4(IPv4("123.45.0.255")), g3_4(IPv4("224.1.0.255"));
    IPvX s4_4(IPv4("123.46.0.1")),   g4_4(IPv4("224.2.0.1"));
    IPvX s5_4(IPv4("123.46.0.1")),   g5_4(IPv4("224.2.0.2"));
    MyMre *mre1_4 = new MyMre(s1_4, g1_4);
    MyMre *mre11_4 = new MyMre(s1_4, g1_4);
    MyMre *mre2_4 = new MyMre(s2_4, g2_4);
    MyMre *mre4_4 = new MyMre(s4_4, g4_4);
    MyMre *mre5_4 = new MyMre(s5_4, g5_4);
    // IPv6 values
    IPvX s1_6(IPv6("2001::1")),   g1_6(IPv6("ff01::2"));
    IPvX s2_6(IPv6("2001::2")),   g2_6(IPv6("ff01::1"));
    IPvX s3_6(IPv6("2001::ff")),  g3_6(IPv6("ff01::ff"));
    IPvX s4_6(IPv6("2002::1")),   g4_6(IPv6("ff02::1"));
    IPvX s5_6(IPv6("2002::1")),   g5_6(IPv6("ff02::2"));
    MyMre *mre1_6 = new MyMre(s1_6, g1_6);
    MyMre *mre11_6 = new MyMre(s1_6, g1_6);
    MyMre *mre2_6 = new MyMre(s2_6, g2_6);
    MyMre *mre4_6 = new MyMre(s4_6, g4_6);
    MyMre *mre5_6 = new MyMre(s5_6, g5_6);

    list<MyMre *> expected_mre_list;
    list<MyMre *> received_mre_list;
    MyMre *t;

    //
    // Install an entry
    //
    t = mrt_4.insert(mre1_4);
    verbose_assert(t == mre1_4,
		   c_format("Installing entry for %s", cstring(*mre1_4)));

    t = mrt_6.insert(mre1_6);
    verbose_assert(t == mre1_6,
		   c_format("Installing entry for %s", cstring(*mre1_6)));

    //
    // Try to install an existing entry. The return value should be NULL.
    //
    t = mrt_4.insert(mre11_4);
    verbose_assert(t == NULL,
		   c_format("Installing entry for %s", cstring(*mre11_4)));

    t = mrt_6.insert(mre11_6);
    verbose_assert(t == NULL,
		   c_format("Installing entry for %s", cstring(*mre11_6)));

    //
    // Install an entry
    //
    t = mrt_4.insert(mre2_4);
    verbose_assert(t == mre2_4,
		   c_format("Installing entry for %s", cstring(*mre2_4)));

    t = mrt_6.insert(mre2_6);
    verbose_assert(t == mre2_6,
		   c_format("Installing entry for %s", cstring(*mre2_6)));

    //
    // Install an entry
    //
    t = mrt_4.insert(mre4_4);
    verbose_assert(t == mre4_4,
		   c_format("Installing entry for %s", cstring(*mre4_4)));

    t = mrt_6.insert(mre4_6);
    verbose_assert(t == mre4_6,
		   c_format("Installing entry for %s", cstring(*mre4_6)));

    //
    // Install an entry
    //
    t = mrt_4.insert(mre5_4);
    verbose_assert(t == mre5_4,
		   c_format("Installing entry for %s", cstring(*mre5_4)));

    t = mrt_6.insert(mre5_6);
    verbose_assert(t == mre5_6,
		   c_format("Installing entry for %s", cstring(*mre5_6)));

    //
    // Lookup an existing entry
    //
    t = mrt_4.find(s1_4, g1_4);
    verbose_assert(t == mre1_4,
		   c_format("Searching for (%s, %s)",
			    cstring(s1_4), cstring(g1_4)));

    t = mrt_6.find(s1_6, g1_6);
    verbose_assert(t == mre1_6,
		   c_format("Searching for (%s, %s)",
			    cstring(s1_6), cstring(g1_6)));
    
    //
    // Lookup an non-existing entry
    //
    t = mrt_4.find(s3_4, g3_4);
    verbose_assert(t == NULL,
		   c_format("Searching for non-existing (%s, %s)",
			    cstring(s3_4), cstring(g3_4)));

    t = mrt_6.find(s3_6, g3_6);
    verbose_assert(t == NULL,
		   c_format("Searching for non-existing (%s, %s)",
			    cstring(s3_6), cstring(g3_6)));

    //
    // Test table size
    //
    verbose_assert(mrt_4.size() == 4,
		   "Testing the multicast routing table size");

    verbose_assert(mrt_6.size() == 4,
		   "Testing the multicast routing table size");

    //
    // Test all entries ordered by source address first
    //
    received_mre_list.clear();
    for (sg_iter = mrt_4.sg_begin(); sg_iter != mrt_4.sg_end(); ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_4);
    expected_mre_list.push_back(mre2_4);
    expected_mre_list.push_back(mre4_4);
    expected_mre_list.push_back(mre5_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    received_mre_list.clear();
    for (sg_iter = mrt_6.sg_begin(); sg_iter != mrt_6.sg_end(); ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_6);
    expected_mre_list.push_back(mre2_6);
    expected_mre_list.push_back(mre4_6);
    expected_mre_list.push_back(mre5_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    //
    // Test all entries ordered by group address first
    //
    received_mre_list.clear();
    for (gs_iter = mrt_4.gs_begin(); gs_iter != mrt_4.gs_end(); ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre2_4);
    expected_mre_list.push_back(mre1_4);
    expected_mre_list.push_back(mre4_4);
    expected_mre_list.push_back(mre5_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    received_mre_list.clear();
    for (gs_iter = mrt_6.gs_begin(); gs_iter != mrt_6.gs_end(); ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre2_6);
    expected_mre_list.push_back(mre1_6);
    expected_mre_list.push_back(mre4_6);
    expected_mre_list.push_back(mre5_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    //
    // Test all entries that match a source prefix
    //
    IPvXNet s_prefix1_4(s1_4, 15);
    received_mre_list.clear();
    sg_iter_begin = mrt_4.source_by_prefix_begin(s_prefix1_4);
    sg_iter_end = mrt_4.source_by_prefix_end(s_prefix1_4);
    for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_4);
    expected_mre_list.push_back(mre2_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    IPvXNet s_prefix1_6(s1_6, 15);
    received_mre_list.clear();
    sg_iter_begin = mrt_6.source_by_prefix_begin(s_prefix1_6);
    sg_iter_end = mrt_6.source_by_prefix_end(s_prefix1_6);
    for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_6);
    expected_mre_list.push_back(mre2_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    //
    // Test all entries that match a source prefix
    //
    IPvXNet s_prefix2_4(s1_4, 0);
    received_mre_list.clear();
    sg_iter_begin = mrt_4.source_by_prefix_begin(s_prefix2_4);
    sg_iter_end = mrt_4.source_by_prefix_end(s_prefix2_4);
    for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_4);
    expected_mre_list.push_back(mre2_4);
    expected_mre_list.push_back(mre4_4);
    expected_mre_list.push_back(mre5_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    IPvXNet s_prefix2_6(s1_6, 0);
    received_mre_list.clear();
    sg_iter_begin = mrt_6.source_by_prefix_begin(s_prefix2_6);
    sg_iter_end = mrt_6.source_by_prefix_end(s_prefix2_6);
    for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ++sg_iter) {
	t = sg_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre1_6);
    expected_mre_list.push_back(mre2_6);
    expected_mre_list.push_back(mre4_6);
    expected_mre_list.push_back(mre5_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    //
    // Test all entries that match a group prefix
    //
    IPvXNet g_prefix1_4(g4_4, 16);
    received_mre_list.clear();
    gs_iter_begin = mrt_4.group_by_prefix_begin(g_prefix1_4);
    gs_iter_end = mrt_4.group_by_prefix_end(g_prefix1_4);
    for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre4_4);
    expected_mre_list.push_back(mre5_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    IPvXNet g_prefix1_6(g4_6, 16);
    received_mre_list.clear();
    gs_iter_begin = mrt_6.group_by_prefix_begin(g_prefix1_6);
    gs_iter_end = mrt_6.group_by_prefix_end(g_prefix1_6);
    for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre4_6);
    expected_mre_list.push_back(mre5_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    //
    // Test all entries that match a group prefix
    //
    IPvXNet g_prefix2_4(g1_4, 0);
    received_mre_list.clear();
    gs_iter_begin = mrt_4.group_by_prefix_begin(g_prefix2_4);
    gs_iter_end = mrt_4.group_by_prefix_end(g_prefix2_4);
    for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre2_4);
    expected_mre_list.push_back(mre1_4);
    expected_mre_list.push_back(mre4_4);
    expected_mre_list.push_back(mre5_4);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));

    IPvXNet g_prefix2_6(g1_6, 0);
    received_mre_list.clear();
    gs_iter_begin = mrt_6.group_by_prefix_begin(g_prefix2_6);
    gs_iter_end = mrt_6.group_by_prefix_end(g_prefix2_6);
    for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ++gs_iter) {
	t = gs_iter->second;
	received_mre_list.push_back(t);
    }
    expected_mre_list.clear();
    expected_mre_list.push_back(mre2_6);
    expected_mre_list.push_back(mre1_6);
    expected_mre_list.push_back(mre4_6);
    expected_mre_list.push_back(mre5_6);
    verbose_match(mre_list_str(received_mre_list),
		  mre_list_str(expected_mre_list));
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
	test_mrt();
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
