// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/neighborhood.hh,v 1.3 2008/10/02 21:56:35 bms Exp $

#ifndef __OLSR_NEIGHBORHOOD_HH__
#define __OLSR_NEIGHBORHOOD_HH__

class Olsr;
class LogicalLink;
class Neighbor;
class TwoHopNeighbor;
class TwoHopLink;
class HelloMessage;
class TopologyManager;
class RouteManager;
class Neighborhood;

/**
 * @short Orders a sequence of Neighbor* in descending order of
 *        MPR candidacy.
 *
 * Model of StrictWeakOrdering.
 */
struct CandMprOrderPred {
    /**
     * Compare two Neighbors based on their MPR candidacy.
     * It is assumed both pointers are valid for the scope of the functor.
     *
     * Collation order: willingness, reachability, degree.
     *
     * @return true if lhs comes before rhs.
     */
    bool operator()(const Neighbor* lhs, const Neighbor* rhs) const;
};

/**
 * @short Orders a sequence of OlsrTypes::LogicalLinkID in descending
 *        order of link preference.
 *
 * Model of StrictWeakOrdering.
 */
struct LinkOrderPred {
    Neighborhood* _nh;

    inline LinkOrderPred(Neighborhood* nh) : _nh(nh) {}

    /**
     * Determine if the left-hand link comes before the the right-hand link.
     *
     * Collation order: symmetric, most recently updated, highest ID.
     *
     * TODO: This is a candidate for moving into class Link.
     * TODO: ETX metrics, collation order will be:
     *  Anything !is_pending is above anything is_pending
     *  Then order in ascending near_etx().
     *  Then order in ascending far_etx().
     *  A change in the best link means a change in routes, unless we
     *  dampen flap somehow.
     *
     * @return true if link lhs is better than link rhs.
     */
    bool operator()(const OlsrTypes::LogicalLinkID lhid,
		    const OlsrTypes::LogicalLinkID rhid);
};

/**
 * @short Orders a sequence of OlsrTypes::TwoHopLinkID in descending
 *        order of link preference.
 *
 * Model of StrictWeakOrdering.
 */
struct TwoHopLinkOrderPred {
    Neighborhood* _nh;

    inline TwoHopLinkOrderPred(Neighborhood* nh) : _nh(nh) {}

    /**
     * Determine if a two-hop link is better than another two-hop link.
     *
     * Collation order: most recently updated, highest ID.
     * TODO: Use ETX measurements.
     *
     * @return true if link lhid is better than link rhid.
     */
    bool operator()(const OlsrTypes::TwoHopLinkID lhid,
		    const OlsrTypes::TwoHopLinkID rhid);
};

/**
 * @short Representation of OLSR node's one-hop and two-hop neighborhood.
 *
 * Responsible for originating TC broadcasts when the node is selected
 * as an MPR by other nodes.
 */
class Neighborhood {
  public:
    Neighborhood(Olsr& olsr, EventLoop& eventloop, FaceManager& fm);
    ~Neighborhood();

    //
    // Top level associations.
    //

    TopologyManager* topology_manager() { return _tm; }
    void set_topology_manager(TopologyManager* tm) { _tm = tm; }

    RouteManager* route_manager() { return _rm; }
    void set_route_manager(RouteManager* rm) { _rm = rm; }

    /**
     * Given an empty HELLO message, fill it out as appropriate for
     * its interface ID property.
     * If the message is not empty the results are undefined.
     *
     * TODO: Support ETX measurements.
     *
     * @param hello the HELLO message to be filled out, which MUST
     *              contain the FaceID of where it is to be sent.
     * @return the number of link addresses, including neighbors
     *         which are not local to this link, filled out in hello.
     */
    size_t populate_hello(HelloMessage* hello);

    /**
     * Push the topology to the RouteManager for route computation.
     *
     * When we do incremental SPT this can partially go away. The SPT
     * can contain only one edge between each node -- the nested methods
     * here deal with this.
     */
    void push_topology();

    //
    // RFC 3626 Section 18.2: Protocol control variables.
    //

    /**
     * Attempt to set the redundancy of links advertised in TC broadcasts.
     *
     * @param type the new redundancy level to set.
     * @return true if the TC_REDUNDANCY was set to type.
     */
    bool set_tc_redundancy(const OlsrTypes::TcRedundancyType type);

    /**
     * @return the value of the TC_REDUNDANCY protocol variable.
     */
    OlsrTypes::TcRedundancyType get_tc_redundancy() const {
	return _tc_redundancy;
    }

    /**
     * @return the value of the MPR_COVERAGE protocol variable.
     */
    inline uint32_t mpr_coverage() const { return _mpr_coverage; }

    /**
     * Attempt to set the required level of MPR coverage.
     *
     * If successful, and OLSR is running on any configured interface,
     * the MPR set will be recalculated.
     *
     * @param coverage the new value of the MPR_COVERAGE variable.
     * @return true if the MPR_COVERAGE was set to coverage.
     */
    bool set_mpr_coverage(const uint32_t coverage);

