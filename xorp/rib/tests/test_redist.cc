// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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





#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"

#include "route.hh"
#include "rib.hh"
#include "rt_tab_redist.hh"
#include "rt_tab_origin.hh"
#include "rt_tab_deletion.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif // HAVE_GETOPT_H

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char* program_name         = "test_redist";
static const char* program_description  = "Test Route Redistribution Components";
static const char* program_version_id   = "0.1";
static const char* program_date         = "April, 2004";
static const char* program_copyright    = "See file LICENSE";
static const char* program_return_value = "0 on success, 1 if test error, "
					  "2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static inline const char*
xorp_path(const char* path)
{
    const char* xorp_path = strstr(path, "xorp");
    if (xorp_path) {
	return xorp_path;
    }
    return path;
}

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", xorp_path(file), line);			\
	printf(x);							\
    }									\
} while(0)



template <typename A>
class TestOutput : public RedistOutput<A> {
public:
    typedef RedistOutput<A>			Base;
    typedef typename RedistTable<A>::RouteIndex	RouteIndex;

    static const IPNet<A>			ANY_NET;
    static const IPNet<A>			NO_NET;

public:
    /**
     * Contructor
     *
     * @param block_after_op announce high water after any route operations.
     * This allows state to be controlled in piecemeal manner.
     */
    TestOutput(Redistributor<A>* r, bool block_after_op = true)
	: Base(r), _blocking(block_after_op),
	  _blocked(false), _expect_net(ANY_NET)
    {}

    void unblock();
    bool blocked() const			{ return _blocked; }

    uint32_t backlog() const			{ return _blocked ? 1 : 0; }
    uint32_t low_water_backlog() const		{ return 0; }
    uint32_t high_water_backlog() const		{ return 1; }

    void add_route(const IPRouteEntry<A>& route);
    void delete_route(const IPRouteEntry<A>& route);

    void starting_route_dump()			{}
    void finishing_route_dump()			{}

    void set_expected_net(const IPNet<A>& n)	{ _expect_net = n; }
    const IPNet<A>& expected_net() const	{ return _expect_net; }

    const RouteIndex& route_index() const	{ return _rt_index; }

protected:
    bool		_blocking;
    bool		_blocked;
    IPNet<A>		_expect_net;	// Generate error if update recv when t
    RouteIndex		_rt_index;	// Nets associated with live routes
};

template <typename A>
const IPNet<A> TestOutput<A>::ANY_NET(A::ZERO(), 0);

template <>
const IPNet<IPv4> TestOutput<IPv4>::NO_NET(IPv4::ALL_ONES(),
					   IPv4::ADDR_BITLEN);
template <>
const IPNet<IPv6> TestOutput<IPv6>::NO_NET(IPv6::ALL_ONES(),
					   IPv6::ADDR_BITLEN);

template <typename A>
void
TestOutput<A>::unblock()
{
    _blocked = false;
    Base::announce_low_water();
}

template <typename A>
void
TestOutput<A>::add_route(const IPRouteEntry<A>& route)
{
    if (_expect_net != ANY_NET && _expect_net != route.net()) {
	XLOG_FATAL("Unexpected add_route call.  Expecting %s got %s",
		   _expect_net.str().c_str(),
		   route.net().str().c_str());
    }

    verbose_log("add_route %s\n", route.net().str().c_str());
    XLOG_ASSERT(_rt_index.find(route.net()) == _rt_index.end());

    _rt_index.insert(route.net());
    if (_blocking) {
	_blocked = true;
	Base::announce_high_water();
    }
}

template <typename A>
void
TestOutput<A>::delete_route(const IPRouteEntry<A>& route)
{
    if (_expect_net != ANY_NET && _expect_net != route.net()) {
	XLOG_FATAL("Unexpected delete_route call.  Expecting %s got %s",
		   _expect_net.str().c_str(),
		   route.net().str().c_str());
    }

    verbose_log("delete_route %s\n", route.net().str().c_str());

    typename RouteIndex::iterator i = _rt_index.find(route.net());
    XLOG_ASSERT(i != _rt_index.end());
    _rt_index.erase(i);

    if (_blocking) {
	_blocked = true;
	Base::announce_high_water();
    }
}

