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

#ident "$XORP: xorp/bgp/harness/test_trie.cc,v 1.8 2003/09/11 10:44:41 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "bgp/bgp_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/test_main.hh"
#include "trie.hh"

#include <map>
#include <vector>

template <class A>
struct ltstr {
    bool operator()(IPNet<A> s1, IPNet<A> s2) const {
// 	printf("ltstr: %s %s %d\n",
// 	       s1.str().c_str(), s2.str().c_str(), s1 < s2);
	return s1 < s2;
    }
};

template <class A>
class Nmap {
public:
//     typedef map<IPNet<A>, A> nmap;
    typedef map<IPNet<A>, A, ltstr<A> > nmap;
};

typedef vector<const UpdatePacket *> Ulist;

/*
** Walk the trie and verify that all the networks in the trie are the expected.
**
** XXX - We should also check that the nexthops are good. They are in
** the map after all.
*/
template <class A>
void
tree_walker(const UpdatePacket *p, const IPNet<A>& net, const TimeVal&,
	    TestInfo info, typename Nmap<A>::nmap nlri)
{
    DOUT(info) << info.test_name() << endl <<
	p->str() <<
	"net: " << net.str() << "" << endl;

    typename Nmap<A>::nmap::const_iterator i;
    for(i = nlri.begin(); i != nlri.end(); i++) {
	DOUT(info) << info.test_name() << " " << (*i).first.str() << endl;
	if(net == (*i).first)
	    return;
    }

    XLOG_FATAL("Net: %s not found in map", net.str().c_str());
}

/*
** Used to verify that a trie is empty. If this function is called
** something is amiss.
*/
template <class A>
void
tree_walker_empty(const UpdatePacket *p, const IPNet<A>& net, const TimeVal&,
		  TestInfo info)
{
    DOUT(info) << info.test_name() << endl <<
	p->str() <<
	"net: " << net.str() << "" << endl;

    XLOG_FATAL("There should be no entries in this trie\n %s:%s",
	       p->str().c_str(), net.str().c_str());
}

template <class A>
void
replay_walker(const UpdatePacket *p, const TimeVal&, TestInfo info,
	      size_t *pos, Ulist ulist)
{
    DOUT(info) << info.test_name() << " " << (*pos) << endl << p->str();

    if(ulist.size() <= *pos)
	XLOG_FATAL("vector limit exceeded: %d %d", ulist.size(), *pos);

    if(*p != *ulist[*pos])
	XLOG_FATAL("%s NOT EQUAL TO %s", p->str().c_str(),
		   ulist[*pos]->str().c_str());

    (*pos)++;
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
    ** Verify that the trie is empty.
    */
    trie.tree_walk_table(callback(tree_walker_empty<A>, info));

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
    ** Add an origin, aspath and next hop to keep it legal.
    */
    bgpupdate->add_pathatt(OriginAttribute(IGP));
    bgpupdate->add_pathatt(ASPathAttribute(AsPath("1,2,3")));
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
    typename Nmap<A>::nmap nlri;
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
    ** Check that the net has been removed from the trie.
    */
    p = trie.lookup(net.str());
    if(0 != p) {
	DOUT(info) << "lookup suceeded in empty trie!!!\n" << p->str() << endl;
	return false;
    }

    delete [] data;
    delete bgpupdate;	// Free up the packet.
    
    /*
    ** Verify that the trie is empty.
    */
    trie.tree_walk_table(callback(tree_walker_empty<A>, info));

    return true;
}