    /**
     * @return the value of the REFRESH_INTERVAL protocol variable.
     */
    TimeVal get_refresh_interval() const { return _refresh_interval; }

    /**
     * Set the value of the REFRESH_INTERVAL protocol variable.
     *
     * @param interval the new value of REFRESH_INTERVAL..
     */
    void set_refresh_interval(const TimeVal& interval) {
	_refresh_interval = interval;
    }

    /**
     * @return the value of the TC_INTERVAL protocol variable.
     */
    TimeVal get_tc_interval() { return _tc_interval; }

    /**
     * Set the interval between TC broadcasts.
     *
     * The timer will only be restarted if previously scheduled. If the
     * period of the TC broadcasts is changed, a TC broadcast is scheduled
     * to take place immediately.
     *
     * @param interval the new value of TC_INTERVAL.
     */
    void set_tc_interval(const TimeVal& interval);

    /**
     * Set the WILLINGNESS protocol variable.
     *
     * @param willingness the new value of the WILLINGNESS protocol variable.
     */
    void set_willingness(const OlsrTypes::WillType willingness);

    /**
     * @return the current value of the WILLINGNESS protocol variable.
     */
    OlsrTypes::WillType willingness() const { return _willingness; }

    /**
     * @return the current value of the NEIGHB_HOLD_TIME protocol variable.
     */
    TimeVal get_neighbor_hold_time() { return _refresh_interval * 3; }

    /**
     * @return the current value of the TOP_HOLD_TIME protocol variable.
     */
    TimeVal get_topology_hold_time() { return _tc_interval * 3; }

    //
    // Face interaction.
    //

    /**
     * Add an interface to the neighborhood.
     * Called whenever an instance of Face is configured administratively up.
     *
     * @param faceid The ID of the interface which has been configured
     *               administratively up.
     */
    void add_face(const OlsrTypes::FaceID faceid);

    /**
     * Delete an interface from the neighborhood.
     * Called whenever an instance of Face is configured administratively down.
     *
     * @param faceid The ID of the interface which has been configured
     *               administratively down.
     */
    void delete_face(const OlsrTypes::FaceID faceid);

    //
    // Link database.
    //

    /**
     * Update a LogicalLink.
     *
     * The link will be created if it does not exist.
     * Links are specified by their remote and local interface addresses.
     *
     * @param faceid the ID of the interface where this link appears.
     * @param remote_addr the protocol address of the remote interface
     *                    at the far end of the link.
     * @param local_addr the protocol address of the local interface
     *                    at the near end of the link.
     * @param vtime the validity time of the new link.
     * @param is_created will be set to true if the link did not
     *                   previously exist and was created by this method.
     * @return the ID of the link tuple.
     * @throw BadLogicalLink if the link could not be updated.
     */
    OlsrTypes::LogicalLinkID update_link(
	const OlsrTypes::FaceID faceid,
	const IPv4& remote_addr,
	const IPv4& local_addr,
	const TimeVal& vtime,
	bool& is_created)
	throw(BadLogicalLink);

    /**
     * Add a link to the local link database.
     *
     * @param vtime the validity time of the new link.
     * @param remote_addr the protocol address of the remote interface
     *                    at the far end of the link.
     * @param local_addr the protocol address of the local interface
     *                    at the near end of the link.
     * @return the ID of the new link.
     * @throw BadLogicalLink if the link could not be created.
     */
    OlsrTypes::LogicalLinkID add_link(
	const TimeVal& vtime,
	const IPv4& remote_addr,
	const IPv4& local_addr)
	throw(BadLogicalLink);

    /**
     * Delete the link tuple specified by the given link id.
     *
     * Note: The associated Neighbor may be deleted. State change is
     * also propagated to the FaceManager.
     *
     * @param linkid the identifier of the link tuple.
     * @return true if the link was deleted.
     */
    bool delete_link(OlsrTypes::LogicalLinkID linkid);

    /**
     * Clear the neighborhood one-hop links.
     *
     * The neighbors and two-hop neighbors associated with these links
     * will be removed. Assertions for this are in the destructor.
     */
    void clear_links();

    /**
     * Look up a LogicalLink's pointer given its LogicalLinkID.
     *
     * @param linkid the ID of the link to look up.
     * @return a pointer to the logical link.
     * @throw BadLogicalLink if the link could not be found.
     */
    const LogicalLink* get_logical_link(const OlsrTypes::LogicalLinkID linkid)
	throw(BadLogicalLink);

    /**
     * Fill out a list of all LogicalLinkIDs in the database.
     *
     * @param l1_list the list to fill out.
     */
    void get_logical_link_list(list<OlsrTypes::LogicalLinkID>& l1_list)
	const;

