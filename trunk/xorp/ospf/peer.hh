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

#ifndef __OSPF_PEER_HH__
#define __OSPF_PEER_HH__

#include <queue>

template <typename A> class Ospf;

/**
 * In OSPF terms this class represents an interface/link; interface is
 * too overloaded a term. The Peer class is also associated with an area. In
 * OSPF v2 there is a one-to-one correspondence. In OSPF v3 an
 * interface/link can belong to multiple areas.
 *
 * Responsibilities:
 *	1) Packet transmission; the outgoing queue lives here.
 *	2) Exchange Hello packets.
 *	3) Bring up adjacency, synchronize databases.
 *	4) Elect designated and backup router.
 */

template <typename A>
class Peer {
 public:

    Peer(Ospf<A>& ospf, const string interface, const string vif, 
	 OspfTypes::LinkType linktype, OspfTypes::AreaID area)
	: _ospf(ospf), _interface(interface), _vif(vif), 
	  _linktype(linktype), _running(false)
    {
	_area.push_back(area);
    }

    /**
     * Add another Area for this peer to be in, should only be allowed
     * for OSPFv3.
     */
    bool add_area(OspfTypes::AreaID area);

    /**
     * This area is being removed.
     *
     * @return true if this peer is no longer associated with any
     * areas. Allowing the caller to delete this peer.
     */
    bool remove_area(OspfTypes::AreaID area);

    void set_state(bool state) {
	_running = state;
    }

    bool get_state() const {
	return _running;
    }

    /**
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool received(/* XXX */);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    const string _interface;	   	// The interface and vif this peer is
    const string _vif;			// responsible for.

    OspfTypes::LinkType _linktype;	// Type of this link.

    list<OspfTypes::AreaID> _area;	// Areas we 

    bool _running;			// True if the peer is up and running

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<TransmitRef>	 _transmit_queue;	
};

#endif // __OSPF_PEER_HH__
