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

#ident "$XORP: xorp/rip/xorp_rip4.cc,v 1.3 2004/01/13 20:43:09 hodson Exp $"

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"

#include "libxipc/finder_constants.hh"
#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "rip/system.hh"
#include "rip/xrl_target4.hh"
#include "rip/xrl_port_manager.hh"
#include "rip/xrl_process_spy.hh"
#include "rip/xrl_rib_notifier.hh"


/**
 * Unary function class to build string containing ServiceBase objects
 * in a particular state.
 */
class service_names_in {
public:
    service_names_in(ServiceStatus in_state, string& out)
	: _st(in_state), _o(out)
    {}

    inline void operator() (const ServiceBase* sb)
    {
	if (sb->status() != _st) return;
	if (_o.empty() == false)
	    _o += ", ";
	_o += sb->service_name();
    }

protected:
    ServiceStatus 	_st;
    string& 		_o;
};


/**
 * Class to receive state changes of services used by RIP and update
 * status in the xrl interface accordingly.
 */
class Service2XrlTarget4Status : public ServiceChangeObserverBase {
protected:
    void status_change(ServiceBase*, ServiceStatus, ServiceStatus)
    {
	update_status();
    }

public:
    Service2XrlTarget4Status(XrlRip4Target& t) : _t(t), _st(READY) {}

    ~Service2XrlTarget4Status()
    {
	while (_services.empty() == false) {
	    _services.front()->unset_observer(this);
	    _services.pop_front();
	}
    }

    bool add_service(ServiceBase* sb)
    {
	if (sb->set_observer(this)) {
	    _services.push_back(sb);
	    update_status();
	    return true;
	}
	return false;
    }

    void remove_service(ServiceBase* sb)
    {
	list<ServiceBase*>::iterator i;
	i = find(_services.begin(), _services.end(), sb);
	if (i != _services.end()) {
	    _services.erase(i);
	}
	sb->unset_observer(this);
    }

    void update_status()
    {
	// XXX This is particularly inefficient and not very good.

	// Clear existing state mask and then fill with current
	// service states.   This is then propagated to the XrlTarget.

	_st = ServiceStatus(0u);

	list<ServiceBase*>::const_iterator i;
	for (i = _services.begin(); i != _services.end(); ++i) {
	    ServiceBase *sb = *i;
	    _st = ServiceStatus(_st | sb->status());
	}

	if (_st & FAILED) {
	    string svcs;
	    for_each(_services.begin(), _services.end(),
		     service_names_in(FAILED, svcs));
	    _t.set_status(PROC_FAILED, "Failure(s): " + svcs);
	    return;
	}

	if (_st & SHUTTING_DOWN) {
	    string svcs;
	    for_each(_services.begin(), _services.end(),
		     service_names_in(SHUTTING_DOWN, svcs));
	    _t.set_status(PROC_SHUTDOWN, "Awaiting shutdown of: " + svcs);
	    return;
	}

	if (_st & (STARTING|READY)) {
	    string svcs;
	    for_each(_services.begin(), _services.end(),
		     service_names_in(STARTING, svcs));
	    _t.set_status(PROC_STARTUP, "Awaiting start up of: " + svcs);
	    return;
	}

	XLOG_ASSERT(_st == RUNNING);
	_t.set_status(PROC_READY, "");
    }
    inline bool have_status(ServiceStatus st) {
	return _st & st;
    }

protected:
    XrlRip4Target& 	_t;
    list<ServiceBase*>	_services;
    ServiceStatus	_st;
};


void
rip_main(const string& finder_host, uint16_t finder_port)
{
    XorpUnexpectedHandler catch_all(xorp_unexpected_handler);
    try {
	EventLoop 	     e;
	System<IPv4> 	     rip_system(e);
	XrlStdRouter 	     xsr(e, "rip4", finder_host.c_str(), finder_port);
	XrlProcessSpy	     xps(xsr);
	IfMgrXrlMirror	     ixm(e, "fea");
	XrlPortManager<IPv4> xpm(rip_system, xsr, ixm);

	bool stop_requested(false);
	XrlRip4Target xr4t(xsr, xps, stop_requested);

	while (xsr.ready() == false) {
	    e.run();
	}

	Service2XrlTarget4Status smon(xr4t);

	// Start up process spy
	xps.startup();
	smon.add_service(&xps);

	XrlRibNotifier<IPv4> xn(e, rip_system.route_db().update_queue(), xsr);
	xn.startup();
	smon.add_service(&xn);

	// Start up interface mirror
	ixm.startup();
	smon.add_service(&ixm);

	// Start up xrl port manager
	xpm.startup();
	smon.add_service(&xpm);

	while (stop_requested == false &&
	       smon.have_status(FAILED) == false) {
	    e.run();
	    printf(".");
	}

	xps.shutdown();
	xn.shutdown();
	ixm.shutdown();
	xpm.shutdown();

	bool flag(false);
	XorpTimer t = e.set_flag_after_ms(5000, &flag);
	while (flag == false &&
	       smon.have_status(ServiceStatus(~(FAILED|SHUTDOWN)))) {
	    printf("x");
	    e.run();
	}

	smon.remove_service(&xps);
	smon.remove_service(&xn);
	smon.remove_service(&ixm);
	smon.remove_service(&xpm);
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
	if (t_port == 0 || t_port > 65535) {
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