    /**
     * Look up a LogicalLink's ID given its interface addresses.
     *
     * @param remote_addr the protocol address of the remote interface
     *                    at the far end of the link.
     * @param local_addr the protocol address of the local interface
     *                    at the near end of the link.
     * @return the ID of the LogicalLink.
     * @throw BadLogicalLink if the link could not be found.
     */
    OlsrTypes::LogicalLinkID get_linkid(const IPv4& remote_addr,
	const IPv4& local_addr)
	throw(BadLogicalLink);

    //get_link_list

    //
    // Neighbor database.
    //

    /**
     * Update or create a Neighbor in the one-hop neighbor database.
     *
     * This method has the weak exception guarantee that BadNeighbor will
     * only be thrown before it is associated with LogicalLink.
     *
     * @param main_addr The main protocol address of the neighbor.
     * @param linkid The ID of the initially created link to the neighbor.
     * @param is_new_link true if the link the neighbor is being created
     *                    with has just been instantiated.
     * @param will The neighbor's advertised willingness-to-forward.
     * @param is_mpr_selector true if the neighbor selects us as an MPR.
     * @param mprs_expiry_time the expiry time for the MPR selector tuple.
     * @param is_created set to true if a new neighbor entry was created.
     * @return the ID of the updated or created neighbor tuple.
     * @throw BadNeighbor if the neighbor entry could not be updated.
     */
    OlsrTypes::NeighborID update_neighbor(const IPv4& main_addr,
	const OlsrTypes::LogicalLinkID linkid,
	const bool is_new_link,
	const OlsrTypes::WillType will,
	const bool is_mpr_selector,
	const TimeVal& mprs_expiry_time,
	bool& is_created)
	throw(BadNeighbor);

    /**
     * Add a new Neighbor to the one-hop neighbor database.
     * A Neighbor must be created with at least one LogicalLink.
     *
     * @param main_addr The main address of the new neighbor.
     * @param linkid The ID of the Neighbor's first link.
     * @return the ID of the newly created neighbor.
     * @throw BadNeighbor if the neighbor entry could not be created.
     */
    OlsrTypes::NeighborID add_neighbor(const IPv4& main_addr,
	OlsrTypes::LogicalLinkID linkid)
	throw(BadNeighbor);

    /**
     * Delete a neighbor from the neighbor database.
     * Called when the last link to the neighbor has been deleted.
     *
     * @param nid The ID of this neighbor.
     * @return true if the neighbor was removed, false if it cannot
     *              be found.
     */
    bool delete_neighbor(const OlsrTypes::NeighborID nid);

    /**
     * Find a Neighbor's pointer, given its NeighborID.
     *
     * @param nid the ID of the Neighbor.
     * @return the pointer to the Neighbor instance.
     * @throw BadNeighbor if the Neighbor does not exist.
     */
    const Neighbor* get_neighbor(const OlsrTypes::NeighborID nid)
	throw(BadNeighbor);

    /**
     * Fill out a list of all NeighborIDs in the database.
     *
     * @param n1_list the list to fill out.
     */
    void get_neighbor_list(list<OlsrTypes::NeighborID>& n1_list) const;

    /**
     * Find a neighbor ID, given its main address.
     *
     * @param main_addr the main protocol address of the OLSR node.
     * @return the neighbor ID.
     * @throw BadNeighbor if the neighbor is not found.
     */
    OlsrTypes::NeighborID get_neighborid_by_main_addr(const IPv4& main_addr)
	throw(BadNeighbor);

    /**
     * Find a neighbor ID, given the address of one of its interfaces.
     *
     * @param remote_addr the address of one of the interfaces of
     *                    an OLSR node.
     * @return the neighbor ID.
     * @throw BadNeighbor if the neighbor is not found.
     */
    OlsrTypes::NeighborID
	get_neighborid_by_remote_addr(const IPv4& remote_addr)
	throw(BadNeighbor);

    /**
     * Check if a remote address belongs to a symmetric one-hop neighbor.
     *
     * Referenced from:
     *  Section 5.4 point 1 MID Message Processing.
     *  Section 9.5 point 1 TC Message Processing.
     *  Section 12.5 point 1 HNA Message Processing.
     *
     * @param addr the interface address of a neighbor.
     * @return true if addr is an interface address of a symmetric one-hop
     *             neighbor.
     */
    bool is_sym_neighbor_addr(const IPv4& addr);

    //
    // Advertised neighbor set.
    //

    /**
     * @return true if the Neighbor n would be advertised in a TC
     *              broadcast, given the current TC_REDUNDANCY.
     */
    inline bool is_tc_advertised_neighbor(Neighbor* n) {
	if ((_tc_redundancy == OlsrTypes::TCR_ALL) ||
	    (_tc_redundancy == OlsrTypes::TCR_MPRS_INOUT && n->is_mpr()) ||
	     n->is_mpr_selector()) {
	    return true;
	}
	return false;
    }

    /**
     * Schedule an update of the Advertised Neighbor Set.
     *
     * @param is_deleted true if the update is being scheduled in
     *                   response to the deletion of a Neighbor.
     */
    void schedule_ans_update(const bool is_deleted);

    //
    // Two hop link database.
    //

