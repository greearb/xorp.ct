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

// $XORP: xorp/rib/rib_varrw.hh,v 1.11 2008/07/23 05:11:30 pavlin Exp $

#ifndef __RIB_RIB_VARRW_HH__
#define __RIB_RIB_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "route.hh"

/**
 * @short Enables reading and writing variables to a RIB route.
 *
 * This class is intended for connected routes only, and supports only
 * policytags being altered.
 */
template <class A>
class RIBVarRW : public SingleVarRW {
public:
    enum {
	VAR_NETWORK4 = VAR_PROTOCOL,
	VAR_NEXTHOP4,
	VAR_NETWORK6,
	VAR_NEXTHOP6,
	VAR_METRIC
    };

    /**
     * @param route route to filter and possibly modify.
     */
    RIBVarRW(IPRouteEntry<A>& route);

    // SingleVarRW interface
    void start_read();

    /**
     * Write a variable.
     *
     * @param id variablea to write.
     * @param e value of variable.
     */
    void single_write(const Id& id, const Element& e);

    Element* single_read(const Id& id);

private:
    /**
     * Specialized template to read nexthop and ip address.
     * If it is a v4 specialization, v6 addresses are set to null
     * and vice-versa.
     *
     * @param r route from which to read addresses.
     */
    void read_route_nexthop(IPRouteEntry<A>& r);

    IPRouteEntry<A>&	_route;
    ElementFactory	_ef;
};

#endif // __RIB_RIB_VARRW_HH__
