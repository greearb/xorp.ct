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

#ident "$XORP: xorp/rib/tools/show_routes.cc,v 1.5 2004/08/03 03:01:08 pavlin Exp $"

#include "rib/rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/targets/show_routes_base.hh"

// ----------------------------------------------------------------------------
// Structure for holding command line options

struct ShowRoutesOptions {
    bool ribin;			// ribin (true), ribout (false)
    bool ipv4;			// IPv4 (true), IPv6(false)
    bool unicast;		// unicast (true), multicast (false)
    const char* protocol;
    string	xrl_target;
    string	finder_host;
    uint16_t	finder_port;

    inline ShowRoutesOptions()
	: ribin(false), ipv4(true), unicast(true), protocol("all")
    {}
};


// ----------------------------------------------------------------------------
// Display functions and helpers

struct AdEntry {
    uint32_t	ad;
    const char*	name;
    inline bool operator<(const AdEntry& o) const { return ad < o.ad; }
};

static const char*
ad2protocol(uint32_t admin_distance)
{
    // XXX These values are cut and paste from rib.cc.  At some point
    // we'd like these values to be soft state in the rib and obviously
    // this won't suffice.
    static const struct AdEntry ads[] = {
	{ 0, "connected"}, { 1, "static"}, { 5, "eigrp-summary" },
	{ 20, "ebgp" },	{ 90, "eigrp-internal" }, { 100, "igrp" },
	{ 110, "ospf" }, { 115, "is-is" }, { 120, "rip" },
	{ 170, "eigrp-external"}, { 200, "ibgp" }, { 254, "fib2mrib"},
	{ 255, "unknown" }
    };
    static const size_t n = sizeof(ads) / sizeof(ads[0]);

    AdEntry s;
    s.ad = admin_distance;
    const AdEntry* e = lower_bound(ads, ads + n, s);
    if (e == ads + n || e->ad != admin_distance) {
	return NULL;
    }
    return e->name;
}

template <typename A>
static void
display_route(const IPNet<A>& 	net,
	      const A& 		nexthop,
	      const string& 	ifname,
	      const string& 	vifname,
	      const uint32_t 	metric,
	      const uint32_t	admin_distance)
{
    cout << "Network " << net.str() << endl;
    cout << "    Nexthop := " << nexthop.str() << endl;

    cout << "    Metric := ";
    cout.width(5);
    cout << metric;

    const char* protocol = ad2protocol(admin_distance);
    if (protocol == 0) {
	cout << "    AD := ";
	cout.width(5);
	cout << admin_distance;
    } else {
	cout << "    Protocol := " << protocol;
    }

    if (ifname.empty() == false)
	cout << "    Interface := " << ifname;

    if (vifname.empty() == false)
	cout << "    Vif := " << vifname;
    cout << endl;
}

// ----------------------------------------------------------------------------
// Class for Querying RIB routes

class ShowRoutesProcessor
    : public XrlShowRoutesTargetBase, public ServiceBase {
public:
    ShowRoutesProcessor(EventLoop&	   e,
			ShowRoutesOptions& opts);
    ~ShowRoutesProcessor();

    bool startup();
    bool shutdown();

public:
    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();

    XrlCmdError finder_event_observer_0_1_xrl_target_birth(const string& cls,
							   const string& ins);
    XrlCmdError finder_event_observer_0_1_xrl_target_death(const string& cls,
							   const string& ins);

    XrlCmdError redist4_0_1_starting_route_dump(const string&	cookie);
    XrlCmdError redist4_0_1_finishing_route_dump(const string&	cookie);
    XrlCmdError redist4_0_1_add_route(const IPv4Net&	dst,
				      const IPv4&	nexthop,
				      const string&	ifname,
				      const string&	vifname,
				      const uint32_t&	metric,
				      const uint32_t&	admin_distance,
				      const string&	cookie);
    XrlCmdError redist4_0_1_delete_route(const IPv4Net&	network,
					 const string&	cookie);

    XrlCmdError redist6_0_1_starting_route_dump(const string&	cookie);
    XrlCmdError redist6_0_1_finishing_route_dump(const string&	cookie);
    XrlCmdError redist6_0_1_add_route(const IPv6Net&	dst,
				      const IPv6&	nexthop,
				      const string&	ifname,
				      const string&	vifname,
				      const uint32_t&	metric,
				      const uint32_t&	admin_distance,
				      const string&	cookie);
    XrlCmdError redist6_0_1_delete_route(const IPv6Net&	network,
					 const string&	cookie);

    bool poll_ready_failed();

    /**
     * @return true if cookie matches instance cookie, false otherwise.
     */
    bool check_cookie(const string& cookie);

    /**
     * Register with Finder to watch RIB birth and death events.  If
     * the RIB crashes we don't want to hang waiting for messages from
     * the RIB that will never arrive (start route dump, add route,
     * finish route dump)
     */
    void step_100_watch_rib();
    void watch_rib_cb(const XrlError& xe);

    /**
     * Request redistribution of user requested protocol.
     */
    void step_200_request_redist();
    void request_redist_cb(const XrlError& xe);

    /**
     * Request redistribution cease.
     */
    void step_1000_request_cease();
    void request_cease_cb(const XrlError& xe);

protected:
    EventLoop&			_e;
    const ShowRoutesOptions& 	_opts;
    XrlRouter*			_rtr;
    XorpTimer			_t;
    string			_cookie;
};

