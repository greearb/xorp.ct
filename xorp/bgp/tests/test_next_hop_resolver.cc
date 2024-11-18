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



#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"

#include "test_next_hop_resolver.hh"
#include "next_hop_resolver.hh"
#include "route_table_nhlookup.hh"
#include "route_table_decision.hh"
#include "bgp.hh"


template<class A>
class DummyNhLookupTable : public NhLookupTable<A> {
public:
    DummyNhLookupTable(TestInfo& info, NextHopResolver<A> *nexthop_resolver) :
	NhLookupTable<A>("tablename", SAFI_UNICAST, nexthop_resolver, 0),
	_done(false), _info(info)
    {
    }

    void
    RIB_lookup_done(const A& /*nexthop*/,
		    const set <IPNet<A> >& /*nets*/,
		    bool /*lookup_succeeded*/)
    {
	DOUT(_info) << "Rib lookup done\n";
	_done = true;
    }

    bool
    done() {
	return _done;
    }

private:
    bool _done;
    TestInfo& _info;
};

template<class A>
class DummyDecisionTable : public DecisionTable<A> {
public:
    DummyDecisionTable(TestInfo& info, NextHopResolver<A>& nexthop_resolver) :
	DecisionTable<A>("tablename", SAFI_UNICAST, nexthop_resolver),
	_done(false), _info(info)
    {
    }

    void
    igp_nexthop_changed(const A& nexthop)
    {
	DOUT(_info) << "next hop changed " << nexthop.str() << endl;
	_done = true;
    }

    bool
    done()
    {
	return _done;
    }

    void
    clear()
    {
	_done = false;
    }
private:
    bool _done;
    TestInfo& _info;
};

template <class A>
class DummyNextHopResolver2 : public NextHopResolver<A> {
public:
    DummyNextHopResolver2(EventLoop& eventloop, BGPMain& bgp) :
	NextHopResolver<A>(0, eventloop, bgp)
    {
	// Must set a ribname to force RIB interactions.
	this->register_ribname("bogus");
    }
};

/**
 * Register interest in a nexthop
 */
template <class A>
bool
nhr_test1(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);

    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * Register interest in a nexthop multiple times.
 */
template <class A>
bool
nhr_test2(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Register interest in this nexthop.
    */
    for(int i = 0; i < reg; i++)
	nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Address not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Deregister interest.
    */
    for(int i = 0; i < reg; i++)
	nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * Register interest in a nexthop multiple times but wait for the
 * first request to resolve before making subsequent calls.
 */
template <class A>
bool
nhr_test3(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Address not in table?\n";
	return false;
    }


    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Register interest in this nexthop a bunch more times
    */
    for(int i = 0; i < reg; i++)
	nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Deregister interest.
    */
    for(int i = 0; i < reg + 1; i++)
	nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * Register interest in a nexthop then deregister before the response
 * from the RIB (pseudo).
 */
template <class A>
bool
nhr_test4(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Deregister interest before the response function is called.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** We have deregistered interest in this nexthop we should not get
    ** this callback.
    */
    if(nht.done()) {
	DOUT(info) << "Call to next hop table should not have occured\n";
	return false;
    }

    /*
    ** A lookup should fail now.
    */
    bool res;
    uint32_t met;
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * The normal sequence of events is:
 * 1) Next hop table registers interest in a nexthop.
 * 2) A query is sent to RIB.
 * 3) The RIB sends back a response.
 * 4) The next hop resolver notifies the next hop table that a
 * response is available.
 * 5) The decision process performs a lookup.
 *
 * It is possible between 4 and 5 that the RIB notifies us that the
 * answer given in 3 is now invalid.
 *
 * The next hop resolver handles this case by returing the old value
 * to the decision process and requesting the new value from the
 * RIB. When the new value arrives the decision process is notified.
 */
template <class A>
bool
nhr_test5(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyDecisionTable<A> dt(info, nhr);
    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Give the next hop resolver a pointer to the decision table.
    */
    nhr.add_decision(&dt);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** The nexthop should still be available.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    metric++;	// Change metric to defeat no change optimisation.
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** Verify that we called the decision process with the new
    ** metrics.
    */
    if(!dt.done()) {
	DOUT(info) << "Callback to decision table failed\n";
	return false;
    }

    /*
    ** The decision process would perform the lookup again.
    */
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * Register interest in a nexthop then deregister before the response
 * from the RIB.
 */
template <class A>
bool
nhr_test6(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Deregister interest immediately.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback did not get back to the next hop table.
    */
    if(nht.done()) {
	DOUT(info) << "Callback to next hop table?\n";
	return false;
    }

    /*
    ** A lookup should fail.
    */
    bool res;
    uint32_t met;
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * 1) Register interest in a nexthop.
 * 2) Get response from RIB
 * 3) Get the RIB to tell us that the nexthop is invalid.
 * 4) The next hop resolver automatically makes another request of the
 * RIB.
 * 5) Before the response gets back from the RIB deregister.
 */
template <class A>
bool
nhr_test7(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyDecisionTable<A> dt(info, nhr);
    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Give the next hop resolver a pointer to the decision table.
    */
    nhr.add_decision(&dt);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = true;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** Respond to the second request that should have been made.
    */
    metric++;	// Change metric to defeat no change optimisation.
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** Verify that we don't call back to the decision table.
    */
    if(dt.done()) {
	DOUT(info) << "Callback to decision table?\n";
	return false;
    }

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * 1) Register interest in a nexthop.
 * 2) Get response from RIB.
 * 3) Send a bunch of metrics have changed calls from the RIB.
 * 4) Verify that decision gets called with the new metrics.
 */
