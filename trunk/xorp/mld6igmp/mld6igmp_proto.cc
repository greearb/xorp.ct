// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_proto.cc,v 1.8 2004/02/22 03:13:20 pavlin Exp $"


//
// Internet Group Management Protocol implementation.
// IGMPv1 and IGMPv2 (RFC 2236)
//
// AND
//
// Multicast Listener Discovery protocol implementation.
// MLDv1 (RFC 2710)
//



#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_vif.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * Mld6igmpVif::other_querier_timer_timeout:
 * 
 * Timeout: the previous querier has expired. I will become the querier.
 **/
void
Mld6igmpVif::other_querier_timer_timeout()
{
    IPvX ipaddr_zero(family());		// XXX: ANY
    
    if (primary_addr() == IPvX::ZERO(family())) {
	// XXX: the vif address is unknown; this cannot happen if the
	// vif status is UP.
	XLOG_ASSERT(! is_up());
	return;
    }
    
    set_querier_addr(primary_addr());
    _proto_flags |= MLD6IGMP_VIF_QUERIER;
    
    if (proto_is_igmp()) {
	// Now I am the querier. Send a general membership query.
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ? 0:
		      (IGMP_QUERY_RESPONSE_INTERVAL * IGMP_TIMER_SCALE),
		      ipaddr_zero);
	_startup_query_count = 0;		// XXX: not a startup case
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(IGMP_QUERY_INTERVAL, 0),
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	// Now I am the querier. Send a general membership query.
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      (MLD_QUERY_RESPONSE_INTERVAL * MLD_TIMER_SCALE),
		      ipaddr_zero);
	_startup_query_count = 0;		// XXX: not a startup case
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(MLD_QUERY_INTERVAL, 0),
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
}

/**
 * Mld6igmpVif::query_timer_timeout:
 * 
 * Timeout: time to send a membership query.
 **/
void
Mld6igmpVif::query_timer_timeout()
{
    IPvX ipaddr_zero(family());			// XXX: ANY
    int query_interval;
    
    if (!(_proto_flags & MLD6IGMP_VIF_QUERIER))
	return;		// I am not the querier anymore. Ignore.
    
    if (proto_is_igmp()) {
	// Send a general membership query
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ? 0:
		      (IGMP_QUERY_RESPONSE_INTERVAL * IGMP_TIMER_SCALE),
		      ipaddr_zero);
	if (_startup_query_count > 0)
	    _startup_query_count--;
	if (_startup_query_count > 0)
	    query_interval = IGMP_STARTUP_QUERY_INTERVAL;
	else
	    query_interval = IGMP_QUERY_INTERVAL;
    }
    
#if HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	// Send a general membership query
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      is_igmpv1_mode() ? 0:
		      (MLD_QUERY_RESPONSE_INTERVAL * MLD_TIMER_SCALE),
		      ipaddr_zero);
	if (_startup_query_count > 0)
	    _startup_query_count--;
	if (_startup_query_count > 0)
	    query_interval = MLD_STARTUP_QUERY_INTERVAL;
	else
	    query_interval = MLD_QUERY_INTERVAL;
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
    
    _query_timer =
	mld6igmp_node().eventloop().new_oneoff_after(
	    TimeVal(query_interval, 0),
	    callback(this, &Mld6igmpVif::query_timer_timeout));
}
