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

// $XORP: xorp/rib/rt_tab_export.hh,v 1.8 2003/05/08 05:51:27 mjh Exp $

#ifndef __RIB_RT_TAB_EXPORT_HH__
#define __RIB_RT_TAB_EXPORT_HH__

#include "rt_tab_base.hh"

class RibClient;

/** 
 * @short ExportTable is used to send routes to the RIB clients.
 *
 * ExportTable is a @ref RouteTable that is used in RIBs to
 * send routes to the RIB clients such as the Forwarding Engine Abstraction
 * (FEA) process.  It is the final RouteTable in the RIB.
 */
template<class A>
class ExportTable : public RouteTable<A> {
public:
    /**
     * ExportTable Constructor
     *
     * @param tablename the name of the table, used for debugging purposes.
     * @param parent the @ref RouteTable immediately preceding this
     * one.  Usually this will be a @ref RegisterTable.
     * @param rib_clients_list a pointer to the RIB's list of @ref RibClient
     * instances.  The list of RibClient instances is used to communicate
     * with the RIB clients using XRLs.
     */
    ExportTable(const string& tablename, RouteTable<A> *parent, 
		list<RibClient *> *rib_clients_list);

    /**
     * ExportTable Destructor
     */
    ~ExportTable();

    /**
     * add_route is called when a new route is successfully added to
     * the RIB, and this needs to be communicated to the RIB clients.
     * 
     * @param route the @ref RouteEntry for the new route.
     * @param caller the @ref RouteTable calling this method. This
     * must be the same as _parent
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int add_route(const IPRouteEntry<A>& route, RouteTable<A> *caller);

    /**
     * delete_route is called when a route is removed from the
     * the RIB, and this needs to be communicated to the RIB clients.
     * 
     * @param route the @ref RouteEntry for the route being deleted.
     * @param caller the @ref RouteTable calling this method. This
     * must be the same as _parent
     * @return XORP_OK on success, XORP_ERROR otherwise.
     */
    int delete_route(const IPRouteEntry<A> *route, RouteTable<A> *caller);

    /**
     * lookup a route in this RIB. This request is simply passed on
     * unchanged to the parent.  
     */
    const IPRouteEntry<A> *lookup_route(const IPNet<A>& net) const ;

    /**
     * lookup a route in this RIB. This request is simply passed on
     * unchanged to the parent.  
     */
    const IPRouteEntry<A> *lookup_route(const A& addr) const;

    /**
     * lookup a route range in this RIB. This request is simply passed
     * on unchanged to the parent.  
     */
    RouteRange<A> *lookup_route_range(const A& addr) const;

    /**
     * @return EXPORT_TABLE
     */
    int type() const { return EXPORT_TABLE; }

    /**
     * replumb to replace the old parent of this table with a new parent
     * 
     * @param old_parent the parent RouteTable being replaced (must be
     * the same as the existing parent)
     * @param new_parent the new parent RouteTable
     */
    void replumb(RouteTable<A> *old_parent, RouteTable<A> *new_parent);

    /**
     * @return this ExportTable's parent RouteTable
     */
    RouteTable<A> *parent() { return _parent; }

    /**
     * @return this ExportTable as a string for debugging purposes
     */
    string str() const;

    /**
     * A single routing change communicated by a routing protocol can
     * cause multiple add_route and delete_route events at the
     * ExportTable (typically a delete_route then an add_route to
     * replace a route).  flush is called explicitly after the end of
     * a batch of changes to allow events to be queued and then
     * amalgamated in the ExportTable to reduce unnecessary changes
     * reaching the RIB clients.
     */
    void flush();

private:
    RouteTable<A>	*_parent;
    list<RibClient *>	*_rib_clients;
};

#endif // __RIB_RT_TAB_EXPORT_HH__
