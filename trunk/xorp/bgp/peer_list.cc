// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer_list.cc,v 1.12 2004/06/10 22:40:32 hodson Exp $"

#include "bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "peer_list.hh"

BGPPeerList::BGPPeerList()
    : _next_token(0)
{
}

BGPPeerList::~BGPPeerList()
{
    list<BGPPeer *>::iterator i;

    for(i = _peers.begin(); i != _peers.end(); i++) {
	(*i)->event_stop();
	delete (*i);
 	*i = 0;
    }
    _peers.clear();
}

void 
BGPPeerList::all_stop() 
{

    list<BGPPeer *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	(*i)->event_stop();
    }
    /* We now need to drop back to the EventLoop - the peers will only
       move to idle and cleanly tear down their state when the EventLoop
       calls their transmit complete callbacks */
    debug_msg("RETURNING FROM ALL_STOP\n");
}

bool
BGPPeerList::not_all_idle()
{
    list<BGPPeer *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++)
	if(STATEIDLE != (*i)->state())
	    return true;
    
    return false;
}

void
BGPPeerList::add_peer(BGPPeer *p)
{
    list<BGPPeer *>::iterator i;
     
    if (_peers.empty() || p->remote_ip_ge_than(*(_peers.back()))) {
	_peers.push_back(p);
	return;
    }
    
    for(i = _peers.begin(); i != _peers.end(); i++) {
	if((*i)->remote_ip_ge_than(*p)) {
	    _peers.insert(i,p);
	    return;
	}
    }
    XLOG_UNREACHABLE();
}

void
BGPPeerList::remove_peer(BGPPeer *p)
{
    //Before we remove a peer from the peer list, we need to check
    //whether there are any readers that point to this peer.  As the
    //number of readers is unlikely to be large, and this is not a
    //really frequest operation, we don't mind iterating through the
    //whole map of readers.
    map <uint32_t, list<BGPPeer *>::iterator>::iterator mi;
    for (mi = _readers.begin(); mi != _readers.end(); mi++) {
	list<BGPPeer *>::iterator pli = mi->second;
	if (*pli == p) {
	    pli++;
	    _readers[mi->first] = pli;
	}
    }

    //Now it's safe to do the deletion.
    list<BGPPeer *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++)
	if(p == *i) {
	    delete p;
	    _peers.erase(i);
	    return;
	}
    XLOG_FATAL("Peer %s not found in peerlist", p->str().c_str());
}


void 
BGPPeerList::dump_list()
{
    list<BGPPeer *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++)
	debug_msg("%s\n", (*i)->str().c_str());
}

bool BGPPeerList::get_peer_list_start(uint32_t& token)
{
    list<BGPPeer *>::iterator i = _peers.begin();
    if (i == _peers.end())
	return false;
    _readers.insert(make_pair(_next_token, i));
    token = _next_token;
    _next_token++;
    return true;
}

bool 
BGPPeerList::get_peer_list_next(const uint32_t& token, 
				IPv4& local_ip, 
				uint32_t& local_port, 
				IPv4& peer_ip, 
				uint32_t& peer_port)
{
    map <uint32_t, list<BGPPeer *>::iterator>::iterator mi;
    mi = _readers.find(token);
    if (mi == _readers.end())
	return false;
    list<BGPPeer *>::iterator i = mi->second;
    if (i == _peers.end()) {
	//this can happen only if the iterator pointed to the last
	//peer, and it was deleted since we were last here.
	local_ip = IPv4("0.0.0.0");
	local_port = 0;
	peer_ip = IPv4("0.0.0.0");
	peer_port = 0;
    } else {
	BGPPeer *peer = *i;
	local_ip = peer->peerdata()->iptuple().get_local_addr();
	local_port = htons(peer->peerdata()->iptuple().get_local_port());
	peer_ip = peer->peerdata()->iptuple().get_peer_addr();
	peer_port = htons(peer->peerdata()->iptuple().get_peer_port());
	i++;
    }
    if (i == _peers.end()) {
	_readers.erase(mi);
	return false;
    }
    _readers[token] = i;
    return true;
}
