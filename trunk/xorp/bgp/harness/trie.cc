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

#ident "$XORP: xorp/bgp/harness/trie.cc,v 1.25 2008/11/08 06:14:45 mjh Exp $"

// #define DEBUG_LOGGING 
// #define DEBUG_PRINT_FUNCTION_NAME 
#define PARANOIA

#include "bgp/bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "trie.hh"


const
UpdatePacket *
Trie::lookup(const string& net) const
{
    IPvXNet n(net.c_str());

    if(n.is_ipv4())
	return lookup(n.get_ipv4net());
    else if(n.is_ipv6())
	return lookup(n.get_ipv6net());
    else
	XLOG_FATAL("Unknown family: %s", n.str().c_str());

    return 0;
}

const
UpdatePacket *
Trie::lookup(const IPv4Net& n) const
{
    TriePayload payload = _head_ipv4.find(n);
    const UpdatePacket *update = payload.get();

    if(0 == update)
	return 0;

    /*
    ** Everything below here is sanity checking.
    */

    /*
    ** Look for this net in the matching update packet.
    */
    list <BGPUpdateAttrib>::const_iterator ni;
    ni = update->nlri_list().begin();
    for(; ni != update->nlri_list().end(); ni++)
	if(ni->net() == n)
	    return update;
    
    XLOG_FATAL("If we found the packet in the trie"
	       " it should contain the nlri %s %s", n.str().c_str(),
	       update->str().c_str());

    return 0;
}

const
UpdatePacket *
Trie::lookup(const IPv6Net& n) const
{
    TriePayload payload = _head_ipv6.find(n);
    const UpdatePacket *update = payload.get();

    if(0 == update)
	return 0;

    /*
    ** Everything below here is sanity checking.
    */

    /*
    ** Look for a multiprotocol path attribute.
    */
    const MPReachNLRIAttribute<IPv6> *mpreach = 0;
#if 0
    list <PathAttribute*>::const_iterator pai;
    for (pai = update->pa_list().begin(); pai != update->pa_list().end();
	 pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai)) {
 	    mpreach = dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai);
	    break;
	}
    }
#endif
    mpreach = update->mpreach<IPv6>(SAFI_UNICAST);
    if (mpreach == 0)
	mpreach = update->mpreach<IPv6>(SAFI_MULTICAST);

    if(0 == mpreach)
	XLOG_FATAL("If we found the packet in the trie"
		   " it should contain the nlri %s %s", n.str().c_str(),
		   update->str().c_str());


    /*
    ** Look for this net in the matching update packet.
    */
    list<IPv6Net>::const_iterator ni;
    ni = mpreach->nlri_list().begin();
    for(; ni != mpreach->nlri_list().end(); ni++)
	if(*ni == n)
	    return update;

    XLOG_FATAL("If we found the packet in the trie"
	       " it should contain the nlri %s %s", n.str().c_str(),
	       update->str().c_str());

    return 0;
}

void
Trie::process_update_packet(const TimeVal& tv, const uint8_t *buf, size_t len,
			    const BGPPeerData *peerdata)
{
    _update_cnt++;

    TriePayload payload(tv, buf, len, peerdata, _first, _last);
    const UpdatePacket *p = payload.get();

    debug_msg("process update packet:\n%s\n", p->str().c_str());

    const MPReachNLRIAttribute<IPv6> *mpreach = 0;
    const MPUNReachNLRIAttribute<IPv6> *mpunreach = 0;

#if 0    
    list <PathAttribute*>::const_iterator pai;
    for (pai = p->pa_list().begin(); pai != p->pa_list().end(); pai++) {
	const PathAttribute* pa;
	pa = *pai;
	
	if (dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai)) {
 	    mpreach = dynamic_cast<MPReachNLRIAttribute<IPv6>*>(*pai);
	    continue;
	}
	
	if (dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(*pai)) {
	    mpunreach = dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(*pai);
	    continue;
	}
    }
