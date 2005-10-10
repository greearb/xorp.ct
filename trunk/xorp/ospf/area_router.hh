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

// $XORP: xorp/ospf/area_router.hh,v 1.66 2005/10/07 07:09:11 atanu Exp $

#ifndef __OSPF_AREA_ROUTER_HH__
#define __OSPF_AREA_ROUTER_HH__

class DataBaseHandle;

/**
 * Area Router
 * 
 */
template <typename A>
class AreaRouter : Subsystem {
 public:
    AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
	       OspfTypes::AreaType area_type);

    /**
     * Required by the class Subsystem.
     * Called on startup.
     */
    void start();

    /**
     * Required by the class Subsystem.
     * Called on shutdown.
     */
    void shutdown();

    /**
     * Required by the class Subsystem.
     * Should return true while the subsystem is running.
     */
    bool running();

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
     */
    bool peer_up(PeerID peer);

    /**
     * Peer went down
     */
    bool peer_down(PeerID peer);

    /**
     * Add area range.
     */
    bool area_range_add(IPNet<A> net, bool advertise);

    /**
     * Delete area range.
     */
    bool area_range_delete(IPNet<A> net);

    /**
     * Change the advertised state of this area.
     */
    bool area_range_change_state(IPNet<A> net, bool advertise);

    /**
     * Is network covered by an area range and if it is should it be
     * advertised.
     */
    bool area_range_covered(IPNet<A> net, bool& advertise);

    /**
     * A new set of router links.
     */
    bool new_router_links(PeerID peer, const list<RouterLink>& router_link);

    /**
     * A new route has been added to the routing table it is being
     * presented to this area for possible Summary-LSA generation.
     */
    void summary_announce(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt);

    /**
     * A route has been deleted from the routing table. It may
     * previously have caused a Summary-LSA which now needs to be
     * withdrawn.
     */
    void summary_withdraw(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt);

    /**
     * Generate a Network-LSA for this peer.
     */
    bool generate_network_lsa(PeerID peer,
			      OspfTypes::RouterID link_state_id,
			      list<OspfTypes::RouterID>& attached_routers,
			      uint32_t network_mask);

    /**
     * Update the Network-LSA for this peer.
     */
    bool update_network_lsa(PeerID peer,
			    OspfTypes::RouterID link_state_id,
			    list<OspfTypes::RouterID>& attached_routers,
			    uint32_t network_mask);

    /**
     * Withdraw the Network-LSA for this peer by prematurely aging.
     */
    bool withdraw_network_lsa(PeerID peer, OspfTypes::RouterID link_state_id);

    /**
     * Refresh the Network-LSAs.
     */
    void refresh_network_lsa(PeerID peerid, Lsa::LsaRef lsar);

    /**
     * Add a network to be announced.
     */
//     void add_network(PeerID peer, IPNet<A> network);

    /**
     * Remove a network to be announced.
     */
