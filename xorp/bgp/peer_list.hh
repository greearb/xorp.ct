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

// $XORP: xorp/bgp/peer_list.hh,v 1.14 2008/10/02 21:56:17 bms Exp $

#ifndef __BGP_PEER_LIST_HH__
#define __BGP_PEER_LIST_HH__



#include "peer.hh"

class BGPPeer;

class BGPPeerList
{
public:
    BGPPeerList();
    ~BGPPeerList();

    /**
     * Stop all the peerings.
     *
     * @param restart if true will bounce the peerings, if false the
     * peerings will be taken down and kept down.
     *
     */
    void all_stop(bool restart = false);
    
    /**
     * Are the peers idle? Used to poll the peers when BGP is being
     * gracefully taken down.
     *
     * @return true while peers are still active.
     */
    bool not_all_idle();

    /**
     * Add this peer to the peer list.
     */
    void add_peer(BGPPeer *p);

    /**
     * Remove the peer from the peer list and delete it.
     */
    void remove_peer(BGPPeer *p);

    /**
     * Detach this peer from the peer list (DO NOT DELETE IT).
     */
    void detach_peer(BGPPeer *p);

    /**
     * Get the list of attached peers.
     */
    list<BGPPeer *>& get_list() {return _peers;} 

    /**
     * Debugging entry point that prints all the peers.
     */
    void dump_list();

    /**
     * Aquire a token that can be used to scan through the peers.
     */
    bool get_peer_list_start(uint32_t& token);

    /**
     * Using the token get information about the peers.
     */
    bool get_peer_list_next(const uint32_t& token, 
			    string& local_ip, 
			    uint32_t& local_port, 
			    string& peer_ip, 
			    uint32_t& peer_port);
protected:
private:
    list<BGPPeer *> _peers;
    map <uint32_t, list<BGPPeer *>::iterator> _readers;
    uint32_t _next_token;
};

#endif	// __BGP_PEER_LIST_HH__
