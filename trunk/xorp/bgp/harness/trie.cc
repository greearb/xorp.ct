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

#ident "$XORP: xorp/bgp/harness/trie.cc,v 1.6 2003/09/09 02:02:03 atanu Exp $"

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
    for(; ni != update->nlri_list().end(); ni++)
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
    const UpdatePacket *update = payload.get();

    /*
    ** First process the withdraws, if any are present.
    */
    list <BGPUpdateAttrib>::const_iterator wi;
    wi = update->wr_list().begin();
    for(; wi != update->wr_list().end(); wi++)
	_head_ipv4.del(wi->net());

    /*
    ** If there are no nlri's present then there is nothing to save so
    ** out of here.
    */
    if(update->nlri_list().empty())
	return;

    /*
    ** Now save the NLRI information.
    */
    list <BGPUpdateAttrib>::const_iterator ni;
    ni = update->nlri_list().begin();
    for(; ni != update->nlri_list().end(); ni++) {
	/* If a previous entry exists remove it */
	const UpdatePacket *up = lookup(ni->net().str());
	if(up)
	    if(!_head_ipv4.del(ni->net()))
		XLOG_FATAL("Could not remove nlri: %s",
			   ni->net().str().c_str());
	if(!_head_ipv4.insert(ni->net(), payload))
	    XLOG_FATAL("Could not add nlri: %s",
		       ni->net().str().c_str());
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

// void
// Trie::save_routing_table(FILE *fp) const
// {
//     _head_ipv4.print(fp);
//     _head_ipv6.print(fp);
// }
