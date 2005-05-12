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
	       OspfTypes::AreaType area_type);

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
     * Receive LSA
     */
    void receive_lsa(PeerID peer, Lsa::LsaRef lsa);

    /**
     * @return true if this is a newer LSA than we already have.
     */
    bool newer_lsa(const Lsa_header&) const;

    /**
     * Open data base
     *
     * @return Database Handle
     */
    DataBaseHandle open_database();

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

    Lsa::LsaRef _router_lsa;		// This routers router LSA.
    vector<Lsa::LsaRef> _db;		// Database of LSAs.
    uint32_t _readers;			// Number of database readers.
    
    /**
     * Internal state that is required about this peer.
     */
    struct PeerState {
	PeerState(OspfTypes::Version version)
	    : _up(false),
	      _link_valid(false),
	      _router_link(version)
	{}
	bool _up;		// True if peer is enabled.
	bool _link_valid;	// True if the router link is valid
	RouterLink _router_link;// Router link for this peer
    };

    typedef ref_ptr<PeerState> PeerStateRef;
    typedef map<PeerID, PeerStateRef> PeerMap;
    PeerMap _peers;		// Peers of this area.

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
     */
    void publish(Lsa::LsaRef lsar);
};

/**
 * DataBase Handle
 *
 * Holds state regarding position in the database
 */
class DataBaseHandle {
 public:
    DataBaseHandle() : _position(0)
    {}

    uint32_t position() const { return _position; }

    void advance() { _position++; }
 private:
    uint32_t _position;	// Position in database.
};

#endif // __OSPF_AREA_ROUTER_HH__
