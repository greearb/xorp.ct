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

#ident "$XORP: xorp/libxorp/test_ref_ptr.cc,v 1.1 2003/04/01 21:50:45 hodson Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#include "libxorp/ref_ptr.hh"

//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name         = "test_ref_ptr";
static const char *program_description  = "Test ref_ptr classes";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April 1, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

//
// printf(3)-like facility to conditionally print a message if verbosity
// is enabled.
//
#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)                                  \
do {                                                                    \
    if (verbose()) {                                                    \
        printf("From %s:%d: ", file, line);                             \
        printf(x);                                                      \
    }                                                                   \
} while(0)

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
 * A simple class that sets a boolean flag to true when destructed.
 */
class FlagSetDestructor {
public:
    FlagSetDestructor(bool& flag_to_set) : _flag(flag_to_set) {}
    ~FlagSetDestructor() { _flag = true; }
protected:
    bool& _flag;
};

/**
 * Run through tests of some common operations on a ref_ptr object.
 */
template <class Rp>
static int
play_with_counts(Rp& r, int32_t initial_cnt)
{
    Rp copy1 = r;
    if (copy1 != r) {
	verbose_log("Failed on operator=");
	return 1;
    }

    {
	Rp copy2 = r;
	if (copy2.at_least(initial_cnt + 2) == false) {
	    verbose_log("Failed with number of copies\n");
	    return 1;
	}
	if (copy2.at_least(initial_cnt + 3) == true) {
	    verbose_log("Failed with number of copies\n");
	    return 1;
	}
	if (copy2 != r) {
	    verbose_log("Failed on operator=");
	    return 1;
	}
	if (copy2 != copy1) {
	    verbose_log("Failed on operator=");
	    return 1;
	}
    }

    if (copy1.at_least(initial_cnt + 1) == false) {
	verbose_log("Failed with number of copies\n");
	return 1;
    }

    if (copy1.at_least(initial_cnt + 2) == true) {
	verbose_log("Failed to remove reference when pointer went out of "
		    "scope\n");
	return 1;
    }

    copy1 = 0;
    if (r.at_least(initial_cnt + 1) == true) {
	verbose_log("Failed to remove reference when pointer assigned\n");
	return 1;
    }
    return 0;
}

static int
run_test()
{
    verbose_log("Running ref_ptr test:\n");

    bool deleted = false;
    {
	ref_ptr<FlagSetDestructor> rp = new FlagSetDestructor(deleted);
	{
	    if (play_with_counts(rp, 1)) {
		return 1;
	    }
	}
    }
    if (deleted == false) {
	verbose_log("Failed to delete object.\n");
	return 1;
    }
    verbose_log("Pass.\n");

    verbose_log("Running cref_ptr test:\n");
    deleted = false;
    {
	cref_ptr<FlagSetDestructor> rp = new FlagSetDestructor(deleted);
	{
	    if (play_with_counts(rp, 1)) {
		return 1;
	    }
	}
    }
    if (deleted == false) {
	verbose_log("Failed to delete object.\n");
	return 1;
    }
    verbose_log("Pass.\n");

    return 0;
};

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
	for (uint32_t i = 0 ; i < 100; i++) {
	    ret_value = run_test();
	    if (ret_value == 0) {
		if (ref_counter_pool::instance().balance() != 0) {
		    verbose_log("Ref count balance (%d != 0) non-zero at end",
				ref_counter_pool::instance().balance());
		    ret_value = 1;
		}
	    }
	}
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
