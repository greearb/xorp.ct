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

class DataBaseHandle;

/**
 * Area Router
 * 
 */
template <typename A>
class AreaRouter {
 public:
    AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
	       OspfTypes::AreaType area_type, uint32_t options);

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
     * Add router link
     *
     * Advertise this router link.
     */
    bool add_router_link(PeerID peer, RouterLink& router_link);

    /**
     * Remove router link
     *
     * Stop advertising this router link.
     */
    bool remove_router_link(PeerID peer);

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
		  list<Lsa::LsaRef>& lsas) const;

    /**
     * Open database
     *
     * @param empty true if the database is empty.
     * @return Database Handle
     */
    DataBaseHandle open_database(bool& empty);

    /**
     * Is there another database entry following this one.
     *
     * This method is for internal use and its use is not recommended.
     *
     * @return true if there is a subsequent entry.
     */
    bool subsequent(DataBaseHandle& dbh) const;

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

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    OspfTypes::AreaID _area;		// Area: That is represented.
    OspfTypes::AreaType _area_type;	// Type of this area.
    const uint32_t _options;		// Options that we are sending.

    Spt<Vertex> _spt;			// SPT computation unit.

    Lsa::LsaRef _router_lsa;		// This routers router LSA.
    vector<Lsa::LsaRef> _db;		// Database of LSAs.
    deque<size_t> _empty_slots;		// Available slots in the Database.
    uint32_t _last_entry;		// One past last entry in
					// database. A value of 0 is
					// an empty database.
    uint32_t _allocated_entries;	// Number of allocated entries.
    
    uint32_t _readers;			// Number of database readers.
    
    DelayQueue<Lsa::LsaRef> _queue;	// Router LSA queue.

    /**
     * Internal state that is required about this peer.
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

    /**
     * Add this LSA to the database.
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
     * @return true on success
     */
    bool delete_lsa(Lsa::LsaRef lsar, size_t index);

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
     * A peer has just changed state so update the router lsa and publish.
     *
     * @param peerid peer that changed state.
     * @return true if something changed.
     */
    bool update_router_links(PeerStateRef psr);

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
     */
    void flood_all_areas(Lsa::LsaRef lsar);

    /**
     * Notify all areas this is the last of the all areas LSAs.
     */
    void push_all_areas();

    /**
     *
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
     * Genearate a BadLSReq event.
     *
     * @param peerid
     * @param nid
     */
    void event_bad_link_state_request(const PeerID peerid,
				      const OspfTypes::NeighbourID nid) const;

    /**
     * Send this LSA directly to the neighbour.
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
     * Prepare for routing changes.
     */
    void routing_begin();

    /**
     * Add this LSA to the routing computation.
     */
    void routing_add(Lsa::LsaRef lsar);

    /**
     * Remove this LSA from the routing computation.
     */
    void routing_delete(Lsa::LsaRef lsar);

    /**
     * Routing changes are completed.
     * 1) Update the routing table if necessary.
     * 2) Possibly generate new sumamry LSAs.
     */
    void routing_end();
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
