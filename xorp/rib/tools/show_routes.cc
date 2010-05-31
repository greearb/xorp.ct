// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rib/rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include <iomanip>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/targets/show_routes_base.hh"

//
// TODO:
// - Beware of multiple routes to the same destination.
//   Something in the upper code should be done?
// - Print the time for each route.
// - Show the route Status (active/last/both)
// - Show the AS Path
//

//
// Print style
//
enum PrintStyle {
    PRINT_STYLE_BRIEF,
    PRINT_STYLE_DETAIL,
    PRINT_STYLE_TERSE,
    PRINT_STYLE_DEFAULT = PRINT_STYLE_BRIEF	// XXX: default to BRIEF
};

// ----------------------------------------------------------------------------
// Structure for holding command line options

struct ShowRoutesOptions {
    bool ribin;			// ribin (true), ribout (false)
    bool ipv4;			// IPv4 (true), IPv6(false)
    bool unicast;		// unicast (true), multicast (false)
    PrintStyle print_style;	// -b (BRIEF), -d (DETAIL), -t (TERSE)
    const char* protocol;
    string	xrl_target;
    string	finder_host;
    uint16_t	finder_port;

    ShowRoutesOptions()
	: ribin(false),
	  ipv4(true),
	  unicast(true),
	  print_style(PRINT_STYLE_DEFAULT),
	  protocol("all")
    {}
};


// ----------------------------------------------------------------------------
// Display functions and helpers

template <typename A>
static void
display_route_brief(const IPNet<A>& 	net,
		    const A& 		nexthop,
		    const string& 	ifname,
		    const string& 	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    const string&	protocol_origin)
{
    cout << "" << net.str() << "\t";
    cout << "[";
    cout << protocol_origin << "(" << admin_distance << ")";
    cout << "/" << metric << "]";
    //
    // XXX: At the end of the line we should print the age of the route,
    // but for now we don't have this information.
    //
    // cout << " 1w0d 01:58:27";
    cout << endl;

    cout << "\t\t> ";
    if (admin_distance != 0)
	cout << "to " << nexthop.str() << " ";

    if (ifname.empty() == false)
	cout << "via " << ifname;

    if (vifname.empty() == false)
	cout << "/" << vifname;
    cout << endl;
}

template <typename A>
static void
display_route_detail(const IPNet<A>& 	net,
		     const A& 		nexthop,
		     const string& 	ifname,
		     const string& 	vifname,
		     uint32_t		metric,
		     uint32_t		admin_distance,
		     const string&	protocol_origin)
{
    string tmp_name;
    string unknown_name = "UNKNOWN";

    cout << "Network " << net.str() << endl;
    cout << "    Nexthop        := " << nexthop.str() << endl;

    if (ifname.empty() == false)
	tmp_name = ifname;
    else
	tmp_name = unknown_name;
    cout << "    Interface      := " << tmp_name << endl;

    if (vifname.empty() == false)
	tmp_name = vifname;
    else
	tmp_name = unknown_name;
    cout << "    Vif            := " << tmp_name << endl;

    cout << "    Metric         := " << metric << endl;
    cout << "    Protocol       := " << protocol_origin << endl;
    cout << "    Admin distance := " << admin_distance << endl;
}

template <typename A>
static void
display_route_terse(const IPNet<A>&	net,
		    const A& 		nexthop,
		    const string& 	ifname,
		    const string& 	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    const string&	protocol_origin)
{
    string protocol_short = protocol_origin.substr(0, 1);

    //
    // TODO: Show status of route:
    //       + = Active Route,
    //       - = Last Active,
    //       * = Both

    if (net.str().size() > 18) {
	cout << net.str() << endl << setw(19) << " ";
    } else {
	cout << setiosflags(ios::left) << setw(19) << net.str();
    }
    cout << resetiosflags(ios::left) << protocol_short << " ";
    cout << setw(3) << admin_distance;
    cout << setw(10) << metric << "  ";
    // XXX: We don't have second metric yet
    if (admin_distance != 0)
	cout << nexthop.str() << " ";

    if ((ifname.empty() == false) && (admin_distance == 0))
	cout << ifname;

    if ((vifname.empty() == false) && (admin_distance == 0))
	cout << "/" << vifname;
    // XXX: We don't have the AS path yet
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

    int startup();
    int shutdown();

public:
    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();
    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

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
				      const string&	cookie,
				      const string&	protocol_origin);
    XrlCmdError redist4_0_1_delete_route(const IPv4Net&	dst,
					 const IPv4&	nexthop,
					 const string&	ifname,
					 const string&	vifname,
					 const uint32_t& metric,
					 const uint32_t& admin_distance,
					 const string&	cookie,
					 const string&	protocol_origin);

    XrlCmdError redist6_0_1_starting_route_dump(const string&	cookie);
    XrlCmdError redist6_0_1_finishing_route_dump(const string&	cookie);
    XrlCmdError redist6_0_1_add_route(const IPv6Net&	dst,
				      const IPv6&	nexthop,
				      const string&	ifname,
				      const string&	vifname,
				      const uint32_t&	metric,
				      const uint32_t&	admin_distance,
				      const string&	cookie,
				      const string&	protocol_origin);
    XrlCmdError redist6_0_1_delete_route(const IPv6Net&	dst,
					 const IPv6&	nexthop,
					 const string&	ifname,
					 const string&	vifname,
					 const uint32_t& metric,
					 const uint32_t& admin_distance,
					 const string&	cookie,
					 const string&	protocol_origin);

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
    IPv4Net			_network_prefix4;
    IPv6Net			_network_prefix6;
    string			_cookie;
};