    /**
     * Update the link state information for a two-hop neighbor.
     *
     * This method may create a TwoHopNeighbor and/or a TwoHopLink if they
     * do not already exist.
     *
     * @param node_info The address information for the two-hop neighbor;
     *                  contains ETX measurements, if applicable.
     * @param nexthop The main address of the immediate neighbor which
     *                advertises this TwoHopLink.
     * @param faceid The interface where the advertisement was heard.
     * @param vtime The time for which the TwoHopLink remains valid.
     * @return the ID of the two-hop neighbor.
     */
    OlsrTypes::TwoHopLinkID update_twohop_link(const LinkAddrInfo& node_info,
					       Neighbor& nexthop,
					       const OlsrTypes::FaceID faceid,
					       const TimeVal& vtime)
	throw(BadTwoHopLink);

    /**
     * Add a TwoHopLink to the Neighborhood.
     *
     * The constructor signature forces us to associate the near
     * end of the TwoHopLink with a Neighbor. It MUST be associated
     * with a TwoHopNeighbor after construction to be considered valid.
     *
     * @param nexthop The strict one-hop neighbor at the near end of
     *                the TwoHopLink being created.
     * @param remote_addr The two-hop neighbor at the far end of
     *                    the TwoHopLink being created.
     * @param vtime The time for which the TwoHopLink remains valid.
     * @return the ID of the newly created TwoHopLink.
     * @throw BadTwoHopLink if the TwoHopLink could not be created.
     */
    OlsrTypes::TwoHopLinkID add_twohop_link(Neighbor* nexthop,
					    const IPv4& remote_addr,
					    const TimeVal& vtime)
	throw(BadTwoHopLink);

    /**
     * Delete the TwoHopLink to a two-hop neighbor.
     *
     * The deletion is propagated to the Neighbor and TwoHopNeighbor
     * instances on the near and far ends of the link respectively.
     *
     * @param tlid ID of the two-hop link which is to be deleted.
     * @return true if the link thus deleted was the last link to the
     *              two-hop node it is used to reach, otherwise false.
     */
    bool delete_twohop_link(OlsrTypes::TwoHopNodeID tlid);

    /**
     * Delete the TwoHopLink to a two-hop neighbor.
     *
     * The link is identified by the near and far end main addresses.
     *
     * @param nexthop_addr The address of the Neighbor used to reach the
     *                two-hop neighbor given by twohop_addr.
     * @param twohop_addr The two-hop neighbor whose link has been lost.
     * @return true if this was the last link to the two-hop neighbor
     *              and it was deleted as a result.
     */
    bool delete_twohop_link_by_addrs(const IPv4& nexthop_addr,
				     const IPv4& twohop_addr);

    /**
     * Given the ID of a TwoHopLink, return its instance pointer.
     *
     * @param tlid the ID of a TwoHopLink.
     * @return the pointer to the TwoHopLink instance.
     * @throw BadTwoHopLink if tlid does not exist.
     */
    TwoHopLink* get_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
	throw(BadTwoHopLink);

    /**
     * Fill out a list of all TwoHopLinkIDs in the database.
     *
     * @param l2_list the list to fill out.
     */
    void get_twohop_link_list(list<OlsrTypes::TwoHopLinkID>& l2_list) const;

    //
    // Two hop node database.
    //

    /**
     * Update a two-hop neighbor.
     *
     * If the TwoHopNeighbor does not exist it will be created. A valid
     * two-hop link must be provided; if the link is also newly created,
     * this method will create the back-reference.
     *
     * @param main_addr the main address of the two-hop neighbor.
     * @param tlid the ID of the two-hop link with which the two-hop
     *             neighbor is initially associated.
     * @param is_new_l2 true if tlid refers to a newly created link.
     * @param is_n2_created set to true if a new TwoHopNeighbor was created.
     * @return the ID of the two-hop neighbor.
     * @throw BadTwoHopNode if the two-hop neighbor could not be updated.
     */
    OlsrTypes::TwoHopNodeID update_twohop_node(
	const IPv4& main_addr,
	const OlsrTypes::TwoHopLinkID tlid,
	const bool is_new_l2,
	bool& is_n2_created)
	throw(BadTwoHopNode);

    /**
     * Add a two-hop neighbor to the two-hop neighborhood.
     *
     * @param main_addr the main address of the two-hop neighbor to create.
     * @param tlid the ID of the initial link to this two-hop neighbor.
     * @return the ID of the newly created two-hop neighbor.
     * @throw BadTwoHopNode if the two-hop neighbor could not be created.
     */
    OlsrTypes::TwoHopNodeID add_twohop_node(
        const IPv4& main_addr,
	const OlsrTypes::TwoHopLinkID tlid)
	throw(BadTwoHopNode);

    /**
     * Delete an entry in the two-hop neighbor table.
     *
     * @param tnid the ID of a two-hop neighbor.
     * @return true if the neighbor was deleted, otherwise false.
     */
    bool delete_twohop_node(OlsrTypes::TwoHopNodeID tnid);

