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

// $XORP: xorp/bgp/route_table_policy.hh,v 1.1 2004/09/17 13:50:55 abittau Exp $

#ifndef __BGP_ROUTE_TABLE_POLICY_HH__
#define __BGP_ROUTE_TABLE_POLICY_HH__

#include "route_table_base.hh"
#include "policy/backend/policy_filters.hh"

/**
 * @short Generic Policy filter table suitable for export filters.
 *
 * All policy filter tables in BGP have very similar functionality. Some have
 * more features than others, but most of the code is shared.
 *
 * The export filter is the most "standard" table, so it is used as a base table
 * for the source match and import filter tables.
 */
template <class A>
class PolicyTable : public BGPRouteTable<A> {
public:
    /**
     * @param tablename name of the table
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
  
    int add_route(const InternalMessage<A> &rtmsg,
                  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
                      const InternalMessage<A> &new_rtmsg,
                      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg,
                     BGPRouteTable<A> *caller);
    virtual int route_dump(const InternalMessage<A> &rtmsg,
                   BGPRouteTable<A> *caller,
                   const PeerHandler *dump_peer);
    int push(BGPRouteTable<A> *caller);

    void route_used(const SubnetRoute<A>* route, bool in_use);
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    const
    SubnetRoute<A> *lookup_route(const IPNet<A> &net,
                                 uint32_t& genid) const;

    // XXX: keep one table type for now
    RouteTableType type() const { return POLICY_TABLE; }
    
    string str() const;
   
    /**
     * Performs policy filtering on a route
     *
     * If null is returned, then the route is rejected.
     * The pointer to the same route may be returned if the route is accepted
     * but not modified.
     * A pointer to a different route will be returned in the case of an
     * accepted and modified route. The old route will be deleted.
     *
     * @param rtmsg the route message to filter.
     * @param no_modify if true, the filter will not modify the route.
     * @return the possibly modified message, Null if rejected by filter.
     */
    const InternalMessage<A>* do_filtering(const InternalMessage<A>& rtmsg, 
					   bool no_modify) const;

private:
    PolicyFilters& _policy_filters;
    const filter::Filter _filter_type;
};

#endif // __BGP_ROUTE_TABLE_POLICY_HH__