ShowRoutesProcessor::ShowRoutesProcessor(EventLoop&		e,
					 ShowRoutesOptions&	o)
    : _e(e),
      _opts(o),
      _rtr(NULL),
      _network_prefix4(IPv4::ZERO(), 0),	// XXX: get the whole table
      _network_prefix6(IPv6::ZERO(), 0)		// XXX: get the whole table
{
}

ShowRoutesProcessor::~ShowRoutesProcessor()
{
    if (_rtr != NULL) {
	delete _rtr;
	_rtr = NULL;
    }
}

int
ShowRoutesProcessor::startup()
{
    if (status() != SERVICE_READY) {
	return (XORP_ERROR);
    }

    // Create XrlRouter
    string process = c_format("show_routes<%d>", XORP_INT_CAST(getpid()));
    _rtr = new XrlStdRouter(_e, process.c_str(),
			    _opts.finder_host.c_str(), _opts.finder_port);

    // Glue the router to the Xrl methods class exports
    this->set_command_map(_rtr);

    // Register methods with Finder
    _rtr->finalize();

    // Start timer to poll whether XrlRouter becomes ready or fails so
    // we can continue processing.
    _t = _e.new_periodic_ms(250,
			    callback(this,
				     &ShowRoutesProcessor::poll_ready_failed));
    set_status(SERVICE_STARTING);
    return (XORP_OK);
}

int
ShowRoutesProcessor::shutdown()
{
    // Withdraw the Xrl methods
    this->set_command_map(NULL);

    ServiceStatus st = this->status();
    if (st == SERVICE_FAILED
	|| st == SERVICE_SHUTTING_DOWN
	|| st == SERVICE_SHUTDOWN)
	return (XORP_ERROR);

    set_status(SERVICE_SHUTTING_DOWN);
    step_1000_request_cease();
    return (XORP_OK);
}