    /**
     * Look up a two-hop neighbor by main address.
     *
     * @param main_addr the main address of a two-hop neighbor.
     * @return the ID of the two-hop neighbor.
     * @throw BadTwoHopNode if the two-hop neighbor could not be found.
     */
    OlsrTypes::TwoHopNodeID get_twohop_nodeid_by_main_addr(
	const IPv4& main_addr)
	throw(BadTwoHopNode);

    /**
     * Given the ID of a TwoHopNeighbor, return its instance pointer.
     *
     * @param tnid the ID of a TwoHopNeighbor.
     * @return the pointer to the TwoHopNeighbor instance.
     * @throw BadTwoHopNode if tnid does not exist.
     */
    const TwoHopNeighbor* get_twohop_neighbor(
	const OlsrTypes::TwoHopNodeID tnid) const throw(BadTwoHopNode);

    /**
     * Fill out a list of all TwoHopNodeIDs in the database.
     *
     * @param n2_list the list to fill out.
     */
    void get_twohop_neighbor_list(list<OlsrTypes::TwoHopNodeID>& n2_list)
	const;

    //
    // MPR selector set methods.
    //

    /**
     * @return true if this node has been selected as an MPR by any
     * one-hop neighbor, that is, the MPR selector set is non-empty. 
     */
    inline bool is_mpr() const { return !_mpr_selector_set.empty(); }

    /**
     * Update a Neighbor's status in the MPR selector set,
     * possibly adding it.
     *
     * If our node now has a non-empty MPR selector set, it
     * must now originate TC advertisements.
     *
     * @param nid the ID of the Neighbor to mark as an MPR selector.
     * @param vtime the duration of the MPR selector set membership.
     */
    void update_mpr_selector(const OlsrTypes::NeighborID nid,
			     const TimeVal& vtime);

    /**
     * Remove a neighbor from the MPR selector set by its ID.
     *
     * If the node no longer has any MPR selectors, it is no longer
     * considered an MPR, and it must continue to originate empty TCs
     * for TOP_HOLD_TIME, after which it shall stop.
     *
     * @param nid the ID of the Neighbor to add as an MPR selector.
     */
    void delete_mpr_selector(const OlsrTypes::NeighborID nid);

    /**
     * Check if an address belongs to a one-hop neighbor which is
     * also an MPR selector.
     *
     * Referenced from:
     *  Section 3.4.1 Default Forwarding Algorithm.
     *
     * @param remote_addr the IPv4 interface address to look up in
     *                    the neighbor database.
     * @return true if addr is an interface address of a symmetric
     *              one-hop neighbor which selects this node as an MPR.
     */
    bool is_mpr_selector_addr(const IPv4& remote_addr);

    /**
     * @return the MPR selector set.
     *
     * For use by simulation framework.
     */
    set<OlsrTypes::NeighborID> mpr_selector_set() const {
	return _mpr_selector_set;
    }

    //
    // MPR set methods.
    //

    /**
     * Trigger a recount of the MPR set.
     *
     * Invoked whenever there is a change of state which would cause the
     * MPR set to change. Calculating the MPR set is an expensive
     * operation, so it is scheduled as a one-off task in the event loop.
     */
    inline void schedule_mpr_recount() {
	_mpr_recount_task.reschedule();
    }

    /**
     * Add a neighbor to the set of MPR candidates for the
     * interfaces from which it is reachable.
     *
     * @param nid the ID of the neighbor to add.
     */
    void add_cand_mpr(const OlsrTypes::NeighborID nid);

    /**
     * Remove a neighbor from the set of MPR candidates for all interfaces.
     *
     * @param nid the ID of the neighbor to remove.
     */
    void withdraw_cand_mpr(const OlsrTypes::NeighborID nid);

    /**
     * Callback method to: recount the MPR set for all configured
     *                     OLSR interfaces.
     */
    void recount_mpr_set();

    /**
     * Clear all existing MPR state for Neighbors.
     */
    void reset_onehop_mpr_state();

    /**
     * Clear all existing MPR state for TwoHopNeighbors.
     * Compute number of now uncovered reachable nodes at radius=2.
     *
     * @return The number of reachable, strict two-hop neighbors
     *         to be considered by MPR selection.
     */
    size_t reset_twohop_mpr_state();

    /**
     * Compute one-hop neighbor reachability and update it in the
     * Neighbor to avoid repetitively computing it on every MPR recount.
     *
     * Coverage must be valid. If this method is called outside of an MPR
     * recount results are undefined.
     *
     * Reachability is defined as: the number of uncovered N2 nodes which
     * have edges to this N. We do this outside of Neighbor for code brevity.
     *
     * @param n Pointer to a Neighbor, which is normally an MPR candidate.
     */
    void update_onehop_reachability(Neighbor* n);

