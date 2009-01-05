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

// $XORP: xorp/bgp/route_table_policy_im.hh,v 1.10 2008/11/08 06:14:39 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_IM_HH__
#define __BGP_ROUTE_TABLE_POLICY_IM_HH__

#include "route_table_policy.hh"

/**
 * @short Import policy tables also deal with propagating policy route dumps.
 *
 * Import tables will detect policy route dumps [NULL in dump_peer]. It will
 * then transport the dump to an add/delete/request based on the outcome of the
 * filter.
 */
template <class A>
class PolicyTableImport : public PolicyTable<A> {
public:
    /**
     * @param tablename name of the table.
     * @param safi the safe.
     * @param parent parent table.
     * @param pfs a reference to the global policy filters.
     */
    PolicyTableImport(const string& tablename, 
		      const Safi& safi,
		      BGPRouteTable<A>* parent,
		      PolicyFilters& pfs,
		      const A& peer,
		      const A& self);

    /**
     * If dump_peer is null, then it is a policy route dump and we need to deal
     * with it.
     *
     * @param rtmsg route being dumped.
     * @param caller table that called this method.
     * @param dump_peer peer we are dumping to. If policy dump,
     * it will be NULL.
     * @return ADD_FILTERED if route is rejected. XORP_OK otherwise.
     */
    int route_dump(InternalMessage<A> &rtmsg,
                   BGPRouteTable<A> *caller,
                   const PeerHandler *dump_peer);
};

#endif // __BGP_ROUTE_TABLE_POLICY_IM_HH__