template <class A>
bool
test_replay(TestInfo& info, A nexthop, IPNet<A> net)
{
    DOUT(info) << info.test_name() << endl;

    Trie trie;

    /*
    ** Verify that the trie is empty.
    */
    trie.tree_walk_table(callback(tree_walker_empty<A>, info));

    /*
    ** The trie should be empty make sure that a lookup fails.
    */
    const UpdatePacket *p = trie.lookup(net.str());
    if(0 != p) {
	DOUT(info) << "lookup suceeded in empty trie!!!\n" << p->str() << endl;
	return false;
    }

    UpdatePacket *packet_nlri, *packet_w1, *packet_w2;
    const uint8_t *data_nlri, *data_w1, *data_w2;
    size_t len_nlri, len_w1, len_w2;

    /*
    ** Create an update packet with two NLRIs.
    */
    packet_nlri = new UpdatePacket();
    add_nlri<A>(packet_nlri, net);
    IPNet<A> net2 = ++net;--net;
    add_nlri<A>(packet_nlri, net2);

    /*
    ** Add an origin, aspath and next hop to keep it legal.
    */
    packet_nlri->add_pathatt(OriginAttribute(IGP));
    packet_nlri->add_pathatt(ASPathAttribute(AsPath("1,2,3")));
    add_nexthop<A>(packet_nlri, nexthop);

    /*
    ** Pass to the trie.
    */
    TimeVal tv;
    data_nlri = packet_nlri->encode(len_nlri);
    trie.process_update_packet(tv, data_nlri, len_nlri);

    /*
    ** Verify that net is in the trie.
    */
    p = trie.lookup(net.str());

    if(0 == p) {
	DOUT(info) << "lookup of " << net.str() << " failed\n";
	return false;
    }

    if(*packet_nlri != *p) {
	DOUT(info) << endl << packet_nlri->str() << 
	    "NOT EQUAL TO\n" << p->str() << endl;
	return false;
    }

    /*
    ** Verify that net2 is in the trie.
    */
    p = trie.lookup(net2.str());

    if(0 == p) {
	DOUT(info) << "lookup of " << net2.str() << " failed\n";
	return false;
    }

    if(*packet_nlri != *p) {
	DOUT(info) << endl << packet_nlri->str() << 
	    "NOT EQUAL TO\n" << p->str() << endl;
	return false;
    }

    /*
    ** Walk the trie and check that both nets are there.
    */
    typename Nmap<A>::nmap nlri;
    nlri[net] = nexthop;
    nlri[net2] = nexthop;
    trie.tree_walk_table(callback(tree_walker<A>, info, nlri));

    /*
    ** Generate a withdraw to remove one entry from the trie.
    */
    packet_w1 = new UpdatePacket();
    withdraw_nlri<A>(packet_w1, net);
    data_w1 = packet_w1->encode(len_w1);
    trie.process_update_packet(tv, data_w1, len_w1);

    /*
    ** Check that net is no longer in the trie.
    */
    p = trie.lookup(net.str());
    if(0 != p) {
	DOUT(info) << "lookup suceeded in empty trie!!!\n" << p->str() << endl;
	return false;
    }

    /*
    ** Now check the replay code.
    */
    size_t pos = 0;
    Ulist ulist;
    ulist.push_back(packet_nlri);
    ulist.push_back(packet_w1);
    DOUT(info) << "0\t" << ulist[0]->str() << endl;
    DOUT(info) << "1\t" << ulist[1]->str() << endl;
    trie.replay_walk(callback(replay_walker<A>, info, &pos, ulist));

    /*
    ** Build another withdraw to remove the net2. This should empty
    ** the trie.
    */
    packet_w2 = new UpdatePacket();
    withdraw_nlri<A>(packet_w2, net2);
    data_w2 = packet_w2->encode(len_w2);
    trie.process_update_packet(tv, data_w2, len_w2);

    /* 
    ** The replay list should be empty.
    */
    pos = 0;
    ulist.clear();
    trie.replay_walk(callback(replay_walker<A>, info, &pos, ulist));

    /*
    ** Push the update packet with the two nlris back in.
    */
    trie.process_update_packet(tv, data_nlri, len_nlri);

    /*
    ** Check the replay code. It should now only contain the most
    ** recent  update.
    */
    pos = 0;
    ulist.clear();
    ulist.push_back(packet_nlri);
    DOUT(info) << "0\t" << ulist[0]->str() << endl;
    trie.replay_walk(callback(replay_walker<A>, info, &pos, ulist));

    /*
    ** Empty out the trie.
    */
    trie.process_update_packet(tv, data_w1, len_w1);
    trie.process_update_packet(tv, data_w2, len_w2);

    /*
    ** The replay list should be empty.
    */
    pos = 0;
    ulist.clear();
    trie.replay_walk(callback(replay_walker<A>, info, &pos, ulist));

    /*
    ** Verify that the trie is empty.
    */
    trie.tree_walk_table(callback(tree_walker_empty<A>, info));

    delete [] data_nlri;
    delete packet_nlri;
    delete [] data_w1;
    delete packet_w1;
    delete [] data_w2;
    delete packet_w2;

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
	    {"replay_ipv4", callback(test_replay<IPv4>,
				     nexthop1_ipv4, net1_ipv4)},
	    {"replay_ipv6", callback(test_replay<IPv6>,
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

