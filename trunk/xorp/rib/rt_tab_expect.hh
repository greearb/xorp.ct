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

// $XORP: xorp/rib/rt_tab_redist.hh,v 1.3 2003/03/16 07:19:00 pavlin Exp $

#ifndef __RIB_RT_TAB_EXPECT_HH__
#define __RIB_RT_TAB_EXPECT_HH__

#include "rt_tab_base.hh"

template<class A>
class ExpectedRouteChange;

/**
 * @short A Route Table for comparing route updates received against
 * expected.
 *
 * Users of this class specify expected updates with @ref expect_add
 * and @ref expect_delete.  As the updates come through they are
 * compared against those expect and generate an assertion failure if
 * those arriving do not match those expected.  An assertion failure
 * will also occur if there are unmatched updates upon instance
 * destruction.
 *
 * This class is strictly for debugging and testing purposes.
 */
template<class A>
class ExpectTable : public RouteTable<A> {
public:
    ExpectTable(const string& tablename, RouteTable<A>* parent);
    ~ExpectTable();
    void expect_add(const IPRouteEntry<A>& route);
    void expect_delete(const IPRouteEntry<A>& route);
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    int type() const { return EXPECT_TABLE; }
    RouteTable<A>* parent() { return _parent; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

private:
    RouteTable<A>* _parent;
    list<ExpectedRouteChange<A> > _expected;
};

#endif // __RIB_RT_TAB_EXPECT_HH__
