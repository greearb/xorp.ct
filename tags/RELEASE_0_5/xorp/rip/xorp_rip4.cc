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

#ident "$XORP$"

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"

#include "libxipc/finder_constants.hh"
#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "rip/system.hh"
#include "rip/xrl_target4.hh"
#include "rip/xrl_process_spy.hh"
#include "rip/xrl_rib_notifier.hh"

#if 0
class RipInstance : public ServiceChangeObserverBase {
public:
    RipInstance(const string& finder_host, uint16_t finder_port);

    // Runs until stop is signalled through XrlTarget
    void run();

    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status);

    void create_process_spy();

private:
    RipInstance(const RipInstance&);
    RipInstance& operator=(const RipInstance&);

private:
    bool _stop;
};
#endif /* 0 */

void
rip_main(const string& finder_host, uint16_t finder_port)
{
    XorpUnexpectedHandler catch_all(xorp_unexpected_handler);
    try {
	EventLoop e;
	System<IPv4> rip_system(e);
	XrlStdRouter xsr(e, "rip4", finder_host.c_str(), finder_port);

	XrlProcessSpy xps(xsr);

	bool stop_requested(false);
	XrlRip4Target xr4t(xsr, xps, stop_requested);

	while (xsr.ready() == false) {
	    e.run();
	}
	xr4t.set_status(PROC_STARTUP, "Checking for FEA and RIB processes.");

	// Start up process spy
	xps.startup();

	XrlRibNotifier<IPv4> xn(e, rip_system.route_db().update_queue(), xsr);

	// Start up rib notifier
	xn.startup();

	// Instantiate and start libfeaclient based interface mirror
	IfMgrXrlMirror ixm(e);
	ixm.startup();

	while (stop_requested == false &&
	       xn.status() != FAILED) {
	    e.run();
	    printf(".");
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
    }
}

static void
usage()
{
    fprintf(stderr, "Usage: xorp_rip4 [options]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -F <host>[:<port>]  Specify Finder host and port\n");
    fprintf(stderr, "  -h                  Display this information\n");
}

/**
 * Extract Host and port from either host string or host colon port string, eg
 * "host" or "<host>:<port>".
 *
 * @param host_colon_port string to be scanned [in].
 * @param host host component of scanned string [out].
 * @param port port component of scanned string [out].
 * @return true on success, false if port value is invalid.
 */
static bool
parse_finder_args(const string& host_colon_port, string& host, uint16_t& port)
{
    string::size_type sp = host_colon_port.find(":");
    if (sp == string::npos) {
	host = host_colon_port;
	// Do not set port, by design it has default finder port value.
    } else {
	host = string(host_colon_port, 0, sp);
	string s_port = string(host_colon_port, sp + 1, 14);
	uint32_t t_port = atoi(s_port.c_str());
	if (t_port == 0 || t_port > 0x65535) {
	    XLOG_ERROR("Finder port %d is not in range 1--65535.\n", t_port);
	    return false;
	}
	port = (uint16_t)t_port;
    }
    return true;
}

int
main(int argc, char * const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string	finder_host = FINDER_DEFAULT_HOST.str();
    uint16_t    finder_port = FINDER_DEFAULT_PORT;
    bool	run_main = true;

    int ch;
    while ((ch = getopt(argc, argv, "F:")) != -1) {
	switch (ch) {
	case 'F':
	    run_main = parse_finder_args(optarg, finder_host, finder_port);
	    break;
	default:
	    usage();
	    run_main = false;
	}
    }

    if (run_main)
	rip_main(finder_host, finder_port);

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    return 0;
}
