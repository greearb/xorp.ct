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

#ident "$XORP: xorp/libxipc/test_xrl_args.cc,v 1.5 2003/10/20 22:38:19 hodson Exp $"

// test_xrl_args: String Serialization Tests

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "xrl_args.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_xrl_args";
static const char *program_description  = "Test Xrl argument class";
static const char *program_version_id   = "0.2";
static const char *program_date         = "October, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, "
					  "2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)


static int
test_serialize_one(const XrlArgs& original)
{
    uint32_t packed_bytes = original.packed_bytes();
    for (uint32_t buf_bytes = 0; buf_bytes < packed_bytes + 10;
	 buf_bytes++) {

	// Pack XrlArgs into a byte vector
	vector<uint8_t> buf(buf_bytes);
	size_t original_bytes = original.pack(&buf[0], buf_bytes);

	// Check reported bytes used against expected
	if (original_bytes > packed_bytes) {
	    verbose_log("Packed more than promised "
			"(buf_bytes = %u)\n", buf_bytes);
	    return 1;
	} else if (original_bytes == packed_bytes) {
	    if (buf_bytes < packed_bytes) {
		verbose_log("Unexpected packing success "
			    "(buf_bytes = %u)\n", buf_bytes);
		return 1;
	    }
	} else if (original_bytes < packed_bytes) {
	    if (original_bytes != 0) {
		verbose_log("Packed less than promised "
			    "(buf_bytes = %u)\n", buf_bytes);
		return 1;
	    }
	    if (buf_bytes >= packed_bytes) {
		verbose_log("Failed to pack within expected space "
			    "(buf_bytes = %u)\n", buf_bytes);
		return 1;
	    }
	}

	if (original_bytes == 0)
	    continue;

	for (uint32_t r_bytes = 0; r_bytes <= buf_bytes; r_bytes++) {
	    XrlArgs rendered;
	    size_t rb = rendered.unpack(&buf[0], r_bytes);

	    if (rb > 0 && rb != packed_bytes) {
		verbose_log("Unpacked wrong byte count %u != %u "
			    "(buf_bytes = %u, r_bytes = %u)\n",
			    static_cast<uint32_t>(rb),
			    static_cast<uint32_t>(packed_bytes),
			    buf_bytes, r_bytes);
		verbose_log("Input:  %s\nOutput: %s\n",
			    original.str().c_str(), rendered.str().c_str());
		return 1;
	    }

	    if (rb == 0) {
		if (r_bytes > packed_bytes) {
		    verbose_log("Unpacked surplus with %u >= %u "
				"(buf_bytes = %u, r_bytes = %u)\n",
				r_bytes, packed_bytes,
				buf_bytes, r_bytes);
		    return 1;
		}
		continue;
	    }

	    if (rendered != original) {
		verbose_log("Unpacked XrlArgs does not match original"
			    "(buf_bytes = %u, r_bytes = %u)\n",
			    buf_bytes, r_bytes);
		return 1;
	    }
	} /* r_bytes */
    } /* buf_bytes */
    return 0;
}

