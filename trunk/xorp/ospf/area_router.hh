// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/ospf/area_router.hh,v 1.132 2007/08/17 22:18:47 atanu Exp $

#ifndef __OSPF_AREA_ROUTER_HH__
#define __OSPF_AREA_ROUTER_HH__

class DataBaseHandle;
class LsaTempStore;

/**
 * Area Router
 * 
 */
template <typename A>
class AreaRouter : public ServiceBase {
 public:
    AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
	       OspfTypes::AreaType area_type);

    /**
     * Required by the class Subsystem.
     * Called on startup.
     */
    bool startup();

    /**
     * Required by the class Subsystem.
     * Called on shutdown.
     */
    bool shutdown();

    /**
     * Add peer
     */
    void add_peer(OspfTypes::PeerID peer);

    /**
     * Delete peer
     */
    void delete_peer(OspfTypes::PeerID peer);

    /**
     * Peer came up
     */
    bool peer_up(OspfTypes::PeerID peer);

    /**
     * Peer went down
     */
    bool peer_down(OspfTypes::PeerID peer);

    /**
     * Track border router transitions.
     *
     * @param up true of the router just became an area border router,
     * false if the router was an area border router and is no longer.
     */
    void area_border_router_transition(bool up);

    /**
     * Change the type of this area.
     */
    void change_area_router_type(OspfTypes::AreaType area_type);

    /**
     * Given an advertising router and type find a global address if
     * present in its associated Intra-Area-Prefix-LSA if present,
     * OSPFv3 only.
     *
     * @param adv advertising router.
     * @param type of Intra-Area-Prefix-LSA (Router-LSA or
     * Network-LSA), Router-LSA expected.
     * @param lsa_temp_store store of all possible Router-LSAs.
     * @param global_address (out) argument.
     *
     * @return true if global address found.
     */
    bool find_global_address(uint32_t adv, uint16_t type,
			     LsaTempStore& lsa_temp_store,
			     A& global_address) const;

    /**
     * @return true if any virtual links are configured through this area.
     */
    bool configured_virtual_link() const;

    /**
     * Add a virtual link endpoint.
     */
    bool add_virtual_link(OspfTypes::RouterID rid);

    /**
     * Remove a virtual link endpoint.
     */
    bool remove_virtual_link(OspfTypes::RouterID rid);

    /**
     * Start looking through the list of routers for a virtual link endpoint.
     */
    void start_virtual_link();

    /**
     * Check this node to see if its a virtual link endpoint.
     *
     * @param rc node under consideration.
     * @param router this router's Router-LSA.
     */
    void check_for_virtual_linkV2(const RouteCmd<Vertex>& rc,
				  Lsa::LsaRef lsar);

    /**
     * Check this node to see if its a virtual link endpoint.
     *
     * @param rc node under consideration.
     * @param lsar RouterLSA belonging to the router under consideration.
     * @param lsa_temp_store store of all possible Router-LSAs.
     */
    void check_for_virtual_linkV3(const RouteCmd<Vertex>& rc,
				  Lsa::LsaRef lsar,
				  LsaTempStore& lsa_temp_store);

    /**
     * End looking through the list of routers for a virtual link endpoint.
     */
    void end_virtual_link();


    /**
     * Given two LSAs find the interface address of the destination
     * LSA. The source LSA can be a Router-LSA or a Network-LSA the
     * destination LSA must be a Router-LSA.
     */
    bool find_interface_address(Lsa::LsaRef src, Lsa::LsaRef dst,
				A& interface)const;

    /**
     * OSPFv3 only.
     * Given a Router ID and interface ID find the associated Link-LSA
     * if present and return the Link-local Interface Address.
     */
    bool find_interface_address(OspfTypes::RouterID rid, uint32_t interface_id,
				A& interface);

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
     * This network falls in a covered area range, return the covering
     * range.
     */
    bool area_range_covering(IPNet<A> net, IPNet<A>& sumnet);

    /**
     * Does this area have any area ranges configured.
     */
    bool area_range_configured();

    /**
     * If this is a "stub" or "nssa" area toggle the sending of a default
     *  route.
     */
    bool originate_default_route(bool enable);

    /**
     *  Set the StubDefaultCost, the default cost sent in a default route in a
     *  "stub" or "nssa" area.
     */
    bool stub_default_cost(uint32_t cost);

    /**
     *  Toggle the sending of summaries into "stub" or "nssa" areas.
     */
    bool summaries(bool enable);

    /**
     * get lsa at index if it exists.
     */
    bool get_lsa(const uint32_t index, bool& valid, bool& toohigh, bool& self,
		 vector<uint8_t>& lsa);
    
    /**
     * A new set of router links.
     */
    bool new_router_links(OspfTypes::PeerID peer,
			  const list<RouterLink>& router_link);

    /**
     * Refresh Router-LSA.
     *
     * Cause the generation of a new Router-LSA if necessary.
     *
     * @param timer true if called by the timer.
     */
    void refresh_router_lsa(bool timer = false);

    /**
     * For OSPFv3 only there are no router links describing stub
     * networks, Intra-Area-Prefix-LSAs that reference the Router-LSA
     * have to be generated.
     */
    void stub_networksV3(bool timer);

    /**
     * A new route has been added to the routing table it is being
     * presented to this area for possible Summary-LSA generation.
     *
     * @param area the route came from
     * @param net
     * @param rt routing entry.
     * @param push true if the routes are arriving as a consquence of
     * calling summary_push()
     */
    void summary_announce(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt, bool push);

    /**
     * A route has been deleted from the routing table. It may
     * previously have caused a Summary-LSA which now needs to be
     * withdrawn.
     */
    void summary_withdraw(OspfTypes::AreaID area, IPNet<A> net,
			  RouteEntry<A>& rt);

     /**
      * @return true if this area should accept an AS-External-LSA or
      * a Type-7-LSA.
      */
    bool external_area_type() const;
 
    /**
     * Copy the net and nexthop information from one AS-External-LSA to
     * another.
     *
     * The first dummy argument A is to allow template specialisation
     * by address family.
     */
    void external_copy_net_nexthop(A,
				   ASExternalLsa *dst,
				   ASExternalLsa *src);

    /**
     * Given an AS-External-LSA generate a Type-7 LSA.
     *
     * @param indb if true the Type-7-LSA is already in the database.
     */
    Lsa::LsaRef external_generate_type7(Lsa::LsaRef lsar, bool& indb);

    /**
     * Given a Type-7 LSA generate an AS-External-LSA.
     */
    Lsa::LsaRef external_generate_external(Lsa::LsaRef lsar);

    /**
     * An AS-External-LSA being announced either from another area or
     * from the RIB as a redist.
     *
     * The LSAs should not be scheduled for transmission until the
     * external_announce_complete() is seen. In many cases a number of
     * LSAs may arrive in a single packet, waiting for the
     * external_announce_complete() offers an opportunity for
     * aggregation.
     *
     * @param lsar the AS-External-LSA
     * @param push set to true if the push is a result of an external_push().
     * @param redist true if this LSA was locally generated due to a
     * redistribution.
     */
    void external_announce(Lsa::LsaRef lsar, bool push, bool redist);

    /**
     * Called to complete a series of calls to external_announce().
     */
    void external_announce_complete();

    /**
     * Refresh this LSA either because a timer has expired or because
     * a newer LSA has arrived from another area. In either cause the
     * LSA should already be in this area's database.
     */
    void external_refresh(Lsa::LsaRef lsar);

    /**
     * An AS-External-LSA being withdrawn either from another area or
     * from the RIB as a redist.
     *
     * @param lsar the AS-External-LSA
     */
    void external_withdraw(Lsa::LsaRef lsar);

    /**
     * Add a Link-LSA for this peer.
     */
    bool add_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar);

    /**
     * Update the Link-LSA for this peer.
     */
    bool update_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar);

    /**
     * Withdraw the Link-LSA for this peer.
     */
    bool withdraw_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar);

    /**
     * Refresh the Link-LSA for this peer.
     */
    void refresh_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar);

    /**
     * Generate a Network-LSA for this peer.
     */
    bool generate_network_lsa(OspfTypes::PeerID peer,
			      OspfTypes::RouterID link_state_id,
			      list<RouterInfo>& attached_routers,
			      uint32_t network_mask);

    /**
     * Update the Network-LSA for this peer.
     */
    bool update_network_lsa(OspfTypes::PeerID peer,
			    OspfTypes::RouterID link_state_id,
			    list<RouterInfo>& attached_routers,
			    uint32_t network_mask);

    /**
     * Withdraw the Network-LSA for this peer by prematurely aging.
     */
    bool withdraw_network_lsa(OspfTypes::PeerID peer,
			      OspfTypes::RouterID link_state_id);

    /**
     * Refresh the Network-LSAs.
     *
     * @param peerid the peer that needs its Network-LSA refreshed.
     * @param lsar the Network-LSA itself.
     * @param timer is the Network-LSA being refreshed due to the
     * timer firing?
     */
    void refresh_network_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar,
			     bool timer = false);

    /**
     * A new Link-LSA has arrived if this router is the designated
     * router then it may be necessary to generate a new
     * Intra-Area-Prefix-LSA.
     */
    void check_link_lsa(OspfTypes::PeerID peerid, LinkLsa *nllsa,
			LinkLsa *ollsa);

    /**
     * Generate a Intra-Area-Prefix-LSA for this peer OSPFv3 only and
     * add it to the database.
     *
     * @param peerid the peer that the generated Intra-Area-Prefix-LSA
     * belongs to.
     * @param lsar the LSA that is referenced by the
     * Intra-Area-Prefix-LSA.
     * @param interface_id that the generated Intra-Area-Prefix-LSA
     * belongs to.
     */
    bool generate_intra_area_prefix_lsa(OspfTypes::PeerID peerid,
					Lsa::LsaRef lsar,
					uint32_t interface_id);

    /**
     * OSPFv3 only.
     * Find the Link-LSA (if it exists) specified by the the tuple LS
     * type, Link State ID and Router ID, and add to the prefix
     * list. If the prefix is already on the list just or in the
     * options field.
     *
     * @return the options associated with this Link-LSA if present
     * otherwise zero.
     */
    uint32_t populate_prefix(OspfTypes::PeerID peeridid,
			     uint32_t interface_id, 
			     OspfTypes::RouterID router_id,
			     list<IPv6Prefix>& prefixes);
    
    /**
     * Update the Intra-Area-Prefix-LSA for this peer OSPFv3 only.
     *
     * @param peerid the peer that needs its Intra-Area-Prefix-LSA refreshed.
     * @param referenced_ls_type
     * @param interface_id that the generated Intra-Area-Prefix-LSA
     * belongs to.
     * @param attached_routers list of fully attached routers.
     *
     * @return the options fields from all the Link-LSAs or'd together.
     */
    uint32_t update_intra_area_prefix_lsa(OspfTypes::PeerID peer,
					  uint16_t referenced_ls_type,
					  OspfTypes::RouterID interface_id,
					  const list<RouterInfo>& 
					  attached_routers);

    /**
     * Withdraw the Intra-Area-Prefix-LSA for this peer by prematurely
     * aging OSPFv3 only.
     *
     * @param peerid the peer that needs its Intra-Area-Prefix-LSA refreshed.
     * @param referenced_ls_type
     * @param interface_id that the generated Intra-Area-Prefix-LSA
     * belongs to.
     */
    bool withdraw_intra_area_prefix_lsa(OspfTypes::PeerID peer,
					uint16_t referenced_ls_type,
					uint32_t interface_id);
					

    /**
     * Refresh the Intra-Area-Prefix-LSA OSPFv3 only.
     *
     * NOT IMPLEMENTED.
     *
     * @param peerid the peer that needs its Intra-Area-Prefix-LSA refreshed.
     * @param referenced_ls_type
     * @param interface_id that the generated Intra-Area-Prefix-LSA
     * belongs to.
     */
    void refresh_intra_area_prefix_lsa(OspfTypes::PeerID peerid,
				       uint16_t referenced_ls_type,
				       uint32_t interface_id);

    /**
     * Create an LSA that will be used to announce the default route
     * into "stub" and "nssa" areas.
     */
    void generate_default_route();

    /**
     * Find the default route LSA in the database if it exists.
     */
    bool find_default_route(size_t& index);

    // Temporary storage used by save_default_route() and
    // restore_default_route().
    Lsa::LsaRef _saved_default_route;
    
    /**
     * If the default route LSA is in the database remove it.
     * Typically to stop it being purged when the area type changes or
     * summarisation is disable.
     */
    void save_default_route();

    /**
     * If the default route LSA should be in the database put it
     * back. Either from the previously saved or generate a new one if
     * necessary. Typically paired with save_default_route().
     */
    void restore_default_route();

    /**
     * Withdraw the default route LSA if it exists.
     * Set the LSA to MaxAge and floods.
     */
    void withdraw_default_route();

    /**
     * Refresh the default route LSA.
     * Increments the sequence and floods updates the cost if it has
     * changed.
     */
    void refresh_default_route();

    /**
     * Add a network to be announced.
     */
