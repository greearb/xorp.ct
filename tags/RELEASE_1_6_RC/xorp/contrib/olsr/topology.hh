// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/contrib/olsr/topology.hh,v 1.2 2008/07/23 05:09:53 pavlin Exp $

#ifndef __OLSR_TOPOLOGY_HH__
#define __OLSR_TOPOLOGY_HH__

class TopologyManager;

/**
 * @short A multiple interface record (MID).
 *
 * There is one MidEntry per interface address advertised by an OLSR peer.
 * As such, a primary address may be associated with multiple MID entries.
 */
class MidEntry {
public:
    MidEntry(EventLoop& ev, TopologyManager* parent,
	     const OlsrTypes::MidEntryID id, const IPv4& iface_addr,
	     const IPv4& main_addr, const uint16_t distance,
	     const TimeVal& vtime)
     : _ev(ev), _parent(parent), _id(id),
       _iface_addr(iface_addr), _main_addr(main_addr),
       _distance(distance)
    {
	update_timer(vtime);
    }

    inline OlsrTypes::MidEntryID id() const { return _id; }
    inline IPv4 iface_addr() const { return _iface_addr; }
    inline IPv4 main_addr() const { return _main_addr; }
    inline uint16_t distance() const { return _distance; }

    inline void set_distance(const uint16_t distance) {
	_distance = distance;
    }

    /**
     * @return the amount of time remaining until this
     *         MID entry expires.
     */
    inline TimeVal time_remaining() const {
	TimeVal tv;
	_expiry_timer.time_remaining(tv);
	return tv;
    }

    /**
     * Update the MidEntry's expiry timer.
     *
     * @param vtime the new value of the expiry timer.
     */
    void update_timer(const TimeVal& vtime);

    /**
     * Callback method to: delete a dying MidEntry.
     */
    void event_dead();

private:
    EventLoop&		_ev;
    TopologyManager*	_parent;
    OlsrTypes::MidEntryID		_id;
    IPv4		_iface_addr;
    IPv4		_main_addr;
    uint16_t		_distance;
    XorpTimer		_expiry_timer;
};

/**
 * @short A topology control record (TC).
 */
class TopologyEntry {
public:
    TopologyEntry(EventLoop& ev, TopologyManager* parent,
		  OlsrTypes::TopologyID id,
		  const IPv4& dest, const IPv4& lasthop,
		  const uint16_t distance,
		  const uint16_t seqno,
		  const TimeVal& vtime)
     : _ev(ev), _parent(parent), _id(id), _destination(dest),
       _lasthop(lasthop), _distance(distance), _seqno(seqno)
    {
	update_timer(vtime);
    }

    inline OlsrTypes::TopologyID id() const { return _id; }
    inline IPv4 destination() const { return _destination; }
    inline IPv4 lasthop() const { return _lasthop; }
    inline uint16_t distance() const { return _distance; }
    inline uint16_t seqno() const { return _seqno; }

    inline void set_distance(const uint16_t d) { _distance = d; }

    /**
     * @return the amount of time remaining until this
     *         TC entry expires.
     */
    inline TimeVal time_remaining() const {
	TimeVal tv;
	_expiry_timer.time_remaining(tv);
	return tv;
    }

    /**
     * Update the TopologyEntry's expiry timer.
     *
     * @param vtime the new value of the expiry timer.
     */
    void update_timer(const TimeVal& vtime);

    /**
     * Callback method to: delete a dying TopologyEntry.
     */
    void event_dead();

private:
    EventLoop&			_ev;
    TopologyManager*		_parent;

    /**
     * @short unique identifier.
     */
    OlsrTypes::TopologyID	_id;

    /**
     * @short destination address.
     */
    IPv4	_destination;	// T_dest_addr

    /**
     * @short The last hop to reach _dest.
     */
    IPv4	_lasthop;	// T_last_addr

    /**
     * @short The number of hops from the origin to the neighbor
     * advertised in this TC entry.
     */
    uint16_t	_distance;	// Not named -- Section 10, 3.1

    /**
     * @short The ANSN of this entry.
     */
    uint16_t	_seqno;		// T_seq

    /**
     * @short The time at which this entry will expire.
     */
    XorpTimer	_expiry_timer;	// T_time
};

/**
 * @short Class which manages topology outside of the one-hop and
 * two-hop neighborhood in the OLSR domain.
 */
class TopologyManager {
  public:
    TopologyManager(Olsr& olsr, EventLoop& eventloop,
		    FaceManager& fm, Neighborhood& nh);
    ~TopologyManager();

    inline RouteManager* route_manager() { return _rm; }

    void set_route_manager(RouteManager* rm) { _rm = rm; }

    //
    // TC entries.
    //

