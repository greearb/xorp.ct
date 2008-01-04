// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/bgp/peer_list.hh,v 1.11 2007/02/16 22:45:14 pavlin Exp $

#ifndef __BGP_PEER_LIST_HH__
#define __BGP_PEER_LIST_HH__

#include <list>
#include <map>
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
