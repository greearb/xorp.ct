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

/**
 * In OSPF terms this class represents an interface/link; interface is
 * too overloaded. The Peer class is also associated with an area. In
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
    enum LinkType {
	BROADCAST,
	NBMA,
	PointToMultiPoint,
	VirtualLink
    };

    Peer(Ospf<A>& ospf, LinkType linktype, Area area)
	: _ospf(ospf), _linktype(linktype), _area(area)
    {}

    /**
     * True if:
     * 	1) Administratively enabled.
     *  2) The interface/link is configured.
     * Used by clients of this class to determine if it is legal to 
     * transmit packets.
     */
    bool running();
    
    /**
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool received(/* XXX */);

 private:
    Ospf<A>& _ospf;		// Reference to the controlling class.
    LinkType _linktype;		// Type of this link.
    Area _area;			// Area: That is represented.

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<TransmitRef>	 _transmit_queue;	
};

#endif // __OSPF_PEER_HH__
