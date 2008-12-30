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

// $XORP: xorp/contrib/olsr/neighbor.hh,v 1.3 2008/07/23 05:09:51 pavlin Exp $

#ifndef __OLSR_NEIGHBOR_HH__
#define __OLSR_NEIGHBOR_HH__

class Neighborhood;

/**
 * @short Wrapper for LogicalLink::is_sym().
 *
 * As LogicalLinkID is an indirect reference, it must be looked up
 * via Neighborhood.
 * Model of UnaryPredicate.
 */
struct IsLinkSymmetricPred {
    Neighborhood* _nh;

    IsLinkSymmetricPred(Neighborhood* nh) : _nh(nh) {}

    /**
     * @return true if the link identified by linkid is symmetric.
     */
    bool operator()(OlsrTypes::LogicalLinkID linkid);
};

/**
 * @short Wrapper for TwoHopNeighbor::is_strict().
 *
 * As TwoHopLinkID is an indirect reference, it must be looked up
 * via Neighborhood.
 * Model of UnaryPredicate.
 */
struct IsTwoHopLinkStrictPred {
    Neighborhood* _nh;

    IsTwoHopLinkStrictPred(Neighborhood* nh) : _nh(nh) {}

    /**
     * @return true if the two-hop neighbor referenced by tlid is a
     *              strict two-hop neighbor.
     */
    bool operator()(OlsrTypes::TwoHopLinkID tlid);
};

/**
 * @short A one-hop neighbor.
 */
class Neighbor {
  public:
    Neighbor(EventLoop& ev,
	     Neighborhood* parent,
	     const OlsrTypes::NeighborID nid,
	     const IPv4& main_addr,
	     const OlsrTypes::LogicalLinkID linkid);

    inline OlsrTypes::NeighborID id() const { return _id; }

    inline const IPv4 main_addr() const { return _main_addr; }

    OlsrTypes::NeighborType neighbor_type() const;

    /**
     * Get the willingness-to-forward for this neighbor.
     *
     * @return the neighbor's willingness.
     */
    inline uint8_t willingness() const { return _willingness; }

    /**
     * Set the willingness-to-forward for this neighbor.
     *
     * This may trigger a recount of the node's MPR status; a node
     * with willingness WILL_NEVER may never be an MPR.
     *
     * @param value the new value for this neighbor's willingness property.
     */
    void set_willingness(OlsrTypes::WillType value); 

    /**
     * Return the MPR selector state of this neighbor.
     *
     * @return true if this neighbor has previously chosen this node as an
     *         MPR in a HELLO message, and the tuple is still valid.
     */
    bool is_mpr_selector() const;

    /**
     * Set the MPR selector state of this neighbor.
     *
     * @param value true if this neighbor selects this OLSR node as an MPR.
     * @param expiry_time The expiry time for MPR selector status. Ignored
     *                    if value is false.
     */
    void set_is_mpr_selector(bool value, const TimeVal& expiry_time);

    /**
     * Return the previously stored result of MPR selection.
     * This is computed by the deferred MPR selection task in Neighborhood.
     *
     * @return true if this neighbor has been selected as an MPR.
     */
    inline bool is_mpr() const { return _is_mpr; }

    inline void set_is_mpr(bool value) { _is_mpr = value; }

    /**
     * @return true if this neighbor is a persistent MPR candidate.
     */
    inline bool is_persistent_cand_mpr() const {
	return _willingness == OlsrTypes::WILL_ALWAYS;
    }

    /**
     * Return the MPR candidacy of this neighbor.
     * MPR candidacy is computed whenever Neighbor's link state changes.
     *
     * TODO: NOTE WELL: If a neighbor is not reachable due to no good ETX
     * links, it MUST NOT be considered as an MPR candidate. Currently
     * the code does not take account of this.
     *
     * @return true if this neighbor is a candidate MPR, that is,
     *              there is at least one link to a strict two-hop neighbor,
     *              and willingness is not WILL_NEVER.
     */
    bool is_cand_mpr();

    inline bool is_sym() const { return _is_sym; }
    inline void set_is_sym(bool value) { _is_sym = value; }

    inline const set<OlsrTypes::LogicalLinkID>& links() const {
	return _links;
    }

    inline const set<OlsrTypes::TwoHopLinkID>& twohop_links() const {
	return _twohop_links;
    }

    /**
     * Associate a link tuple with a Neighbor or update an
     * existing one. N_status is updated.
     *
     * @param linkid the ID of the link to associate with this Neighbor.
     */
    void update_link(const OlsrTypes::LogicalLinkID linkid);