template <typename A>
static bool
unblock_output(TestOutput<A>* to)
{
    static TimeVal t0;
    TimeVal t1;

    if (t0 == TimeVal::ZERO()) {
	TimerList::system_gettimeofday(&t0);
	t1 = t0;
    } else {
	TimerList::system_gettimeofday(&t1);
    }

    TimeVal t = t1 - t0;
    verbose_log("Unblock output at %s\n", t.str().c_str());

    bool r = to->blocked();
    to->unblock();
    return r;
}

template <typename A>
static void
add_route_before_current(OriginTable<A>* 	ot,
			 TestOutput<A>*		to,
			 IPRouteEntry<A>*	ipr)

{
    verbose_log("add_route_before_current %s\n", ipr->net().str().c_str());
    IPNet<A> saved = to->expected_net();
    to->set_expected_net(ipr->net());
    ot->add_route(ipr);
    to->set_expected_net(saved);
}

template <typename A>
static void
add_route_after_current(OriginTable<A>* 	ot,
			TestOutput<A>*		to,
			IPRouteEntry<A>*	ipr)

{
    verbose_log("add_route_after_current %s\n", ipr->net().str().c_str());
    IPNet<A> saved = to->expected_net();
    to->set_expected_net(TestOutput<A>::NO_NET);
    ot->add_route(ipr);
    to->set_expected_net(saved);
}

template <typename A>
static void
delete_route_before_current(OriginTable<A>* 	ot,
			    TestOutput<A>*	to,
			    IPRouteEntry<A>	ipr)

{
    verbose_log("delete_route_before_current %s\n", ipr.net().str().c_str());
    IPNet<A> saved = to->expected_net();
    to->set_expected_net(ipr.net());
    ot->delete_route(ipr.net());
    to->set_expected_net(saved);
}

template <typename A>
static void
delete_route_after_current(OriginTable<A>* 	ot,
			   TestOutput<A>*	to,
			   IPRouteEntry<A>	ipr)

{
    verbose_log("delete_route_after_current %s\n", ipr.net().str().c_str());
    IPNet<A> saved = to->expected_net();
    to->set_expected_net(TestOutput<A>::NO_NET);
    ot->delete_route(ipr.net());
    to->set_expected_net(saved);
}

