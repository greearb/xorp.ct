// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/vlink.hh,v 1.13 2008/07/23 05:11:10 pavlin Exp $

#ifndef __OSPF_VLINK_HH__
#define __OSPF_VLINK_HH__

/**
 * Manage all the state for virtual links.
 */
template <typename A>
class Vlink {
 public:
    /**
     * Add a virtual link router ID to the database.
     */
    bool create_vlink(OspfTypes::RouterID rid);

    /**
     * Remove this router ID from the database.
     */
    bool delete_vlink(OspfTypes::RouterID rid);

    /**
     * Set the transmit area through which the virtual link is formed.
     */
    bool set_transit_area(OspfTypes::RouterID rid,
			  OspfTypes::AreaID transit_area);

    /**
     * Get the transmit area information.
     */
    bool get_transit_area(OspfTypes::RouterID rid,
			  OspfTypes::AreaID& transit_area) const;

    /**
     * Set state to know if the area has been notified about this
     * virtual link.
     */
    bool set_transit_area_notified(OspfTypes::RouterID rid, bool state);

    /**
     * Has the area been notified?
     */
    bool get_transit_area_notified(OspfTypes::RouterID rid) const;

    /**
     * Associate the endpoint addresses with this virtual link.
     */
    bool add_address(OspfTypes::RouterID rid, A source, A destination);

    /**
     * Get the endpoint addresses of this virtual link.
     */
    bool get_address(OspfTypes::RouterID rid, A& source, A& destination);

    /**
     * Provide an interface and vif for this router ID. Must not be
     * called before the address information has been provided.
     */
    bool get_interface_vif(OspfTypes::RouterID rid, string& interface,
			   string& vif) const;

    /**
     * Save the peerid that has been allocted to this virtual link.
     */
    bool add_peerid(OspfTypes::RouterID rid, OspfTypes::PeerID peerid);

    /**
     * Get the associated peerid.
     */
    OspfTypes::PeerID get_peerid(OspfTypes::RouterID rid) const;

    /**
     * The phyical interface and vif that should be used for transmission.
     */
    bool set_physical_interface_vif(OspfTypes::RouterID rid, string& interface,
				    string& vif);

    /**
     * Given the source and destination addresses return the interface
     * and vif.
     */
    bool get_physical_interface_vif(A source, A destination, string& interface,
				    string& vif) const;

    /**
     * Given the source and destination address find the PeerID of the
     * relevant virtual link.
     */
    OspfTypes::PeerID get_peerid(A source, A destination) const;

    /**
     * Get the list of virtual links (router ids) that flow through
     * this area.
     */
    void get_router_ids(OspfTypes::AreaID transit_area,
			list<OspfTypes::RouterID>& rids) const;

    /**
     * This area has been removed mark all the notified for this area
     * to false. Allowing an area to be removed and then brought back.
     */
    void area_removed(OspfTypes::AreaID area);

 private:
    /**
     * State about each virtual link.
     */
    struct Vstate {
	Vstate() : 
	    _peerid(OspfTypes::ALLPEERS),	// Illegal value for a PeerID.
	    _transit_area(OspfTypes::BACKBONE), // Again an illegal value.
	    _notified(false)
	{}

	OspfTypes::PeerID _peerid;		// PeerID of virtual link
	OspfTypes::AreaID _transit_area;	// Transit area for the link
	// True if the transit area has been notified.
	bool _notified;
	A _source;				// Source address
	A _destination;				// Destination address
	// Required for transmission.
	string _physical_interface;		// Actual interface
	string _physical_vif;			// Actual vif
    };

    map <OspfTypes::RouterID, Vstate> _vlinks;
};

#endif // __OSPF_VLINK_HH__
