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

#ident "$XORP: xorp/rip/test_route_walk.cc,v 1.1 2003/07/11 22:10:59 hodson Exp $"

#include "rip_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "port.hh"
#include "peer.hh"
#include "route_db.hh"
#include "system.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_route_walk";
static const char *program_description  = "Test RIP route walking";
static const char *program_version_id   = "0.1";
static const char *program_date         = "July, 2003";
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
	fflush(stdout);							\
    }									\
} while(0)


// ----------------------------------------------------------------------------
// Spoof Port that supports just a single Peer
//

template <typename A>
class SpoofPort : public Port<A> {
public:
    SpoofPort(PortManagerBase<A>& pm, A addr) : Port<A>(pm)
    {
	_peers.push_back(new Peer<A>(*this, addr));
	verbose_log("Constructing SpoofPort instance\n");
    }
    ~SpoofPort()
    {
	verbose_log("Destructing SpoofPort instance\n");
	while (_peers.empty() == false) {
	    delete _peers.front();
	    _peers.pop_front();
	}
    }
};


// ----------------------------------------------------------------------------
// Pseudo random number and network generation

static uint32_t
fake_random()
{
    static uint64_t r = 883652921;
    r = r * 37 + 1;
    r = r & 0xffffffff;
    return r & 0xffffffff;
}

static void
make_nets(vector<IPv4Net>& nets, uint32_t n)
{
    uint32_t fails = 0;
    // attempt at deterministic nets sequence
    while (nets.size() != n) {
	IPv4 addr(htonl(fake_random()));
	IPv4Net net = IPv4Net(addr, 1 + n % 23 + fake_random() % 8);
	if (find(nets.begin(), nets.end(), net) == nets.end()) {
	    nets.push_back(net);
	    fails = 0;
	} else {
	    // Does not occur with test parameters in practice
	    if (++fails == 5) {
		verbose_log("Failed to generate nets.\n");
	    }
	}
    }
}

// ----------------------------------------------------------------------------
// Type specific helpers

template <typename A>
struct DefaultPeer {
    static A get();
};

template <>
IPv4 DefaultPeer<IPv4>::get() { return IPv4("10.0.0.1"); }

template <>
IPv6 DefaultPeer<IPv6>::get() { return IPv6("10:1"); }

// ----------------------------------------------------------------------------
// Spoof Port Manager instance support a single Spoof Port which in turn
// contains a single Peer.
//

template <typename A>
class SpoofPortManager : public PortManagerBase<A> {
public:
    SpoofPortManager(System<A>& s)
	: PortManagerBase<A>(s)
    {
	_ports.push_back(new SpoofPort<A>(*this, DefaultPeer<A>::get()));
    }

    ~SpoofPortManager()
    {
	while (!_ports.empty()) {
	    delete _ports.front();
	    _ports.pop_front();
	}
    }

    Port<A>* the_port()
    {
	XLOG_ASSERT(_ports.size() == 1);
	return _ports.front();
    }

    Peer<A>* the_peer()
    {
	XLOG_ASSERT(_ports.size() == 1);
	XLOG_ASSERT(_ports.front()->peers().size() == 1);
	return _ports.front()->peers().front();
    }
};


///////////////////////////////////////////////////////////////////////////////
//
// Walk tester class
//

template <typename A>
class RouteWalkTester
{
public:
    RouteWalkTester()
	: _e(), _rip_system(_e), _pm(_rip_system)
    {
	_pm.the_port()->constants().set_expiry_secs(3);
	_pm.the_port()->constants().set_deletion_secs(2);
    }

    ~RouteWalkTester()
    {
	RouteDB<A>& rdb = _rip_system.route_db();
	rdb.flush_routes();
    }

    bool
    walk_routes(RouteWalker<A>* rw,
		bool wait_on_expiring,
		uint32_t* done,
		uint32_t todo)
    {
	TimeVal now;
	_e.current_time(now);

	TimeVal ten_ms(0, 10000);

	verbose_log("walked routes = %u\n", *done);
	rw->resume();
	while (todo != 0) {
	    if (rw->current_route() == 0) {
		verbose_log("Halting with %u routes done.\n", *done);
		return false;
	    }
	    if (wait_on_expiring) {
		TimeVal delta = rw->current_route()->timer().expiry() - now;
		if (delta < ten_ms) {
		    verbose_log("Pausing on route about to be expired "
				"or deleted (expiry in %d.%06d secs).\n",
				delta.sec(), delta.usec());
		    break;
		}
	    }
	    rw->next_route();
	    *done = *done + 1;
	    todo--;
	}
	rw->pause(1);
	return true;
    }

    int
    run_test()
    {
	const uint32_t n_routes = 20000;

	verbose_log("Generating nets\n");
	vector<IPNet<A> > nets;
	make_nets(nets, n_routes);

	verbose_log("Creating routes for nets\n");
	RouteDB<A>& rdb = _rip_system.route_db();
	for(uint32_t i = 0; i < nets.size(); ++i) {
	    if (rdb.update_route(nets[i], A::ZERO(), 5, 0,
				 _pm.the_peer()) == false) {
		verbose_log("Failed to add route for %s\n",
			    nets[i].str().c_str());
		return 1;
	    }
	}

	// Walk routes on 1ms timer
	// We make 2 passes over routes with 97 routes read per 1ms
	// Total time taken 2 * 20000 / 97 * 0.001 = 407ms
	uint32_t routes_done = 0;
	RouteWalker<A> rw(rdb);
	XorpTimer t;
	for (int i = 0; i < 2; i++) {
	    verbose_log("Starting full route walk %d\n", i);
	    t = _e.new_periodic(1,
				callback(this,
					 &RouteWalkTester<A>::walk_routes,
					 &rw, false,
					 &routes_done, 97u));
	    while (t.scheduled()) {
		_e.run();
		if (routes_done > n_routes) {
		    verbose_log("Walked more routes than exist!\n");
		    return 1;
		}
	    }
	    if (routes_done != n_routes) {
		verbose_log("Read %u routes, expected to read %d\n",
			    routes_done, n_routes);
		return 1;
	    }
	    routes_done = 0;
	    rw.reset();
	}

	// Routes start being deleted after 5 seconds so if we walk
	// slowly we have a reasonable chance of standing on a route as
	// it gets deleted.
	//
	// Read 10 routes every 30ms.
	// 20000 routes -> 20000 / 10 * 0.030 = 60 seconds
	//
	// Except it doesn't run that long as the routes are collapsing
	//
	verbose_log("Starting walk as routes are deleted.\n");
	rw.reset();
	routes_done = 0;
	t = _e.new_periodic(30,
			    callback(this,
				     &RouteWalkTester<A>::walk_routes,
				     &rw, true,
				     &routes_done, 10u));
	while (t.scheduled()) {
	    _e.run();
	    if (routes_done > n_routes) {
		verbose_log("Walked more routes than exist!\n");
		return 1;
	    }
	}
	verbose_log("Read %u routes, %u available at end.\n",
		    routes_done, rdb.route_count());

	rdb.flush_routes();
	if (rdb.route_count() != 0) {
	    verbose_log("Had routes left when none expected.\n");
	    return 1;
	}
	return 0;
    }

protected:
    EventLoop		_e;
    System<A>		_rip_system;
    SpoofPortManager<A> _pm;
};


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

/*
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
main(int argc, char* const argv[])
{
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

    int rval = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	RouteWalkTester<IPv4> rwt;
	rval = rwt.run_test();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        rval = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return rval;
}