    /**
     * Compute two-hop neighbor reachability.
     *
     * It will be updated it in the TwoHopNeighbor to avoid computing it
     * more than once during an MPR recount.
     * If an N2 is reachable via an N with WILL_ALWAYS this takes precedence.
     *
     * TODO: WHEN ETX IS IMPLEMENTED, A LINK WITH NO 'GOOD' LINKS
     * MUST BE CONSIDERED UNREACHABLE.
     *
     * Two-hop reachability is defined as: the number of MPR candidates with
     * edges linking them to N2.
     * Note: This is NOT THE SAME as a one-hop neighbor's reachability.
     *
     * We do this outside of TwoHopNeighbor to avoid playing too many tedious
     * C++ accessor games. MPR candidacy of linked neighbors must be valid.
     * If this method is called outside of an MPR recount, its results
     * are undefined.
     *
     * @param tn Pointer to a TwoHopNeighbor.
     */
    void update_twohop_reachability(TwoHopNeighbor* tn);

    /**
     * Consider persistent MPR candidates for MPR selection.
     *
     * 8.3.1, 1: Start with an MPR set made of all members of N with
     * willingness equal to WILL_ALWAYS.
     *
     * This introduces the funky situation that a neighbor may be selected
     * as an MPR even if it has no two-hop links. Such neighbors are always
     * chosen as MPRs before other neighbors.
     *
     * @return The number of two-hop neighbors which have been covered
     *         by considering the persistent MPR candidates.
     */
    size_t consider_persistent_cand_mprs();

    /**
     * Consider MPR coverage of poorly covered two-hop neighbors.
     *
     * 8.3.1, 3: Ensure that for all uncovered strict N2 reachable
     * *only via 1 edge*, their neighbor N is selected as an MPR.
     *
     * TODO: Use ETX measurements.
     *
     * @return The number of two-hop neighbors which have been covered
     *         by considering the persistent MPR candidates.
     */
    size_t consider_poorly_covered_twohops();

    /**
     * Consider remaining MPR candidates for MPR selection.
     *
     * Candidates are considered in descending order of willingness,
     * reachability and degree.
     *
     * Note: As we only use the result of the insertion sort in this
     * scope, this block is a candidate for a Boost++ filter_iterator.
     * However a filter iterator might keep scanning the N space and
     * blowing the l2/l3 cache.
     *
     * @param n2_count The total number of N2 which must be reachable by
     *                 the MPR set, used as a recursion upper bound.
     * @param covered_n2_count A reference to the cumulative number of
     *                         N2 which are reachable by the MPR set, and
     *                         which this method will update.
     */
    void consider_remaining_cand_mprs(const size_t n2_count,
				      size_t& covered_n2_count);

    /**
     * Mark all N1 neighbors was MPRs.
     *
     * Considers all reachable one-hop neighbors with willingness of
     * other than WILL_NEVER as MPRs.
     * This feature is typically used as a workaround in dense OLSR
     * topologies which are not sufficiently partitioned.
     *
     * @param final_mpr_set will have the result set of this method
     *                      merged with it.
     * @return the number of neighbors which have been selected as MPRs.
     */
    size_t mark_all_n1_as_mprs(set<OlsrTypes::NeighborID>& final_mpr_set);

    /**
     * Minimize the MPR set, based on the MPR coverage parameter.
     *
     * Produces the final MPR set in a std::set container,
     * for debugging purposes.
     * Section 8.3.1, 4.
     *
     * @param final_mpr_set reference to an empty MPR set that shall
     *                      contain the resultant MPR set after it
     *                      has been minimized.
     * @return the number of elements removed from the MPR set, as it
     *         appears in the one-hop neighbor database.
     * @throw BadTwoHopCoverage if the MPR minimization algorithm
     *        detects that a two-hop node is now uncovered by any MPRs.
     */
    size_t minimize_mpr_set(set<OlsrTypes::NeighborID>& final_mpr_set)
	throw(BadTwoHopCoverage);

    /**
     * Determine if an MPR is essential to covering the entire two-hop
     * neighborhood.
     *
     * @param n the Neighbor to evaluate.
     * @return true if any of N's links cover a poorly covered strict
     *              TwoHopNeighbor.
     */
    bool is_essential_mpr(const Neighbor* n);

    /**
     * @return the MPR selector set.
     *
     * For use by simulation framework.
     */
    set<OlsrTypes::NeighborID> mpr_set() const {
	return _mpr_set;
    }

    //
    // Event handlers.
    //

    /**
     * Callback method to: service a LogicalLink's SYM timer.
     *
     * Whilst the SYM interval timer is pending, the link is considered
     * symmetric. When the timer fires, the link is considered ASYM.
     * If both the ASYM and SYM timers fire in the same event loop quantum,
     * the ASYM timer is considered to have priority.
     *
     * @param linkid the ID of the link whose SYM timer has fired.
     */
    void event_link_sym_timer(OlsrTypes::LogicalLinkID linkid);

