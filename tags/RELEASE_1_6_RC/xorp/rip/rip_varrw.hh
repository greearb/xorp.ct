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

// $XORP: xorp/rip/rip_varrw.hh,v 1.12 2008/08/06 08:24:12 abittau Exp $

#ifndef __RIP_RIP_VARRW_HH__
#define __RIP_RIP_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "route_entry.hh"

/**
 * @short Enables reading and writing variables of RIP routes.
 */
template <class A>
class RIPVarRW : public SingleVarRW {
public:
    enum {
	VAR_NETWORK4 = VAR_PROTOCOL,
	VAR_NEXTHOP4,
	VAR_NETWORK6,
	VAR_NEXTHOP6,
	VAR_METRIC
    };
    
    /**
     * @param route route to read/write values from.
     */
    RIPVarRW(RouteEntry<A>& route);

    // SingleVarRW interface
    void start_read();
    Element* single_read(const Id& id);
    void single_write(const Id& id, const Element& e);

private:
    /**
     * @param route route to read nexthop and network from.
     */
    void read_route_nexthop(RouteEntry<A>& route);

    /**
     * Specialized template method to write correct nexthop value based on
     * address family of route.
     *
     * @param id variable to write.
     * @param e value of variable.
     * @return true if write was performed, false otherwise.
     */
    bool write_nexthop(const Id& id, const Element& e);

    RouteEntry<A>&	_route;
};

#endif // __RIP_RIP_VARRW_HH__