//     void remove_network(PeerID peer, IPNet<A> network);

    /**
     * @return the type of this area.
     */
    OspfTypes::AreaType get_area_type() const { return _area_type; }

    /**
     * Get the options that are sent in hello packets, data
     * description packets, LSA headers (OSPFv2),  Router-LSAs
     * (OSPFv3) and Network-LSAs (OSPFv3).
     */
    uint32_t get_options() {
	return _ospf.get_peer_manager().compute_options(get_area_type());
    }

    /**
     * Receive LSAs
     * 
     * @param peerid that the LSAs arrived on.
     * @param nid neighbourID that the LSAs arrived on.
     * @param lsas list of recived lsas.
     * @param direct_ack list of direct acks to send in response to the LSA
     * @param delayed_ack list of delayed acks to send in response to the LSA
     * @param backup true if the receiving interface was in state backup.
     * @param dr true if the LSA was received from the designated router.
     */
    void receive_lsas(PeerID peerid, OspfTypes::NeighbourID nid,
		      list<Lsa::LsaRef>& lsas, 
		      list<Lsa_header>& direct_ack,
		      list<Lsa_header>& delayed_ack,
		      bool backup, bool dr);

    /**
     * Returned by compare_lsa.
     */
    enum LsaSearch {
	NOMATCH,	// No matching LSA was found.
	EQUIVALENT,	// The two LSAs are considered equivalent.
	NEWER,		// The offered LSA is newer than the database copy.
	OLDER,		// The offered LSA is older than the database copy.
    };

    /**
     * Compare two LSAs.
     *
     * @param candidate offered LSA
     * @param current equivalent to the database copy.
     *
     * @return LsaSearch that describes the type of match.
     */
    LsaSearch compare_lsa(const Lsa_header& candidate,
			  const Lsa_header& current) const;

    /**
     * Compare this LSA to 
     *
     * @param Lsa_header that is being sought.
     * 
     * @return LsaSearch that describes the type of match.
     */
    LsaSearch compare_lsa(const Lsa_header&) const;

    /**
     * @return true if this is a newer LSA than we already have.
     */
    bool newer_lsa(const Lsa_header&) const;

    /**
     * Fetch a list of lsas given a list of requests.
     *
     * The age fields of the returned LSAs will be correctly set.
     *
     * @param requests list of requests
     * @param lsas list of LSAs
     *
     * @return True if *all* the requests have been satisfied. If an LSA
     * can not be found False is returned and the state of the lsas
     * list is undefined; hence should not be used.
     * 
     */
    bool get_lsas(const list<Ls_request>& requests,
		  list<Lsa::LsaRef>& lsas);

    /**
     * Open database
     *
     * Used only by the peer to generate the database description packets.
     *
     * @param empty true if the database is empty.
     * @return Database Handle
     */
    DataBaseHandle open_database(bool& empty);

    /**
     * Is this a valid entry to be returned by the database.
     *
     * This method is for internal use and its use is not recommended.
     *
     * @return true if this entry is valid.
     */
    bool valid_entry_database(size_t index);

    /**
     * Is there another database entry following this one.
     *
     * This method is for internal use and its use is not recommended.
     *
     * @return true if there is a subsequent entry.
     */
    bool subsequent(DataBaseHandle& dbh);

    /**
     * Next database entry
     *
     * @param last true if this is the last entry.
     *
     * @return The next LSA in the database.
     */
    Lsa::LsaRef get_entry_database(DataBaseHandle& dbh, bool& last);

    /**
     * Close the database
     *
     * @param dbd Database descriptor
     */
    void close_database(DataBaseHandle& dbh);

    /**
     * Is this the backbone area?
     */
    bool backbone() const {
	return OspfTypes::BACKBONE == _area;
    }

    /**
     * Print link state database.
     */
    void testing_print_link_state_database() const;

    /**
     * Testing entry point to add this router Router-LSA to the
     * database replacing the one that is already there.
     */
    bool testing_replace_router_lsa(Lsa::LsaRef lsar) {
	RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
	XLOG_ASSERT(rlsa);
	XLOG_ASSERT(rlsa->get_self_originating());
	XLOG_ASSERT(_ospf.get_router_id() ==
		    rlsa->get_header().get_link_state_id());
	XLOG_ASSERT(_ospf.get_router_id() ==
		    rlsa->get_header().get_advertising_router());

	size_t index;
	if (find_lsa(_router_lsa, index)) {
	    delete_lsa(_router_lsa, index, true);
	}
	_router_lsa = lsar;
	add_lsa(_router_lsa);
	return true;
    }

    /**
     * Testing entry point to add an LSA to the database.
     */
    bool testing_add_lsa(Lsa::LsaRef lsar) {
	return add_lsa(lsar);
    }

    /**
     * Testing entry point to delete an LSA from the database.
     */
    bool testing_delete_lsa(Lsa::LsaRef lsar) {
	size_t index;
	if (find_lsa(lsar, index)) {
	    delete_lsa(lsar, index, true);
	    return true;
	}
	XLOG_FATAL("Attempt to delete LSA that is not in database \n%s",
		   cstring(*lsar));
	return false;
    }

    /**
     * Testing enty point to force a toal routing computation.
     */
    void testing_routing_total_recompute() {
	routing_total_recompute();
    }

    string str() {
	return "Area " + pr_id(_area);
    }

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    OspfTypes::AreaID _area;		// Area: That is represented.
    OspfTypes::AreaType _area_type;	// Type of this area.
    bool _summaries;			// True if summaries should be
					// generated into a stub area.
    uint32_t _stub_default_cost;	// The cost of the default
					// route that is injected into
					// a stub area.

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
    Spt<Vertex> _spt;			// SPT computation unit.
