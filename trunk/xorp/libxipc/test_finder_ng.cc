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

#ident "$XORP: xorp/libxipc/test_finder_ng.cc,v 1.5 2003/03/08 20:56:52 hodson Exp $"

#include "finder_module.h"

#include "libxorp/eventloop.hh"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "sockutil.hh"

#include "finder_ng.hh"
#include "finder_ng_xrl_target.hh"
#include "finder_ng_client.hh"
#include "finder_ng_client_xrl_target.hh"
#include "finder_tcp_messenger.hh"
#include "permits.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_finder_ng";
static const char *program_description  = "Test carrying and execution of "
					  "Finder and Finder client";
static const char *program_version_id   = "0.1";
static const char *program_date         = "February, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

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

static void
resolve_callback(const XrlError&	e,
		 const FinderDBEntry*	fdbe,
		 const string*		s,
		 string*		result,
		 bool*			success)
{
    if (e != XrlError::OKAY()) {
	verbose_log("Failed to resolve \"%s\" Error \"%s\"\n",
		    s->c_str(), e.str().c_str());
	*success = false;
	return;
    }
    verbose_log("Success: \"%s\" resolves as:\n", s->c_str());
    const list<string>& v = fdbe->values();
    for (list<string>::const_iterator vi = v.begin(); vi != v.end(); ++vi) {
	*result = *vi;
	verbose_log("\t %s\n", vi->c_str());
    }
    *success = true;
}

static int
test_xrls_resolve(EventLoop& e, FinderNGClient& fc1, list<string>& xrls)
{
    //
    // Resolve set of xrl's
    //
    bool expired = false;
    XorpTimer t = e.set_flag_after_ms(1000, &expired);

    string result;
    bool success = true;

    for (list<string>::const_iterator ci = xrls.begin();
	 ci != xrls.end(); ++ci) {
	verbose_log("Resolving %s...\n", ci->c_str());
	fc1.query(*ci, callback(resolve_callback, &(*ci), &result, &success));
    }

    while (!expired && success)
	e.run();

    if (success == false) {
	return 1;
    }
    return 0;
}

#if 0
static int
test_xrls_locally_resolve(EventLoop& e, FinderNGClient& fc1, list<string>& xrls)
{
    //
    // Check resolved xrl is in local cache.
    //
    verbose_log("Looking up local resolution for \"%s\"\n",
		result.c_str());

    string local_xrl_command;
    Xrl lx(result.c_str());
    if (fc1. query_self(lx.command(), local_xrl_command) == false) {
	verbose_log("Failed to look up local xrl command");
	return 1;
    }
    verbose_log("Xrl command \"%s\" resolves as \"%s\"\n",
		lx.command().c_str(), local_xrl_command.c_str());
    
    return 0;
}    
#endif // 0

class FinderPackage {
public:
    FinderPackage(EventLoop& e, IPv4 host, uint16_t port) 
	: _finder(), _finder_xrl_handler(_finder),
	  _finder_tcp4_source(e, _finder, _finder.commands(), IPv4::ANY(), port)
    {
	add_permitted_host(host);
    }
    FinderNG& finder() { return _finder; }

protected:
    FinderNG		_finder;
    FinderNGXrlTarget	_finder_xrl_handler;
    FinderNGTcpListener	_finder_tcp4_source;
};



///////////////////////////////////////////////////////////////////////////////
//
// test main
//

