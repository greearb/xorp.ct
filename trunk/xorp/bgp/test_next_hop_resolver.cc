// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/test_next_hop_resolver.cc,v 1.1 2002/12/18 03:06:06 atanu Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"

#include "next_hop_resolver.hh"
#include "route_table_nhlookup.hh"
#include "route_table_decision.hh"

#define DOUT(info)	info.out() << __FUNCTION__ << ":" << \
				      __LINE__ << ":" \
				               << info.test_name() << ": "

template<class A>
class DummyNhLookupTable : public NhLookupTable<A> {
public:
    DummyNhLookupTable(TestInfo& info, NextHopResolver<A> *nexthop_resolver) :
	NhLookupTable<A>("tablename", nexthop_resolver, 0),
	_done(false), _info(info)
    {
    }

    void
    RIB_lookup_done(const A& /*nexthop*/, 
		    const set <IPNet<A> >& /*nets*/,
		    bool /*lookup_succeeded*/)
    {
	if(_info.verbose())
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
	DecisionTable<A>("tablename", nexthop_resolver),
	_done(false), _info(info)
    {
    }

    void
    igp_nexthop_changed(const A& nexthop)
    {
	if(_info.verbose())
	    DOUT(_info) << "next hop changed " << nexthop.str() << endl;
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

template <class A>
class DummyNextHopResolver : public NextHopResolver<A> {
public:
    DummyNextHopResolver() :
	NextHopResolver<A>(0)	
    {
	// Must set a ribname to force RIB interactions.
	register_ribname("bogus");	
    }
};


/**
 * Register interest in a nexthop 
 */
template <class A>
int
test1(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    if(info.verbose())
	DOUT(info) << "nexthop: " << nexthop.str() << endl;

    DummyNextHopResolver<A> nhr = DummyNextHopResolver<A>();

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
    uint32_t prefix = 16;
    uint32_t real_prefix = 16;
    A addr = nexthop.mask_by_prefix(prefix);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	if(info.verbose())
	    DOUT(info) << "Callback to next hop table failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop not in table?\n";
	return TestMain::FAILURE;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	if(info.verbose())
	    DOUT(info) << "Metrics did not match\n";
	return TestMain::FAILURE;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop in table?\n";
	return TestMain::FAILURE;
    }

    return TestMain::SUCCESS;
}

/**
 * Register interest in a nexthop multiple times.
 */
template <class A>
int
test2(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg)
{
    if(info.verbose())
	DOUT(info) << "nexthop: " << nexthop.str() << endl;

    DummyNextHopResolver<A> nhr = DummyNextHopResolver<A>();

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
    uint32_t prefix = 16;
    uint32_t real_prefix = 16;
    A addr = nexthop.mask_by_prefix(prefix);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	if(info.verbose())
	    DOUT(info) << "Callback to next hop table failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Address not in table?\n";
	return TestMain::FAILURE;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	if(info.verbose())
	    DOUT(info) << "Metrics did not match\n";
	return TestMain::FAILURE;
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
	if(info.verbose())
	    DOUT(info) << "Nexthop in table?\n";
	return TestMain::FAILURE;
    }

    return TestMain::SUCCESS;
}

/**
 * Register interest in a nexthop multiple times but wait for the
 * first request to resolve before making subsequent calls.
 */
template <class A>
int
test3(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg)
{
    if(info.verbose())
	DOUT(info) << "nexthop: " << nexthop.str() << endl;

    DummyNextHopResolver<A> nhr = DummyNextHopResolver<A>();

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
    uint32_t prefix = 16;
    uint32_t real_prefix = 16;
    A addr = nexthop.mask_by_prefix(prefix);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	if(info.verbose())
	    DOUT(info) << "Callback to next hop table failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** The nexthop should now be resolvable.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Address not in table?\n";
	return TestMain::FAILURE;
    }


    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	if(info.verbose())
	    DOUT(info) << "Metrics did not match\n";
	return TestMain::FAILURE;
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
	if(info.verbose())
	    DOUT(info) << "Nexthop in table?\n";
	return TestMain::FAILURE;
    }

    return TestMain::SUCCESS;
}

/**
 * Register interest in a nexthop then deregister before the response
 * from the RIB (pseudo).
 */
