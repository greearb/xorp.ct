// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/rib/rt_tab_log.hh,v 1.10 2008/10/02 21:58:12 bms Exp $

#ifndef __RIB_RT_TAB_LOG_HH__
#define __RIB_RT_TAB_LOG_HH__



#include "libxorp/xorp.h"
#include "rt_tab_base.hh"


/**
 * @short A Base for Route Tables that log updates.
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
template<typename A>
class LogTable : public RouteTable<A> {
public:
    LogTable(const string& tablename, RouteTable<A>* parent);
    ~LogTable();

    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return LOG_TABLE; }
    RouteTable<A>* parent() { return _parent; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

    uint32_t update_number() const;

private:
    RouteTable<A>* _parent;
    uint32_t	   _update_number;
};



/**
 * @short Route Table that passes through updates whilst logging them
 * to an ostream instance.
 *
 * -*- UNTESTED -*-
 */
template <typename A>
class OstreamLogTable : public LogTable<A> {
public:
    ostream& get();
    OstreamLogTable(const string& 	tablename,
		    RouteTable<A>*	parent,
		    ostream& 		out);
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* , RouteTable<A>* caller);
    string str() const;

private:
    ostream& _o;
};


/**
 * @short Route Table that passes that through updates whilst writing
 * them to the logging them via XORP's XLOG_TRACE.
 *
 * -*- UNTESTED -*-
 */
template <typename A>
class XLogTraceTable : public LogTable<A> {
public:
    XLogTraceTable(const string& 	tablename,
		   RouteTable<A>* 	parent);

    int add_route(const IPRouteEntry<A>& 	route,
		  RouteTable<A>* 		caller);

    int delete_route(const IPRouteEntry<A>* 	proute,
		     RouteTable<A>* 		caller);

    string str() const;
};

/**
 * @short Route Table that passes that passes through updates whilst
 * writing them to the logging them via XORP's XLOG_TRACE.
 *
 * -*- UNTESTED -*-
 */
template <typename A>
class DebugMsgLogTable : public LogTable<A> {
public:
    DebugMsgLogTable(const string& 	tablename,
		     RouteTable<A>* 	parent);

    int add_route(const IPRouteEntry<A>& 	route,
		  RouteTable<A>* 		caller);

    int delete_route(const IPRouteEntry<A>* 	proute,
		     RouteTable<A>* 		caller);

    string str() const;
};

#endif // __RIB_RT_TAB_LOG_HH__
