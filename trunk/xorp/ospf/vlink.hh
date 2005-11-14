// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ifndef __OSPF_VLINK_HH__
#define __OSPF_VLINK_HH__

/**
 * Manage all the state for virtual links.
 */
template <typename A>
class Vlink {
 public:
    bool create_vlink(OspfTypes::RouterID rid);
    bool delete_vlink(OspfTypes::RouterID rid);
    bool set_transit_area(OspfTypes::RouterID rid,
			  OspfTypes::AreaID transit_area);
    bool get_transit_area(OspfTypes::RouterID rid,
			  OspfTypes::AreaID& transit_area);
 private:
    /**
     * State about each virtual link.
     */
    struct Vstate {
	Vstate() : 
	    _peerid(ALLPEERS),	// An illegal value for a PeerID.
	    _transit_area(OspfTypes::BACKBONE) // Again an illegal value.
	{}

	PeerID _peerid;				// PeerID of virtual link
	OspfTypes::AreaID _transit_area;	// Transit area for the link
    };

    map <OspfTypes::RouterID, Vstate> _vlinks;
};

#endif // __OSPF_VLINK_HH__
