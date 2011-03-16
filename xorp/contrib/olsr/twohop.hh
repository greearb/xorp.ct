// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/contrib/olsr/twohop.hh,v 1.3 2008/10/02 21:56:36 bms Exp $

#ifndef __OLSR_TWOHOP_HH__
#define __OLSR_TWOHOP_HH__

class Neighbor;
class Neighborhood;

// From now on, N1 is shorthand for "one-hop neighbor", and
// N2 is shorthand for "two-hop neighbor".

/**
 * @short A two-hop neighbor.
 *
 * When the last link expires, the entire instance expires.
 */
class TwoHopNeighbor {
  public:
    TwoHopNeighbor(EventLoop& ev,
		   Neighborhood* parent,
		   const OlsrTypes::TwoHopNodeID id,
		   const IPv4& main_addr,
		   const OlsrTypes::TwoHopLinkID tlid);

    inline OlsrTypes::TwoHopNodeID id() const { return _id; }
    inline IPv4 main_addr() const { return _main_addr; }

    string toStringBrief();

    /**
     * Associate this N2 with an N1, using an instance of
     * the association class TwoHopLink.
     *
     * @param tlid the ID of a two-hop link.
     */
    void add_twohop_link(const OlsrTypes::TwoHopLinkID tlid);

    /**
     * Delete the given N1 from the set of Neighbors which may
     * be used to reach this N2.
     *
     * @param tlid the ID of a two-hop link.
     * @return true if the last link to the TwoHopNeighbor was removed.
     */
    bool delete_twohop_link(const OlsrTypes::TwoHopLinkID tlid);

    size_t delete_all_twohop_links();

    inline const set<OlsrTypes::TwoHopLinkID>& twohop_links() const {
	return _twohop_links;
    }

    /**
     * Add the given neighbor as an MPR which covers this node.
     *
     * @param nid the N to count as one of this N2's covering nodes.
     */
    void add_covering_mpr(const OlsrTypes::NeighborID nid);

    /**
     * Withdraw the given neighbor as an MPR which covers this node.
     *
     * @param nid the N to count as one of this N2's covering nodes.
     */
    void withdraw_covering_mpr(const OlsrTypes::NeighborID nid);

    /**
     * Reset the MPR coverage state for this N2 node.
     */
    void reset_covering_mprs();

    /**
     * @return The number of MPRs which cover this node.
     */
    inline uint32_t coverage() const { return _coverage; }

    /**
     * @return true if this N2 is covered by at least N1.
     */
    inline bool is_covered() const { return _coverage > 0; }

    /**
     * @return The number of MPR candidates which cover this node.
     * This is not computed here but by Neighborhood, the parent.
     * Note: This is NOT the same as a neighbor's reachability.
     */
    inline uint32_t reachability() const { return _reachability; }

    /**
     * Set this N2's reachability.
     */
    inline void set_reachability(uint32_t value) { _reachability = value; }

    /**
     * @return true if this N2 is reachable.
     */
    inline bool is_reachable() const { return _reachability > 0; }

    /**
     * @return true if this N2 is strict.
     *
     * An N2 is strict if it appears only in the
     * two-hop neighborhood, and not in the one-hop neighborhood.
     */
    bool is_strict() const { return _is_strict; }

    /**
     * Set if this N2 is a strict two-hop neighbor.
     *
     * @param is_strict true if this N2 does NOT also appear as an N1.
     */
    void set_is_strict(bool is_strict) {
	_is_strict = is_strict;
    }

    /**
     * @return the set of two-hop links pointing to this N2 node.
     */
    const set<OlsrTypes::TwoHopLinkID>& twohop_links() {
	return _twohop_links;
    }

  private:
    EventLoop&		    _ev;
    Neighborhood*	    _parent;

    /**
     * Unique ID of this N2 node.
     */
    OlsrTypes::TwoHopNodeID	_id;

    /**
     * N_2hop_addr protocol variable.
     * Main protocol address of this N2 node.
     */
    IPv4		    _main_addr;

    /**
     * true if this N2 is NOT also an N1 node.
     */
    bool		    _is_strict;

    /**
     * The number of strict N1 which can
     * reach this strict N2, and which have been
     * selected as MPRs by the Neighborhood.
     */
    uint32_t		    _coverage;

    /**
     * The number of strict N1 which can reach this strict N2.
     */
    uint32_t		    _reachability;

    /**
     * Links by which this two-hop neighbor is reachable.
     */
    set<OlsrTypes::TwoHopLinkID>	    _twohop_links;
};