static int
test_deterministic()
{
    EventLoop e;

    // Create an OriginTable
    TypedOriginTable<IPv4, IGP> typed_origin("static", 1, e);
    OriginTable<IPv4>& origin = typed_origin;
    IPPeerNextHop<IPv4> nh("22.0.0.1");
    Protocol&	protocol = typed_origin.protocol();
    protocol.increment_genid();

    Vif			tmp_vif("vif0");
    RibVif<IPv4>		vif(NULL, tmp_vif);

    // Attach redist table
    RedistTable<IPv4> redist_table("StaticRedistTable", &origin);

    Redistributor<IPv4>* r = new Redistributor<IPv4>(e, "static_redist");
    r->set_redist_table(&redist_table);

    TestOutput<IPv4>* 	output = new TestOutput<IPv4>(r, true);
    r->set_output(output);

    // Add some initial routes
    origin.add_route(new IPRouteEntry<IPv4>("10.0.0.0/8",
					&vif, nh.get_copy(), &protocol, 10));
    origin.add_route(new IPRouteEntry<IPv4>("10.3.0.0/16",
					&vif, nh.get_copy(), &protocol, 10));
    origin.add_route(new IPRouteEntry<IPv4>("10.5.0.0/16",
					&vif, nh.get_copy(), &protocol, 10));
    origin.add_route(new IPRouteEntry<IPv4>("10.6.0.0/16",
					&vif, nh.get_copy(), &protocol, 10));
    origin.add_route(new IPRouteEntry<IPv4>("10.3.128.0/17",
					&vif, nh.get_copy(), &protocol, 10));
    origin.add_route(new IPRouteEntry<IPv4>("10.3.192.0/18",
					&vif, nh.get_copy(), &protocol, 10));

    verbose_log("RedistTable index size = %u\n",
		XORP_UINT_CAST(redist_table.route_index().size()));

    // Each route added causes the output block. This timer unblocks the output
    // once per second.
    XorpTimer u = e.new_periodic_ms(1000,
				    callback(&unblock_output<IPv4>, output));
    // After 1 second the last route added is 10.3.0.0
    //
    // Add one route after this
    XorpTimer a0 = e.new_oneoff_after_ms(1250,
			callback(&add_route_after_current<IPv4>,
				 &origin, output,
				 new IPRouteEntry<IPv4>("10.4.0.0/16", &vif, nh.get_copy(),
						    &protocol, 10)));
    // And two routes before
    XorpTimer a1 = e.new_oneoff_after_ms(1500,
			callback(&add_route_before_current<IPv4>,
				 &origin, output,
				 new IPRouteEntry<IPv4>("10.1.0.0/16", &vif, nh.get_copy(),
						    &protocol, 10)));

    XorpTimer a2 = e.new_oneoff_after_ms(1750,
			callback(&add_route_before_current<IPv4>,
				 &origin, output,
				 new IPRouteEntry<IPv4>("10.2.0.0/16", &vif, nh.get_copy(),
						    &protocol, 10)));

    // Delete first route
    XorpTimer d1 = e.new_oneoff_after_ms(2250,
			callback(&delete_route_before_current<IPv4>,
				 &origin, output,
				 IPRouteEntry<IPv4>("10.0.0.0/8", &vif, nh.get_copy(),
						    &protocol, 10)));
    // Delete current route
    XorpTimer d2 = e.new_oneoff_after_ms(2500,
			callback(&delete_route_before_current<IPv4>,
				 &origin, output,
				 IPRouteEntry<IPv4>("10.4.0.0/16", &vif, nh.get_copy(),
						    &protocol, 10)));
    // Delete last route
    XorpTimer d3 = e.new_oneoff_after_ms(2750,
			callback(&delete_route_before_current<IPv4>,
				 &origin, output,
				 IPRouteEntry<IPv4>("10.3.192.0/18", &vif, nh.get_copy(),
						    &protocol, 10)));

    bool done = false;
    XorpTimer t = e.set_flag_after_ms(10000, &done);
    while (done == false) {
	e.run();
    }

    // Dump should be finished by now
    XLOG_ASSERT(r->dumping() == false);

    // Expect indexed routes of output routes to match the RedistTable
    // index, ie they should all have been output during dump.
    XLOG_ASSERT(output->route_index() == redist_table.route_index());

    // Expect updates after dump to be propagated immediately.
    add_route_before_current<IPv4>(&origin, output,
				  new IPRouteEntry<IPv4>("1.1.0.0/9", &vif, nh.get_copy(),
						     &protocol, 10));

    add_route_before_current<IPv4>(&origin, output,
				   new IPRouteEntry<IPv4>("20.1.127.0/24", &vif, nh.get_copy(),
						      &protocol, 10));

    delete_route_before_current<IPv4>(&origin, output,
				      IPRouteEntry<IPv4>("20.1.127.0/24",
							 &vif, nh.get_copy(),
							 &protocol, 10));

    // Expect output route index to still match the redist table index.
    XLOG_ASSERT(output->route_index() == redist_table.route_index());
    XLOG_ASSERT(find(redist_table.route_index().begin(),
		     redist_table.route_index().end(),
		     IPv4Net("1.1.0.0/9"))
		!= redist_table.route_index().end());

    return 0;
}


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
    verbose_log("usage: %s [-v] [-h]\n", progname);
    verbose_log("       -h          : usage (this message)\n");
    verbose_log("       -v          : verbose output\n");
}

int
main(int argc, char* const argv[])
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

    //
    // Parse command line arguments
    //
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
	    if (ch == 'h')
		return 0;
	    else
		return 1;
	}
    }
    argc -= optind;
    argv += optind;

    //
    // Run test
    //
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	test_deterministic();
    } catch (...) {
	xorp_catch_standard_exceptions();
	return 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    return 0;
}