    /**
     * Disassociate a link tuple from a Neighbor.
     * N_status is updated.
     *
     * @param linkid the ID of the link to disassociate from this Neighbor.
     *
     * @return true if there are no links to this Neighbor and it should
     * now be deleted.
     */
    bool delete_link(const OlsrTypes::LogicalLinkID linkid);

    /**
     * Associate a two-hop link with a Neighbor.
     * MPR candidacy and degree are updated.
     *
     * @param tlid the ID of the two-hop link to associate with this Neighbor.
     */
    void add_twohop_link(const OlsrTypes::TwoHopLinkID tlid);

    /**
     * Disassociate a two-hop link from a Neighbor.
     * MPR candidacy and degree are updated.
     *
     * @param tlid the ID of the two-hop link to disassociate from
     *             this Neighbor.
     * @return true if this Neighbor has no more two-hop links.
     */
    bool delete_twohop_link(const OlsrTypes::TwoHopLinkID tlid);

    /**
     * Disassociate all two-hop links from a Neighbor.
     * MPR candidacy and degree are updated.
     *
     * @return the number of links which have been deleted.
     */
    size_t delete_all_twohop_links();

    /**
     * Callback method to: process expiry of an MPR selector tuple.
     */
    void event_mpr_selector_expired();

    /**
     * Return the degree of this neighbor.
     *
     * @return The number of symmetric neighbors of this neighbor,
     *         excluding this node, and other one-hop neighbors.
     */
    inline uint32_t degree() const { return _degree; }

    /**
     * Return the previously computed reachability of this neighbor.
     *
     * @return The number of strict, uncovered, two-hop neighbors to which
     *         this neighbor has a link.
     */
    inline uint32_t reachability() const { return _reachability; }

    /**
     * Store the reachability of this neighbor.
     *
     * Typically called during MPR selection, as reachability can only
     * be computed during MPR selection.
     *
     * @param value The number of strict, uncovered, two-hop neighbors
     *              to which this neighbor has a link.
     */
    inline void set_reachability(uint32_t value) { _reachability = value; }

    /**
     * Return if this neighbor is advertised in TC messages.
     *
     * @return true if this neighbor is advertised in TC messages.
     */
    inline bool is_advertised() const { return _is_advertised; }

    /**
     * Store the "is in advertised neighbor set" status.
     *
     * @param value true if this neighbor is advertised in TC.
     */
    inline void set_is_advertised(bool value) { _is_advertised = value; }

  protected:
    /**
     * Re-count the degree of this strict one-hop Neighbor.
     *
     * Section 8.3.1 defines the degree of a one-hop neighbor as the number
     * of symmetric neighbors of the neighbor, excluding any members of the
     * one-hop neighborhood, and excluding this node.
     *
     * Triggered by a two-hop link state change.
     * TODO: Distribute the computation by pushing the responsibility for
     * signalling the change of state to TwoHopNeighbor, rather than
     * doing it here.
     */
    void recount_degree();

    /**
     * Update the MPR candidacy of this neighbor.
     *
     * Triggered by a one-hop or two-hop link state change.
     * The Neighborhood is notified of the change.
     *
     * @param was_cand_mpr true if the node was an MPR candidate before
     *                     any other criteria changed.
     * @return true if the node is now an MPR candidate.
     */
    bool update_cand_mpr(bool was_cand_mpr);

  private:
    EventLoop&		    _eventloop;
    Neighborhood*	    _parent;

    OlsrTypes::NeighborID   _id;

    /**
     * @short main address of this neighbor.
     */
    IPv4		    _main_addr;

    /**
     * @short true if neighbor is an MPR.
     *
     * Note: This may be out of sync with the actual MPR state; it
     * does not get updated until an MPR selection is performed
     * from Neighborhood (the computation is not fully distributed).
     */
    bool		    _is_mpr;

    /**
     * @short true if neighbor is symmetric.
     */
    bool		    _is_sym;

    /**
     * @short willingness to forward traffic.
     */
    OlsrTypes::WillType	    _willingness;

    /**
     * @short the degree of this neighbor.
     */
    uint32_t		    _degree;

    /**
     * @short the reachability of this neighbor.
     */
    uint32_t		    _reachability;

    /**
     * @short true if neighbor is advertised in TC messages.
     */
    bool		    _is_advertised;

    /**
     * @short A timer which is scheduled if and only if the neighbor
     * selects this node as an MPR.
     */
    XorpTimer		    _mpr_selector_timer;

    /**
     * @short Links which reference this neighbor.
     */
    set<OlsrTypes::LogicalLinkID>	_links;

    /**
     * @short Links to all N2, strict and non-strict, reachable via this N.
     */
    set<OlsrTypes::TwoHopLinkID>	_twohop_links;
};

#endif // __OLSR_NEIGHBOR_HH__
