// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/rib/rt_tab_redist.hh,v 1.1.1.1 2002/12/11 23:56:14 hodson Exp $

#ifndef __RIB_RT_TAB_REDIST_HH__
#define __RIB_RT_TAB_REDIST_HH__

#include "rt_tab_base.hh"
#include "rt_tab_origin.hh"

/**
 * @short RouteTable used to redistribute routes (not yet working)
 *
 * RedistTable is used to redistribute routes from a routing table
 * back out to a routing protocol.  For example, when you want to
 * redistribute BGP routes into OSPF.
 *
 * RedistTable is a work-in-progress.
 */
template<class A>
class RedistTable : public RouteTable<A> {
public:
    RedistTable(const string& tablename, RouteTable<A>* from_table, 
		OriginTable<A>* to_table);
    ~RedistTable();
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    int type() const {return REDIST_TABLE;}
    RouteTable<A>* parent() {return _from_table;}
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;
private:
    RouteTable<A>* _from_table;
    OriginTable<A>* _to_table;
};

#endif // __RIB_RT_TAB_REDIST_HH__
