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

#ident "$XORP: xorp/fea/tools/show_interfaces.cc,v 1.1 2004/05/21 08:08:03 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_std_router.hh"

#include "show_interfaces.hh"

InterfaceMonitor::InterfaceMonitor(XrlRouter& xrl_router,
				   EventLoop& eventloop,
				   const string& fea_target)
    : ServiceBase("ShowInterfaces"),
      _xrl_router(xrl_router),
      _eventloop(eventloop),
      _ifmgr(eventloop, fea_target.c_str(), xrl_router.finder_address(),
	     xrl_router.finder_port()),
      _startup_requests_n(0),
      _shutdown_requests_n(0)
{
    //
    // Set myself as an observer when the node status changes
    //
    set_observer(this);

    _ifmgr.set_observer(this);
    _ifmgr.attach_hint_observer(this);
}

InterfaceMonitor::~InterfaceMonitor()
{
    //
    // Unset myself as an observer when the node status changes
    //
    unset_observer(this);

    shutdown();

    _ifmgr.detach_hint_observer(this);
    _ifmgr.unset_observer(this);
}

bool
InterfaceMonitor::startup()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == STARTING)
	|| (ServiceBase::status() == RUNNING))
	return true;

    if (ServiceBase::status() != READY)
	return false;

    //
    // Transition to RUNNING occurs when all transient startup operations
    // are completed (e.g., after we have the interface/vif/address state
    // available, etc.)
    //
    ServiceBase::set_status(STARTING);

    //
    // Startup the interface manager
    //
    if (ifmgr_startup() != true) {
	XLOG_ERROR("Cannot startup the interface mirroring manager");
	ServiceBase::set_status(FAILED);
	return false;
    }

    return true;
}

bool
InterfaceMonitor::shutdown()
{
    //
    // We cannot shutdown if our status is SHUTDOWN or FAILED.
    //
    if ((ServiceBase::status() == SHUTDOWN)
	|| (ServiceBase::status() == FAILED)) {
	return true;
    }

    if (ServiceBase::status() != RUNNING)
	return false;

    //
    // Transition to SHUTDOWN occurs when all transient shutdown operations
    // are completed (e.g., after we have deregistered with the FEA, etc.)
    //
    ServiceBase::set_status(SHUTTING_DOWN);

    //
    // Shutdown the interface manager
    //
    if (ifmgr_shutdown() != true) {
	XLOG_ERROR("Cannot shutdown the interface mirroring manager");
	ServiceBase::set_status(FAILED);
	return false;
    }

    return true;
}

void
InterfaceMonitor::status_change(ServiceBase*  service,
				ServiceStatus old_status,
				ServiceStatus new_status)
{
    if (service == this) {
	// My own status has changed
	if ((old_status == STARTING) && (new_status == RUNNING)) {
	    // The startup process has completed
	    return;
	}

	if ((old_status == SHUTTING_DOWN) && (new_status == SHUTDOWN)) {
	    // The shutdown process has completed
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SHUTTING_DOWN) && (new_status == SHUTDOWN)) {
	    decr_shutdown_requests_n();
	}
    }
}

void
InterfaceMonitor::incr_startup_requests_n()
{
    _startup_requests_n++;
    XLOG_ASSERT(_startup_requests_n > 0);
}

void
InterfaceMonitor::decr_startup_requests_n()
{
    XLOG_ASSERT(_startup_requests_n > 0);
    _startup_requests_n--;

    update_status();
}

void
InterfaceMonitor::incr_shutdown_requests_n()
{
    _shutdown_requests_n++;
    XLOG_ASSERT(_shutdown_requests_n > 0);
}

void
InterfaceMonitor::decr_shutdown_requests_n()
{
    XLOG_ASSERT(_shutdown_requests_n > 0);
    _shutdown_requests_n--;

    update_status();
}

void
InterfaceMonitor::update_status()
{
    //
    // Test if the startup process has completed
    //
    if (ServiceBase::status() == STARTING) {
	if (_startup_requests_n > 0)
	    return;

	// The startup process has completed
	ServiceBase::set_status(RUNNING);
	return;
    }

    //
    // Test if the shutdown process has completed
    //
    if (ServiceBase::status() == SHUTTING_DOWN) {
	if (_shutdown_requests_n > 0)
	    return;

	// The shutdown process has completed
	ServiceBase::set_status(SHUTDOWN);
	return;
    }

    //
    // Test if we have failed
    //
    if (ServiceBase::status() == FAILED) {
	return;
    }
}