    /**
     * Update or create a topology entry in the TC database.
     *
     * @param dest_addr the destination of the topology entry.
     * @param origin_addr the origin of the topology entry.
     * @param distance the distance in hops from this node to the origin,
     *                 calculated from the message hop count.
     * @param ansn the advertised sequence number of the topology entry.
     * @param vtime the time for which this topology entry
     *              remains valid.
     * @param is_created a boolean which is set to true if a new entry
     *                   has been created.
     * @throw BadTopologyEntry if the entry could not be created.
     */
    void update_tc_entry(const IPv4& dest_addr,
			 const IPv4& origin_addr,
			 const uint16_t distance,
			 const uint16_t ansn,
			 const TimeVal& vtime,
			 bool& is_created)
	throw(BadTopologyEntry);

    /**
     * Add a topology entry to the database.
     *
     * @param dest_addr the destination of the new topology entry.
     * @param origin_addr the origin of the new topology entry.
     * @param distance the distance in hops from this node to the origin,
     *                 calculated from the message hop count.
     * @param ansn the advertised sequence number of the topology entry.
     * @param expiry_time the time for which this topology entry
     *                    remains valid.
     */
    OlsrTypes::TopologyID add_tc_entry(const IPv4& dest_addr,
				       const IPv4& origin_addr,
				       const uint16_t distance,
				       const uint16_t ansn,
				       const TimeVal& expiry_time)
	throw(BadTopologyEntry);

    /**
     * Delete a topology entry by ID.
     * It must be removed from last-hop and destination maps.
     *
     * @param tcid the ID of the toplogy entry to delete.
     * @return true if the topology entry was deleted, false if it could
     *              not be found.
     */
    bool delete_tc_entry(const OlsrTypes::TopologyID tcid);

    /**
     * Clear the TC database.
     */
    void clear_tc_entries();

    /**
     * Apply the Advertised Neighbor Sequence Number in the given TC
     * message to the Topology Set.
     * Section 9.5: TC Message Processing, 2-3.
     *
     * @param ansn the ANSN to apply.
     * @param origin_addr the origin of the TC message containing @param ansn
     * @return true if the provided ANSN is valid and has been applied,
     *              otherwise false if the message is stale and should be
     *              rejected.
     */
    bool apply_tc_ansn(const uint16_t ansn, const IPv4& origin_addr);

    /**
     * Return a topology entry ID given its destination and origin.
     *
     * Note: This is not declared 'const' due to the undeclared
     * mutability of map::operator[].
     *
     * @param dest_addr the destination of the TC entry to look up.
     * @param lasthop_addr the origin of the TC entry to look up.
     * @return the topology ID.
     * @throw BadTopologyEntry if the entry could not be found.
     */
    OlsrTypes::TopologyID get_topologyid(const IPv4& dest_addr,
					 const IPv4& lasthop_addr)
	throw(BadTopologyEntry);

    /**
     * Get a pointer to a topology entry given its ID.
     *
     * Used by the XRL layer.
     *
     * @param tcid the ID of the TC entry to look up.
     * @return the MID pointer.
     * @throw BadTopologyEntry if the entry could not be found.
     */
    const TopologyEntry* get_topology_entry_by_id(
	const OlsrTypes::TopologyID tcid) const
	throw(BadTopologyEntry);

    /**
     * Fill out a list of all topology entry IDs.
     *
     * Used by the XRL layer.
     *
     * @param tclist the list to fill out.
     */
    void get_topology_list(list<OlsrTypes::TopologyID>& tclist) const;

    /**
     * Retrieve the Advertised Neighbor Set (ANS) for a given OLSR peer.
     * Typically used by protocol simulator.
     *
     * Given the address of a node in the topology, retrieve the addresses
     * from all TC entries originated by that node.
     * Assumes that the "all entries for origin have same ANSN" invariant holds.
     *
     * TODO: Also return the per-link ETX information.
     *
     * @param origin_addr the originating node to look up in the TC database.
     * @param ansn the sequence number of origin_addr's neighbor set.
     * @throw BadTopologyEntry if origin_addr was not found.
     */
    vector<IPv4> get_tc_neighbor_set(const IPv4& origin_addr, uint16_t& ansn)
	throw(BadTopologyEntry);

    /*
     * Look up the distance of a TC entry.
     * Used by protocol simulator.
     *
     * @param origin_addr the address of the node originating this TC entry.
     * @param neighbor_addr the address of the destination node.
     * @return the number of hops to reach neighbor_addr via origin_addr.
     * @throw BadTopologyEntry if the entry does not exist.
     */
    uint16_t get_tc_distance(const IPv4& origin_addr, const IPv4& dest_addr)
	throw(BadTopologyEntry);

    /**
     * Count the number of TC entries which point to a given destination.
     * Used by protocol simulator.
     *
     * @param dest_addr the TC destination.
     * @return The number of TC entries pointing to dest_addr.
     */
    size_t get_tc_lasthop_count_by_dest(const IPv4& dest_addr);

