// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __OSPF_AREA_ROUTER_HH__
#define __OSPF_AREA_ROUTER_HH__

/**
 * Area Router
 * 
 */
template <typename A>
class AreaRouter {
 public:
    AreaRouter(Ospf<A>& ospf, Area area, OspfTypes::AreaType area_type) 
	: _ospf(ospf), _area(area), _area_type(area_type)
    {}

    /**
     * Add peer
     */
    void add_peer(PeerID peer);

    /**
     * Delete peer
     */
    void delete_peer(PeerID peer);

    /**
     * Peer came up
     * Callback registered with the peer manager.
     */
    void peer_up(PeerID peer);

    /**
     * Peer went down
     * Callback registered with the peer manager.
     */
    void peer_down(PeerID peer);

    /**
     * Receive LSA
     * Callback registered with the peer manager.
     */
    void receive_lsa(PeerID peer, LsaRef lsa);
 private:
    Ospf<A>& _ospf;		// Reference to the controlling class.
    Area _area;			// Area: That is represented.
    OspfTypes::AreaType _area_type;

    set<PeerID>	_peers;		// Peers that this area is associated with.
};

#endif // __OSPF_AREA_ROUTER_HH__
