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

#ident "$XORP: xorp/libxipc/test_finder_msgs.cc,v 1.1 2003/01/21 18:51:36 hodson Exp $"

#include "finder_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "finder_msgs.hh"

///////////////////////////////////////////////////////////////////////////////
// Constants

static const char *program_name         = "test_finder_msgs";
static const char *program_description  = "Test Finder messages and parsing";
static const char *program_version_id   = "0.1";
static const char *program_date         = "January, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

///////////////////////////////////////////////////////////////////////////////
// Verbosity level control

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

///////////////////////////////////////////////////////////////////////////////
//

enum MsgType {
    INVALID_MSG,
    XRL_MSG,
    XRL_RESPONSE_MSG
};

/**
 * @return true on success, false otherwise.
 */
static bool
test_parser(const string& msg, MsgType e)
{
    try {
	ParsedFinderXrlMessage p(msg.c_str());
	verbose_log("Got Xrl message (seqno = %u)\n", p.seqno());
    } catch (const WrongFinderMessageType&) {
	if (e == XRL_MSG) {
	    verbose_log("Expected Xrl message, but got:\n%s\n", msg.c_str());
	    return false;
	}
    } catch (const XorpException& xe) {
	verbose_log("\nGot exception %s from \n\"%s\"\n",
		    xe.str().c_str(), msg.c_str());
	return false;
    }

    try {
	ParsedFinderXrlResponse p(msg.c_str());
	verbose_log("Got Xrl Response message (seqno = %u)\n", p.seqno());
    } catch (const WrongFinderMessageType&) {
	if (e == XRL_RESPONSE_MSG) {
	    verbose_log("Unexpected XrlResponse message, but got:\n%s\n",
			msg.c_str());
	    return false;
	}
    } catch (const XorpException& xe) {
	verbose_log("\nGot exception %s from \n\"%s\"\n",
		    xe.str().c_str(), msg.c_str());
	return false;
    }

    return true;
}

static int
test_main()
{
    // Test render message and parse for FinderXrlMessage
    verbose_log("Checking XrlMessage\n");
    FinderXrlMessage fm0(Xrl("finder", "finder", "put_sock_in"));
    if (false == test_parser(fm0.str(), XRL_MSG)) {
	return 1;
    }
    ParsedFinderXrlMessage p0(fm0.str().c_str());

    // Test render message and parse for FinderXrlResponse with note
    verbose_log("Checking XrlResponse with XrlError note\n");
    FinderXrlResponse rm0(p0.seqno(),
			  XrlCmdError::COMMAND_FAILED("Not written."), 0);
    if (false == test_parser(rm0.str(), XRL_RESPONSE_MSG)) {
	return 1;
    }

    // Create another FinderXrlMessage to get another seqno
    verbose_log("Checking seqno increment\n");
    FinderXrlMessage fm1(Xrl("finder", "finder", "put_sock_in"));
    if (false == test_parser(fm1.str(), XRL_MSG)) {
	return 1;
    }
    ParsedFinderXrlMessage p1(fm1.str().c_str());

    // Check seqno incremented by 1
    verbose_log("Checking seqno increment\n");
    if (p1.seqno() != p0.seqno() + 1) {
	verbose_log("Bad sequence number increment\n");
	return 1;
    }

    // Check parsring of response with no note associated with XrlError
    verbose_log("Checking XrlResponse with no XrlError note\n");
    FinderXrlResponse rm1(p1.seqno(), XrlCmdError::OKAY(), 0);
    if (false == test_parser(rm1.str().c_str(), XRL_RESPONSE_MSG)) {
	return 1;
    }

    // Attempt to parse crap
    verbose_log("Checking junk messages do not parse "
		"(expect exception in next output line)\n");
    if (true == test_parser("ABSJKDDDD /d/d/ds sss", INVALID_MSG)) {
	verbose_log("Accepted junk");
	return 1;
    }

    // Play with message corruption, create a good message and
    // scramble a byte
    verbose_log("Checking corrupted messages do not parse\n");
    for (int i = 0; i < 256; i++) {
	FinderXrlMessage fm2(Xrl("finder", "finder", "pull_sock_out"));
	string good = fm2.str();
	
	string::size_type n = good.find("MsgData ");
	if (n == string::npos) {
	    verbose_log("Test error could not find field MsgData in message");
	}
	
	for (uint32_t j = 0; j < n; j++) {
	    string corrupted = good;
	    if (corrupted[j] == i)
		continue;	// corruption would be a straight substitution
	    if (isdigit(i) && isdigit(corrupted[j]))
		continue;	// corrupting digit with digit has no effect
	    corrupted[j] = i;
	    try {
		ParsedFinderXrlMessage p(corrupted.c_str());

		verbose_log("Corruption char 0x%02x accepted in header at "
			    " position %d in:\n%s\n", i, j, corrupted.c_str());
		verbose_log("Original message:\n%s\n",  good.c_str());

		return 1;
	    } catch (...) {
		// An exception in this case is a good thing.
	    }
	}
    }

    verbose_log("Checking XrlArgs handling in XrlResponses\n");
    XrlArgs a;
    a.add_int32("an-int", 67);
    a.add_ipv4("an-ipv4", IPv4("10.0.0.1"));
    uint32_t seqno = 1999111;

    FinderXrlResponse fxr(seqno, XrlError::OKAY(), &a);
    ParsedFinderXrlResponse pfxr(fxr.str().c_str());
    //    verbose_log("Message is \"%s\"\n", fxr.str().c_str());
    if (pfxr.seqno() != seqno) {
	verbose_log("Failed on seqno (%u != %u)\n", pfxr.seqno(), seqno);
	return 1;
    }
    if (pfxr.xrl_error() != XrlError::OKAY()) {
	verbose_log("Failed on XrlError (%s != %s)\n",
		    pfxr.xrl_error().str().c_str(), XrlError::OKAY().str().c_str());
	return 1;
    }
    if (pfxr.xrl_args() == 0) {
	verbose_log("Missing arguments in parsed response\n");
	return 1;
    }
    if (*pfxr.xrl_args() != a) {
	verbose_log("Arguments mismatch \"%s\" != \"%s\"\n",
		    pfxr.xrl_args()->str().c_str(), a.str().c_str());
	return 1;
    }
    
    verbose_log("All is well on the western front\n");
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
	ret_value = test_main();
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
