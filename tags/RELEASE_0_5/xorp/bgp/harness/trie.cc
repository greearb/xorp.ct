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

#ident "$XORP: xorp/bgp/harness/trie.cc,v 1.11 2003/09/11 08:19:15 atanu Exp $"

// #define DEBUG_LOGGING 
#define DEBUG_PRINT_FUNCTION_NAME 
#define PARANOIA

#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "bgp/bgp_module.h"
#include "config.h"

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
    MPReachNLRIAttribute<IPv6> *mpreach = 0;
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
Trie::process_update_packet(const TimeVal& tv, const uint8_t *buf, size_t len)
{
    _update_cnt++;

    TriePayload payload(tv, buf, len, _first, _last);
    const UpdatePacket *p = payload.get();

    debug_msg("%s\n", p->str().c_str());

    MPReachNLRIAttribute<IPv6> *mpreach = 0;
    MPUNReachNLRIAttribute<IPv6> *mpunreach = 0;
    
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
Trie::replay_walk(const ReplayWalker uw) const
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
	size_t len;
	UpdatePacket *packet = const_cast<UpdatePacket *>(p->data());
	const uint8_t *data = packet->encode(len);
	trie.process_update_packet(p->tv(), data, len);
	delete [] data;
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
