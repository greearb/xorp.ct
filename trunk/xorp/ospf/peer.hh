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
	    const A address,
	    OspfTypes::LinkType linktype, OspfTypes::AreaID area,
	    OspfTypes::AreaType area_type);

    ~PeerOut();

    /**
     * @return interface/vif address.
     */
    A get_address() { return _address; }

    /**
     * Add another Area for this peer to be in, should only be allowed
     * for OSPFv3.
     */
    bool add_area(OspfTypes::AreaID area, OspfTypes::AreaType area_type);

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
    bool transmit(typename Transmit<A>::TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet) throw(BadPeer);

    /**
     * @return the link type.
     */
    OspfTypes::LinkType get_linktype() const { return _linktype; }

    // Configure the peering.

    /**
     * Set the network mask OSPFv2 only.
     */
    bool set_network_mask(OspfTypes::AreaID area, uint32_t network_mask);

    /**
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(OspfTypes::AreaID area, uint32_t interface_id);

    /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(OspfTypes::AreaID area, uint16_t hello_interval);

    /**
     * Set options.
     */
    bool set_options(OspfTypes::AreaID area, uint32_t options);

    /**
     * Set router priority.
     */
    bool set_router_priority(OspfTypes::AreaID area, uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(OspfTypes::AreaID area,
				  uint32_t router_dead_interval);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    const string _interface;	   	// The interface and vif this peer is
    const string _vif;			// responsible for.
    const A	_address;		// Address of this interface/vif.

    OspfTypes::LinkType _linktype;	// Type of this link.

					//  Areas being served.
    map<OspfTypes::AreaID, Peer<A> *>  _areas; 

    bool _running;			// True if the peer is up and running

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<typename Transmit<A>::TransmitRef> _transmit_queue;	

    void bring_up_peering();
    void take_down_peering();
};

template <typename A> class Neighbour;

/**
 * A peer represents a single area and is bound to a PeerOut.
 */
template <typename A>
class Peer {
 public:
    Peer(Ospf<A>& ospf, PeerOut<A>& peerout, OspfTypes::AreaID area,
	 OspfTypes::AreaType area_type)
	: _ospf(ospf), _peerout(peerout), _area(area), _area_type(area_type),
	  _hello_packet(ospf.get_version())
    {
	_hello_packet.set_area_id(area);

	// Some defaults taken from the Juniper manual. These values
	// should be overriden by the values in the templates files.
	// For testing set some useful values
	_hello_packet.set_hello_interval(10);
	_hello_packet.set_router_priority(128);

	// RFC 2328 Appendix C.3 Router Interface Parameters
	_hello_packet.
	    set_router_dead_interval(4 * _hello_packet.get_hello_interval());

	_interface_state = Down;
    }

    /**
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet);

    /**
     * Start the protocol machinery running
     */
    void start();

    /**
     * Stop the protocol machinery running
     */
    void stop();

    /**
     * Event: InterfaceUP
     */
    void event_interface_up();

    /**
     * Event: WaitTimer
     */
    void event_wait_timer();

    /**
     * Event: BackupSeen
     */
    void event_backup_seen();

    /**
     * Event: NeighborChange
     */
    void event_neighbour_change();

    /**
     * Event: LoopInd
     */
    void event_loop_ind();

    /**
     * Event: UnLoopInd
     */
    void event_unloop_ind();

    /**
     * Event: InterfaceDown
     */
    void event_interface_down();

    /**
     * Set the network mask OSPFv2 only.
     */
    bool set_network_mask(uint32_t network_mask);

    /**
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(uint32_t interface_id);

    /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(uint16_t hello_interval);

    /**
     * Set options.
     */
    bool set_options(uint32_t options);

    /**
     * Set router priority.
     */
    bool set_router_priority(uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(uint32_t router_dead_interval);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    PeerOut<A>& _peerout;		// Reference to PeerOut class.
    const OspfTypes::AreaID _area;	// Area that is being represented.
    const OspfTypes::AreaType _area_type;// BORDER or STUB or NSSA.

    XorpTimer _hello_timer;		// Timer used to fire hello messages.
    XorpTimer _wait_timer;		// Wait to discover other DRs.

    /**
     * Interface as defined by OSPF not XORP.
     */
    enum InterfaceState {
	Down,
	Loopback,
	Waiting,
	Point2Point,
	DR_other,
	Backup,
	DR,
    };

    InterfaceState _interface_state;

    map<A, Neighbour<A> *> _neighbours;

    HelloPacket _hello_packet;		// Packet that is sent by this peer.

    /**
     * Possible DR or BDR candidates.
     */
    struct Candidate {
	Candidate(OspfTypes::RouterID router_id, OspfTypes::RouterID dr,
		  OspfTypes::RouterID bdr, uint8_t router_priority) 
	    : _router_id(router_id), _dr(dr), _bdr(bdr), 
	      _router_priority(router_priority)
	{}

	OspfTypes::RouterID _router_id;	// Candidate's ID
	OspfTypes::RouterID _dr;	// Designated router.
	OspfTypes::RouterID _bdr;	// Backup Designated router.
	uint8_t  _router_priority;	// Router Priority.
    };

    void start_hello_timer();

    void start_wait_timer();

    bool send_hello_packet();
    
    OspfTypes::RouterID
    backup_designated_router(list<Candidate>& candidates) const;
    OspfTypes::RouterID
    designated_router(list<Candidate>& candidates) const;

    void compute_designated_router_and_backup_designated_router();

    /**
     * Stop all timers.
     */
    void tear_down_state();

    /**
     * Pretty print the interface state.
     */
    string pp_interface_state(InterfaceState is);
};

/**
 * Neighbour specific information.
 */
template <typename A>
class Neighbour {
 public:
    /**
     * NOTE: The ordering is important (used in the DR and BDR election).
     */
    enum State {
	Down = 1,
	Attempt = 2,
	Init = 3,
	TwoWay = 4,
	ExStart = 5,
	Exchange = 6,
	Loading = 7,
	Full = 8
    };

    Neighbour(Ospf<A>& ospf, Peer<A>& peer, OspfTypes::RouterID router_id,
	      A src)
	: _ospf(ospf), _peer(peer), _router_id(router_id), _src(src),
	  _neighbour_state(Down), _hello_packet(0)
    {}

    ~Neighbour() {
	delete _hello_packet;
    }

    OspfTypes::RouterID get_router_id() const { return _router_id; }

    A get_source_address() const { return _src; }

    void set_neigbour_state(State state) {_neighbour_state = state; }

    State get_neighbour_state() const { return _neighbour_state; }

    HelloPacket *get_hello_packet() { return _hello_packet; }
    HelloPacket *get_hello_packet() const { return _hello_packet; }

    void set_hello_packet(HelloPacket *packet) {
	delete _hello_packet;
	_hello_packet = packet;
    }

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    Peer<A>& _peer;			// Reference to Peer class.
    const OspfTypes::RouterID _router_id;// Neighbour's RouterID.
    const A _src;			// Neighbour's source address.
    State _neighbour_state;		// State of this neighbour.
    HelloPacket *_hello_packet;		// Last hello packet received
					// from this neighbour.
};

#endif // __OSPF_PEER_HH__
