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

// $XORP$

#ifndef __RIB_RT_TAB_POL_REDIST_HH__
#define __RIB_RT_TAB_POL_REDIST_HH__

#include "rt_tab_base.hh"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "policy/backend/policy_redist_map.hh"
#include "xrl/interfaces/policy_redist4_xif.hh"
#include "xrl/interfaces/policy_redist6_xif.hh"

/**
 * @short This table redistributes routes to protocols according to policytags.
 *
 * Based on the policy-tags in a route, this table will request a protocol to
 * start or end a redistribution of a route, depending whether the route is
 * being added or deleted.
 */
template<class A>
class PolicyRedistTable : public RouteTable<A> {
public:
    static const string table_name;

    PolicyRedistTable(RouteTable<A>* parent, XrlRouter& rtr, PolicyRedistMap&,
		      bool multicast);

    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return POLICY_REDIST_TABLE; }
    RouteTable<A>* parent() { return _parent; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

    void xrl_cb(const XrlError&, string); 

    /**
     * If policy-tags of a route changed, this table will need to figure out
     * which protocol should stop advertising a route, and which protocol should
     * continue or start.
     *
     * @param route the route with its new policy tags.
     * @param prevtags the previous policytags of the route.
     * @param caller the table which invoked this method.
     */
    void replace_policytags(const IPRouteEntry<A>& route,
                            const PolicyTags& prevtags,
                            RouteTable<A>* caller);
 

private:
    typedef set<string> Set;

    /**
     * Start a redistribution of a route.
     *
     * @param route route to redistribute.
     * @param protos the set of protocols which should do the redistribution.
     */
    void add_redist(const IPRouteEntry<A>& route, const Set& protos);

    /**
     * End a route redistribution.
     *
     * @param route the route which should no longer be redistributed.
     * @param protos the protocols which should stop advertising the route.
     */
    void del_redist(const IPRouteEntry<A>& route, const Set& protos);
   
    /**
     * Start a route redistribution.
     *
     * @param route route to be redistributed.
     * @param proto protocol which should advertise route.
     */
    void add_redist(const IPRouteEntry<A>& route, const string& proto);

    /**
     * End a route redistribution.
     *
     * @param route route which should no longer be redistributed.
     * @param proto protocol which should stop advertising the route.
     */
    void del_redist(const IPRouteEntry<A>& route, const string& proto);


    RouteTable<A>*  _parent;
    
    XrlRouter&	    _xrl_router;
    EventLoop&	    _eventloop;

    PolicyRedistMap&	_redist_map;

    XrlPolicyRedist4V0p1Client _redist4_client;
    XrlPolicyRedist6V0p1Client _redist6_client;

    bool _multicast;
};

#endif // __RIB_RT_TAB_POL_REDIST_HH__
