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

// $XORP: xorp/ospf/peer_manager.hh,v 1.62 2006/01/13 23:14:28 atanu Exp $

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
template <typename A> class External;
template <typename A> class Vlink;

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
    PeerManager(Ospf<A>& ospf)
	: _ospf(ospf), _next_peerid(ALLPEERS + 1), _external(ospf, _areas)
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
     *  @param self if true this LSA was originated by this router.
     *  @param lsa if valid is true the LSA at index.
     */
    bool get_lsa(const OspfTypes::AreaID area, const uint32_t index,
		 bool& valid, bool& toohigh, bool& self, vector<uint8_t>& lsa);

    /**
     *  Get a list of all the configured areas.
     */
    bool get_area_list(list<OspfTypes::AreaID>& areas) const;

    /**
     *  Get a list of all the neighbours.
     */
    bool get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const;

    /**
     * Get state information about this neighbour.
     *
     * @param nid neighbour information is being request about.
     * @param ninfo if neighbour is found its information.
     *
     */
    bool get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const;

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
     * Transmit packets
     */
    bool transmit(const string& interface, const string& vif,
		  A dst, A src,
		  uint8_t* data, uint32_t len);

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
     * Upcall from a peer to notify the peer manager that a full
     * adjacency has been achieved or lost.
     *
     * @param peerid
     * @param rid neighbours router ID.
     * @param up true if the adjacency has become full, false if a
     * full adjacency has been lost.
     */
    void adjacency_changed(const PeerID peerid, OspfTypes::RouterID rid,
			   bool up);

    /**
     * Send a new Router-LSA in all areas.
     *
     * Typically called when one of the Router-LSA flags changes state.
     */
    void refresh_router_lsas() const;

    /**
     * Create a virtual link peer.
     */
    bool create_virtual_peer(OspfTypes::RouterID rid);

    /**
     * Delete a virtual link peer.
     */
    bool delete_virtual_peer(OspfTypes::RouterID rid);

    /**
     * Are any of neighbours of this area a virtual link endpoint.
     *
     * @return true if any are.
     */
    bool virtual_link_endpoint(OspfTypes::AreaID area) const;

    /**
     * Create a virtual link (Configuration).
     *
     * @param rid neighbours router ID.
     */
    bool create_virtual_link(OspfTypes::RouterID rid);

    /**
     * Attach this transit area to the neighbours router ID  (Configuration).
     */
    bool transit_area_virtual_link(OspfTypes::RouterID rid,
				   OspfTypes::AreaID transit_area);

    /**
     * Delete a virtual link (Configuration).
     *
     * @param rid neighbours router ID.
     */
    bool delete_virtual_link(OspfTypes::RouterID rid);

    /**
     * Bring virtual link up (Upcall from area router).
     *
     * @param rid neighbours router ID.
     * @param source address of packets sent to this neighbour.
     * @param interface_cost
     * @param destination address of the neighbour router.
     */
    void up_virtual_link(OspfTypes::RouterID rid, A source,
			 uint16_t interface_cost, A destination);

    /**
     * Take this virtual link down (Upcall from area router).
     */
    void down_virtual_link(OspfTypes::RouterID rid);
    
    /**
     * A packet was sent to a peer that rejected it so this may be a
     * virtual link candidate.
     */
    bool receive_virtual_link(A dst, A src, Packet *packet);

    /**
     * Return the number of areas of the specified type.
     */
    uint32_t area_count(OspfTypes::AreaType area_type) const;

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

    /**
     * Compute the options that are sent in hello packets, data
     * description packets, LSA headers (OSPFv2),  Router-LSAs
     * (OSPFv3) and Network-LSAs (OSPFv3).
     *
     */
    uint32_t compute_options(OspfTypes::AreaType area_type);

    // Config (begin)

    /**
     * The router ID is about to change.
     */
    void router_id_changing();

#if	0
    /**
     * Set options.
     */
    bool set_options(const PeerID, OspfTypes::AreaID area,
		     uint32_t options);
#endif

    /**
     * Set the interface address of this peer.
     */
    bool set_interface_address(const PeerID, A address);

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
     * Set RxmtInterval
     */
    bool set_retransmit_interval(const PeerID, OspfTypes::AreaID area,
				 uint16_t retransmit_interval);

    /**
     * Set InfTransDelay
     */
    bool set_inftransdelay(const PeerID, OspfTypes::AreaID area,
			   uint16_t inftransdelay);

    /**
     *  Configure authentication.
     */
    bool set_authentication(const PeerID, OspfTypes::AreaID area,
			    string& type, string& password);

    /**
     * Toggle the passive status of an interface.
     */
    bool set_passive(const PeerID, OspfTypes::AreaID area,
		     bool passive);

    // Config (end)

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
    
    /**
     * Does the specified area have any area ranges configured.
     *
     * The primary purpose is to discover if the backbone area has any
     * area ranges configured, this is required when an area becomes a
     * transit area.
     */
    bool area_range_configured(OspfTypes::AreaID area);

    /**
     * An AS-External-LSA has arrived from this area announce it to
     * all others.
     *
     * The LSAs should not be scheduled for transmission until the
     * external_shove() is seen. In many cases a number of LSAs may
     * arrive in a single packet, waiting for the external_shove()
     * offers an opportunity for aggregation.
     *
     */
    bool external_announce(OspfTypes::AreaID area, Lsa::LsaRef lsar);

    /**
     * Create an AS-External-LSA and announce it to all appropriate
     * areas.
     */
    bool external_announce(const IPNet<A>& net, const A& nexthop,
			   const uint32_t& metric,
			   const PolicyTags& policytags);

    /**
     * An AS-External-LSA is being withdrawn from this area withdraw from
     * all others.
     */
    bool external_withdraw(OspfTypes::AreaID area, Lsa::LsaRef lsar);

    /**
     * Withdraw a previously createed and announced AS-External-LSA
     * from all areas.
     */
    bool external_withdraw(const IPNet<A>& net);

    /**
     * Called to complete a series of calls to external_announce(area).
     */
    bool external_shove(OspfTypes::AreaID area);

    /**
     * When a new area is configured it can use this method to 
     *
     * @param area that all AS-External-LSAs should be sent to.
     */
    void external_push(OspfTypes::AreaID area);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    
    PeerID _next_peerid;		// Next PeerID to allocate.
    map<string, PeerID> _pmap;		// Map from interface/vif to PeerID.

    map<PeerID, PeerOut<A> *> _peers;	// All of our peers

    map<OspfTypes::AreaID, AreaRouter<A> *> _areas;	// All the areas

    External<A> _external;		// Management of AS-External-LSAs.
    Vlink<A> _vlink;			// Management of virtual links

    uint32_t	_normal_cnt;		// Number of normal areas.
    uint32_t	_stub_cnt;		// Number of stub areas.
    uint32_t	_nssa_cnt;		// Number of nssa areas.

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

    /**
     * Track the number of areas of each type.
     *
     * @param area_type being tracked.
     * @param up true if the area is being created, false if it is
     * being deleted.
     */
    void track_area_count(OspfTypes::AreaType area_type, bool up);
};

#endif // __OSPF_PEER_MANAGER_HH__