#endif

    Lsa::LsaRef _invalid_lsa;		// An invalid LSA to overwrite slots

    Lsa::LsaRef _router_lsa;		// This routers router LSA.
    vector<Lsa::LsaRef> _db;		// Database of LSAs.
    deque<size_t> _empty_slots;		// Available slots in the Database.
    uint32_t _last_entry;		// One past last entry in
					// database. A value of 0 is
					// an empty database.
    uint32_t _allocated_entries;	// Number of allocated entries.
    
    uint32_t _readers;			// Number of database readers.
    
    DelayQueue<Lsa::LsaRef> _queue;	// Router LSA queue.

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
    uint32_t _TransitCapability;	// Used by the spt computation.

    // XXX - This needs a better name.
    struct Bucket {
	Bucket(size_t index, bool known) : _index(index), _known(known)
	{}
	size_t _index;
	bool _known;
    };

    list<Bucket> _new_lsas;		// Indexes of new LSAs that
					// have arrived recently.
#else
    bool _TransitCapability;		// Does this area support
					// transit traffic?

    void set_transit_capability(bool t) {
	_TransitCapability = t;
    }

    bool get_transit_capability() const {
	return _TransitCapability;
    }
#endif

    /**
     * Internal state that is required about each peer.
     */
    struct PeerState {
	PeerState()
	    : _up(false)
	{}
	bool _up;			// True if peer is enabled.
	list<RouterLink> _router_links;	// Router links for this peer
    };

    typedef ref_ptr<PeerState> PeerStateRef;
    typedef map<PeerID, PeerStateRef> PeerMap;
    PeerMap _peers;		// Peers of this area.

    uint32_t _routing_recompute_delay;	// How many seconds to wait
					// before recompting.
    XorpTimer _routing_recompute_timer;	// Timer to cause recompute.
    
    /**
     * Range to be summarised or suppressed from other areas.
     */
    struct Range {
	bool _advertise;	// Should this range be advertised.
    };

    Trie<A, Range> _area_range;	// Area range for summary generation.

    /**
     * Set the network components of the LSA.
     */
    void summary_network_lsa_set_net(SummaryNetworkLsa *snlsa, IPNet<A> net);

    /**
     * Generate a Summary-LSA for an intra area path taking into
     * account area ranges. An announcement may not generate an LSA
     * and a withdraw may not cause an LSA to be removed.
     */
    Lsa::LsaRef summary_network_lsa_intra_area(OspfTypes::AreaID area,
					       IPNet<A> net,
					       RouteEntry<A>& rt,
					       bool announce);
    /**
     * Construct a summary LSA if appropriate.
     * @param announce true if this in an announce false if its a withdraw.
     * @return the LSA can be empty.
     */
    Lsa::LsaRef summary_build(OspfTypes::AreaID area, IPNet<A> net,
			      RouteEntry<A>& rt, bool announce);

    /**
     * Start aging LSA.
     *
     * All non self originated LSAs must be aged and when they reach
     * MAXAGE flooded and flushed.
     */
    bool age_lsa(Lsa::LsaRef lsar);

    /**
     * This LSA has reached MAXAGE so flood it to all the peers of
     * this area. If its an external LSA flood it to all areas.
     */
    void maxage_reached(Lsa::LsaRef lsar, size_t index);

    /**
     * Prematurely age self originated LSAs, remove them from the
     * database flood with age set to MAXAGE.
     */
    void premature_aging(Lsa::LsaRef lsar, size_t index);

    /**
     * Add this LSA to the database.
     *
     * The LSA must have an updated age field. 
     *
     * @param lsar LSA to add.
     * @return true on success
     */
    bool add_lsa(Lsa::LsaRef lsar);

    /**
     * Delete this LSA from the database.
     *
     * @param lsar LSA to delete.
     * @param index into database.
     * @param invalidate if true as well as removing from the database
     * mark the LSA as invalid.
     * @return true on success
     */
    bool delete_lsa(Lsa::LsaRef lsar, size_t index, bool invalidate);

    /**
     * Update this LSA in the database.
     *
     * @param lsar LSA to update.
     * @param index into database.
     * @return true on success
     */
    bool update_lsa(Lsa::LsaRef lsar, size_t index);

    /**
     * Find LSA matching this request.
     *
     * @param lsr that is being sought.
     * @param index into LSA database if search succeeded.
     *
     * @return true if an LSA was found. 
     */
    bool find_lsa(const Ls_request& lsr, size_t& index) const;

    /**
     * Find LSA matching this request.
     *
     * @param lsar that is being sought.
     * @param index into LSA database if search succeeded.
     *
     * @return true if an LSA was found. 
     */
    bool find_lsa(Lsa::LsaRef lsar, size_t& index) const;

    /**
     * Find Network-LSA.
     *
     * @param link_state_id
     * @param index into LSA database if search succeeded.
     *
     * @return true if an LSA was found. 
     */
    bool find_network_lsa(uint32_t link_state_id, size_t& index) const;

    /**
     * Compare this LSA to 
     *
     * @param Lsa_header that is being sought.
     * @param index into LSA database if search succeeded.
     * 
     * @return LsaSearch that describes the type of match.
     */
    LsaSearch compare_lsa(const Lsa_header&, size_t& index) const;

    /**
     * Update router links.
     *
     * A peer has just changed state, or the refresh timer has fired,
     * so update the router lsa and publish.
     *
     * @return true if something changed.
     */
    bool update_router_links();

    /**
     * Refresh Router-LSA.
     *
     * Called from the refresh timer.
     */
    void refresh_router_lsa();

    /*
     * Send this LSA to all our peers.
     *
     * @param peerid The peer this LSA arrived on.
     * @param nid The neighbour this LSA arrived on so don't reflect.
     * @param lsar The LSA to publish
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     */
    void publish(const PeerID peerid, const OspfTypes::NeighbourID nid,
		 Lsa::LsaRef lsar, bool& multicast_on_peer) const;

    /*
     * Send this LSA to all our peers.
     *
     * @param lsar The LSA to publish
     */
    void publish_all(Lsa::LsaRef lsar);

    /**
     * Send (push) any queued LSAs.
     */
    void push_lsas();

    /**
     * Send this LSA to all area's.
     *
     * This is an External-LSA being sent to other areas.
     *
     * @param lsar The LSA to publish
     * @param add if true add to the database as well as flooding, if
     * false it is already in the database remove and flood.
     */
    void flood_all_areas(Lsa::LsaRef lsar, bool add);

    /**
     * Notify all areas this is the last of the all areas LSAs.
     */
    void push_all_areas();

    /**
     * @return true if any of the neigbours are in state Exchange or Loading.
     */
    bool neighbours_exchange_or_loading() const;

    /**
     * Is this LSA on this neighbours link state request list.
     * @param peerid
     * @param nid
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(const PeerID peerid,
				    const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar) const;

    /**
     * Generate a BadLSReq event.
     *
     * @param peerid
     * @param nid
     */
    bool event_bad_link_state_request(const PeerID peerid,
				      const OspfTypes::NeighbourID nid) const;

    /**
     * Send this LSA directly to the neighbour. Do not place on
     * retransmission list.
     *
     * @param peerid
     * @param nid
     * @param lsar
     *
     * @return true on success
     */
    bool send_lsa(const PeerID peerid, const OspfTypes::NeighbourID nid,
		  Lsa::LsaRef lsar) const;

    /**
     * Check this LSA to see if its link state id matched any of the
     * routers interfaces.
     *
     * @param lsar LSA to check.
     * A dummy argument is used to force an IPv4 and an IPv6 instance
     * of this method to be generated. Isn't C++ cool?
     */
    bool self_originated_by_interface(Lsa::LsaRef lsar, A = A::ZERO()) const;

    /**
     * If this is a self-originated LSA do the appropriate processing.
     * RFC 2328 Section 13.4. Receiving self-originated LSAs
     *
     * @param lsar received LSA.
     * @param match if true this LSA has matched a self-originated LSA
     * already in the database.
     * @param index if match is true the index into the database for
     * the matching LSA.
     *
     * @return true if this is a self-orignated LSA.
     */
    bool self_originated(Lsa::LsaRef lsar, bool match, size_t index);

    /**
     * Create a vertex for this router.
     */
    void RouterVertex(Vertex& v);

    /**
     * Prepare for routing changes.
     */
    void routing_begin();

    /**
     * Add this LSA to the routing computation.
     *
     * The calls to this method are bracketed between a call to
     * routing begin and routing end.
     *
     * @param lsar LSA to be added to the database.
     * @param known true if this LSA is already in the database.
     */
    void routing_add(Lsa::LsaRef lsar, bool known);

    /**
     * Remove this LSA from the routing computation.
     *
     * The calls to this method are *NOT* bracketed between a call to
     * routing begin and routing end.
     */
    void routing_delete(Lsa::LsaRef lsar);

    /**
     * Routing changes are completed.
     * 1) Update the routing table if necessary.
     * 2) Possibly generate new summary LSAs.
     */
    void routing_end();

    /**
     * Schedule recomputing the whole table.
     */
    void routing_schedule_total_recompute();

    /**
     * Callback routine that causes route recomputation.
     */
    void routing_timer();

    /**
     * Totally recompute the routing table from the LSA database.
     */
    void routing_total_recompute();
    void routing_total_recomputeV2();
    void routing_total_recomputeV3();

    /**
     * Compute the discard routes related to area ranges.
     */
    void routing_area_ranges(list<RouteCmd<Vertex> >& r);
    void routing_area_rangesV2(list<RouteCmd<Vertex> >& r);
    void routing_area_rangesV3(list<RouteCmd<Vertex> >& r);

    /**
     * Compute the inter-area routes.
     */
    void routing_inter_area();
    void routing_inter_areaV2();
    void routing_inter_areaV3();

    /**
     * Compute the transit area routes.
     */
    void routing_transit_area();
    void routing_transit_areaV2();
    void routing_transit_areaV3();

    /**
     * Compute the AS external routes.
     */
    void routing_as_external();
    void routing_as_externalV2();
    void routing_as_externalV3();

    /**
     * Does this Router-LSA point back to the router link that points
     * at it.
     *
     * @param rl_type type of link p2p or vlink
     * @param link_state_id from RouterLSA that points at rlsa.
     * @param rl link from RouterLSA that points at rlsa.
     * @param rlsa that is pointed at by previous two arguments.
     * @param metric (out argument) from rlsa back to link_state_id if the back
     * pointer exists.
     * @param interface_address (out argument) if the back pointer exists.
     *
     * @return true if the back pointer exists also fill in the metric
     * and interface address.
     * value.
     */
    bool bidirectional(RouterLink::Type rl_type,
		       const uint32_t link_state_id,
		       const RouterLink& rl,
		       RouterLsa *rlsa,
		       uint16_t& metric,
		       uint32_t& interface_address);

    /**
     * Does this Network-LSA point back to the router link that points
     * at it.
     */
    bool bidirectional(const RouterLink& rl, NetworkLsa *nlsa);

    /**
     * Add this newly arrived or changed Router-LSA to the SPT.
     */
    void routing_router_lsaV2(Spt<Vertex>& spt, const Vertex& src,
			      RouterLsa *rlsa);

    void routing_router_link_p2p_vlinkV2(Spt<Vertex>& spt, const Vertex& src,
					 RouterLsa *rlsa, RouterLink rl);

    void routing_router_link_transitV2(Spt<Vertex>& spt, const Vertex& src,
				       RouterLsa *rlsa, RouterLink rl);

    void routing_router_link_stubV2(Spt<Vertex>& spt, const Vertex& src,
				    RouterLsa *rlsa, RouterLink rl);

    /**
     * Add this newly arrived or changed Router-LSA to the SPT.
     */
    void routing_router_lsaV3(Spt<Vertex>& spt, const Vertex& v,
			      RouterLsa *rlsa);

};

/**
 * DataBase Handle
 *
 * Holds state regarding position in the database
 */
class DataBaseHandle {
 public:
    DataBaseHandle() : _position(0), _last_entry(0), _valid(false)
    {}

    DataBaseHandle(bool v, uint32_t last_entry)
	: _position(0), _last_entry(last_entry), _valid(v)
    {}

    uint32_t position() const {
	XLOG_ASSERT(valid());
	return _position;
    }

    uint32_t last() const {
	XLOG_ASSERT(valid());
	return _last_entry;
    }

    void advance(bool& last) { 
	XLOG_ASSERT(valid());
	XLOG_ASSERT(_last_entry != _position);
	_position++; 
	last = _last_entry == _position;
    }

    bool valid() const { return _valid; }

    void invalidate() { _valid = false; }

 private:
    uint32_t _position;		// Position in database.
    uint32_t _last_entry;	// One past last entry, for an empty
				// database value would be 0.
    bool _valid;		// True if this handle is valid.
};

#endif // __OSPF_AREA_ROUTER_HH__