bool
ShowRoutesProcessor::poll_ready_failed()
{
    if (_rtr == 0) {
	return false;
    } else if (_rtr->ready()) {
	set_status(SERVICE_RUNNING);
	step_100_watch_rib();
	return false;
    } else if (_rtr->failed()) {
	set_status(SERVICE_FAILED, "Failed: No Finder?");
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
	set_status(SERVICE_FAILED,
		   c_format("Failed to register interest in %s",
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
    set_status(SERVICE_FAILED,
	       c_format("Failed to register interest in %s",
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
				       _network_prefix4, _cookie, cb);

    } else {
	sent = rib.send_redist_enable6(_opts.xrl_target.c_str(),
				       _rtr->instance_name(),
				       protocol, _opts.unicast, !_opts.unicast,
				       _network_prefix6, _cookie, cb);
    }

    if (sent == false) {
	set_status(SERVICE_FAILED, "Failed to request redist.");
    }
}

void
ShowRoutesProcessor::request_redist_cb(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	return;
    }
    set_status(SERVICE_FAILED,
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
	set_status(SERVICE_FAILED, "Failed to request redistribution end.");
	return;
    }
    set_status(SERVICE_SHUTTING_DOWN);
}

void
ShowRoutesProcessor::request_cease_cb(const XrlError& /* xe */)
{
    set_status(SERVICE_SHUTDOWN);
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
    case SERVICE_READY:		status = PROC_NULL;		break;
    case SERVICE_STARTING:	status = PROC_STARTUP;		break;
    case SERVICE_RUNNING:	status = PROC_READY;		break;
    case SERVICE_PAUSED:					/* FALLTHRU */
    case SERVICE_PAUSING:					/* FALLTHRU */
    case SERVICE_RESUMING:	status = PROC_NOT_READY;	break;
    case SERVICE_SHUTTING_DOWN:	status = PROC_SHUTDOWN;		break;
    case SERVICE_SHUTDOWN:	status = PROC_DONE;		break;
    case SERVICE_FAILED:	status = PROC_FAILED;		break;
    case SERVICE_ALL:						break;
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
					   const string&	cookie,
					   const string&	protocol_origin)
{
    if (this->status() != SERVICE_RUNNING || check_cookie(cookie) == false) {
	return XrlCmdError::OKAY();
    }

    switch (this->_opts.print_style) {
    case PRINT_STYLE_BRIEF:
	display_route_brief(dst, nexthop, ifname, vifname, metric,
			    admin_distance, protocol_origin);
	break;
    case PRINT_STYLE_DETAIL:
	display_route_detail(dst, nexthop, ifname, vifname, metric,
			     admin_distance, protocol_origin);
	break;
    case PRINT_STYLE_TERSE:
	display_route_terse(dst, nexthop, ifname, vifname, metric,
			    admin_distance, protocol_origin);
	break;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist4_0_1_delete_route(const IPv4Net&	dst,
					      const IPv4&	nexthop,
					      const string&	ifname,
					      const string&	vifname,
					      const uint32_t&	metric,
					      const uint32_t&	admin_distance,
					      const string&	cookie,
					      const string&	protocol_origin)
{
    // TODO: XXX: For now we ignore deletions that occur during route dump
    UNUSED(dst);
    UNUSED(nexthop);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(metric);
    UNUSED(admin_distance);
    UNUSED(cookie);
    UNUSED(protocol_origin);

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
					   const string&	cookie,
					   const string&	protocol_origin)
{
    if (this->status() != SERVICE_RUNNING || check_cookie(cookie) == false) {
	return XrlCmdError::OKAY();
    }

    switch (this->_opts.print_style) {
    case PRINT_STYLE_BRIEF:
	display_route_brief(dst, nexthop, ifname, vifname, metric,
			    admin_distance, protocol_origin);
	break;
    case PRINT_STYLE_DETAIL:
	display_route_detail(dst, nexthop, ifname, vifname, metric,
			     admin_distance, protocol_origin);
	break;
    case PRINT_STYLE_TERSE:
	display_route_terse(dst, nexthop, ifname, vifname, metric,
			    admin_distance, protocol_origin);
	break;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
ShowRoutesProcessor::redist6_0_1_delete_route(const IPv6Net&	dst,
					      const IPv6&	nexthop,
					      const string&	ifname,
					      const string&	vifname,
					      const uint32_t&	metric,
					      const uint32_t&	admin_distance,
					      const string&	cookie,
					      const string&	protocol_origin)
{
    // TODO: XXX: For now we ignore deletions that occur during route dump
    UNUSED(dst);
    UNUSED(nexthop);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(metric);
    UNUSED(admin_distance);
    UNUSED(cookie);
    UNUSED(protocol_origin);

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
    fprintf(stderr, "\t -d                             "
	    "Detailed output.\n");
    fprintf(stderr, "\t -t                             "
	    "Terse output.\n");
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
            XLOG_ERROR("Finder port %u is not in range 1--65535.\n",
		       XORP_UINT_CAST(t_port));
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

	ShowRoutesOptions sr_opts;

	sr_opts.finder_host = FinderConstants::FINDER_DEFAULT_HOST().str();
        sr_opts.finder_port = FinderConstants::FINDER_DEFAULT_PORT();
	sr_opts.xrl_target  = "rib";

	int ch;
	while ((ch = getopt(argc, argv, "bdtF:T:")) != -1) {
	    switch (ch) {
	    case 'b':
		sr_opts.print_style = PRINT_STYLE_BRIEF;
		break;
	    case 'd':
		sr_opts.print_style = PRINT_STYLE_DETAIL;
		break;
	    case 't':
		sr_opts.print_style = PRINT_STYLE_TERSE;
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

	//
	// Print the headers
	//
	switch (sr_opts.print_style) {
	case PRINT_STYLE_BRIEF:
	    // XXX: no header
	    break;
	case PRINT_STYLE_DETAIL:
	    // XXX: no header
	    break;
	case PRINT_STYLE_TERSE:
	    cout << "Destination        P Prf  Metric 1  Next hop          ";
	    // XXX: Will we have all this info in the RIB - Metric2, AS path?
	    // cout << "A Destination        P Prf  Metric 1  Metric 2  "
	    // "Next hop          AS path";
	    cout << endl;
	    break;
	}

	while ((srp.status() != SERVICE_FAILED)
	       && (srp.status() != SERVICE_SHUTDOWN)) {
	    e.run();
	}

	if (srp.status() == SERVICE_FAILED) {
	    srp.shutdown();
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

