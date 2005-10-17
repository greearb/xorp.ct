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

#ident "$XORP: xorp/ospf/external.cc,v 1.2 2005/10/17 03:37:58 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "external.hh"

template <typename A>
External<A>::External(Ospf<A>& ospf,
		      map<OspfTypes::AreaID, AreaRouter<A> *>& areas)
    : _ospf(ospf), _areas(areas)
{
}

template <typename A>
bool
External<A>::announce(OspfTypes::AreaID area, Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
    XLOG_ASSERT(!lsar->get_self_originating());

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_announce(lsar, false /* push */);
    }

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::MaxAge -
				 lsar->get_header().get_ls_age(), 0),
			 callback(this, &External<A>::maxage_reached, lsar));
    
    return true;
}

template <typename A>
bool
External<A>::shove(OspfTypes::AreaID area)
{
    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_shove();
    }

    return true;
}

template <typename A>
void 
External<A>::push(AreaRouter<A> *area_router)
{
    XLOG_ASSERT(area_router);

    ASExternalDatabase::iterator i;
    for(i = _lsas.begin(); i != _lsas.end(); i++)
	area_router->external_announce((*i), true /* push */);
}

template <typename A>
ASExternalDatabase::iterator
External<A>::find_lsa(Lsa::LsaRef lsar)
{
    return _lsas.find(lsar);
}

template <typename A>
void
External<A>::update_lsa(Lsa::LsaRef lsar)
{
    ASExternalDatabase::iterator i = _lsas.find(lsar);
    if (i != _lsas.end()) {
	(*i)->invalidate();
	_lsas.erase(i);
    }
    _lsas.insert(lsar);
}

template <typename A>
void
External<A>::delete_lsa(Lsa::LsaRef lsar)
{
    ASExternalDatabase::iterator i = find_lsa(lsar);
    XLOG_ASSERT(i != _lsas.end());
    _lsas.erase(i);
}

template <typename A>
void
External<A>::maxage_reached(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
    XLOG_ASSERT(!lsar->get_self_originating());

    ASExternalDatabase::iterator i = find_lsa(lsar);
    if (i != _lsas.end())
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));

    if (!lsar->maxage()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	lsar->update_age(now);
    }

    if (!lsar->maxage())
	XLOG_FATAL("LSA is not MaxAge %s", cstring(*lsar));
    
    delete_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator ia;
    for (ia = _areas.begin(); ia != _areas.end(); ia++) {
	(*ia).second->external_withdraw(lsar);
    }

    // Clear the timer otherwise there is a circular dependency.
    // The LSA contains a XorpTimer that points back to the LSA.
    lsar->get_timer().clear();
}

ASExternalDatabase::iterator
ASExternalDatabase::find(Lsa::LsaRef lsar)
{
    return _lsas.find(lsar);
#if	0
    Lsa_header& header = lsar->get_header();
    uint32_t link_state_id = header.get_link_state_id();
    uint32_t advertising_router = header.get_advertising_router();
    list <Lsa::LsaRef>::iterator i;
    for(i = _lsas.begin(); i != _lsas.end(); i++) {
	if ((*i)->get_header().get_link_state_id() == link_state_id &&
	    (*i)->get_header().get_advertising_router() == advertising_router){
	    return i;
	}
    }	

    return _lsas.end();
#endif
}

template class External<IPv4>;
template class External<IPv6>;