#endif
    mpreach = p->mpreach<IPv6>(SAFI_UNICAST);
    if (!mpreach)
	mpreach = p->mpreach<IPv6>(SAFI_MULTICAST);
    mpunreach = p->mpunreach<IPv6>(SAFI_UNICAST);
    if (!mpunreach)
	mpunreach = p->mpunreach<IPv6>(SAFI_MULTICAST);

    /*
    ** IPv4 Withdraws
    */
    if (!p->wr_list().empty()) {
	BGPUpdateAttribList::const_iterator wi;
	for(wi = p->wr_list().begin(); wi != p->wr_list().end(); wi++)
	    del(wi->net(), payload);
    }

    /*
    ** IPv6 Withdraws
    */
    if (mpunreach) {
	list<IPNet<IPv6> >::const_iterator wi6;
	for(wi6 = mpunreach->wr_list().begin();
	    wi6 != mpunreach->wr_list().end(); wi6++) {
	    del(*wi6, payload);
	}
    }

    /*
    ** IPv4 Route add.
    */
    if (!p->nlri_list().empty()) {
	BGPUpdateAttribList::const_iterator ni4;
	for(ni4 = p->nlri_list().begin(); ni4 != p->nlri_list().end(); ni4++)
	    add(ni4->net(), payload);
    }

    /*
    ** IPv6 Route add.
    */
    if (mpreach) {
	list<IPNet<IPv6> >::const_iterator ni6;
	for(ni6 = mpreach->nlri_list().begin();
	    ni6 != mpreach->nlri_list().end(); ni6++)
	    add(*ni6, payload);
    }
}

void
Trie::tree_walk_table(const TreeWalker_ipv4& tw) const
{
    _head_ipv4.tree_walk_table(tw);
}

void
Trie::tree_walk_table(const TreeWalker_ipv6& tw) const
{
    _head_ipv6.tree_walk_table(tw);
}

void
Trie::replay_walk(const ReplayWalker uw, const BGPPeerData *peerdata) const
{
    /*
    ** It should be sufficient to just walk the list of stored
    ** packets. However it is possible that we have saved withdraws
    ** that in effect will be a noop. So insert the packets into a
    ** local trie, only if the packet changes the local trie do we
    ** propogate it back to the caller.
    */
    Trie trie;
    trie.set_warning(false);
    uint32_t changes = 0;

    for(const TrieData *p = _first; p; p = p->next()) {
	changes = trie.changes();
	UpdatePacket *packet = const_cast<UpdatePacket *>(p->data());
	uint8_t data[BGPPacket::MAXPACKETSIZE];
	size_t len = BGPPacket::MAXPACKETSIZE;
	debug_msg("Trie::replay_walk\n");
	packet->encode(data, len, peerdata);
	trie.process_update_packet(p->tv(), data, len, peerdata);
	if(trie.changes() != changes)
	    uw->dispatch(p->data(), p->tv());
    }
}

template <>
void
Trie::get_heads<IPv4>(RealTrie<IPv4>*& head, RealTrie<IPv4>*& del)
{
    head = &_head_ipv4;
    del = &_head_ipv4_del;
}

template <>
void
Trie::get_heads<IPv6>(RealTrie<IPv6>*& head, RealTrie<IPv6>*& del)
{
    head = &_head_ipv6;
    del = &_head_ipv6_del;
}

template <class A>
void
Trie::add(IPNet<A> net, TriePayload& payload)
{
    debug_msg("%s %s\n", net.str().c_str(), payload.get()->str().c_str());
    _changes++;

    RealTrie<A> *head, *del;
    get_heads<A>(head, del);

    /* If a previous entry exists remove it */
    const UpdatePacket *up = lookup(net);
    if(up)
	if(!head->del(net))
	    XLOG_FATAL("Could not remove nlri: %s", net.str().c_str());
    if(!head->insert(net, payload))
	XLOG_FATAL("Could not add nlri: %s", net.str().c_str());

    /*
    ** We save deleted state for replays. 
    */
    up = del->find(net).get();
    if(up)
	if(!del->del(net))
	    XLOG_FATAL("Could not remove nlri: %s", net.str().c_str());
}

template <class A>
void
Trie::del(IPNet<A> net, TriePayload& payload)
{
    RealTrie<A> *head, *del;
    get_heads<A>(head, del);

    if(!head->del(net)) {
	if(_warning)
	    XLOG_WARNING("Could not delete %s", net.str().c_str());
    } else {
	_changes++;
    }

    /*
    ** We save deleted state for replays. 
    */
    /* If a previous entry exists remove it */
    const UpdatePacket *up = del->find(net).get();
    if(up)
	if(!del->del(net))
	    XLOG_FATAL("Could not remove nlri: %s", net.str().c_str());
    if(!del->insert(net, payload))
	XLOG_FATAL("Could not add nlri: %s", net.str().c_str());
}
