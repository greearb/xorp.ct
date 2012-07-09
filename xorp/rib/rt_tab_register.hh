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

// $XORP: xorp/rib/rt_tab_register.hh,v 1.21 2008/10/02 21:58:13 bms Exp $

#ifndef __RIB_RT_TAB_REGISTER_HH__
#define __RIB_RT_TAB_REGISTER_HH__




#include "libxorp/debug.h"

#include "rt_tab_base.hh"


class RegisterServer;

/**
 * Holds information about an XRL module that requested to be
 * notified about a change.
 */
class ModuleData {
public:
    /**
     * ModuleData Constructor
     *
     * @param modulename the XRL target name of the module that
     * requested notification about a route change.
     */
    ModuleData(const string& modulename)
	: _modulename(modulename), _is_set(false)	{ }

    ModuleData() { }

    /**
     * @return the XRL target name of the module.
     */
    const string& name() const			{ return _modulename;	}

    /**
     * @return true if the XRL module needs to be notified about a change.
     */
    bool is_set() const				{ return _is_set;	}

    /**
     * Set state indicating the XRL module needs to be notified about
     * a change.
     */
    void set() const				{ _is_set = true;	}

    /**
     * Clear state indicating the XRL module needs to be notified about
     * a change.
     */
    void clear() const 				{ _is_set = false;	}

    /**
     * @return string representation of this ModuleData for debugging purposes.
     */
    string str() const				{
	    string s;
	    s = _modulename + (_is_set ? " (SET)" : " (cleared)");
	    return s;
    }

    /**
     * Comparison operator for ModuleData class.
     *
     * Two ModuleData instances are considered equal if they refer
     * to the same XRL target, irrespective of the state of their flags.
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is considered equal to the
     * right-hand operand (i.e., if both operands refer to the same XRL
     * target).
     */
    bool operator==(const ModuleData& other) const {
	return name() == other.name();
    }

    /**
     * Less-than operator for ModuleData class.
     *
     * This is needed so that ModuleData instances can be stored in
     * some STL containers.
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is considered smaller than the
     * right-hand operand.
     */
    bool operator<(const ModuleData& other) const {
	return name() < other.name();
    }

private:
    string		_modulename;
    mutable bool	_is_set;
};

/**
 * @short RouteRegister stores a registration of interest in a subset
 * of a route.
 *
 * A RouteRegister instance is used to store the registration of
 * interest in a route.  Suppose there are two overlapping routes:
 * 1.0.0.0/16 and 1.0.1.0/24.  Now a routing protocol "bgp" expresses
 * interest in route changes that affect 1.0.0.27, which is routed
 * using 1.0.0.0/16.  The relevant RouteRegister would then hold:
 *     route: route entry for 1.0.0.0/16
 *     valid_subnet: 1.0.0.0/24
 *     moduledata: {"bgp"}
 * The valid_subnet is 1.0.0.0/24 because this is the largest subset of
 * 1.0.0.0/16 that encompasses 1.0.0.27 and doesn't overlap any other
 * route.
 *
 * If a subsequent request from routing protocol "pim" were to come
 * along for 1.0.0.54, then this would be stored on the same
 * RouteRegister instance, but the ModuleData would be expanded to
 * include "pim".
 */
template <class A>
class RouteRegister {
public:
    /**
     * RouteRegister Constructor
     *
     * @param net the subset of the route for which this registration
     * is valid.
     * @param route the route in which interest was registered.
     * @param module the ModuleData instance refering to the routing
     * protocol that registered interest.
     */
    RouteRegister(const IPNet<A>& valid_subnet,
		  const IPRouteEntry<A>* route,
		  const ModuleData& module)
	: _valid_subnet(valid_subnet), _route(route) {
	_modules[module.name()] = module;
    }

    /**
     * Destructor
     */
    ~RouteRegister() {
	_route = NULL;
    }

    /**
     * add_registrant is called when an additional routing protocol
     * expresses interest in the routing information already held by
     * this RouteRegister instance.
     *
     * @param module the ModuleData instance refering to the
     * additional routing protocol that just registered interest.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_registrant(const ModuleData& module) {
	debug_msg("add_registrant: Module: %s\n", module.str().c_str());
	if (_modules.find(module.name()) != _modules.end())
	    return XORP_ERROR;
	_modules[module.name()] = module;
	return XORP_OK;
    }

    /**
     * delete_registrant is called when a routing protocol that was
     * previously interested in this RouteRegister de-registers
     * itself.
     *
     * @param module the ModuleData instance of the routing protocol
     * that de-registered.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_registrant(const ModuleData& module);

    /**
     * mark_modules is called when the routing information matching
     * this registration changes.  It marks the original instances of
     * the ModuleData as needing nitification.
     */
    void mark_modules() const {
	map<string, ModuleData>::const_iterator i;
	for (i = _modules.begin(); i != _modules.end(); ++i)
	    i->second.set();
    }

    /**
     * @return the number of modules interested in this RouteRegister.
     */
    int size() const			{ return _modules.size(); }

    /**
     * @return the subnet of this RouteRegister's route for which this
     * registration is valid.
     */
    const IPNet<A>& valid_subnet() const { return _valid_subnet; }

    /**
     * @return the RouteRegister's route.
     */
    const IPRouteEntry<A>* route() const { return _route; }

    /**
     * @return the module names interested in this RouteRegister as a
     * list of strings.
     */
    list<string> module_names() const {
	list<string> names;
	map<string, ModuleData>::const_iterator i;
	for (i = _modules.begin(); i != _modules.end(); ++i)
	    names.push_back(i->second.name());
	return names;
    }

