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

#ident "$XORP: xorp/rib/add_route.cc,v 1.1 2004/09/28 00:38:13 atanu Exp $"

// Add routes to the RIB.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sysexits.h>
#include <fstream>
#include <cstdlib>
#include <vector>

#include "config.h"
#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/rib_xif.hh"

template <typename A>
class Route {
 public:
    bool add;
    IPNet<A> net;
    A	nexthop;
};

template <typename A>
class Fire {
 public:
    typedef vector<Route<A> > Routes;

    Fire(XrlStdRouter& xrl_router, Routes &r,
	 string ribname, string tablename, bool max_inflight)
	: _xrl_router(xrl_router), _r(r), _ribname(ribname),
	  _tablename(tablename),
	  _flying(0),
	  _max_inflight(max_inflight)
    {}

    void start() {
	_next = _r.begin();
	send();
    }

    bool busy() {
	return 0 != _flying;
    }
 private:
    bool send();
    void done(const XrlError& error, string comment);

 private:    
    XrlStdRouter &_xrl_router;
    Routes &_r;
    string _ribname;
    string _tablename;

    int _flying;			// Number of XRLs in flight.
    bool _max_inflight;			// Try and get the maximum
					// number of XRLs in flight.
    typename Routes::iterator  _next;	// Next route to send.
};

template<> bool Fire<IPv4>::send();

void
done(const XrlError& error, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
    }
}

int
read_routes(const string& route_file, Fire<IPv4>::Routes& v)
{
    //
    // Read in the file of routes to send
    //
    // The file contains:
    // Subnet followed by nexthop
    std::ifstream routein(route_file.c_str());
    if (!routein) {
	XLOG_ERROR("Failed to open file: %s %s", route_file.c_str(),
		   strerror(errno));
	return EX_NOINPUT;
    }

    while (routein) {
	Route<IPv4> r;

	string which;
	routein >> which;
	if ("add" == which) {
	    r.add = true;
	} else if ("delete" == which) {
	    r.add = false;
	} else if ("#" == which) {
	    char buf[1024];
	    routein.getline(buf, sizeof(buf));
	    continue;
	} else if ("" == which) {
	    break;
	} else {
	    XLOG_ERROR("Allowed command are \"add\" or \"delete\""
		       " not <%s>", which.c_str());
	    return EX_DATAERR;
	}

	string word;
	routein >> word;
	r.net = IPNet<IPv4>(word.c_str());
	routein >> word;
	r.nexthop = IPv4(word.c_str());
	v.push_back(r);
    }

    routein.close();

    return 0;
}

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s -r routes [-t tablename] [-R][-u][-s][-d]\n",
	    myname);
    return EX_USAGE;
}

int 
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string route_file;
    string ribname = "rib";
    string tablename = "ebgp";
    bool repeat = false;
    bool max_inflight = false;
    bool silent = false;
    bool dont_add_table = false;

    int c;
    while ((c = getopt(argc, argv, "r:t:Rusd")) != -1) {
	switch (c) {
	case 'r':
	    route_file = optarg;
	    break;
	case 't':
	    tablename = optarg;
	    break;
	case 'R':
	    repeat = true;
	    break;
	case 'u':
	    max_inflight = true;
	    break;
	case 's':
	    silent = true;
	    break;
	case 'd':
	    dont_add_table = true;
	    break;
	default:
	    return usage(argv[0]);
	}
    }

    if (route_file == "")
	return usage(argv[0]);

    try {
	//
	// Init stuff
	//
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "add_route");

	debug_msg("Waiting for router");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	debug_msg("\n");

	// Create table.
	if (!dont_add_table) {
	    XrlRibV0p1Client rib(&xrl_router);
	    rib.send_add_egp_table4(ribname.c_str(),
				    tablename,
				    xrl_router.class_name(),
				    xrl_router.instance_name(), true, true,
				    callback(done, "add_table"));
	}

	Fire<IPv4>::Routes v;
	int ret = read_routes(route_file, v);
	if (0 != ret)
	    return ret;

	Fire<IPv4> f(xrl_router, v, ribname, tablename, max_inflight);

	//
	// Main loop
	//
	do {
	    timeval now;
	    gettimeofday(&now, 0);
	    TimeVal start = TimeVal(now);
	    f.start();
	    while (f.busy()) {
		eventloop.run();
	    }
	    gettimeofday(&now, 0);
	    TimeVal end = TimeVal(now);
	    TimeVal delta = end - start;
	    if (!silent)
		printf("%s seconds taken to add/delete %d routes\n",
		       delta.str().c_str(), static_cast<uint32_t>(v.size()));
	} while (repeat);
	       
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    // 
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

template<>
bool
Fire<IPv4>::send()
{
    if (_next == _r.end())
	return false;

    XrlRibV0p1Client rib(&_xrl_router);
    bool sent;
    if (_next->add) {
	sent = rib.send_add_route4(_ribname.c_str(),
				   _tablename,
				   true /*unicast*/,
				   false /*multicast*/,
				   _next->net,
				   _next->nexthop,
				   0/*metric*/,
				   XrlAtomList(),
				   callback(this,
					    &Fire::done,
					    c_format("add net: %s nexthop: %s",
						     _next->net.str().c_str(),
						     _next->nexthop.str().
						     c_str())));
    } else {
	sent = rib.send_delete_route4(_ribname.c_str(),
				   _tablename,
				   true /*unicast*/,
				   false /*multicast*/,
				   _next->net,
				   callback(this,
					    &Fire::done,
					    c_format("delete net: %s",
						     _next->net.str().c_str()
						     )));
    }

    if (sent) {
	_next++;
	_flying++;
    }

    return sent;
}

template<typename A>
void
Fire<A>::done(const XrlError& error, string comment)
{
    _flying--;
    debug_msg("callback %s\n %s\n", comment.c_str(), error.str().c_str());
	
    switch (error.error_code()) {
    case OKAY:
	break;

    case REPLY_TIMED_OUT:
    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
    case NO_FINDER:
    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_ERROR("callback: %s\n%s",
		   comment.c_str(), error.str().c_str());
	break;
    }
	
    if (_max_inflight) {
	if (0 == _flying) {
	    while(send())
		;
	    printf("Flying %d\n", _flying);
	}
    } else {
	send();
    }
}
