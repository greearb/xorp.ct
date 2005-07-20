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

#ifndef __OSPF_PEER_MANAGER_HH__
#define __OSPF_PEER_MANAGER_HH__

/**
 * An opaque handle that identifies a peer.
 */
typedef uint32_t PeerID;

/**
 * An identifier meaning all peers. No single peer can have this identifier.
 */
static const PeerID ALLPEERS = 0;

template <typename A> class Ospf;
template <typename A> class PeerOut;
template <typename A> class AreaRouter;

/**
 * Peer Manager:
 *	1) Monitor the state of the interfaces. An interface going
 *	up/down will be monitored and trigger either adjacency
 *	attempts or new (withdraw) LSAs.
 *	2) Manage interface configuration state. Control peers and
 *	area state.
 *	3) Accept incoming hello's and demultiplex to the correct peer.
 *	4) Manage the set of peers (peers are bound to interface and
 *	area). The peers themselves are hidden and are only
 *	exposed by reference (PeerID).
 */
template <typename A>
class PeerManager {
 public:
    PeerManager(Ospf<A> &ospf)
	: _ospf(ospf), _next_peerid(ALLPEERS + 1)
    {}

    ~PeerManager();

    /**
     * Create an area router.
     */
    bool create_area_router(OspfTypes::AreaID area,
			    OspfTypes::AreaType area_type);

    AreaRouter<A> *get_area_router(OspfTypes::AreaID area);

    /**
     * Destroy an area router.
     */
    bool destroy_area_router(OspfTypes::AreaID area);


    /**
     * Convert an interface/vif to a PeerID.
     * Throw an exception if no mapping is found.
     */
    PeerID get_peerid(const string& interface, const string& vif) 
	throw(BadPeer);


    /**
     * Create a peer.
     * @param interface
     * @param vif
     * @param source address of transmitted packets.
     * @param interface_mtu, MTU of this interface.
     * @param linktype broadcast or point-2-point, etc...
     * @param area ID of area
     *
     * @return PeerID on success otherwise throw an exception.
     */
    PeerID create_peer(const string& interface, const string& vif,
		       const A source, uint16_t interface_mtu,
		       OspfTypes::LinkType linktype, OspfTypes::AreaID area)
	throw(BadPeer);
	
    /**
     * Delete a peer.
     */
    bool delete_peer(const PeerID);

    /**
     * Take a peer up or down.
     */
    bool set_state_peer(const PeerID, bool state);

    /**
     * Demultiplex incoming packets to the associated peer. If the
     * packet contains LSAs send it to the LSA database manager if
     * appropriate.
     *
     * @param interface that packet arrived on
     * @param vif that packet arrived on
     * @param packet
     * @return true if the packet is now owned by the peer manager.
     */
    bool receive(const string& interface, const string& vif, A dst, A src,
		 Packet *packet)
	throw(BadPeer);

    /**
     * Queue an LSA for transmission.
     *
     * @param peerid to queue to LSA on.
     * @param peer the LSA arrived on.
     * @param nid the LSA arrived on.
     * @param lsar the lsa
     * @return true on success.
     * @return multicast_on_peer Did this LSA get multicast on this peer.
     */
    bool queue_lsa(const PeerID peerid, const PeerID peer,
		   OspfTypes::NeighbourID nid, Lsa::LsaRef lsar,
		   bool &multicast_on_peer);

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas(const PeerID peerid);

    // Configure the peering.

    /**
     * Set the network mask OSPFv2 only.
     */
    bool set_network_mask(const PeerID, OspfTypes::AreaID area, 
			  uint32_t network_mask);

    /**
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(const PeerID, OspfTypes::AreaID area,
			  uint32_t interface_id);

    /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(const PeerID, OspfTypes::AreaID area,
			    uint16_t hello_interval);

#if	0
    /**
     * Set options.
     */
    bool set_options(const PeerID, OspfTypes::AreaID area,
		     uint32_t options);
#endif
    /**
     * Compute the options that are sent in hello packets, data
     * description packets, LSA headers (OSPFv2),  Router-LSAs
     * (OSPFv3) and Network-LSAs (OSPFv3).
     *
     */
    uint32_t compute_options(OspfTypes::AreaType area_type);

    /**
     * Set router priority.
     */
    bool set_router_priority(const PeerID, OspfTypes::AreaID area,
			     uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(const PeerID, OspfTypes::AreaID area,
				  uint32_t router_dead_interval);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    
    PeerID _next_peerid;		// Next PeerID to allocate.
    map<string, PeerID> _pmap;		// Map from interface/vif to PeerID.

    map<PeerID, PeerOut<A> *> _peers;	// All of our peers

    map<OspfTypes::AreaID, AreaRouter<A> *> _areas;	// All the areas

    /**
     * Generate PeerID.
     * Internally we want to deal with peers as simple IDs not
     * interface/vif.
     * Throw an exception a mapping already exists.
     */
    PeerID create_peerid(const string& interface, const string& vif)
	throw(BadPeer);

    /**
     * Get rid of this mapping.
     */
    void destroy_peerid(const string& interface, const string& vif)
	throw(BadPeer);
};

#endif // __OSPF_PEER_MANAGER_HH__
