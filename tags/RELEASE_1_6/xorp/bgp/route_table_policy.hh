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

// $XORP: xorp/bgp/route_table_policy.hh,v 1.15 2008/11/08 06:14:39 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_HH__
#define __BGP_ROUTE_TABLE_POLICY_HH__

#include "route_table_base.hh"
#include "bgp_varrw.hh"
#include "policy/backend/policy_filters.hh"

/**
 * @short Generic Policy filter table suitable for export filters.
 *
 * All policy filter tables in BGP have very similar functionality. Some have
 * more features than others, but most of the code is shared.
 *
 * The export filter is the most "standard" table, so it is used as a base
 * table for the source match and import filter tables.
 */
template <class A>
class PolicyTable : public BGPRouteTable<A> {
public:
    /**
     * @param tablename name of the table.
     * @param safi the safi.
     * @param parent the parent table.
     * @param pfs a reference to the global policy filters.
     * @param type the type of policy filter used in this table.
     */
    PolicyTable(const string& tablename, const Safi& safi, 
		BGPRouteTable<A>* parent,
		PolicyFilters& pfs,
		const filter::Filter& type);

    virtual ~PolicyTable();

    int add_route(InternalMessage<A> &rtmsg,
                  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
                      InternalMessage<A> &new_rtmsg,
                      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg,
                     BGPRouteTable<A> *caller);
    virtual int route_dump(InternalMessage<A> &rtmsg,
                   BGPRouteTable<A> *caller,
                   const PeerHandler *dump_peer);
    int push(BGPRouteTable<A> *caller);

    void route_used(const SubnetRoute<A>* route, bool in_use);
    bool get_next_message(BGPRouteTable<A> *next_table);

    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;

    // XXX: keep one table type for now
    RouteTableType type() const { return POLICY_TABLE; }
   
    string str() const;

    /**
     * Performs policy filtering on a route.
     *
     * If false is returned, then the route is rejected, otherwise it
     * was accepted and may have been modified.
     *
     * @param rtmsg the route message to filter.
     * @param no_modify if true, the filter will not modify the route.
     * @return whether the route was accepted by the filter.
     */
    bool do_filtering(InternalMessage<A>& rtmsg, 
		      bool no_modify) const;

    void enable_filtering(bool on);
protected:
    virtual void init_varrw();

    const filter::Filter	_filter_type;
    BGPVarRW<A>*		_varrw;

private:
    PolicyFilters&		_policy_filters;
    bool _enable_filtering;
};

#endif // __BGP_ROUTE_TABLE_POLICY_HH__
