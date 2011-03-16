// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/rip/xrl_redist_manager.hh,v 1.16 2008/10/02 21:58:19 bms Exp $

#ifndef __RIP_XRL_REDIST_MANAGER__
#define __RIP_XRL_REDIST_MANAGER__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"
#include "libxorp/service.hh"




#include "redist.hh"

template <typename A> class System;

/**
 * Xrl Route redistribution manager.
 */
template <typename A>
class XrlRedistManager : public ServiceBase {
public:
    typedef A		Addr;
    typedef IPNet<A>	Net;

public:
    XrlRedistManager(System<A>& system);
    ~XrlRedistManager();

    int startup();
    int shutdown();

    void add_route(const Net& net, const Addr& nh, const string& ifname,
		   const string& vifname, uint16_t cost, uint16_t tag,
		   const PolicyTags& policytags);

    void delete_route(const Net& net);

protected:
    RouteRedistributor<A> _rr;
};

#endif // __RIP_XRL_REDIST_MANAGER__