void
InterfaceMonitor::tree_complete()
{
    decr_startup_requests_n();

    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
InterfaceMonitor::updates_made()
{

}

bool
InterfaceMonitor::ifmgr_startup()
{
    bool ret_value;

    // TODO: XXX: we should startup the ifmgr only if it hasn't started yet
    incr_startup_requests_n();

    ret_value = _ifmgr.startup();

    //
    // XXX: when the startup is completed, IfMgrHintObserver::tree_complete()
    // will be called.
    //

    return ret_value;
}

bool
InterfaceMonitor::ifmgr_shutdown()
{
    bool ret_value;

    incr_shutdown_requests_n();

    ret_value = _ifmgr.shutdown();

    //
    // XXX: when the shutdown is completed, InterfaceMonitor::status_change()
    // will be called.
    //

    return ret_value;
}

void
InterfaceMonitor::print_interfaces(const string& print_iface_name) const
{
    bool iface_found = false;
    IfMgrIfTree::IfMap::const_iterator ifmgr_iface_iter;
    IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;
    IfMgrVifAtom::V4Map::const_iterator a4_iter;
    IfMgrVifAtom::V6Map::const_iterator a6_iter;

    //
    // Iterate for all interface, vifs, and addresses, and print them
    //
    for (ifmgr_iface_iter = ifmgr_iftree().ifs().begin();
	 ifmgr_iface_iter != ifmgr_iftree().ifs().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();

	if (print_iface_name.size() > 0) {
	    if (print_iface_name != ifmgr_iface_name) {
		// Print only the specified interface and ignore the other
		continue;
	    }
	    iface_found = true;
	}

	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();

	    //
	    // Print the interface name, flags and MTU
	    //
	    bool prev = false;
	    fprintf(stdout, "%s/%s: Flags:<",
		   ifmgr_iface_name.c_str(),
		   ifmgr_vif_name.c_str());

	    if (ifmgr_vif.enabled()) {
		if (prev)
		    fprintf(stdout, ",");
		fprintf(stdout, "ENABLED");
		prev = true;
	    }
	    if (ifmgr_vif.broadcast_capable()) {
		if (prev)
		    fprintf(stdout, ",");
		fprintf(stdout, "BROADCAST");
		prev = true;
	    }
	    if (ifmgr_vif.multicast_capable()) {
		if (prev)
		    fprintf(stdout, ",");
		fprintf(stdout, "MULTICAST");
		prev = true;
	    }
	    if (ifmgr_vif.loopback()) {
		if (prev)
		    fprintf(stdout, ",");
		fprintf(stdout, "LOOPBACK");
		prev = true;
	    }
	    if (ifmgr_vif.p2p_capable()) {
		if (prev)
		    fprintf(stdout, ",");
		fprintf(stdout, "POINTTOPOINT");
		prev = true;
	    }
	    fprintf(stdout, "> mtu %d\n", ifmgr_iface.mtu_bytes());

	    //
	    // Print the IPv6 addresses
	    //
	    for (a6_iter = ifmgr_vif.ipv6addrs().begin();
		 a6_iter != ifmgr_vif.ipv6addrs().end();
		 ++a6_iter) {
		const IfMgrIPv6Atom& a6 = a6_iter->second;
		const IPv6& addr = a6.addr();
		fprintf(stdout, "        inet6 %s prefixlen %d",
		       addr.str().c_str(), a6.prefix_len());
		if (a6.has_endpoint()) {
		    fprintf(stdout, " --> %s",
			    a6.endpoint_addr().str().c_str());
		}
		fprintf(stdout, "\n");
	    }

	    //
	    // Print the IPv4 addresses
	    //
	    for (a4_iter = ifmgr_vif.ipv4addrs().begin();
		 a4_iter != ifmgr_vif.ipv4addrs().end();
		 ++a4_iter) {
		const IfMgrIPv4Atom& a4 = a4_iter->second;
		const IPv4& addr = a4.addr();
		fprintf(stdout, "        inet %s subnet %s",
		       addr.str().c_str(),
		       IPv4Net(addr, a4.prefix_len()).str().c_str());
		if (a4.has_broadcast()) {
		    fprintf(stdout, " broadcast %s",
			    a4.broadcast_addr().str().c_str());
		}
		if (a4.has_endpoint()) {
		    fprintf(stdout, " --> %s",
			    a4.endpoint_addr().str().c_str());
		}
		fprintf(stdout, "\n");
	    }

	    //
	    // Print the physical interface index and MAC address
	    //
	    fprintf(stdout, "        physical index %d\n",
		    ifmgr_vif.pif_index());
	    if (!ifmgr_iface.mac().empty())
		fprintf(stdout, "        ether %s\n",
			ifmgr_iface.mac().str().c_str());
	}
    }

    if (print_iface_name.size() > 0) {
	if (! iface_found) {
	    fprintf(stderr, "Interface \"%s\" not found\n",
		    print_iface_name.c_str());
	}
    }
}