template <class A>
bool
nhr_test8(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyDecisionTable<A> dt(info, nhr);
    DummyNhLookupTable<A> nht(info, &nhr);

    /*
    ** Give the next hop resolver a pointer to the decision table.
    */
    nhr.add_decision(&dt);

    /*
    ** Register interest in this nexthop.
    */
    nhr.register_nexthop(nexthop, subnet, &nht);

    /*
    ** Get a handle to the rib request class so the response method
    ** can be called.
    */
    NextHopRibRequest<A> *next_hop_rib_request =
	nhr.get_next_hop_rib_request();

    /*
    ** This is the response that would typically come from the RIB.
    */
    bool resolves = false;
    uint32_t prefix_len = 16;
    uint32_t real_prefix_len = 16;
    A addr = nexthop.mask_by_prefix_len(prefix_len);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	DOUT(info) << "Callback to next hop table failed\n";
	return false;
    }

    /*
    ** Case 1: A nexthop has been marked invalid we re-register
    ** interest but the metrics have not changed so we don't get a
    ** callback to the decision table.
    */

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** The metrics haven't changed we shouldn't call back to the
    ** decision process.
    */
    if(dt.done()) {
	DOUT(info) << "Callback to decision table?\n";
	return false;
    }

    /*
    ** Make sure the lookup still works.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Case 2: A nexthop has been marked invalid we re-register
    ** interest. We will change the metric value but the route does
    ** not resolve so the metric value is irrelevant. We still should
    ** not get the callback.
    */

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    metric++;	// Change the metric
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** The metrics haven't changed we shouldn't call back to the
    ** decision process.
    */
    if(dt.done()) {
	DOUT(info) << "Callback to decision table?\n";
	return false;
    }

    /*
    ** Make sure the lookup still works.
    */
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Case 3: A nexthop has been marked invalid we re-register
    ** interest. This time make the next hop resolvable we should get
    ** the callback.
    */

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    resolves = true;
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** The next hop now resolves we expect a callback to the decison process.
    */
    if(!dt.done()) {
	DOUT(info) << "Callback to decision table failed\n";
	return false;
    }

    /*
    ** Make sure the lookup still works.
    */
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Case 4: A nexthop has been marked invalid we re-register
    ** interest. The next hop is already resolvable just change the
    ** metric.
    */

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix_len)) {
	DOUT(info) << "Marking address as invalid failed\n";
	return false;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    dt.clear();
    metric++;
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix_len,
						     &real_prefix_len,
						     &real_nexthop,
						     &metric,
						     nexthop,
						     comment);

    /*
    ** The next hop now resolves we expect a callback to the decison process.
    */
    if(!dt.done()) {
	DOUT(info) << "Callback to decision table failed\n";
	return false;
    }

    /*
    ** Make sure the lookup still works.
    */
    if(!nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop not in table?\n";
	return false;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	DOUT(info) << "Metrics did not match\n";
	return false;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	DOUT(info) << "Nexthop in table?\n";
	return false;
    }

    return true;
}

/**
 * On a loaded system many registrations and de-registrations may be
 * queued while waiting for the RIB to respond.
 *
 * In this test the RIB will never respond. These used to be a bug in
 * the next_hop_resolver that if a registration was made twice the
 * first deregistration would throw away all the state. The second
 * deregistration would then fail.
 */
template <class A>
bool
nhr_test9(TestInfo& info, A nexthop, A /*real_nexthop*/, IPNet<A> subnet,
	  int reg)
{
    DOUT(info) << "nexthop: " << nexthop.str() << endl;

    EventLoop eventloop;
    BGPMain bgp(eventloop);
    DummyNextHopResolver2<A> nhr = DummyNextHopResolver2<A>(eventloop, bgp);

    DummyNhLookupTable<A> nht(info, &nhr);

    nhr.register_nexthop(nexthop, subnet, &nht);
    nhr.register_nexthop(nexthop, subnet, &nht);
    nhr.deregister_nexthop(nexthop, subnet, &nht);
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** Register interest in this nexthop.
    */
    for(int i = 0; i < reg; i++)
	nhr.register_nexthop(nexthop, subnet, &nht);

    for(int i = 0; i < reg; i++)
	nhr.deregister_nexthop(nexthop, subnet, &nht);

    return true;
}

/*
** This function is never called it exists to instantiate the
** templatised functions.
*/
void
dummy()
{
    XLOG_FATAL("Not to be called");

    callback(nhr_test1<IPv4>);
    callback(nhr_test1<IPv6>);

    callback(nhr_test2<IPv4>);
    callback(nhr_test2<IPv6>);

    callback(nhr_test3<IPv4>);
    callback(nhr_test3<IPv6>);

    callback(nhr_test4<IPv4>);
    callback(nhr_test4<IPv6>);

    callback(nhr_test5<IPv4>);
    callback(nhr_test5<IPv6>);

    callback(nhr_test6<IPv4>);
    callback(nhr_test6<IPv6>);

    callback(nhr_test7<IPv4>);
    callback(nhr_test7<IPv6>);

    callback(nhr_test8<IPv4>);
    callback(nhr_test8<IPv6>);

    callback(nhr_test9<IPv4>);
    callback(nhr_test9<IPv6>);
}

template bool nhr_test1<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test1<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test2<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet, int reg);
template bool nhr_test2<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet, int reg);
template bool nhr_test3<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet, int reg);
template bool nhr_test3<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet, int reg);
template bool nhr_test4<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test4<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test5<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test5<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test6<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test6<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test7<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test7<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test8<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet);
template bool nhr_test8<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet);
template bool nhr_test9<IPv4>(TestInfo&, IPv4 nexthop, IPv4, IPNet<IPv4> subnet, int reg);
template bool nhr_test9<IPv6>(TestInfo&, IPv6 nexthop, IPv6, IPNet<IPv6> subnet, int reg);

