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

// $XORP: xorp/bgp/route_table_policy_ex.hh,v 1.8 2008/10/02 21:56:20 bms Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_EX_HH__
#define __BGP_ROUTE_TABLE_POLICY_EX_HH__

#include "route_table_policy.hh"

/**
 * @short Export policy tables match neighbor in a different way.
 *
 * Export tables will supply a specialized varrw which will match neighbor
 * against the peer which the advertisement is about to be sent out, rather than
 * the originating peer.
 */
template <class A>
class PolicyTableExport : public PolicyTable<A> {
public:
    /**
     * @param tablename name of the table.
     * @param safi the safe.
     * @param parent parent table.
     * @param pfs a reference to the global policy filters.
     * @param neighbor the id of the peer on this output branch.
     */
    PolicyTableExport(const string& tablename, 
		      const Safi& safi,
		      BGPRouteTable<A>* parent,
		      PolicyFilters& pfs,
		      const string& neighbor,
		      const A& self);

protected:
    void init_varrw();

private:
    const string _neighbor;
};

#endif // __BGP_ROUTE_TABLE_POLICY_EX_HH__