    /**
     * Calculate the number of unique OLSR nodes with TC entries in this
     * node's TC database.
     *
     * @return the number of unique main addresses in the TC lasthop map.
     */
    size_t tc_node_count() const;

    //
    // MID entries.
    //

    /**
     * Update a Multiple Interface Declaration (MID) entry.
     *
     * The entry will be created if it does not exist.
     *
     * @param main_addr the main address of the node originating
     *                  the MID entry.
     * @param iface_addr the interface address of the MID entry.
     * @param distance the distance in hops of the origin of the
     *                 MIS message being processed.
     * @param vtime the time for which the MID entry remains valid.
     * @param is_mid_created set to true if a new MID entry was created.
     * @throw BadMidEntry if the entry could not be created or updated.
     */
    void update_mid_entry(const IPv4& main_addr, const IPv4& iface_addr,
		          const uint16_t distance, const TimeVal& vtime,
			  bool& is_mid_created)
	throw(BadMidEntry);

    /**
     * Create a new entry in the MID database.
     *
     * TODO Find the next available ID if already taken, as the range may
     * recycle quickly in large, dynamically changing topologies.
     *
     * @param main_addr the main address of the node originating
     *                  the MID entry.
     * @param iface_addr the interface address of the MID entry.
     * @param distance the distance in hops of the origin of the
     *                 MIS message being processed.
     * @param vtime the time for which the MID entry remains valid.
     * @throw BadMidEntry if the entry could not be created or updated.
     */
    void add_mid_entry(const IPv4& main_addr, const IPv4& iface_addr,
		       const uint16_t distance, const TimeVal& vtime)
	throw(BadMidEntry);

    /**
     * Delete a MID entry by ID.
     *
     * @param mid_id the ID of the MID entry to delete.
     * @return true if the MID entry was deleted.
     */
    bool delete_mid_entry(const OlsrTypes::MidEntryID mid_id);

    /**
     * Clear MID entries.
     */
    void clear_mid_entries();

    /**
     * Given the main address of an OLSR node, return a vector containing
     * all other interface addresses for it, as learned via the MID part
     * of the OLSR protocol.
     *
     * @param main_addr the main address to look up
     * @return a vector of protocol addresses, which may be empty.
     */
    vector<IPv4> get_mid_addresses(const IPv4& main_addr);

    /**
     * Look up the most recently seen distance of a MID entry, given its
     * origin and interface address.
     *
     * Internal method. Stubbed out if DETAILED_DEBUG is not defined.
     *
     * @param main_addr the main address of the OLSR node to look up.
     * @param iface_addr the interface address of the OLSR node to look up.
     * @throw BadMidEntry if the entry could not be found.
     */
    uint16_t get_mid_address_distance(const IPv4& main_addr,
				      const IPv4& iface_addr)
	throw(BadMidEntry);

    /**
     * Given an address possibly corresponding to a MID entry, return
     * the main address to which it would map.
     *
     * Used by the protocol simulator. Requires a linear search of MID space,
     * returns first match, there should be no other matches; no invariant.
     *
     * @param mid_addr the interface address to look up.
     * @return the main address of the node with the given interface address.
     * @throw BadMidEntry if mid_addr was not found.
     */
    IPv4 get_main_addr_of_mid(const IPv4& mid_addr)
	throw(BadMidEntry);

    /**
     * Count the number of unique OLSR main addresses in this node's MID
     * database.
     *
     * @return the number of OLSR main addresses in _mid_addr.
     */
    size_t mid_node_count() const;

    /**
     * Get a pointer to a MID entry given its ID.
     *
     * Used by the XRL layer.
     *
     * @param midid the ID of the MID entry to look up.
     * @return the MID entry pointer.
     * @throw BadTopologyEntry if the entry could not be found.
     */
    const MidEntry* get_mid_entry_by_id(
	const OlsrTypes::MidEntryID midid) const
	throw(BadTopologyEntry);

    /**
     * Fill out a list of all MID entry IDs.
     *
     * Used by the XRL layer.
     *
     * @param midlist the list to fill out.
     */
    void get_mid_list(list<OlsrTypes::MidEntryID>& midlist) const;


    //
    // RouteManager interaction.
    //

    /**
     * Push topology set to the RouteManager for SPT computation.
     * Section 10: Route computation.
     *
     * In ascending order, we push the rest of the known network topology,
     * starting with the TC entries which would have been originated by
     * two-hop neighbors.
     * If we encounter incomplete TC information for the network topology,
     * that is, there are no known nodes at a particular distance, we stop
     * pushing topology to RouteManager.
     *
     * TODO: Use ETX measurements for edge choice.
     */
    void push_topology();