//     void add_network(OspfTypes::PeerID peer, IPNet<A> network);

    /**
     * Remove a network to be announced.
     */
//     void remove_network(OspfTypes::PeerID peer, IPNet<A> network);

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
     * @param is_router_bdr true if the receiving interface was in
     * state backup.
     * @param is_neighbour_dr true if the LSA was received from the
     * designated router.
     */
    void receive_lsas(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		      list<Lsa::LsaRef>& lsas, 
		      list<Lsa_header>& direct_ack,
		      list<Lsa_header>& delayed_ack,
		      bool is_router_bdr, bool is_neighbour_dr);

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
     * @param peerid
     * @param empty true if the database is empty.
     * @return Database Handle
     */
    DataBaseHandle open_database(OspfTypes::PeerID peerid, bool& empty);

    /**
     * Is this a valid entry to be returned by the database.
     *
     * This method is for internal use and its use is not recommended.
     *
     * @return true if this entry is valid.
     */
    bool valid_entry_database(OspfTypes::PeerID peerid, size_t index);

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
     * Clear the database.
     */
    void clear_database();

    /**
     * All self originated LSAs of this type MaxAge them.
     */
    void maxage_type_database(uint16_t type);

    /**
     * Is this the backbone area?
     */
    bool backbone() const {
	return OspfTypes::BACKBONE == _area;
    }

    bool backbone(OspfTypes::AreaID area) const {
	return OspfTypes::BACKBONE == area;
    }