ShowRoutesProcessor::ShowRoutesProcessor(EventLoop&		e,
					 ShowRoutesOptions&	o)
    : _e(e), _opts(o)
{
    _rtr = 0;
}

ShowRoutesProcessor::~ShowRoutesProcessor()
{
    delete _rtr;
}

bool
ShowRoutesProcessor::startup()
{
    if (status() != READY) {
	return false;
    }

    // Create XrlRouter
    _rtr = new XrlStdRouter(_e, "show_routes",
			    _opts.finder_host.c_str(), _opts.finder_port);

    // Glue the router to the Xrl methods class exports
    this->set_command_map(_rtr);

    // Register methods with Finder
    _rtr->finalize();

    // Start timer to poll whether XrlRouter becomes ready or fails so
    // we can continue processing.
    _t = _e.new_periodic(250,
			 callback(this,
				  &ShowRoutesProcessor::poll_ready_failed));
    set_status(STARTING);
    return true;
}

bool
ShowRoutesProcessor::shutdown()
{
    ServiceStatus st = this->status();
    if (st == FAILED || st == SHUTTING_DOWN || st == SHUTDOWN)
	return false;

    // Withdraw the Xrl methods
    this->set_command_map(NULL);

    set_status(SHUTTING_DOWN);
    step_1000_request_cease();
    return true;
}

bool
ShowRoutesProcessor::poll_ready_failed()
{
    if (_rtr == 0) {
	return false;
    } else if (_rtr->ready()) {
	set_status(RUNNING);
	step_100_watch_rib();
	return false;
    } else if (_rtr->failed()) {
	set_status(FAILED, "Failed: No Finder?");
	return false;
    }
    return true;
}

void
ShowRoutesProcessor::step_100_watch_rib()
{
    XrlFinderEventNotifierV0p1Client fen(_rtr);

    if (fen.send_register_class_event_interest(
		"finder", _rtr->instance_name(), _opts.xrl_target,
		callback(this, &ShowRoutesProcessor::watch_rib_cb)) == false) {
	set_status(FAILED, c_format("Failed to register interest in %s",
				   _opts.xrl_target.c_str()));
	return;
    }
}

void
ShowRoutesProcessor::watch_rib_cb(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	step_200_request_redist();
	return;
    }
    set_status(FAILED, c_format("Failed to register interest in %s",
				_opts.xrl_target.c_str()));
    return;
}

inline string
rib_table_name(const string& protocol, bool ribin)
{
    if (protocol == "all")
	return protocol;
    // "all" is a special name in the RIB for the rib out table.
    // "all-<proto>" adds  redist with a filter for <proto>
    return c_format("%s%s", (ribin ? "" : "all-"), protocol.c_str());
}

