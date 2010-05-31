// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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
