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

#ident "$XORP: xorp/rip/test_update_queue.cc,v 1.1 2003/07/15 17:40:43 hodson Exp $"

#include <set>

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

static const char *program_name         = "test_updates";
static const char *program_description  = "Test RIP update queue";
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
make_nets(set<IPv4Net>& nets, uint32_t n)
{
    uint32_t fails = 0;
    // attempt at deterministic nets sequence
    while (nets.size() != n) {
	IPv4 addr(htonl(fake_random()));
	IPv4Net net = IPv4Net(addr, 1 + n % 23 + fake_random() % 8);
	if (nets.find(net) == nets.end()) {
	    nets.insert(net);
	    fails = 0;
	} else {
	    // Does not occur with test parameters in practice
	    if (++fails == 5) {
		verbose_log("Failed to generate nets.\n");
	    }
	}
    }
}

template <typename A>
class UpdateQueueTester
{
public:
    UpdateQueueTester()
	: _e(), _rip_system(_e), _pm(_rip_system)
    {
	_pm.the_port()->constants().set_expiry_secs(3);
	_pm.the_port()->constants().set_deletion_secs(2);
    }

    ~UpdateQueueTester()
    {
	RouteDB<A>& rdb = _rip_system.route_db();
	rdb.flush_routes();
	UpdateQueue<A>& uq = rdb.update_queue();
	uq.flush();
    }

    int run_test()
    {
	const uint32_t n_routes = 20000;

	RouteDB<A>& rdb = _rip_system.route_db();
	UpdateQueue<A>& uq = _rip_system.route_db().update_queue();

	_fast_reader = uq.create_reader();
	_slow_reader = uq.create_reader();

	if (uq.updates_queued() != 0) {
	    verbose_log("Have updates when none expected\n");
	    return 1;
	}

	verbose_log("Generating nets\n");
	set<IPNet<A> > nets;
	make_nets(nets, n_routes);

	verbose_log("Creating routes for nets\n");

	for (typename set<IPNet<A> >::const_iterator n = nets.begin();
	     n != nets.end(); ++n) {
	    if (rdb.update_route(*n, A::ZERO(), 5, 0,
				 _pm.the_peer()) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return 1;
	    }
	}

	if (uq.updates_queued() != n_routes) {
	    verbose_log("%u updates queued, expected %u\n",
			uq.updates_queued(), n_routes);
	    return 1;
	}

	uint32_t n = 0;
	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", n);
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	bool expire = false;
	XorpTimer t = _e.set_flag_after_ms(6000, &expire);
	while (t.scheduled()) {
	    _e.run();
	}

	verbose_log("%u updates queued, expected %u\n",
		    uq.updates_queued(), n_routes);

	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", n);
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	for (n = 0; n < 2 * n_routes; n++) {
	    if (uq.get(_slow_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", n);
		return 1;
	    }
	    uq.next(_slow_reader);
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	uq.destroy_reader(_fast_reader);
	uq.destroy_reader(_slow_reader);

	if (uq.updates_queued() != 0) {
	    verbose_log("Updates queued (%u) when no readers present\n",
			uq.updates_queued());
	    return 1;
	}

	_slow_reader = uq.create_reader();
	_fast_reader = uq.create_reader();

	verbose_log("Creating routes for nets (again)\n");

	for (typename set<IPNet<A> >::const_iterator n = nets.begin();
	     n != nets.end(); ++n) {
	    if (rdb.update_route(*n, A::ZERO(), 5, 0,
				 _pm.the_peer()) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return 1;
	    }
	}

	verbose_log("flushing and waiting for route updates\n");

	uq.ffwd(_fast_reader);
	uq.flush();

	if (uq.updates_queued() != 0) {
	    verbose_log("Flushed and route count != 0\n");
	    return 1;
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
 	    return 1;
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	expire = false;
	t = _e.set_flag_after_ms(6000, &expire);
	while (t.scheduled()) {
	    _e.run();
	}

	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_slow_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", n);
		return 1;
	    }
	    uq.next(_slow_reader);

	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", n);
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	verbose_log("Success\n");
	return 0;
    }

protected:
    EventLoop		_e;
    System<A>		_rip_system;
    SpoofPortManager<A> _pm;

    typename UpdateQueue<A>::ReadIterator _fast_reader;
    typename UpdateQueue<A>::ReadIterator _slow_reader;
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
	UpdateQueueTester<IPv4> uqt;
	rval = uqt.run_test();
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