    //
    // Event handlers.
    //

    /**
     * Callback method to: process an incoming TC message.
     * Section 9.5: TC Message Processing.
     *
     * @param msg Pointer to a message which is derived from TcMessage.
     * @param remote_addr The source address of the packet containing msg.
     * @param local_addr The address of the interface where this packet was
     *                   received.
     * @return true if this function consumed msg.
     */
    bool event_receive_tc(Message* msg,
	const IPv4& remote_addr, const IPv4& local_addr);

    /**
     * Callback method to: delete an expiring TopologyEntry.
     *
     * @param tcid the ID of the expiring TopologyEntry.
     */
    void event_tc_dead(OlsrTypes::TopologyID tcid);

    /**
     * Callback method to: process an incoming MID message.
     * Section 5.4: MID Message Processing.
     *
     * @param msg the message to process.
     * @param remote_addr the source address of the Packet containing msg
     * @param local_addr the address of the interface where @param msg was
     *                   received.
     * @return true if this method consumed @param msg
     */
    bool event_receive_mid(Message* msg,
	const IPv4& remote_addr, const IPv4& local_addr);

    /**
     * Callback method to: delete a dying MidEntry.
     */
    void event_mid_dead(const OlsrTypes::MidEntryID mid_id);

protected:
    /**
     * Internal method to: update a TC entry's distance.
     *
     * It is necessary to maintain an insertion sort collation order in the
     * TcDistanceMap as it is used for route calculation.
     *
     * @param tc pointer to the TopologyEntry to update.
     * @param distance the new distance of the TopologyEntry.
     */
    void update_tc_distance(TopologyEntry* tc, uint16_t distance);

    /**
     * Internal method to: assert that a TC's distance is unique.
     *
     * Verify that the given TopologyID appears once, and only once, in
     * the _tc_distances array used for route computation.
     * Stubbed out if DETAILED_DEBUG is not defined.
     *
     * @param tcid the topology entry ID to verify.
     * @throw BadTopologyEntry if tcid appears more than once in _tc_distances.
     */
    void assert_tc_distance_is_unique(const OlsrTypes::TopologyID tcid)
	throw(BadTopologyEntry);

    /**
     * Internal method to: assert that the ANSNs for all TC entries
     * originated by the node origin_addr are identical.
     *
     * A full linear search of the TC record space is performed.
     * Stubbed out if DETAILED_DEBUG is not defined.
     *
     * TODO: Eliminate this function by refactoring the data structures;
     *       we SHOULD be able to check if the last ANSN from origin was
     *       empty, currently we don't do that.
     *
     * @param origin_addr the node for which to verify the ANSNs.
     * @throw BadTopologyEntry if the ANSNs for origin_addr are not identical.
     */
    void assert_tc_ansn_is_identical(const IPv4& origin_addr)
	throw(BadTopologyEntry);

private:
    Olsr&		_olsr;
    EventLoop&		_eventloop;
    FaceManager&	_fm;
    Neighborhood&	_nh;
    RouteManager*	_rm;

    OlsrTypes::MidEntryID	_next_mid_id;
    OlsrTypes::TopologyID	_next_tcid;

    /**
     * Multiple Interface Association Database.
     */
    typedef map<OlsrTypes::MidEntryID, MidEntry*> MidIdMap;
    MidIdMap _mids;

    /**
     * A map providing lookup of the MidEntries corresponding to the
     * main protocol address of an OLSR node.
     */
    typedef multimap<IPv4, OlsrTypes::MidEntryID> MidAddrMap;
    MidAddrMap _mid_addr;

    /**
     * Topology Information Base.
     */
    typedef map<OlsrTypes::TopologyID, TopologyEntry*> TcIdMap;
    TcIdMap _topology;

    /**
     * A multimap providing lookup of topology entries by
     * their distance.
     * Used for routing computation, the most common case.
     */
    typedef multimap<uint16_t, OlsrTypes::TopologyID> TcDistanceMap;
    TcDistanceMap _tc_distances;

    /**
     * A multimap providing lookup of topology entries by destination.
     * Used by TC updates.
     */
    typedef multimap<IPv4, OlsrTypes::TopologyID> TcDestMap;
    TcDestMap _tc_destinations;

    /**
     * A multimap providing lookup of topology entries by last hop.
     * Used by ANSN processing.
     */
    typedef multimap<IPv4, OlsrTypes::TopologyID> TcLasthopMap;
    TcLasthopMap _tc_lasthops;

    /**
     * A map of final ANSN numbers for each origin from which we have seen
     * TC messages with empty neighbor sets.
     * Used by the simulator for verification.
     */
    typedef map<IPv4, uint16_t>	    TcFinalSeqMap;
    TcFinalSeqMap _tc_final_seqnos;
};

#endif // __OLSR_TOPOLOGY_HH__