    /**
     * Callback method to: service a LogicalLink's ASYM timer.
     *
     * Whilst the ASYM interval timer is pending, the link is considered
     * asymmetric. When the timer fires, the link is considered LOST.
     *
     * @param linkid the ID of the link whose ASYM timer has fired.
     */
    void event_link_asym_timer(OlsrTypes::LogicalLinkID linkid);

    /**
     * Callback method to: service a LogicalLink's LOST timer.
     *
     * Section 13: Link Layer Notification.
     *
     * TODO: Not yet implemented, as it relies on link layer support from
     * the host platform which does not yet exist. In practice this
     * should not pose a problem, as 802.11 IBSS disassociation is often
     * unreliable anyway.
     *
     * @param linkid the ID of the link whose LOST timer has fired.
     */
    void event_link_lost_timer(OlsrTypes::LogicalLinkID linkid);

    /**
     * Callback method to: service a LogicalLink's DEAD timer.
     *
     * @param linkid the ID of the link whose DEAD timer has fired.
     */
    void event_link_dead_timer(OlsrTypes::LogicalLinkID linkid);

    /**
     * Callback method to: service a TwoHopLink's DEAD timer.
     *
     * @param tlid the ID of the two-hop link whose DEAD timer has fired.
     */
    void event_twohop_link_dead_timer(const OlsrTypes::TwoHopLinkID tlid);

    /**
     * Callback method to: service an MPR selector's EXPIRY timer.
     *
     * @param nid the ID of the Neighbor whose MPR selector tuple
     *            has expired.
     */
    void event_mpr_selector_expired(const OlsrTypes::NeighborID nid);

    /**
     * Callback method to: process an incoming HELLO message.
     * Section 7.1.1: HELLO Message Processing.
     *
     * @param msg Pointer to a message which is derived from HelloMessage.
     * @param remote_addr The source address of the packet containing msg.
     * @param local_addr The address of the interface where this packet was
     *                   received.
     * @return true if this function consumed msg.
     */
    bool event_receive_hello(Message* msg,
	const IPv4& remote_addr, const IPv4& local_addr);

    /**
     * Callback method to: service the TC transmission timer.
     *
     * Section 9.2: Advertised Neighbor Set.
     *
     * Flood a TC message to the rest of the OLSR domain
     * which contains our Advertised Neighbor Set (ANS).
     * This method should only be called if the TC timer is running
     * or finishing. The finishing state is entered when a node has
     * stopped being an MPR or when the ANS set becomes empty.
     *
     * TODO: Account for ETX metrics in selecting advertised neighbors.
     * TODO: Fish-eye TC emission optimization; transmit only.
     *
     * @return true if the callback should be rescheduled, otherwise false.
     */
    bool event_send_tc();

    //
    // Timer methods.
    //

    /**
     * Stop all timers in Neighborhood.
     */
    void stop_all_timers();

    /**
     * Start the TC transmission interval timer.
     */
    void start_tc_timer();

    /**
     * Stop the TC transmission interval timer.
     */
    void stop_tc_timer();

    /**
     * Restart the TC transmission interval timer.
     */
    void restart_tc_timer();

protected:
    //
    // One-hop link selection.
    //

    /**
     * Find the best link to a neighbor N.
     *
     * @param n Pointer to a neighbor N.
     * @return Pointer to a LogicalLink l which is the best link to N.
     * @throw BadLinkCoverage if none of the links are reachable or
     *                        is of suitable ETX criteria.
     */
    const LogicalLink* find_best_link(const Neighbor* n)
        throw(BadLinkCoverage);

    /**
     * Push a single Neighbor, and its links, to the RouteManager.
     *
     * The SPT structure can only hold a single edge between each
     * neighbor at the moment. We also need to select each node by
     * its ETX. In the absence of ETX measurements, we select
     * the most recently heard symmetric link which is up.
     *
     * @param n the neighbor to push.
     * @return true if the neighbor was pushed, false if it was not.
     */
    bool push_neighbor(const Neighbor* n);

    //
    // Two-hop link selection.
    //

    /**
     * Find the best link to a two-hop neighbor N2.
     *
     * @param n2 Pointer to a neighbor N2.
     * @return Pointer to a TwoHopLink l2 which is the best link to N2.
     * @throw BadTwoHopCoverage if none of the links are reachable,
     *        or of suitable ETX criteria.
     */
    const TwoHopLink* find_best_twohop_link(const TwoHopNeighbor* n2)
	throw(BadTwoHopCoverage);

    /**
     * Push a single TwoHopNeigbor, and its links, to the RouteManager.
     * Here, we select the best link to this TwoHopNeighbor.
     *
     * @param n2 the two-hop neighbor to push.
     * @return true if the two-hop neighbor was pushed, false if it was not.
     */
    bool push_twohop_neighbor(TwoHopNeighbor* n2);

    /**
     * Transition to the 'finish' state for the TC timer.
     */
    void finish_tc_timer();

    /**
     * Schedule the TC timer as soon as possible.
     */
    void reschedule_immediate_tc_timer();

    /**
     * Reschedule the TC timer if the TC interval changed.
     */
    void reschedule_tc_timer();

