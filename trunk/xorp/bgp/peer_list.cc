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

#ident "$XORP: xorp/bgp/peer_list.cc,v 1.3 2003/01/17 04:07:23 mjh Exp $"

#include "bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "peer_list.hh"

BGPPeerList::BGPPeerList()
{
}

BGPPeerList::~BGPPeerList()
{
    list<BGPPeer *>::iterator i;

    for(i = _peers.begin(); i != _peers.end(); i++) {
	(*i)->action(EVENTBGPSTOP);
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
	(*i)->action(EVENTBGPSTOP);
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
	if(STATEIDLE != (*i)->get_ConnectionState())
	    return true;
    
    return false;
}

void
BGPPeerList::add_peer(BGPPeer *p)
{
    _peers.push_back(p);
}

void
BGPPeerList::remove_peer(BGPPeer *p)
{
    list<BGPPeer *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++)
	if(p != *i) {
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
    _readers[_next_token] = i;
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
    //XXX the code below assumes the peer list doesn't change while
    //we're working our way through it.
    map <uint32_t, list<BGPPeer *>::iterator>::iterator mi;
    mi = _readers.find(token);
    if (mi == _readers.end())
	return false;
    list<BGPPeer *>::iterator i = mi->second;
    BGPPeer *peer = *i;
    local_ip = peer->peerdata()->iptuple().get_local_addr();
    local_port = peer->peerdata()->iptuple().get_local_port();
    peer_ip = peer->peerdata()->iptuple().get_peer_addr();
    peer_port = peer->peerdata()->iptuple().get_peer_port();

    i++;
    if (i == _peers.end()) {
	_readers.erase(mi);
	return false;
    }
    _readers[token] = i;
    return true;
}