static int
run_serialization_test()
{
    vector<uint8_t> test_binary(53);

    XrlAtomList test_list;
    test_list.append(XrlAtom("part_1", "Goodbye Pork Pie Hat"));
    test_list.append(XrlAtom("part_2", "Tic-Tocative, Orlando, Red's Favorite"));

    XrlAtom test_args[] = {
	XrlAtom("integer32",	 int32_t(-12345678)),
	XrlAtom("uinteger32",	 uint32_t(12345678)),
	XrlAtom("frank_ipv4",	 IPv4("128.16.6.31")),
	XrlAtom("frank_ipv4net", IPv4Net("128.16.8.0/24")),
	XrlAtom("some_ipv6",	 IPv6("fe80::20a:95ff:feda:7c7a")),
	XrlAtom("an_ipv6net",	 IPv6Net("fe80::20a:95ff:feda:7c7a/128")),
	XrlAtom("a_mac_addr",	 EtherMac("00:ab:10:11:12:13")),
	XrlAtom("string",	 string("hello world, kippers, yum")),
	XrlAtom("binary_data",	 test_binary),
	XrlAtom("a_list",	 test_list)
    };
    uint32_t n_test_args = sizeof(test_args) / sizeof(test_args[0]);

    for (uint32_t st = 0; st < n_test_args; st++) {
	for (uint32_t len = 0; len < n_test_args; len++) {
	    verbose_log("Testing XrlArgs (st = %u, len = %u)\n", st, len);
	    XrlArgs original;
	    // Build XrlArgs with 0 -- n_test_args arguments with starting
	    // position from 0 -- n_test_args.
	    for (uint32_t i = st; i < st + len; i++) {
		original.add(test_args[i % n_test_args]);
	    }
	    int r = test_serialize_one(original);
	    if (r) {
		verbose_log("Failed with start = %u, length = %u\n", st, len);
		return r;
	    }
	} /* len */
    } /* st */
    return 0;
}

static int
run_test()
{
    XrlArgs al;
    // Add some atoms.  NB most of these classes have constructors
    // that take string arguments.
    al.add_ipv4("a_named_ipv4", "128.16.6.31");
    al.add_ipv4net("a_named_ipv4net", "128.16.6.31/24");
    al.add_ipv6("a_named_ipv6", "fe80::2c0:f0ff:fe74:9f39");
    al.add_ipv6net("a_named_ipv6net", "fe80::2c0:f0ff:fe74:9f39/96");
    al.add_int32("first_int", 1).add_int32("second_int", 0xffffffff);
    al.add_uint32("only_uint", 0xffffffff);
    al.add_string("a_named_string", "the cat sat on the mat.");
    al.add_mac("a_named_mac", Mac("01:02:03:04:05:06"));
    al.add_string(0, "potentially hazardous null pointer named string object");
    al.add_string("bad_karma", "");

    XrlAtomList xal;
    xal.append(XrlAtom("first", string("fooo")));
    xal.append(XrlAtom("second", string("baar")));
    al.add(XrlAtom("a_list", xal));

    XrlArgs b(al.str().c_str());
    if (b != al) {
	verbose_log("String serialization failed.");
	return 1;
    }

    b = al;
    if (b != al) {
	verbose_log("Assignment operator failed.\n");
	return 1;
    }

    // Iterate through the list
    for (XrlArgs::const_iterator i = al.begin(); i != al.end(); i++) {
    	const XrlAtom& atom = *i;
	atom.has_data(); // No-op
    }

    try {
	al.get_uint32("only_uint");
    } catch (const XrlArgs::XrlAtomNotFound &e) {
	verbose_log("Atom not Found\n");
	return 1;
    } catch (...) {
	verbose_log("Unanticipated exception.");
	return 1;
    }

    try {
	al.get_ipv4("a_named_ipv4");
	al.get_ipv4net("a_named_ipv4net");
	al.get_ipv6("a_named_ipv6");
	al.get_ipv6net("a_named_ipv6net");
	al.get_int32("first_int");
	al.get_int32("second_int");
	al.get_uint32("only_uint");
	al.get_string("a_named_string");
	al.get_mac("a_named_mac");
	al.get_string(0);
	al.get_string("");
	al.get_string("bad_karma");
    } catch (XrlArgs::XrlAtomNotFound& e) {
	verbose_log("Failed to find atom.\n");
	return 1;
    }
    return 0;
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

int
main(int argc, char * const argv[])
{
    int ret_value;
    const char* const argv0 = argv[0];

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
	    if (ch == 'h')
		return 0;
	    else
		return 1;
	}
    }
    argc -= optind;
    argv += optind;

    //
    // Initialize and start xlog
    //
    xlog_init(argv0, NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	ret_value = run_test();
	if (ret_value == 0) {
	    ret_value = run_serialization_test();
	}
    }
    catch (...) {
	xorp_catch_standard_exceptions();
	ret_value = 2;
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return ret_value;
}