    enum TcTimerState {
	TC_STOPPED = 0,
	TC_RUNNING = 1,
	TC_FINISHING = 2
    };

private:
    Olsr&		_olsr;
    EventLoop&		_eventloop;
    FaceManager&	_fm;
    TopologyManager*	_tm;
    RouteManager*	_rm;

    LinkOrderPred		_link_order_pred;
    TwoHopLinkOrderPred		_twohop_link_order_pred;

    OlsrTypes::LogicalLinkID	_next_linkid;
    OlsrTypes::NeighborID	_next_neighborid;
    OlsrTypes::TwoHopLinkID	_next_twohop_linkid;
    OlsrTypes::TwoHopNodeID	_next_twohop_nodeid;

    /**
     * The count of administratively up and running OLSR interfaces
     * in this OLSR routing process.
     */
    uint32_t			_enabled_face_count;

    /**
     * Willingness of this node to forward packets for other nodes.
     */
    OlsrTypes::WillType		_willingness;

    /**
     * The REFRESH_INTERVAL protocol control variable.
     * RFC 3626 Section 18.2.
     */
    TimeVal			_refresh_interval;

    /*
     * MPR sets.
     */

    /**
     * true if the MPR algorithm is enabled, false if all one-hop
     * neighbors willing to forward should always be considered as MPRs.
     */
    bool			_mpr_computation_enabled;

    /**
     * Section 16.1: MPR_COVERAGE Parameter.
     */
    uint32_t			_mpr_coverage;

    /**
     * A task which may be scheduled to recompute the global MPR set.
     */
    XorpTask			_mpr_recount_task;

    /**
     * The neighbors which select this node as an MPR.
     */
    set<OlsrTypes::NeighborID>	_mpr_selector_set;

    /**
     * The global set of neighbors which this node has selected as MPRs.
     */
    set<OlsrTypes::NeighborID>	_mpr_set;

    /*
     * Topology Control
     */

    /**
     * The TC_INTERVAL protocol control variable.
     * RFC 3626 Section 18.2.
     */
    TimeVal			_tc_interval;

    /**
     * The TC_REDUNDANCY protocol control variable.
     * Section 15.
     */
    OlsrTypes::TcRedundancyType	_tc_redundancy;

    /**
     * The TC broadcast timer.
     */
    XorpTimer			 _tc_timer;

    /**
     * The current state of the TC timer: STOPPED, RUNNING or FINISHING.
     */
    TcTimerState		_tc_timer_state;

    /**
     * The number of ticks remaining until the TC timer transitions from
     * FINISHING state to STOPPED state.
     */
    uint32_t			_tc_timer_ticks_remaining;

    /**
     * The current advertised neighbor sequence number.
     */
    uint16_t			_tc_current_ansn;

    /**
     * The previous advertised neighbor count.
     */
    uint16_t			_tc_previous_ans_count;

    /**
     * true if TC messages should be flooded immediately when an
     * MPR selector is deleted.
     */
    bool			_loss_triggered_tc_enabled;

    /**
     * true if TC messages should be flooded immediately when any
     * change in the ANSN is detected.
     */
    bool			_change_triggered_tc_enabled;

    /*
     * Link state databases.
     */

    /**
     * This node's links to neighbors.
     * RFC 3626 Section 4.2.1 Local Link Information Base
     */
    map<OlsrTypes::LogicalLinkID, LogicalLink*>	_links;

    /**
     * A map providing lookup of Link ID based on
     * remote and local protocol addresses in that order.
     * Used for processing incoming HELLOs.
     */
    map<pair<IPv4, IPv4>, OlsrTypes::LogicalLinkID>	_link_addr;

    /**
     * This node's neighbors.
     * RFC 3626 Section 4.3 Neighborhood Information Base
     */
    map<OlsrTypes::NeighborID, Neighbor*>   _neighbors;

    /**
     * A map providing lookup of Neighbor ID based on
     * the node's main address.
     */
    map<IPv4, OlsrTypes::NeighborID>	    _neighbor_addr;

    /**
     * The two-hop link table.
     *
     * This is not part of the RFC however we break the implementation
     * of two-hop neighbors into links and nodes to facilitate faster
     * convergence of MPR sets when links change in the vicinity.
     */
    map<OlsrTypes::TwoHopLinkID, TwoHopLink*>		_twohop_links;

    /**
     * A map providing lookup of two-hop link ID based on
     * the main addresses of the one-hop and two-hop neighbors, in
     * that order.
     */
    map<pair<IPv4, IPv4>, OlsrTypes::TwoHopLinkID>	_twohop_link_addrs;

    /**
     * The two-hop neighbor table.
     */
    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>	_twohop_nodes;

    /**
     * A map providing lookup of two-hop neighbor ID based on
     * its main protocol address and the next-hop used to reach it.
     */
    map<IPv4, OlsrTypes::TwoHopNodeID>			_twohop_node_addrs;
};

#endif // __OLSR_NEIGHBORHOOD_HH__
