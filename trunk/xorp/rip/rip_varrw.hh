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

// $XORP: xorp/rip/rip_varrw.hh,v 1.1 2004/09/17 13:57:15 abittau Exp $

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
    /**
     * @param route route to read/write values from.
     */
    RIPVarRW(RouteEntry<A>& route);

    // SingleVarRW interface
    void single_start();
    void single_write(const string& id, const Element& e);
    void single_end();

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
    bool write_nexthop(const string& id, const Element& e);

    RouteEntry<A>&	_route;

    ElementFactory	_ef;
};

#endif // __RIP_RIP_VARRW_HH__