    /**
     * @return this RouteRegister as a string for debugging purposes.
     */
    string str() const;

private:
    map<string, ModuleData> _modules;

    // _net duplicates the storage of this in the RegisterTable map -
    // not very efficient
    IPNet<A> _valid_subnet;

    // _valid_subnet is the subset of _route->net() that this
    // registration is valid for
    const IPRouteEntry<A>* _route;
};


/**
 * @short @ref RouteTable which stores routing protocols' registration
 * of interest in changes to certain routes.
 *
 * RegisterTable is a @ref RouteTable that is plumbed into the RIB
 * after all the @ref MergedTable and @ref ExtIntTable instances.
 * Thus it seems all winning route updates that will be propagated
 * to the forwarding engine.
 *
 * It's purpose is to track route changes that affect specific
 * addresses in which routing protocols have expressed an interest,
 * and to notify these routing protocols of any changes.
 */
template<class A>
class RegisterTable : public RouteTable<A> {
public:
    /**
     * RegisterTable constructor
     *
     * Unlike other RouteTable constructors, this doesn't plumb itself
     * into the table graph because it is set up before any origin
     * tables, so it doesn't yet have a parent.
     * RIB::initialize_register will do the plumbing later on.
     *
     * @param tablename human-readable name for this table for
     * debugging purposes.
     * @param register_server a reference to the RIB's @ref RegisterServer
     * instance. The RegisterServer handles IPC requests and responses related
     * to registration of interest.
     * @param multicast indicates whether or not this RegisterTable is in
     * a multicast RIB.  The information is needed when notifying a
     * routing protocol of a change, because the same route might be
     * in both unicast and multicast RIBs.
     */
    RegisterTable(const string& tablename, RegisterServer& register_server,
		  bool multicast);

    /**
     * RegisterTable destructor
     */
    ~RegisterTable();

    int add_igp_route(const IPRouteEntry<A>& route);
    int add_egp_route(const IPRouteEntry<A>& route);

    int delete_igp_route(const IPRouteEntry<A>* route);
    int delete_egp_route(const IPRouteEntry<A>* route);

    /**
     * Lookup a route in the RIB.  This request will be propagated to
     * the parent table unchanged.
     */
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const {
	    return _parent->lookup_route(net);
    }

    /**
     * Lookup a route in the RIB.  This request will be propagated to
     * the parent table unchanged.
     */
    const IPRouteEntry<A>* lookup_route(const A& addr) const {
	    return _parent->lookup_route(addr);
    }

    /**
     * Lookup a route_range in the RIB.  This request will be
     * propagated to the parent table unchanged.  It is not expected
     * this will be called, but not prohibited.
     */
    RouteRange<A>* lookup_route_range(const A& addr) const {
	    return _parent->lookup_route_range(addr);
    }

    /**
     * @return the parent @ref RouteTable of this RegisterTable.
     */
    const RouteTable<A>* parent() const		{ return _parent;	}
    RouteTable<A>* parent()			{ return _parent;	}

    /**
     * @return this RegisterTable as a string for debugging purposes.
     */
    string str() const;

    /**
     * Print the contents of this RegisterTable as a string for
     * debugging purposes.
     */
#ifdef DEBUG_LOGGING
    void print();
#endif

    // Stuff specific to a Register Table

    /**
     * register_route_range is called to register interest in routing
     * changes that affect a specific IP address.
     *
     * @param addr the IP address of interest.
     * @param module the XRL target name of the module (typically a
     * routing protocol) that is interested in being notified about
     * changes to this address.
     * @return a RouteRegister instance detailing the route that is
     * currently being used to route packets to this address and the
     * subset of this route (including the address of interest) for
     * which this answer also applies.
     */
    RouteRegister<A>* register_route_range(const A& addr,
					   const string& module);

    /**
     * deregister_route_range is called to de-register interest in routing
     * changes following a prior call to register_route_range.
     *
     * @param valid_subnet the subnet of validity from the
     * RouteRegister returned by a prior call to register_route_range.
     * @param module the XRL target name of the module that is no
     * longer interested in being notified.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int deregister_route_range(const IPNet<A>& subnet, const string& module);

    /**
     * @return the table type (@ref TableType).
     */
    TableType type() const	{ return REGISTER_TABLE; }

    void set_parent(RouteTable<A>* new_parent);

    /**
     * Cause the register server to push out queued changes to the
     * routing protocols.
     */
    void flush();

private:
    RouteRegister<A>* add_registration(const IPNet<A>& net,
				       const IPRouteEntry<A>* route,
				       const string& module);
    int delete_registration(const IPNet<A>& net, const string& module);
    int notify_relevant_modules(bool add,
				const IPRouteEntry<A>& changed_route);
    int find_matches(const IPRouteEntry<A>& route);

    void notify_invalidated(typename Trie<A, RouteRegister<A>* >::iterator trie_iter);
    void notify_route_changed(typename Trie<A, RouteRegister<A>* >::iterator trie_iter,
			      const IPRouteEntry<A>& changed_route);

    /**
     * Invalidate a RouteRegister referencing this
     * route.
     *
     * @param route the new route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    void generic_add_route(const IPRouteEntry<A>& route);

    /**
     * Invalidate a RouteRegister referencing this
     * route.
     *
     * @param route the route being deleted.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    void generic_delete_route(const IPRouteEntry<A>* route);

    map<string, ModuleData>		_module_names;
    Trie<A, RouteRegister<A>* >		_ipregistry;
    RouteTable<A>*			_parent;
    RegisterServer&			_register_server;
    bool				_multicast;  // true if a multicast rib
};

#endif // __RIB_RT_TAB_REGISTER_HH__
