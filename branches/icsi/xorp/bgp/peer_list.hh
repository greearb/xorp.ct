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

// $XORP: xorp/bgp/peer_list.hh,v 1.8 2002/12/09 18:28:45 hodson Exp $

#ifndef __BGP_PEER_LIST_HH__
#define __BGP_PEER_LIST_HH__

#include <list>
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
protected:
private:
    list<BGPPeer *> _peers;
};

#endif	// __BGP_PEER_LIST_HH__