template <class A>
int
test4(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    if(info.verbose())
	DOUT(info) << "nexthop: " << nexthop.str() << endl;

    DummyNextHopResolver<A> nhr = DummyNextHopResolver<A>();

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
    uint32_t prefix = 16;
    uint32_t real_prefix = 16;
    A addr = nexthop.mask_by_prefix(prefix);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);
    /*
    ** We have deregistered interest in this nexthop we should not get
    ** this callback.
    */
    if(nht.done()) {
	if(info.verbose())
	    DOUT(info) << "Call to next hop table should not have occured\n";
	return TestMain::FAILURE;
    }

    /*
    ** A lookup should fail now.
    */
    bool res;
    uint32_t met;
    if(nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop in table?\n";
	return TestMain::FAILURE;
    }

    return TestMain::SUCCESS;
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
int
test5(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet)
{
    if(info.verbose())
	DOUT(info) << "nexthop: " << nexthop.str() << endl;

    DummyNextHopResolver<A> nhr = DummyNextHopResolver<A>();

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
    uint32_t prefix = 16;
    uint32_t real_prefix = 16;
    A addr = nexthop.mask_by_prefix(prefix);
    uint32_t metric = 1;
    string comment = "testing";

    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);
    /*
    ** Verify that the callback went all the way to the next hop
    ** table. This must be true before a lookup will succeed.
    */
    if(!nht.done()) {
	if(info.verbose())
	    DOUT(info) << "Callback to next hop table failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** Simulate the RIB calling us back and telling us this sucker is
    ** invalid.
    */
    if(!nhr.rib_client_route_info_invalid(addr, prefix)) {
	if(info.verbose())
	    DOUT(info) << "Marking address as invalid failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** The nexthop should still be available.
    */
    bool res;
    uint32_t met;
    if(!nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop not in table?\n";
	return TestMain::FAILURE;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	if(info.verbose())
	    DOUT(info) << "Metrics did not match\n";
	return TestMain::FAILURE;
    }

    /*
    ** Respond to the second request that should have been made.
    */
    metric++;	// Change metric to defeat no change optimisation.
    next_hop_rib_request->register_interest_response(XrlError::OKAY(),
						     &resolves,
						     &addr,
						     &prefix,
						     &real_prefix,
						     &real_nexthop,
						     &metric,
						     comment);

    /*
    ** Verify that we called the decision process with the new
    ** metrics.
    */
    if(!dt.done()) {
	if(info.verbose())
	    DOUT(info) << "Callback to decision table failed\n";
	return TestMain::FAILURE;
    }

    /*
    ** The decision process would perform the lookup again.
    */
    if(!nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop not in table?\n";
	return TestMain::FAILURE;
    }

    /*
    ** Make sure that the metrics match the ones that we returned to
    ** the response function.
    */
    if(resolves != res || metric != met) {
	if(info.verbose())
	    DOUT(info) << "Metrics did not match\n";
	return TestMain::FAILURE;
    }

    /*
    ** Deregister interest.
    */
    nhr.deregister_nexthop(nexthop, subnet, &nht);

    /*
    ** A lookup should fail now.
    */
    if(nhr.lookup(nexthop, res, met)) {
	if(info.verbose())
	    DOUT(info) << "Nexthop in table?\n";
	return TestMain::FAILURE;
    }

    return TestMain::SUCCESS;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test_name =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    try {
	struct test {
	    string test_name;
	    XorpCallback1<int, TestInfo&>::RefPtr cb;
	} tests[] = {
	    {"test1", callback(test1<IPv4>, IPv4("128.16.64.1"), 
			       IPv4("1.1.1.1"), IPv4Net("22.0.0.0/8"))},
	    {"test1.ipv6", callback(test1<IPv6>, IPv6("::128.16.64.1"), 
				    IPv6("::128.16.64.1"),
				    IPv6Net("::22.0.0.0/8"))},
	    {"test2", callback(test2<IPv4>, IPv4("128.16.64.1"), 
			       IPv4("1.1.1.1"), IPv4Net("22.0.0.0/8"), 1000)},
	    {"test2.ipv6", callback(test2<IPv6>, IPv6("::128.16.64.1"), 
				    IPv6("::128.16.64.1"),
				    IPv6Net("::22.0.0.0/8"), 1000)},
	    {"test3", callback(test3<IPv4>, IPv4("128.16.64.1"), 
			       IPv4("1.1.1.1"), IPv4Net("22.0.0.0/8"), 1000)},
	    {"test3.ipv6", callback(test3<IPv6>, IPv6("::128.16.64.1"), 
				    IPv6("::128.16.64.1"),
				    IPv6Net("::22.0.0.0/8"), 1000)},
	    {"test4", callback(test4<IPv4>, IPv4("128.16.64.1"), 
			       IPv4("1.1.1.1"), IPv4Net("22.0.0.0/8"))},
	    {"test4.ipv6", callback(test4<IPv6>, IPv6("::128.16.64.1"), 
				    IPv6("::128.16.64.1"),
				    IPv6Net("::22.0.0.0/8"))},
	    {"test5", callback(test5<IPv4>, IPv4("128.16.64.1"), 
			       IPv4("1.1.1.1"), IPv4Net("22.0.0.0/8"))},
	    {"test5.ipv6", callback(test5<IPv6>, IPv6("::128.16.64.1"), 
				    IPv6("::128.16.64.1"),
				    IPv6Net("::22.0.0.0/8"))},
	};

	if("" == test_name) {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		if(test_name == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test_name + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    return t.exit();
}
