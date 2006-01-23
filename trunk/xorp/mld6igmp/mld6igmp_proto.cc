// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/mld6igmp/mld6igmp_proto.cc,v 1.17 2005/08/25 19:01:32 pavlin Exp $"


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
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

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
    string dummy_error_msg;

    UNUSED(ipaddr_zero);
    
    if (primary_addr() == IPvX::ZERO(family())) {
	// XXX: the vif address is unknown; this cannot happen if the
	// vif status is UP.
	XLOG_ASSERT(! is_up());
	return;
    }
    
    set_querier_addr(primary_addr());
    _proto_flags |= MLD6IGMP_VIF_QUERIER;
    
#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp()) {
	// Now I am the querier. Send a general membership query.
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * IGMP_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ? 0: scaled_max_resp_time.sec(),
		      ipaddr_zero,
		      dummy_error_msg);
	_startup_query_count = 0;		// XXX: not a startup case
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		query_interval().get(),
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	// Now I am the querier. Send a general membership query.
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * MLD_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      scaled_max_resp_time.sec(),
		      ipaddr_zero,
		      dummy_error_msg);
	_startup_query_count = 0;		// XXX: not a startup case
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		query_interval().get(),
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
    TimeVal interval;
    string dummy_error_msg;

    UNUSED(ipaddr_zero);
    UNUSED(interval);
    UNUSED(dummy_error_msg);

    if (!(_proto_flags & MLD6IGMP_VIF_QUERIER))
	return;		// I am not the querier anymore. Ignore.

#if HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp()) {
	// Send a general membership query
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * IGMP_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ? 0: scaled_max_resp_time.sec(),
		      ipaddr_zero,
		      dummy_error_msg);
	if (_startup_query_count > 0)
	    _startup_query_count--;
	if (_startup_query_count > 0)
	    interval = query_interval().get() / 4; // "Startup Query Interval"
	else
	    interval = query_interval().get();

	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
	        interval,
		callback(this, &Mld6igmpVif::query_timer_timeout)
		);
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING

#if HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	// Send a general membership query
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * MLD_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      is_igmpv1_mode() ? 0: scaled_max_resp_time.sec(),
		      ipaddr_zero,
		      dummy_error_msg);
	if (_startup_query_count > 0)
	    _startup_query_count--;
	if (_startup_query_count > 0)
	    interval = query_interval().get() / 4; // "Startup Query Interval"
	else
	    interval = query_interval().get();

	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		interval,
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
}
