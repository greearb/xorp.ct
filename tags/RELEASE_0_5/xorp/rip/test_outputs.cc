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

#ident "$XORP: xorp/rip/test_outputs.cc,v 1.3 2003/08/05 06:55:14 hodson Exp $"

#include <set>

#include "rip_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "auth.hh"
#include "output_table.hh"
#include "output_updates.hh"
#include "port.hh"
#include "peer.hh"
#include "packet_queue.hh"
#include "route_db.hh"
#include "system.hh"
#include "update_queue.hh"

///////////////////////////////////////////////////////////////////////////////
//
// This test comprises of a RIP system with 2 ports injecting routes.  One of
// the ports has an associated Output class instance that generates packets
// containing the routes.  We test both the update packets and the unsolicted
// response packets.
//
//			   +---	System ----+
//	       		TestPort	OtherPort
// 	 Routes ------> TestPeer	OtherPeer <------ Routes
//			   |
//			   V
//		    response packets (inspected)
//
// We look at the routes in the response packets and compare them against
// what we'd expect to see against differing horizon policies.
//

///////////////////////////////////////////////////////////////////////////////
//
// Generic Constants
//

static const char *program_name         = "test_output_update";
static const char *program_description  = "Test RIP Port processing";
static const char *program_version_id   = "0.1";
static const char *program_date         = "August, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, "
					  "2 if internal error";

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
// BlockedPortIO
//
// This is a port IO class that reports it's always pending so packets
// build up in the packet queue and never leave...

template <typename A>
class BlockedPortIO : public PortIOBase<A>
{
public:
    typedef A			Addr;
    typedef PortIOUserBase<A>	PortIOUser;

public:
    BlockedPortIO(PortIOUserBase<A>& user)
	: PortIOBase<A>(user, "if0", "vif0", IPv4("10.0.0.1"))
    {
    }

    /**
     * Called by RIP Port instance.
     */
    bool send(const Addr&,
	      uint16_t,
	      const uint8_t*,
	      size_t)
    {
	XLOG_FATAL("Called send inappropriately");
	return true;
    }

    bool pending() const
    {
	return true;
    }

private:
};

// ----------------------------------------------------------------------------
// Type specific helpers

template <typename A>
struct DefaultPeer {
    static A get();
};

template <typename A>
struct OtherPeer {
    static A get();
};

template <>
IPv4 DefaultPeer<IPv4>::get() { return IPv4("10.0.0.1"); }

template <>
IPv4 OtherPeer<IPv4>::get() { return IPv4("192.168.0.1"); }

template <>
IPv6 DefaultPeer<IPv6>::get() { return IPv6("10::1"); }

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
	_ports.push_back(new SpoofPort<A>(*this, OtherPeer<A>::get()));
    }

    ~SpoofPortManager()
    {
	while (!_ports.empty()) {
	    delete _ports.front();
	    _ports.pop_front();
	}
    }

    Port<A>* test_port()
    {
	return _ports.front();
    }

    const Port<A>* test_port() const
    {
	return _ports.front();
    }

    Port<A>* other_port()
    {
	return _ports.back();
    }

    const Port<A>* other_port() const
    {
	return _ports.back();
    }

    Peer<A>* test_peer()
    {
	return test_port()->peers().front();
    }

    const Peer<A>* test_peer() const
    {
	return test_port()->peers().front();
    }

    Peer<A>* other_peer()
    {
	return other_port()->peers().front();
    }

    const Peer<A>* other_peer() const
    {
	return other_port()->peers().front();
    }
};


//----------------------------------------------------------------------------
// Horizon checkers

template <typename A>
class HorizonValidatorBase
{
public:
    HorizonValidatorBase(const set<IPNet<A> >& tpn, const set<IPNet<A> >& opn)
	: _total_routes_seen(0), _test_peer_routes_seen(0),
	  _tpn(tpn), _opn(opn)
    {}

    /* Check an individual response packet is valid */
    virtual bool valid_response(const RipPacket<A>* p) = 0;

    /* Final check responses valid */
    virtual bool valid_in_sum() const = 0;

    inline uint32_t total_routes_seen() const
    {
	return _total_routes_seen;
    }

    inline uint32_t test_peer_routes_seen() const
    {
	return _test_peer_routes_seen;
    }

protected:
    uint32_t _total_routes_seen;
    uint32_t _test_peer_routes_seen;
    const set<IPNet<A> >& _tpn;
    const set<IPNet<A> >& _opn;
};

template <typename A>
class NoHorizonValidator : public HorizonValidatorBase<A> {
public:
    NoHorizonValidator(const set<IPNet<A> >& tpn, const set<IPNet<A> >& opn)
	: HorizonValidatorBase<A>(tpn, opn)
    {}