/**
 * usage:
 * @argv0: Argument 0 when the program was called (the program name itself).
 * @exit_value: The exit value of the program.
 *
 * Print the program usage.
 * If @exit_value is 0, the usage will be printed to the standart output,
 * otherwise to the standart error.
 **/
static void
usage(const char *argv0, int exit_value)
{
    FILE *output;
    const char *progname = strrchr(argv0, '/');

    if (progname != NULL)
	progname++;		// Skip the last '/'
    if (progname == NULL)
	progname = argv0;

    //
    // If the usage is printed because of error, output to stderr, otherwise
    // output to stdout.
    //
    if (exit_value == 0)
	output = stdout;
    else
	output = stderr;

    fprintf(output, "Usage: %s [-F <finder_hostname>[:<finder_port>]] [-i <interface>]\n",
	    progname);
    fprintf(output, "           -F <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -i <interface>                        : the interface name\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

//
// Wait until the XrlRouter becomes ready
//
static void
wait_until_xrl_router_is_ready(EventLoop& eventloop, XrlRouter& xrl_router)
{
    bool timed_out = false;

    XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
    while (xrl_router.ready() == false && timed_out == false) {
	eventloop.run();
    }

    if (xrl_router.ready() == false) {
	XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
    }
}

static void
interface_monitor_main(const char* finder_hostname, uint16_t finder_port,
		       const string& print_iface_name)
{
    string process;
    process = c_format("interface_monitor%d", getpid());

    //
    // Init stuff
    //
    EventLoop eventloop;

    XrlStdRouter xrl_std_router(eventloop, process.c_str(),
				finder_hostname, finder_port);
    xrl_std_router.finalize();
    InterfaceMonitor ifmon(xrl_std_router, eventloop, "fea");

    wait_until_xrl_router_is_ready(eventloop, xrl_std_router);

    //
    // Startup
    //
    ifmon.startup();

    //
    // Main loop
    //
    while (ifmon.status() == STARTING) {
	eventloop.run();
    }

    if (ifmon.status() == RUNNING) {
	ifmon.print_interfaces(print_iface_name);
    }

    //
    // Shutdown
    //
    ifmon.shutdown();
    while (ifmon.status() == SHUTTING_DOWN) {
	eventloop.run();
    }

    //
    // Wait for all pending XRL operations
    //
    while (xrl_std_router.pending()) {
	eventloop.run();
    }
}

int
main(int argc, char* const argv[])
{
    int ch;
    string::size_type idx;
    const char *argv0 = argv[0];
    string finder_hostname = FINDER_DEFAULT_HOST.str();
    uint16_t finder_port = FINDER_DEFAULT_PORT; // XXX: default (in host order)
    string print_iface_name = "";	// XXX; empty string means print all
    string command;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:i:h")) != -1) {
	switch (ch) {
	case 'F':
	    // Finder hostname and port
	    finder_hostname = optarg;
	    idx = finder_hostname.find(':');
	    if (idx != string::npos) {
		if (idx + 1 >= finder_hostname.length()) {
		    // No port number
		    usage(argv0, 1);
		    // NOTREACHED
		}
		char* p = &finder_hostname[idx + 1];
		finder_port = static_cast<uint16_t>(atoi(p));
		finder_hostname = finder_hostname.substr(0, idx);
	    }
	    break;
	case 'i':
	    // Network interface name
	    print_iface_name = optarg;
	    break;
	case 'h':
	case '?':
	    // Help
	    usage(argv0, 0);
	    // NOTREACHED
	    break;
	default:
	    usage(argv0, 1);
	    // NOTREACHED
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    // Get the command itself
    if (argc == 0) {
	usage(argv0, 1);
	// NOTREACHED
    }
    while (argc != 0) {
	if (! command.empty())
	    command += " ";
	command += string(argv[0]);
	argc--;
	argv++;
    }

    if (command.empty()) {
	usage(argv0, 1);
	// NOTREACHED
    }

    //
    // Run everything
    //
    try {
	interface_monitor_main(finder_hostname.c_str(), finder_port,
			       print_iface_name);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit (0);
}