void
ShowRoutesProcessor::step_200_request_redist()
{
    string protocol = rib_table_name(_opts.protocol, _opts.ribin);

    XrlRibV0p1Client::RedistEnable4CB cb;
    cb = callback(this, &ShowRoutesProcessor::request_redist_cb);

    XrlRibV0p1Client rib(_rtr);

    bool sent = false;
    if (_opts.ipv4) {
	sent = rib.send_redist_enable4(_opts.xrl_target.c_str(),
				       _rtr->instance_name(),
				       protocol, _opts.unicast, !_opts.unicast,
				       _cookie, cb);

    } else {
	sent = rib.send_redist_enable6(_opts.xrl_target.c_str(),
				       _rtr->instance_name(),
				       protocol, _opts.unicast, !_opts.unicast,
				       _cookie, cb);
    }

    if (sent == false) {
	set_status(FAILED, "Failed to request redist.");
    }
}

void
ShowRoutesProcessor::request_redist_cb(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	return;
    }
    set_status(FAILED,
	       c_format("Request for routes to be redistributed from %s "
			"failed.\nThe protocol is probably not active.",
			_opts.protocol));
    return;
}

void
ShowRoutesProcessor::step_1000_request_cease()
{
    string proto = rib_table_name(_opts.protocol, _opts.ribin);

    XrlRibV0p1Client::RedistDisable4CB cb;
    cb = callback(this, &ShowRoutesProcessor::request_cease_cb);

    XrlRibV0p1Client rib(_rtr);

    bool sent = false;
    if (_opts.ipv4) {
	sent = rib.send_redist_disable4(_opts.xrl_target.c_str(),
					_rtr->instance_name(),
					proto, _opts.unicast, !_opts.unicast,
					_cookie, cb);

    } else {
	sent = rib.send_redist_disable6(_opts.xrl_target.c_str(),
					_rtr->instance_name(),
					proto, _opts.unicast, !_opts.unicast,
					_cookie, cb);
    }

    if (sent == false) {
	set_status(FAILED, "Failed to request redistribution end.");
	return;
    }
    set_status(SHUTTING_DOWN);
}

void
ShowRoutesProcessor::request_cease_cb(const XrlError& /* xe */)
{
    set_status(SHUTDOWN);
    return;
}


bool
ShowRoutesProcessor::check_cookie(const string& cookie)
{
    if (cookie != _cookie) {
	cerr << "Bug detected cookie mismatch \"" << cookie << "\" != \""
	     << _cookie << "\"" << endl;
    }
    return true;
}


