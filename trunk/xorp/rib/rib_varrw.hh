// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rib/rib_varrw.hh,v 1.3 2004/09/18 02:05:52 pavlin Exp $

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
    void single_write(const string& id, const Element& e);

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