    bool valid_response(const RipPacket<A>* p)
    {
	for (uint32_t i = 0; i < p->max_entries(); i++) {
	    _total_routes_seen++;

	    if (_tpn.find(p->route_entry(i)->net()) != _tpn.end()) {
		_test_peer_routes_seen++;
	    } else if (_opn.find(p->route_entry(i)->net()) != _opn.end()) {
		// No-op
	    } else {
		// Not a test peer net and not an other peer net
		// ==> it's bogus
		verbose_log("Failed Processing entry %d / %d %s cost %d\n",
			    i, p->max_entries(),
			    p->route_entry(i)->net().str().c_str(),
			    p->route_entry(i)->metric());
		return false;
	    }
	}
	return true;
    }

    bool valid_in_sum() const
    {
	if (test_peer_routes_seen() != _tpn.size()) {
	    verbose_log("Test routes seen (%d) does not match expected (%d)\n",
			test_peer_routes_seen(), (int32_t)_tpn.size());
	    return false;
	}
	verbose_log("total routes seen %d, test peer routes seen = %d\n",
		    total_routes_seen(), test_peer_routes_seen());
	return test_peer_routes_seen() * 2 == total_routes_seen();
    }
};

template <typename A>
class SplitHorizonValidator : public HorizonValidatorBase<A> {
public:
    SplitHorizonValidator(const set<IPNet<A> >& tpn, const set<IPNet<A> >& opn)
	: HorizonValidatorBase<A>(tpn, opn)
    {}

    bool valid_response(const RipPacket<A>* p)
    {
	for (uint32_t i = 0; i < p->max_entries(); i++) {
	    _total_routes_seen++;
	    if (_opn.find(p->route_entry(i)->net()) == _opn.end()) {
		verbose_log("Saw own or alien route with split horizon\n");
		// ==> it's bogus
		verbose_log("Failed Processing entry %d / %d %s cost %d\n",
			    i, p->max_entries(),
			    p->route_entry(i)->net().str().c_str(),
			    p->route_entry(i)->metric());
		return false;
	    }
	}
	return true;
    }

    bool valid_in_sum() const
    {
	if (test_peer_routes_seen() != 0) {
	    verbose_log("Test peer routes seen (%d) does not match expected "
			"(%d)\n", test_peer_routes_seen(), 0);
	    return false;
	}
	verbose_log("total routes seen %d, test peer routes seen = %d\n",
		    total_routes_seen(), test_peer_routes_seen());
	return total_routes_seen() == (uint32_t)_opn.size();
    }
};

template <typename A>
class PoisonReverseValidator : public HorizonValidatorBase<A> {
public:
    PoisonReverseValidator(const set<IPNet<A> >& tpn,
			   const set<IPNet<A> >& opn)
	: HorizonValidatorBase<A>(tpn, opn)
    {}

    bool valid_response(const RipPacket<A>* p)
    {
	for (uint32_t i = 0; i < p->max_entries(); i++) {
	    _total_routes_seen++;

	    if (_tpn.find(p->route_entry(i)->net()) != _tpn.end() &&
		p->route_entry(i)->metric() == RIP_INFINITY) {
		_test_peer_routes_seen++;
	    } else if (_opn.find(p->route_entry(i)->net()) != _opn.end()) {
		// No-op
	    } else {
		// Not a test peer net and not an other peer net
		// ==> it's bogus
		verbose_log("Failed Processing entry %d / %d %s cost %d\n",
			    i, p->max_entries(),
			    p->route_entry(i)->net().str().c_str(),
			    p->route_entry(i)->metric());
		return false;
	    }
	}
	return true;
    }

    bool valid_in_sum() const
    {
	if (test_peer_routes_seen() != _tpn.size()) {
	    verbose_log("Test routes seen (%d) does not match expected (%d)\n",
			test_peer_routes_seen(), (int32_t)_tpn.size());
	    return false;
	}
	verbose_log("total routes seen %d, test peer routes seen = %d\n",
		    total_routes_seen(), test_peer_routes_seen());
	return test_peer_routes_seen() * 2 == total_routes_seen();
    }
};


// ----------------------------------------------------------------------------
// OutputTester
//
// This is a bit nasty, the OutputClass is either OutputUpdates or OutputTable
// class.  These classes have the same methods and so it seems a waste to
// write this code out twice.  OutputClass is only referenced in one location
// so it's not rocket science to comprehend this.
//
template <typename A, typename OutputClass>
class OutputTester
{
public:
    OutputTester(const set<IPNet<A> >& test_peer_nets,
			const set<IPNet<A> >& other_peer_nets)
	: _e(), _rip_system(_e), _pm(_rip_system),
	  _tpn(test_peer_nets), _opn(other_peer_nets)
    {
	_pm.test_port()->constants().set_expiry_secs(10);
	_pm.test_port()->constants().set_deletion_secs(5);
	_pm.test_port()->set_advertise_default_route(false);

	_pm.other_port()->constants().set_expiry_secs(10);
	_pm.other_port()->constants().set_deletion_secs(5);

	// IPv4 specific despite being in a template
	_pm.test_port()->af_state().set_auth_handler(new NullAuthHandler());
	_pm.other_port()->af_state().set_auth_handler(new NullAuthHandler());

	_pm.test_port()->set_io_handler(new BlockedPortIO<A>(*_pm.test_port()),
					true);
	_pm.other_port()->set_io_handler(
			new BlockedPortIO<A>(*_pm.other_port()), true);
    }

