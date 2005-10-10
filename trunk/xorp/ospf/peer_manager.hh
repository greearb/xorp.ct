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

// $XORP: xorp/ospf/peer_manager.hh,v 1.40 2005/10/10 06:03:25 atanu Exp $

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
template <typename A> class RouteEntry;

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
     * Add area range.
     */
    bool area_range_add(OspfTypes::AreaID area, IPNet<A> net, bool advertise);

    /**
     * Delete area range.
     */
    bool area_range_delete(OspfTypes::AreaID area, IPNet<A> net);

    /**
     * Change the advertised state of this area.
     */
    bool area_range_change_state(OspfTypes::AreaID area, IPNet<A> net,
				 bool advertise);

    /**
     *  Get a single lsa from an area. A stateless mechanism to get LSAs. The
     *  client of this interface should start from zero and continue to request
     *  LSAs (incrementing index) until toohigh becomes true.
     *
     *  @param area database that is being searched.
     *  @param index into database starting from 0.
     *  @param valid true if a LSA has been returned. Some index values do not
     *  contain LSAs. This should not be considered an error.
     *  @param toohigh true if no more LSA exist after this index.
     *  @param lsa if valid is true the LSA at index.
     */
    bool get_lsa(const OspfTypes::AreaID area, const uint32_t index,
		 bool& valid, bool& toohigh, vector<uint8_t>& lsa);

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
		       const A source, uint16_t interface_prefix_length,
		       uint16_t interface_mtu,
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
     * Add a neighbour to the peer.
     */
    bool add_neighbour(const PeerID,
		       OspfTypes::AreaID area, A neighbour_address,
		       OspfTypes::RouterID);

    /**
     * Remove a neighbour from the peer.
     */
    bool remove_neighbour(const PeerID,
			  OspfTypes::AreaID area, A neighbour_address,
			  OspfTypes::RouterID rid);


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
     * @param multicast_on_peer will this LSA get multicast on this peer.
     *
     * @return true on success.
     */
    bool queue_lsa(const PeerID peerid, const PeerID peer,
		   OspfTypes::NeighbourID nid, Lsa::LsaRef lsar,
		   bool &multicast_on_peer);

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas(const PeerID peerid);

    /*
     * Is this one of the routers interface addresses, used to try and
     * detect self-originated LSAs.
     *
     * @param address under test
     *
     * @return true if this a known interface address.
     */
    bool known_interface_address(const A address) const;

    /**
     * Are any of the neighbours of this peer in the state exchange or
     * loading.
     *
     * @param peerid
     * @param area
     *
     * @return true if any of the neighbours are in state exchange or loading.
     */
    bool neighbours_exchange_or_loading(const PeerID peerid,
					OspfTypes::AreaID area);

    /**
     * Is this LSA on this neighbours link state request list.
     * @param peerid
     * @param area
     * @param nid
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(const PeerID peerid,
				    OspfTypes::AreaID area,
				    const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar);
    
    /**
     * Generate a BadLSReq event.
     *
     * @param peerid
     * @param area
     * @param nid
     *
     * @return true if it is.
     */
    bool event_bad_link_state_request(const PeerID peerid,
				      OspfTypes::AreaID area,
				      const OspfTypes::NeighbourID nid);

    /**
     * Send this LSA directly to the neighbour. Do not place on
     * retransmission list.
     *
     * @param peerid
     * @param area
     * @param nid
     * @param lsar
     *
     * @return true on success
     */
    bool send_lsa(const PeerID peerid, OspfTypes::AreaID area,
		  const OspfTypes::NeighbourID nid,
		  Lsa::LsaRef lsar);


    /**
     * Are any of neighbours of this area a virtual link endpoint.
     *
     * @return true if any are.
     */
    bool virtual_link_endpoint(OspfTypes::AreaID area) const;

    /**
     * Is this an internal router?
     */
    bool internal_router_p() const;

    /**
     * Is this an area border router?
     */
    bool area_border_router_p() const;

    /**
     * Is this a backbone router?
     */
    bool backbone_router_p() const;

    /**
     * Is this an AS boundary router?
     */
    bool as_boundary_router_p() const;

    // Configure the peering.

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

    /**
     * Set interface cost
     */
    bool set_interface_cost(const PeerID, OspfTypes::AreaID area,
			    uint16_t interface_cost);

    /**
     * Set InfTransDelay
     */
    bool set_inftransdelay(const PeerID, OspfTypes::AreaID area,
			   uint16_t inftransdelay);

    /**
     * Number of areas this router serves.
     */
    size_t number_of_areas() const { return _areas.size(); }

    /**
     * A new route has been added to the routing table announce it to
     * all areas as it is a candidate for Summary-LSA generation.
     *
     * @param area that the route was introduced by.
     */
    void summary_announce(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt);

    /**
     * A route has been deleted from the routing table. It may
     * previously have caused a Summary-LSA which now needs to be
     * withdrawn.
     *
     * @param area that the route was introduced by.
     */
    void summary_withdraw(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt);

    /**
     * Send all the summary information to specified area.  New areas
     * or stub areas that change from do not advertise can use this
     * hook to force all routes to be sent to the specified area.
     *
     * @param area that all routes should be sent to.
     */
    void summary_push(OspfTypes::AreaID area);

    /**
     * In the specified area is the net covered by an area range.
     *
     * @param area being checked.
     * @param net that may be covered.
     * @param advertise if the area is covered set to advertise or do
     * not advertise.
     *
     * @return true if the area is covered.
     */
    bool area_range_covered(OspfTypes::AreaID area, IPNet<A> net,
			    bool& advertise);
    
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


    /**
     * @return true if this route is a candidate for summarisation.
     */
    bool summary_candidate(OspfTypes::AreaID area, IPNet<A> net,
			   RouteEntry<A>& rt);

    /**
     * Saved summaries that can be introduced into a new area.
     */
    struct Summary {
	Summary()
	{}
	Summary(OspfTypes::AreaID area, RouteEntry<A>& rt)
	    : _area(area), _rtentry(rt)
	{}

	OspfTypes::AreaID _area;
	RouteEntry<A> _rtentry;
    };

    map<IPNet<A>, Summary> _summaries;
};

#endif // __OSPF_PEER_MANAGER_HH__
