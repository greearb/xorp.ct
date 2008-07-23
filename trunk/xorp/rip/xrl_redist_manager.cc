// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rip/xrl_redist_manager.cc,v 1.20 2008/01/04 03:17:34 pavlin Exp $"

// #define DEBUG_LOGGING

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "system.hh"
#include "xrl_redist_manager.hh"


// ----------------------------------------------------------------------------
// XrlRedistManager Implementation

template <typename A>
XrlRedistManager<A>::XrlRedistManager(System<A>& system)
    : _rr(system.route_db())
{
}

template <typename A>
XrlRedistManager<A>::~XrlRedistManager()
{
}

template <typename A>
int
XrlRedistManager<A>::startup()
{
    if (status() == SERVICE_READY) {
	set_status(SERVICE_RUNNING);
	return (XORP_OK);
    }
    return (XORP_ERROR);
}

template <typename A>
int
XrlRedistManager<A>::shutdown()
{
    if (status() != SERVICE_RUNNING)
	return (XORP_ERROR);

    _rr.withdraw_routes();

    set_status(SERVICE_SHUTDOWN);

    return (XORP_OK);
}

template <typename A>
void
XrlRedistManager<A>::add_route(const Net&		net,
			       const Addr&		nh,
			       const string&		ifname,
			       const string&		vifname,
			       uint16_t			cost,
			       uint16_t			tag,
			       const PolicyTags&	policytags)
{
    debug_msg("got redist add_route for %s %s\n",
	      net.str().c_str(), nh.str().c_str());

    // XXX: Ignore link-local routes
    if (net.masked_addr().is_linklocal_unicast())
	return;

    _rr.add_route(net, nh, ifname, vifname, cost, tag, policytags);
}

template <typename A>
void
XrlRedistManager<A>::delete_route(const Net& net)
{
    // XXX: Ignore link-local routes
    if (net.masked_addr().is_linklocal_unicast())
	return;

    _rr.expire_route(net);
}

#ifdef INSTANTIATE_IPV4
template class XrlRedistManager<IPv4>;
#endif /* INSTANTIATE_IPV4 */

#ifdef INSTANTIATE_IPV6
template class XrlRedistManager<IPv6>;
#endif /* INSTANTIATE_IPV6 */