static int
test_main(void)
{
    EventLoop e;

    IPv4 test_host = if_get_preferred();
    uint16_t test_port = 16666;

    //
    // Construct finder and messenger source for finder
    //
    FinderPackage* finder_box = new FinderPackage(e, test_host, test_port);

    //
    // Construct first finder client and messenger source for it.
    //
    FinderNGClient fc1;
    FinderNGClientXrlTarget fc1_xrl_handler(&fc1, &fc1.commands());
    FinderNGTcpAutoConnector fc1_connector(e, fc1, fc1.commands(),
					   test_host, test_port);

    //
    // Start an expiry timer
    //
    bool expired = false;
    XorpTimer t = e.set_flag_after_ms(1000, &expired);

    //
    // Establish connection
    //
    verbose_log("Establishing connection between first finder client and "
		"finder...\n");
    while (!expired && fc1.connected() == false && 
	   finder_box->finder().messengers() == 0)
	e.run();

    if (expired) {
	verbose_log("failed.\n");
	return 1;
    }
    verbose_log("succeeded.\n");

    //
    // Register target
    //
    verbose_log("Registering target...\n");
    uint32_t id;
    if (fc1.register_xrl_target("test_target", "experimental", id) != true) {
	verbose_log("failed.\n");
	return 1;
    }
    verbose_log("succeeded.\n");

    while (!expired)
	e.run();

    //
    // Register a set of xrl's
    //
    list<string> xrls;
    xrls.push_back("finder://test_target/test_command1");
    xrls.push_back("finder://test_target/test_command2");
    xrls.push_back("finder://test_target/test_command3");

    expired = false;
    t = e.set_flag_after_ms(1000, &expired);

    for (list<string>::const_iterator ci = xrls.begin();
	 ci != xrls.end(); ++ci) {
	verbose_log("Registering %s...\n", ci->c_str());
	if (fc1.register_xrl(id, *ci,
			     "stcp", "localhost:10000") == false) {
	    verbose_log("failed.\n");
	    return 1;
	}
	verbose_log("succeeded\n");
    }
    fc1.enable_xrls(id);
    
    while (!expired)
	e.run();

    int r = test_xrls_resolve(e, fc1, xrls); 
    if (r)
	return r;

#if 0
    r = test_xrls_locally_resolve(e, fc1, xrls);
    if (r)
	return r;
    // Close connection
    FinderTcpMessenger* ftm =
	dynamic_cast<FinderTcpMessenger*>(fc1.messenger());
    ftm->close();
#endif    

    expired = false;
    t = e.set_flag_after_ms(1000, &expired);
    while (expired == false)
	e.run();
    
    r = test_xrls_resolve(e, fc1, xrls); 
    if (r)
	return r;

    //
    // Construct second finder client and messenger source for it.
    //
    FinderNGClient fc2;
    FinderNGClientXrlTarget fc2_xrl_handler(&fc2, &fc2.commands());
    FinderNGTcpAutoConnector fc2_connector(e, fc2, fc2.commands(),
					   test_host, test_port);

    // Register test client
    uint32_t id2;
    verbose_log("Registering client...\n");
    if (fc2.register_xrl_target("test_client", "experimental", id2) != true) {
	verbose_log("failed.\n");
	return 1;
    }
    verbose_log("succeeded.\n");
    if (fc2.enable_xrls(id2) == false) {
	verbose_log("Failed to enable xrls on test client\n");
	return 1;
    }
    
    //
    // Start an expiry timer
    //
    expired = false;
    t = e.set_flag_after_ms(1000, &expired);

    //
    // Establish connection
    //
    verbose_log("Establishing connection between second finder client and "
		"finder...\n");
    while (!expired &&
	   (fc2.connected() == false || 
	    finder_box->finder().messengers() < 2))
	e.run();

    if (expired) {
	verbose_log("failed.\n");
	return 1;
    }
    verbose_log("succeeded.\n");

    verbose_log("Testing Client 2 can resolve Client 1 registrations\n");
    // Test second finder client can resolve xrls on first finder
    r = test_xrls_resolve(e, fc2, xrls); 
    if (r)
	return r;

    verbose_log("Killing finder\n");
    expired = false;
    t = e.set_flag_after_ms(1000, &expired);

    delete finder_box;
    finder_box = 0;

    while (expired == false) {
	debug_msg("Expired %d\n", expired);
	e.run();
    }
    debug_msg("Expired %d\n", expired);

    // Test we can restart finder
    verbose_log("Restarting finder\n");    
    finder_box = new FinderPackage(e, test_host, test_port);
    expired = false;
    t = e.set_flag_after_ms(1000, &expired);
    while (expired == false)
	e.run();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Standard test program rubble.
//

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
