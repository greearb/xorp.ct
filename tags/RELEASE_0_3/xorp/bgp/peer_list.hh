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

// $XORP: xorp/bgp/peer_list.hh,v 1.3 2003/01/17 04:07:24 mjh Exp $

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
    void all_stop();
    bool not_all_idle();
    void add_peer(BGPPeer *p);
    void remove_peer(BGPPeer *p);
    list<BGPPeer *>& get_list() {return _peers;} 
    void dump_list();
    bool get_peer_list_start(uint32_t& token);
    bool get_peer_list_next(const uint32_t& token, 
			    IPv4& local_ip, 
			    uint32_t& local_port, 
			    IPv4& peer_ip, 
			    uint32_t& peer_port);
protected:
private:
    list<BGPPeer *> _peers;
    map <uint32_t, list<BGPPeer *>::iterator> _readers;
    uint32_t _next_token;
};

#endif	// __BGP_PEER_LIST_HH__