#ifndef	UNFINISHED_INCREMENTAL_UPDATE
    bool get_transit_capability() const {
	return _TransitCapability;
    }
#endif

    /**
     * Totally recompute the routing table from the LSA database.
     */
    void routing_total_recompute();

    /**
     * Testing entry point to force a total routing computation.
     */
    void testing_routing_total_recompute() {
	routing_total_recompute();
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
	switch (_ospf.get_version()) {
	case OspfTypes::V2:
	    XLOG_ASSERT(_ospf.get_router_id() ==
			rlsa->get_header().get_link_state_id());
	    break;
	case OspfTypes::V3:
	    break;
	}
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

    string str() {
	return "Area " + pr_id(_area);
    }

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    OspfTypes::AreaID _area;		// Area: That is represented.
    OspfTypes::AreaType _area_type;	// Type of this area.
    map<OspfTypes::RouterID,bool> _vlinks;	// Virtual link endpoints.
    set<OspfTypes::RouterID> _tmp;	// temporary storage for
					// virtual links.
    bool _summaries;			// True if summaries should be
					// generated into a stub area.
    bool _stub_default_announce;	// Announce a default route into
					// stub or nssa
    uint32_t _stub_default_cost;	// The cost of the default
					// route that is injected into
					// a stub area.
    bool _external_flooding;		// True if AS-External-LSAs
					// are being flooded.

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

    XorpTimer _reincarnate_timer;	// Wait for the LSAs to be purged.
    list<Lsa::LsaRef> _reincarnate;	// Self-originated LSAs where
					// the sequence number has
					// reached MaxSequenceNumber.

    uint32_t _lsid;			// OSPFv3 only next Link State ID.
    map<IPNet<IPv6>, uint32_t> _lsmap; 	// OSPFv3 only

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
    typedef map<OspfTypes::PeerID, PeerStateRef> PeerMap;
    PeerMap _peers;		// Peers of this area.

    uint32_t _routing_recompute_delay;	// How many seconds to wait
					// before recompting.
    XorpTimer _routing_recompute_timer;	// Timer to cause recompute.
    
    // How to handle Type-7 LSAs at the border.
    OspfTypes::NSSATranslatorRole _translator_role;
    OspfTypes::NSSATranslatorState _translator_state;
    bool _type7_propagate;		// How to set the propagate bit.

    /**
     * Range to be summarised or suppressed from other areas.
     */
    struct Range {
	bool _advertise;	// Should this range be advertised.
    };

    Trie<A, Range> _area_range;	// Area range for summary generation.

    /**
     * Networks with same network number but different prefix lengths
     * can generate the same link state ID. When generating a new LSA
     * if a collision occurs use: 
     * RFC 2328 Appendix E. An algorithm for assigning Link State IDs
     * to resolve the clash. Summary-LSAs only.
     */
    void unique_link_state_id(Lsa::LsaRef lsar);

    /**
     * Networks with same network number but different prefix lengths
     * can generate the same link state ID. When looking for an LSA
     * make sure that there the lsar that matches the net is
     * found. Summary-LSAs only.
     */
    bool unique_find_lsa(Lsa::LsaRef lsar, const IPNet<A>& net, size_t& index);

    /**
     * Set network and link state ID in a Summary-LSA/Inter-Area-Prefix-LSA.
     */
    void summary_network_lsa_set_net_lsid(SummaryNetworkLsa *snlsa,
					  IPNet<A> net);


    Lsa::LsaRef summary_network_lsa(IPNet<A> net, RouteEntry<A>& rt);

    /**
     * Generate a Summary-LSA for an intra area path taking into
     * account area ranges. An announcement may not generate an LSA
     * and a withdraw may not cause an LSA to be removed.
     */
    Lsa::LsaRef summary_network_lsa_intra_area(OspfTypes::AreaID area,
					       IPNet<A> net,
					       RouteEntry<A>& rt,
					       bool& announce);
    /**
     * Construct a summary LSA if appropriate.
     * @param announce true if this in an announce false if its a withdraw.
     * @return the LSA can be empty.
     */
    Lsa::LsaRef summary_build(OspfTypes::AreaID area, IPNet<A> net,
			      RouteEntry<A>& rt, bool& announce);

    /**
     * Announce this Summary-LSA to all neighbours and refresh it as
     * appropriate.
     */
    void refresh_summary_lsa(Lsa::LsaRef lsar);

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
     * Increment the sequence number of of this LSA, most importantly
     * handle the sequence number reaching MaxSequenceNumber. 
     */
    void increment_sequence_number(Lsa::LsaRef lsar);

    /**
     * Update the age and increment the sequence number of of this
     * LSA, most importantly handle the sequence number reaching
     * MaxSequenceNumber.
     */
    void update_age_and_seqno(Lsa::LsaRef lsar, const TimeVal& now);

    /**
     * Process an LSA where the sequence number has reached MaxSequenceNumber.
     */
    void max_sequence_number_reached(Lsa::LsaRef lsar);

    /**
     * Reincarnate LSAs that have gone through the MaxSequenceNumber
     * transition. Called on a periodic timer.
     */
    bool reincarnate();

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
     * Find Router-LSA. OSPFv3 only
     *
     * In OSPFv3 a router may generate more that one Router-LSA. In
     * order to find all the Router-LSAs that a router may have
     * generated the index must be seeded.
     *
     * @param advertising_router
     * @param index into LSA database if search succeeded. On input
     * the index should be set to one past the last search result.
     *
     * @return true if an LSA was found. 
     */
    bool find_router_lsa(uint32_t advertising_router, size_t& index) const;

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

    /*
     * Send this LSA to all our peers.
     *
     * @param peerid The peer this LSA arrived on.
     * @param nid The neighbour this LSA arrived on so don't reflect.
     * @param lsar The LSA to publish
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     */
    void publish(const OspfTypes::PeerID peerid,
		 const OspfTypes::NeighbourID nid,
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
     * Return the setting of the propagate bit in a Type-7-LSA.
     */
    bool external_propagate_bit(Lsa::LsaRef lsar) const {
	XLOG_ASSERT(lsar->type7());
	return Options(_ospf.get_version(),lsar->get_header().get_options())
	    .get_p_bit();
    }

    /**
     * Take a Type-7-LSA that has arrived on the wire and translate if
     * required.
     */
    void external_type7_translate(Lsa::LsaRef lsar);

    /**
     * Send this LSA to all area's.
     *
     * This is an AS-External-LSA being sent to other areas.
     *
     * @param lsar The LSA to publish
     */
    void external_flood_all_areas(Lsa::LsaRef lsar);

    /**
     * Notify all areas this is the last of the AS-External-LSAs.
     */
    void external_push_all_areas();

    /**
     * @return true if any of the neigbours are in state Exchange or Loading.
     */
    bool neighbours_exchange_or_loading() const;

    /**
     * @param rid Router ID of neighbouring router.
     *
     * @return true if this router is at least twoway with the
     * specified router.
     */
    bool neighbour_at_least_two_way(OspfTypes::RouterID rid) const;

    /**
     * OSPFv3 Only
     * Neighbour's source address.
     *
     * @param rid Router ID
     * @param interface_id Interface ID.
     * @param neighbour_address set if neighbour is found.
     *
     * @return true if the neighbour is found.
     */
    bool get_neighbour_address(OspfTypes::RouterID rid, uint32_t interface_id,
			       A& neighbour_address)
	const;

    /**
     * Is this LSA on this neighbours link state request list.
     * @param peerid
     * @param nid
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(const OspfTypes::PeerID peerid,
				    const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar) const;

    /**
     * Generate a BadLSReq event.
     *
     * @param peerid
     * @param nid
     */
    bool event_bad_link_state_request(const OspfTypes::PeerID peerid,
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
    bool send_lsa(const OspfTypes::PeerID peerid,
		  const OspfTypes::NeighbourID nid,
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
    void routing_total_recomputeV2();
    void routing_total_recomputeV3();

    /**
     * Add an entry to the routing table making sure that an entry
     * doesn't already exist.
     */
    void routing_table_add_entry(RoutingTable<A>& routing_table,
				 IPNet<A> net,
				 RouteEntry<A>& route_entry);

    /**
     * Compute the discard routes related to area ranges.
     */
    void routing_area_ranges(list<RouteCmd<Vertex> >& r);
    void routing_area_rangesV2(const list<RouteCmd<Vertex> >& r);
    void routing_area_rangesV3(const list<RouteCmd<Vertex> >& r,
			       LsaTempStore& lsa_temp_store);

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


#if	0
    /**
     * Given a Router-LSA and the list of Intra-Area-Prefix-LSAs
     * generated by the same router, return the list of prefixes that
     * are associated with the Router-LSA.
     *
     * @param rlsa Router-LSA
     * @param iapl List of Intra-Area-Prefix-LSAs same router as the Router-LSA
     * @param prefixes (out argument) List if prefixes associated with
     * the Router-LSA.
     */
    bool associated_prefixesV3(const RouterLsa *rlsa,
			       const list<IntraAreaPrefixLsa *>& iapl,
			       list<IPv6Prefix>& prefixes);

    /**
     * Given a Network-LSA and the list of Intra-Area-Prefix-LSAs
     * generated by the same router, return the list of prefixes that
     * are associated with the Network-LSA.
     *
     * @param nlsa Network-LSA
     * @param iapl List of Intra-Area-Prefix-LSAs same router as the
     * Network-LSA
     * @param prefixes (out argument) List if prefixes associated with
     * the Network-LSA.
     */
    bool associated_prefixesV3(NetworkLsa *nlsa,
			       const list<IntraAreaPrefixLsa *>& iapl,
			       list<IPv6Prefix>& prefixes);
#endif

    /**
     * Given a LS type and a referenced link state ID and the list of
     * Intra-Area-Prefix-LSAs generated by the same router, return the
     * list of prefixes that are associated with the LSA.
     *
     * @param ls_type From Router-LSA or Network-LSA
     * @param referenced_link_state_id zero for a Router-LSA and the
     * link state ID from a Network-LSA (in fact its interface ID).
     * @param iapl List of Intra-Area-Prefix-LSAs same router as the Router-LSA
     * @param prefixes (out argument) List if prefixes associated with LSA
     */
    bool associated_prefixesV3(uint16_t ls_type,
			       uint32_t referenced_link_state_id,
			       const list<IntraAreaPrefixLsa *>& lsai,
			       list<IPv6Prefix>& prefixes) const;

    /**
     * RFC 3101 Section 2.5. (6) (e) Calculating Type-7 AS external routes.
     *
     * Return true if the current LSA should be replaced by the
     * candidate LSA.
     */
    bool routing_compare_externals(Lsa::LsaRef current,
				   Lsa::LsaRef candidate) const;

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
    bool bidirectionalV2(RouterLink::Type rl_type,
			 const uint32_t link_state_id,
			 const RouterLink& rl,
			 RouterLsa *rlsa,
			 uint16_t& metric,
			 uint32_t& interface_address);

    /**
     * Does this Network-LSA point back to the router link that points
     * at it.
     *
     * @param link_state_id_or_adv OSPFv2 Link State ID, OSPFv3
     * Advertising Router.
     *
     */
    bool bidirectional(const uint32_t link_state_id_or_adv,
		       const RouterLink& rl,
		       NetworkLsa *nlsa) const;

    /**
     * Does the Router-LSA point at the Network-LSA that points at it.
     *
     * @param interface_address (out argument) if the back pointer exists.
     *
     * @return true if Router-LSA points at the Network-LSA.
     */
    bool bidirectionalV2(RouterLsa *rlsa, NetworkLsa *nlsa,
			 uint32_t& interface_address);

    /**
     * Does the Router-LSA point at the Network-LSA that points at it,
     * OSPFv3.
     *
     * @param interface_id (out argument) interface ID if
     * the Router-LSA points at the Network-LSA.
     * 
     * @return true if Router-LSA points at the Network-LSA.
     */
    bool bidirectionalV3(RouterLsa *rlsa, NetworkLsa *nlsa,
		       uint32_t& interface_id);

    /**
     * Does this Router-LSA point back to the router link that points
     * at it, OSPFv3.
     *
     * @param rl_type type of link p2p or vlink
     * @param advertising_router
     * @param rlsa that is searched for a router link pointing at the
     * advertising router.
     * @param metric (out argument) from rlsa back to advertising
     * router if the back pointer exists.
     *
     * @return true if the Router-LSA points to the advertising router.
     */
    bool bidirectionalV3(RouterLink::Type rl_type,
			 uint32_t advertising_router,
			 RouterLsa *rlsa,
			 uint16_t& metric);

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
    void routing_router_lsaV3(Spt<Vertex>& spt, const Vertex& src,
			      RouterLsa *rlsa);

    void routing_router_link_p2p_vlinkV3(Spt<Vertex>& spt, const Vertex& src,
					 RouterLsa *rlsa, RouterLink rl);

    void routing_router_link_transitV3(Spt<Vertex>& spt, const Vertex& src,
				       RouterLsa *rlsa, RouterLink rl);
};

/**
 * DataBase Handle
 *
 * Holds state regarding position in the database
 */
class DataBaseHandle {
 public:
    DataBaseHandle() : _position(0), _last_entry(0), _valid(false),
		       _peerid(OspfTypes::ALLPEERS)
    {}

    DataBaseHandle(bool v, uint32_t last_entry, OspfTypes::PeerID peerid)
	: _position(0), _last_entry(last_entry), _valid(v),
	  _peerid(peerid)
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

    OspfTypes::PeerID get_peerid() const { return _peerid; }

 private:
    uint32_t _position;		// Position in database.
    uint32_t _last_entry;	// One past last entry, for an empty
				// database value would be 0.
    bool _valid;		// True if this handle is valid.
    OspfTypes::PeerID _peerid;	// The Peer ID that opened the database.
};

/**
 * Router-LSA and Intra-Area-Prefix-LSA store.
 *
 * In OSPFv3 a router can generate multiple Router-LSAs and
 * Intra-Area-Prefix-LSAs, hold a store of the LSAs indexed by router
 * ID.
 * NOTE: LsaTempStore should only be used as temporary storage during
 * the routing computation.
 */
class LsaTempStore {
public:
    void add_router_lsa(Lsa::LsaRef lsar) {
	_router_lsas[lsar->get_header().get_advertising_router()].
	    push_back(lsar);
    }

    list<Lsa::LsaRef>& get_router_lsas(OspfTypes::RouterID rid) {
	return _router_lsas[rid];
    }

    void add_intra_area_prefix_lsa(IntraAreaPrefixLsa *iaplsa) {
	XLOG_ASSERT(iaplsa);
	_intra_area_prefix_lsas[iaplsa->get_header().get_advertising_router()].
	    push_back(iaplsa);
    }

    list<IntraAreaPrefixLsa *>& 
    get_intra_area_prefix_lsas(OspfTypes::RouterID rid) {
	return _intra_area_prefix_lsas[rid];
    }

private:
    map<OspfTypes::RouterID, list<Lsa::LsaRef> > _router_lsas;
    map<OspfTypes::RouterID, list<IntraAreaPrefixLsa *> > 
    _intra_area_prefix_lsas;
};

#endif // __OSPF_AREA_ROUTER_HH__