    ~OutputTester()
    {
	RouteDB<A>& rdb = _rip_system.route_db();
	rdb.flush_routes();
	delete _pm.test_port()->af_state().auth_handler();
	delete _pm.other_port()->af_state().auth_handler();
    }

    int run_test(RipHorizon horizon, HorizonValidatorBase<A>& validator)
    {
	_pm.test_port()->set_horizon(horizon);

	RouteDB<A>&    rdb = _rip_system.route_db();

	PacketQueue<A> op_out;				      // Output pkt qu.
	OutputClass    ou(_e, *_pm.test_port(), op_out, rdb); // Output pkt gen

	verbose_log("Injecting routes from test peer.\n");
	for (typename set<IPNet<A> >::const_iterator n = _tpn.begin();
	     n != _tpn.end(); n++) {
	    RouteEntryOrigin<A>* reo = _pm.test_peer();
	    if (rdb.update_route(*n, A::ZERO(), 5u, 0u, reo) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return 1;
	    }
	}

	verbose_log("Injecting routes from other peer.\n");
	for (typename set<IPNet<A> >::const_iterator n = _opn.begin();
	     n != _opn.end(); n++) {
	    RouteEntryOrigin<A>* reo = _pm.other_peer();
	    if (rdb.update_route(*n, A::ZERO(), 5u, 0u, reo) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return 1;
	    }
	}

	bool timeout = false;
	XorpTimer tot = _e.set_flag_after_ms(3000, &timeout);
	ou.run();

	while (ou.running() && timeout == false) {
	    _e.run();
	}
	verbose_log("%d bytes buffered in packet queue.\n",
		    op_out.buffered_bytes());
	if (timeout) {
	    verbose_log("Timed out!\n");
	    return 1;
	}

	uint32_t cnt = 0;
	while (op_out.empty() == false) {
	    if (validator.valid_response(op_out.head()) == false) {
		verbose_log("Failed on packet validation.\n");
		return 1;
	    }
	    op_out.pop_head();
	    cnt++;
	}

	if (validator.valid_in_sum() == false) {
	    verbose_log("Not valid in sum.\n");
	    return 1;
	}
	return 0;
    }

protected:
    EventLoop		_e;
    System<A>		_rip_system;
    SpoofPortManager<A> _pm;
    const set<IPNet<A> >& _tpn;
    const set<IPNet<A> >& _opn;
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


// ----------------------------------------------------------------------------
// Injected Network state

static void
make_nets(const IPv4Net& base, uint32_t n, set<IPv4Net>& nets)
{
    IPv4Net ipn = base;
    while (n > 0) {
	nets.insert(ipn);
	++ipn;
	--n;
    }
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
	static const uint32_t n_routes = 577;
	set<IPNet<IPv4> > tpn;	// networks associated with peer under test
	set<IPNet<IPv4> > opn;	// networks associated with other peer.

	make_nets(IPNet<IPv4>(DefaultPeer<IPv4>::get(), 16), n_routes, tpn);
	make_nets(IPNet<IPv4>(OtherPeer<IPv4>::get(), 16), n_routes, opn);

	//
	// OutputUpdates class tests
	//
	{
	    verbose_log("=== No Horizon updates test ===\n");
	    OutputTester<IPv4, OutputUpdates<IPv4> > tester(tpn, opn);
	    NoHorizonValidator<IPv4> nohv(tpn, opn);
	    rval |= tester.run_test(NONE, nohv);
	}
	{
	    verbose_log("=== Split Horizon updates test ===\n");
	    OutputTester<IPv4, OutputUpdates<IPv4> > tester(tpn, opn);
	    SplitHorizonValidator<IPv4> shv(tpn, opn);
	    rval |= tester.run_test(SPLIT, shv);
	}
	{
	    verbose_log("=== Split Horizon Poison Reverse updates test ===\n");
	    OutputTester<IPv4, OutputUpdates<IPv4> > tester(tpn, opn);
	    PoisonReverseValidator<IPv4> prv(tpn, opn);
	    rval |= tester.run_test(SPLIT_POISON_REVERSE, prv);
	}

	//
	// OutputTable class tests
	//
	{
	    verbose_log("=== No Horizon table test ===\n");
	    OutputTester<IPv4, OutputTable<IPv4> > tester(tpn, opn);
	    NoHorizonValidator<IPv4> nohv(tpn, opn);
	    rval |= tester.run_test(NONE, nohv);
	}
	{
	    verbose_log("=== Split Horizon table test ===\n");
	    OutputTester<IPv4, OutputTable<IPv4> > tester(tpn, opn);
	    SplitHorizonValidator<IPv4> shv(tpn, opn);
	    rval |= tester.run_test(SPLIT, shv);
	}
	{
	    verbose_log("=== Split Horizon Poison Reverse table test ===\n");
	    OutputTester<IPv4, OutputTable<IPv4> > tester(tpn, opn);
	    PoisonReverseValidator<IPv4> prv(tpn, opn);
	    rval |= tester.run_test(SPLIT_POISON_REVERSE, prv);
	}
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
