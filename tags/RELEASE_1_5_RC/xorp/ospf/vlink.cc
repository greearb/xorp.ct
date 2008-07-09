// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/vlink.cc,v 1.14 2007/02/23 00:01:13 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "vlink.hh"

template <typename A>
bool
Vlink<A>::create_vlink(OspfTypes::RouterID rid)
{
    if (0 != _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s already exists", pr_id(rid).c_str());
	return false;
    }

    Vstate v;
    _vlinks[rid] = v;

    return true;
}

template <typename A>
bool
Vlink<A>::delete_vlink(OspfTypes::RouterID rid)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    _vlinks.erase(_vlinks.find(rid));

    return true;
}

template <typename A>
bool
Vlink<A>::set_transit_area(OspfTypes::RouterID rid,
			   OspfTypes::AreaID transit_area)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._transit_area = transit_area;

    return true;
}

template <typename A>
bool
Vlink<A>::get_transit_area(OspfTypes::RouterID rid,
			   OspfTypes::AreaID& transit_area) const
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::const_iterator i =
	_vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    transit_area = i->second._transit_area;

    return true;
}

template <typename A>
bool
Vlink<A>::set_transit_area_notified(OspfTypes::RouterID rid,
				    bool notified)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._notified = notified;

    return true;
}

template <typename A>
bool
Vlink<A>::get_transit_area_notified(OspfTypes::RouterID rid) const
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::const_iterator i =
	_vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    return i->second._notified;
}

template <typename A>
bool
Vlink<A>::add_address(OspfTypes::RouterID rid, A source, A destination)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._source = source;
    i->second._destination = destination;

    return true;
}

template <typename A>
bool
Vlink<A>::get_address(OspfTypes::RouterID rid, A& source, A& destination)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    source = i->second._source;
    destination = i->second._destination;

    return true;
}

template <typename A>
bool
Vlink<A>::get_interface_vif(OspfTypes::RouterID rid, string& interface,
			    string& vif) const
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::const_iterator i =
	_vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

//     XLOG_ASSERT(A::ZERO() != i->second._source);
//     XLOG_ASSERT(A::ZERO() != i->second._destination);

    interface = VLINK;
    vif = pr_id(rid);

    return true;
}

template <typename A>
bool
Vlink<A>::add_peerid(OspfTypes::RouterID rid, OspfTypes::PeerID peerid)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._peerid = peerid;

    return true;
}

template <typename A>
OspfTypes::PeerID
Vlink<A>::get_peerid(OspfTypes::RouterID rid) const
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return OspfTypes::ALLPEERS;
    }

    typename map <OspfTypes::RouterID, Vstate>::const_iterator i = 
	_vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    return i->second._peerid;
}

template <typename A>
bool
Vlink<A>::set_physical_interface_vif(OspfTypes::RouterID rid,
				     string& interface,
				     string& vif)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._physical_interface = interface;
    i->second._physical_vif = vif;

    return true;
}

template <typename A>
bool
Vlink<A>::get_physical_interface_vif(A source, A destination,
				     string& interface,
				     string& vif) const
{
    typename map<OspfTypes::RouterID, Vstate>::const_iterator i;
    for(i = _vlinks.begin(); i != _vlinks.end(); i++) {
	if (i->second._source == source &&
	    i->second._destination == destination) {
	    interface = i->second._physical_interface; 
	    vif = i->second._physical_vif;
	    return true;
	}
    }

    return false;
}

template <typename A>
OspfTypes::PeerID
Vlink<A>::get_peerid(A source, A destination) const
{
    typename map<OspfTypes::RouterID, Vstate>::const_iterator i;
    for(i = _vlinks.begin(); i != _vlinks.end(); i++) {
	if (i->second._source == source &&
	    i->second._destination == destination) {
	    return i->second._peerid; 
	}
    }

    return OspfTypes::ALLPEERS;
}

template <typename A>
void
Vlink<A>::get_router_ids(OspfTypes::AreaID transit_area,
			 list<OspfTypes::RouterID>& rids) const
{
    typename map<OspfTypes::RouterID, Vstate>::const_iterator i;
    for(i = _vlinks.begin(); i != _vlinks.end(); i++) {
	if (i->second._transit_area == transit_area) {
	    rids.push_back(i->first);
	}
    }
}

template <typename A>
void
Vlink<A>::area_removed(OspfTypes::AreaID area)
{
    typename map<OspfTypes::RouterID, Vstate>::iterator i;
    for(i = _vlinks.begin(); i != _vlinks.end(); i++) {
	if (i->second._transit_area == area) {
	    i->second._notified = false;
	}
    }
}

template class Vlink<IPv4>;
template class Vlink<IPv6>;
