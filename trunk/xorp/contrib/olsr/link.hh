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

// $XORP: xorp/contrib/olsr/link.hh,v 1.3 2008/10/02 21:56:34 bms Exp $

#ifndef __OLSR_LINK_HH__
#define __OLSR_LINK_HH__

class Neighbor;
class Neighborhood;

/**
 * @short Logical link used to reach a directly reachable Neighbor.
 *
 * LogicalLink is so named because it is mostly independent of the
 * underlying link layer. OLSR uses link-layer broadcasts or multicasts
 * for the purposes of neighbor discovery, and links are created through
 * the exchange of HELLO messages. LogicalLink may be associated with
 * link-layer specific state, e.g. that of an 802.11s mesh portal.
 */
class LogicalLink {
  public:
    LogicalLink(Neighborhood* tm,
	 EventLoop& eventloop,
	 const OlsrTypes::LogicalLinkID id,
	 const TimeVal& vtime,
	 const IPv4& remote_addr,
	 const IPv4& local_addr);

    /**
     * @return the ID of this link.
     */
    inline OlsrTypes::LogicalLinkID id() const { return _id; }

    /**
     * @return the ID of the neighbor at the far end of this link.
     */
    inline OlsrTypes::NeighborID neighbor_id() const { return _neighborid; }

    /**
     * Set the ID of the neighbor at the far end of this link.
     *
     * @param neighborid the ID of the neighbor.
     */
    inline void set_neighbor_id(OlsrTypes::NeighborID neighborid) {
	_neighborid = neighborid;
    }

    /**
     * @return the cached Neighbor pointer.
     */
    inline Neighbor* destination() const {
	XLOG_ASSERT(0 != _destination);	// Pilot error if we get here.
	return _destination;
    }

    /**
     * Set the cached neighbor pointer.
     *
     * @param n the Neighbor pointer.
     */
    inline void set_destination(Neighbor* n) { _destination = n; }

    /**
     * @return the ID of the local interface where this link resides.
     */
    inline OlsrTypes::FaceID faceid() const { return _faceid; }

    /**
     * Set the ID of the local interface where this link resides.
     *
     * @param faceid the ID of the local interface.
     */
    inline void set_faceid(OlsrTypes::FaceID faceid) {
	_faceid = faceid;
    }

    /**
     * @return the protocol address of the local interface.
     */
    inline IPv4 local_addr() const { return _local_addr; }

    /**
     * @return the protocol address of the neighbor's interface.
     */
    inline IPv4 remote_addr() const { return _remote_addr; }

    /**
     * @return the near end ETX measurement.
     */
    inline double near_etx() const { return _near_etx; }

    /**
     * @return the far end ETX measurement.
     */
    inline double far_etx() const { return _far_etx; }

    /**
     * Update the link timers, based on the information present
     * in a HELLO message.
     * Section 7.1.1, 2: HELLO Message Processing.
     *
     * @param vtime The validity time present in the HELLO message.
     * @param saw_self true if the neighbor advertised our own address.
     * @param lc the link code with which our address was advertised.
     */
    void update_timers(const TimeVal& vtime,
		       bool saw_self = false,
		       const LinkCode lc = LinkCode());

    /**
     * Determine the current state of the link.
     * Section 6.2, 1: HELLO Message Generation, Link Type.
     *
     * @return The link type code representing this link's state.
     */
    OlsrTypes::LinkType link_type() const;

    /**
     * @return true if this link is symmetric.
     */
    inline bool is_sym() const {
	return link_type() == OlsrTypes::SYM_LINK;
    }

    /**
     * @return the amount of time remaining until the SYM timer fires.
     */
    inline TimeVal sym_time_remaining() const {
	TimeVal tv;
	_sym_timer.time_remaining(tv);
	return tv;
    }

    /**
     * @return the amount of time remaining until the ASYM timer fires.
     */
    inline TimeVal asym_time_remaining() const {
	TimeVal tv;
	_asym_timer.time_remaining(tv);
	return tv;
    }

    /**
     * @return the amount of time remaining until the DEAD timer fires.
     */
    inline TimeVal time_remaining() const {
	TimeVal tv;
	_dead_timer.time_remaining(tv);
	return tv;
    }

    /**
     * Determine if a link is pending (i.e. not yet established).
     *
     * @return true if the link is pending.
     */
    inline bool pending() const { return _is_pending; }

    /**
     * Callback method to: service a LogicalLink's SYM timer.
     */
    void event_sym_timer();

    /**
     * Callback method to: service a LogicalLink's ASYM timer.
     */
    void event_asym_timer();

    /**
     * Callback method to: service a LogicalLink's LOST timer.
     */
    void event_lost_timer();

    /**
     * Callback method to: service a LogicalLink's DEAD timer.
     * This is immediately passed to the parent Neighborhood as
     * the link may be deleted.
     */
    void event_dead_timer();

  private:
    Neighborhood*	_nh;
    EventLoop&		_eventloop;

    /**
     * Unique identifier of link.
     */
    OlsrTypes::LogicalLinkID	    _id;

    /**
     * ID of interface where this link resides.
     */
    OlsrTypes::FaceID	    _faceid;

    /**
     * ID of neighbor at other end of link.
     */
    OlsrTypes::NeighborID   _neighborid;

    /**
     * Cached pointer to Neighbor.
     */
    Neighbor*		    _destination;

    /**
     * L_neighbor_iface_addr protocol variable.
     * The protocol address of the neighbor's interface.
     */
    IPv4		    _remote_addr;

    /**
     * L_local_iface_addr protocol variable.
     * The protocol address of our local interface.
     */
    IPv4		    _local_addr;

    //
    // Link state timers.
    //

    /**
     * L_SYM_time protocol variable.
     */
    XorpTimer		    _sym_timer;

    /**
     * L_ASYM_time protocol variable.
     */
    XorpTimer		    _asym_timer;

    /**
     * L_LOST_LINK_time protocol variable.
     */
    XorpTimer		    _lost_timer;

    /**
     * L_time protocol variable.
     */
    XorpTimer		    _dead_timer;

    //
    // Link etx fields.
    //

    /**
     * L_link_pending protocol variable.
     */
    bool		    _is_pending;

    /**
     * L_link_etx protocol variable.
     */
    double		    _near_etx;

    /**
     * ETX far-end measurement.
     */
    double		    _far_etx;
};

#endif // __OLSR_LINK_HH__