/**
 * @short A link between a Neighbor and a TwoHopNeighbor.
 *
 * Association class between TwoHopNeighbor and Neighbor.
 * TwoHopNeighbor cannot exist without TwoHopLink. Like Link and Neighbor,
 * there is a co-dependent relationship. Both TwoHopNeighbor and TwoHopLink
 * cannot exist without Neighbor.
 *
 * TwoHopLinks are uniquely identified by the ID of the two-hop neighbor
 * at the far end, and the Neighbor at the near end. When the last
 * TwoHopLink associated with a TwoHopNeighbor is deleted, the
 * TwoHopNeighbor MUST be deleted, to conform to the relations in the RFC.
 */
class TwoHopLink {
public:
    TwoHopLink(EventLoop& ev, Neighborhood* parent,
	       OlsrTypes::TwoHopLinkID tlid,
	       Neighbor* nexthop, const TimeVal& vtime);

    /**
     * @return A unique identifier for this link in the two-hop
     * neighborhood.
     */
    inline OlsrTypes::TwoHopLinkID id() const { return _id; }

    /**
     * @return A pointer to the strict one-hop neighbor at the
     * near end of this link.
     */
    inline Neighbor* nexthop() const { return _nexthop; }

    /**
     * @return A pointer to the strict two-hop neighbor at the
     * far end of this link.
     */
    inline TwoHopNeighbor* destination() const {
	XLOG_ASSERT(0 != _destination);	    // Catch programming errors.
	return _destination;
    }

    /**
     * Set the cached destination pointer.
     *
     * @param destination the TwoHopNeighbor at the end of this link.
     */
    inline void set_destination(TwoHopNeighbor* destination) {
	// Invariant: set_destination() should only be called once,
	// immediately after the TwoHopLink has been created, to
	// associate it with a TwoHopNeighbor.
	XLOG_ASSERT(0 == _destination);
	_destination = destination;
    }

    /**
     * @return The ID of the interface where the advertisement of this
     * TwoHopLink was last heard by a Neighbor.
     */
    inline OlsrTypes::FaceID face_id() const { return _face_id; }

    /**
     * @return ETX measurement at near end of link.
     */
    inline double near_etx() const { return _near_etx; }

    /**
     * @return ETX measurement at far end of link.
     */
    inline double far_etx() const { return _far_etx; }

    /**
     * Set the ID of the interface where this link was last heard.
     *
     * @param faceid the ID of the interface.
     */
    inline void set_face_id(const OlsrTypes::FaceID faceid) {
	_face_id = faceid;
    }

    /**
     * Set the ETX measurement at near end of link.
     *
     * @param near_etx near end ETX measurement.
     */
    inline void set_near_etx(const double& near_etx) {
	_near_etx = near_etx;
    }

    /**
     * Set the ETX measurement at far end of link.
     *
     * @param far_etx far end ETX measurement.
     */
    inline void set_far_etx(const double& far_etx) {
	_far_etx = far_etx;
    }

    /**
     * @return the amount of time remaining until this two-hop link expires.
     */
    inline TimeVal time_remaining() const {
	TimeVal tv;
	_expiry_timer.time_remaining(tv);
	return tv;
    }

    /**
     * Update this link's expiry timer.
     *
     * @param vtime the amount of time from now til when the link expires.
     */
    void update_timer(const TimeVal& vtime);

    /**
     * Callback method to: tell the parent that this two-hop link
     * has now expired.
     */
    void event_dead();

private:
    EventLoop&			_ev;
    Neighborhood*		_parent;

    /**
     * A unique identifier for this object.
     */
    OlsrTypes::TwoHopLinkID	_id;

    /**
     * The strict one-hop neighbor used to reach the two-hop neighbor
     * referenced by _far_id.
     */
    Neighbor*			_nexthop;

    /**
     * The two-hop neighbor at the end of this link.
     */
    TwoHopNeighbor*		_destination;

    /**
     * The ID of the interface where the advertisement of this link
     * was last heard.
     */
    OlsrTypes::FaceID		_face_id;

    /**
     * The time at which this two-hop link expires.
     */
    XorpTimer			_expiry_timer;

    /**
     * The ETX as measured at the near end of this link.  Optional.
     */
    double			_near_etx;

    /**
     * The ETX as measured at the far end of this link.  Optional.
     */
    double			_far_etx;
};

#endif // __OLSR_TWOHOP_HH__
