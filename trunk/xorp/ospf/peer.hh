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

template <typename A> class Ospf;
template <typename A> class Peer;

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
class PeerOut {
 public:

    PeerOut(Ospf<A>& ospf, const string interface, const string vif, 
	    OspfTypes::LinkType linktype, OspfTypes::AreaID area);

    ~PeerOut();

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

    /**
     * Set the state of this peer.
     */
    void set_state(bool state);

    /**
     * Get the state of this peer.
     */
    bool get_state() const {
	return _running;
    }

    /**
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(Transmit::TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool received(/* XXX */);

    // Configure the peering.

    /**
     * Set the network mask OSPFv2 only.
     */
    void set_network_mask(OspfTypes::AreaID area, uint32_t network_mask);

    /**
     * Set the interface ID OSPFv3 only.
     */
    void set_interface_id(OspfTypes::AreaID area, uint32_t interface_id);

    /**
     * Set the hello interval in seconds.
     */
    void set_hello_interval(OspfTypes::AreaID area, uint16_t hello_interval);

    /**
     * Set options.
     */
    void set_options(OspfTypes::AreaID area, uint32_t options);

    /**
     * Set router priority.
     */
    void set_router_priority(OspfTypes::AreaID area, uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    void set_router_dead_interval(OspfTypes::AreaID area,
				  uint32_t router_dead_interval);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    const string _interface;	   	// The interface and vif this peer is
    const string _vif;			// responsible for.

    OspfTypes::LinkType _linktype;	// Type of this link.

					//  Areas we are serving.
    map<OspfTypes::AreaID, Peer<A> *>  _areas; 

    bool _running;			// True if the peer is up and running

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<Transmit::TransmitRef> _transmit_queue;	

    void bring_up_peering();
    void take_down_peering();
};

const uint32_t HELLO_INTERVAL = 1000;	// Default hello interval in ms.

/**
 * A peer represents a single area and is bound to a PeerOut.
 */
template <typename A>
class Peer {
 public:
    Peer(Ospf<A>& ospf, PeerOut<A>& peerout, OspfTypes::AreaID area) 
	: _ospf(ospf), _peerout(peerout), _area(area),
	  _hello_interval(HELLO_INTERVAL), _hello_packet(ospf.get_version())
    {}

    /**
     * Start the protocol machinery running
     */
    void start();

    /**
     * Stop the protocol machinery running
     */
    void stop();

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    PeerOut<A>& _peerout;		// Reference to PeerOut class.
    OspfTypes::AreaID _area;		// Area that we are represent.

    uint32_t  _hello_interval;		// Time between sending hello messages.
    XorpTimer _hello_timer;		// Timer used to fire hello messages.

    enum PeerState {
	Down,
	Attempt,
	Init,
	TwoWay,
	ExStart,
	Exchange,
	Loading,
	Full
    };

    PeerState _state;			// The state of this peer.

    HelloPacket _hello_packet;		// The hello packet we will send.

    void start_hello_timer();

    bool send_hello_packet();
};

#endif // __OSPF_PEER_HH__
