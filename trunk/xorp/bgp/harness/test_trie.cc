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

#ident "$XORP: xorp/bgp/harness/test_trie.cc,v 1.5 2003/09/11 03:42:26 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "bgp/bgp_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"
#include "trie.hh"

#include <map>

/*
** 
*/
template <class A>
void
tree_walker(const UpdatePacket *p, const IPNet<A>& net, const TimeVal&,
	    TestInfo info, map<IPNet<A>, A> nlri)
{
    DOUT(info) << info.test_name() << endl <<
	p->str() <<
	"net: " << net.str() << "" << endl;

    typename map<IPNet<A>, A>::const_iterator i;
    for(i = nlri.begin(); i != nlri.end(); i++)
	if(net == (*i).first)
	    return;

    XLOG_FATAL("Net: %s not found in map", net.str().c_str());
}

template <class A> void add_nlri(UpdatePacket *p, IPNet<A> net);
template <class A> void withdraw_nlri(UpdatePacket *p, IPNet<A> net);
template <class A> void add_nexthop(UpdatePacket *p, A nexthop);

template <>
void
add_nlri<IPv4>(UpdatePacket *p, IPNet<IPv4> net)
{
    p->add_nlri(BGPUpdateAttrib(net));
}

template <>
void
withdraw_nlri<IPv4>(UpdatePacket *p, IPNet<IPv4> net)
{
    p->add_withdrawn(BGPUpdateAttrib(net));
}

template <>
void
add_nexthop<IPv4>(UpdatePacket *p, IPv4 nexthop)
{
    p->add_pathatt(IPv4NextHopAttribute(nexthop));
}

template <>
void
add_nlri<IPv6>(UpdatePacket *p, IPNet<IPv6> net)
{
    debug_msg("%s <%s>\n", p->str().c_str(), net.str().c_str());
    
    /*
    ** Look for a multiprotocol path attribute, if there is one
    ** present just add the net. Otherwise add a multiprotocol path
    ** attribute and then add the net. Note: if we add the path
    ** attribute we need operate on a pointer hence the goto.
    */
 top:
    MPReachNLRIAttribute<IPv6> *mpreach = 0;
    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai)) {
 	    mpreach = dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai);
	    mpreach->add_nlri(net);
	    mpreach->encode();

	    debug_msg("%s\n", p->str().c_str());
	    return;
	}
    }

    if(0 == mpreach) {
	MPReachNLRIAttribute<IPv6> mp;
	p->add_pathatt(mp);
	goto top;
    }
}

template <>
void
withdraw_nlri<IPv6>(UpdatePacket *p, IPNet<IPv6> net)
{
    debug_msg("%s <%s>\n", p->str().c_str(), net.str().c_str());

    /*
    ** Look for a multiprotocol path attribute, if there is one
    ** present just add the net. Otherwise add a multiprotocol path
    ** attribute and then add the net. Note: if we add the path
    ** attribute we need operate on a pointer hence the goto.
    */
 top:
    MPUNReachNLRIAttribute<IPv6> *mpunreach = 0;
    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(*pai)) {
 	    mpunreach = dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(*pai);
	    mpunreach->add_withdrawn(net);
	    mpunreach->encode();

	    debug_msg("%s\n", p->str().c_str());
	    return;
	}
    }

    if(0 == mpunreach) {
	MPUNReachNLRIAttribute<IPv6> mp;
	p->add_pathatt(mp);
	goto top;
    }
}

template <>
void
add_nexthop<IPv6>(UpdatePacket *p, IPv6 nexthop)
{
    /*
    ** Look for a multiprotocol path attribute, if there is one
    ** present just add the net. Otherwise add a multiprotocol path
    ** attribute and then add the nexthop. Note: if we add the path
    ** attribute we need operate on a pointer hence the goto.
    */
 top:
    MPReachNLRIAttribute<IPv6> *mpreach = 0;
    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai)) {
 	    mpreach = dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai);
	    mpreach->set_nexthop(nexthop);
	    mpreach->encode();

	    debug_msg("%s\n", p->str().c_str());
	    return;
	}
    }

    if(0 == mpreach) {
	MPReachNLRIAttribute<IPv6> mp;
	p->add_pathatt(mp);
	goto top;
    }
}

template <class A>
bool
test_single_update(TestInfo& info, A nexthop, IPNet<A> net)
{
    DOUT(info) << info.test_name() << endl;

    Trie trie;

    /*
    ** The trie should be empty make sure that a lookup fails.
    */
    const UpdatePacket *p = trie.lookup(net.str());
    if(0 != p) {
	DOUT(info) << "lookup suceeded in empty trie!!!\n" << p->str() << endl;
	return false;
    }

    /*
    ** Create an update packet with a single NLRI.
    */
    UpdatePacket *bgpupdate = new UpdatePacket();
    add_nlri<A>(bgpupdate, net);

    /*
    ** Add an origin and a next hop to keep it legal.
    */
    bgpupdate->add_pathatt(OriginAttribute(IGP));
    add_nexthop<A>(bgpupdate, nexthop);

    /*
    ** Pass to the trie.
    */
    TimeVal tv;
    size_t len;
    const uint8_t *data = bgpupdate->encode(len);
    trie.process_update_packet(tv, data, len);

    /*
    ** Verify that this net is in the trie.
    */
    p = trie.lookup(net.str());

    if(0 == p) {
	DOUT(info) << "lookup of " << net.str() << " failed\n";
	return false;
    }


    if(*bgpupdate != *p) {
	DOUT(info) << endl << bgpupdate->str() << 
	    "NOT EQUAL TO\n" << p->str() << endl;
	return false;
    }

    /*
    ** Walk the trie and check that the net is there.
    */
    map<IPNet<A>, A> nlri;
    nlri[net] = nexthop;
    trie.tree_walk_table(callback(tree_walker<A>, info, nlri));

    /*
    ** Generate a withdraw to remove the entry from the trie.
    */
    delete [] data;
    delete bgpupdate;
    bgpupdate = new UpdatePacket();
    withdraw_nlri<A>(bgpupdate, net);
    data = bgpupdate->encode(len);
    trie.process_update_packet(tv, data, len);

    /*
    ** Check that the trie is now empty.
    */
    p = trie.lookup(net.str());
    if(0 != p) {
	DOUT(info) << "lookup suceeded in empty trie!!!\n" << p->str() << endl;
	return false;
    }

    delete [] data;
    delete bgpupdate;	// Free up the packet.
    
    /*
    ** Print the trie.
    */
//     printf("printing routing table:\n");
//     trie.save_routing_table(stdout);

    return true;
}

int
main(int argc, char** argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test_name =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();
     
    IPNet<IPv4> net1_ipv4("10.10.0.0/16");
    IPv4 nexthop1_ipv4("20.20.20.20");

    IPNet<IPv6> net1_ipv6("2000::/3");
    IPv6 nexthop1_ipv6("20:20:20:20:20:20:20:20");

    try {
	uint8_t edata[2];
	edata[0]=1;
	edata[1]=2;

	struct test {
	    string test_name;
	    XorpCallback1<bool, TestInfo&>::RefPtr cb;
	} tests[] = {
	    {"single_ipv4", callback(test_single_update<IPv4>,
				     nexthop1_ipv4, net1_ipv4)},
  	    {"single_ipv6", callback(test_single_update<IPv6>,
				     nexthop1_ipv6, net1_ipv6)},
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