XrlCmdError
ShowRoutesProcessor::common_0_1_get_target_name(string& name)
{
    name = this->name();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::common_0_1_get_version(string& version)
{
    version = this->version();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::common_0_1_get_status(uint32_t&	status,
					   string&	/* reason */)
{
    switch (this->status()) {
    case READY:		status = PROC_NULL;		break;
    case STARTING:	status = PROC_STARTUP;		break;
    case RUNNING:	status = PROC_READY;		break;
    case PAUSED:					/* FALLTHRU */
    case PAUSING:					/* FALLTHRU */
    case RESUMING:	status = PROC_NOT_READY;	break;
    case SHUTTING_DOWN:	status = PROC_SHUTDOWN;		break;
    case SHUTDOWN:	status = PROC_DONE;		break;
    case FAILED:	status = PROC_FAILED;		break;
    case ALL:						break;
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::common_0_1_shutdown()
{
    this->shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::finder_event_observer_0_1_xrl_target_birth(
						const string& /* cls */,
						const string& /* ins */
						)
{
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::finder_event_observer_0_1_xrl_target_death(
						const string& cls,
						const string& /* ins */)
{
    if (cls == _opts.xrl_target)
	this->shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist4_0_1_starting_route_dump(const string& cookie)
{
    _cookie = cookie;
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist4_0_1_finishing_route_dump(const string& cookie)
{
    check_cookie(cookie);

    this->shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist4_0_1_add_route(const IPv4Net&	dst,
					   const IPv4&		nexthop,
					   const string&	ifname,
					   const string&	vifname,
					   const uint32_t&	metric,
					   const uint32_t&	admin_distance,
					   const string&	cookie)
{
    if (this->status() != RUNNING || check_cookie(cookie) == false) {
	return XrlCmdError::OKAY();
    }

    display_route(dst, nexthop, ifname, vifname, metric, admin_distance);
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist4_0_1_delete_route(const IPv4Net&	/* network */,
					      const string&	/* cookie  */)
{
    // XXX For now we ignore deletions that occur during route dump
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist6_0_1_starting_route_dump(const string& cookie)
{
    check_cookie(cookie);
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist6_0_1_finishing_route_dump(const string& cookie)
{
    check_cookie(cookie);
    this->shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist6_0_1_add_route(const IPv6Net&	dst,
					   const IPv6&		nexthop,
					   const string&	ifname,
					   const string&	vifname,
					   const uint32_t&	metric,
					   const uint32_t&	admin_distance,
					   const string&	cookie)
{
    if (this->status() != RUNNING || check_cookie(cookie) == false) {
	return XrlCmdError::OKAY();
    }
    display_route(dst, nexthop, ifname, vifname, metric, admin_distance);
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist6_0_1_delete_route(const IPv6Net&	/* network */,
					      const string&	/* cookie */)
{
    // XXX For now we ignore deletions that occur during route dump
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// Utility methods

static void
usage()
{
    fprintf(stderr,
	    "Usage: %s [options] (ribin|ribout) (ipv4|ipv6) (unicast|multicast) <protocol>\n",
	    xlog_process_name());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t -F <finder_host>:<finder_port> "
	    "Specify Finder host and port to use.\n");
    fprintf(stderr, "\t -T <targetname>                "
	    "Specify XrlTarget to query.\n\n");
    fprintf(stderr, "\t -b                             "
	    "Brief output.\n");
    exit(-1);
}

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

static int
find_option(const char* s, const char* opts[], size_t n_opts)
{
    for (size_t i = 0; i < n_opts; i++) {
	if (strcmp(s, opts[i]) == 0) {
	    return (int)i;
	}
    }
    fprintf(stderr, "Invalid option \"%s\", expected one of:", s);
    for (size_t i = 0; i < n_opts; i++) {
	fprintf(stderr, "%s\"%s\"", (i == 0) ? " " : ", ", opts[i]);
    }
    fprintf(stderr, "\n");
    return -1;
}

static inline bool
match_binary_option(const char* input,
		    const char* option1,
		    const char* option2,
		    bool&	matches_first)
{
    const char* argv[2] = { option1, option2 };
    int i = find_option(input, argv, 2);
    if (i < 0)
	return false;
    matches_first = (i == 0) ? true : false;
    return true;
}

// ----------------------------------------------------------------------------
// Main

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	bool		do_run 	    = true;
	bool		brief 	    = false;

	ShowRoutesOptions sr_opts;

	sr_opts.finder_host = FINDER_DEFAULT_HOST.str();
        sr_opts.finder_port = FINDER_DEFAULT_PORT;
	sr_opts.xrl_target  = "rib";

	int ch;
	while ((ch = getopt(argc, argv, "bF:T:")) != -1) {
	    switch (ch) {
	    case 'b':
		brief = true;
		break;
	    case 'F':
		do_run = parse_finder_args(optarg,
					   sr_opts.finder_host,
					   sr_opts.finder_port);
		break;
	    case 'T':
		sr_opts.xrl_target = optarg;
		break;
	    default:
		usage();
		do_run = false;
	    }
	}
	argc -= optind;
	argv += optind;

	if (argc != 4) {
	    usage();
	}

	if (match_binary_option(argv[0], "ribin", "ribout", sr_opts.ribin)
	    == false) {
	    usage();
	}

	if (match_binary_option(argv[1], "ipv4", "ipv6", sr_opts.ipv4)
	    == false) {
	    usage();
	}

	if (match_binary_option(argv[2], "unicast", "multicast",
				sr_opts.unicast) == false) {
	    usage();
	}

	sr_opts.protocol = argv[3];

	EventLoop e;
	ShowRoutesProcessor srp(e, sr_opts);

	srp.startup();
	while (srp.status() != FAILED && srp.status() != SHUTDOWN) {
	    e.run();
	}

	if (srp.status() == FAILED) {
	    if (srp.status_note().empty() == false) {
		cout << srp.status_note() << endl;
	    } else {
		cout << "Failed" << endl;
	    }
	    cout << endl;
	}

    } catch (...) {
	xorp_print_standard_exceptions();
    }


    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

