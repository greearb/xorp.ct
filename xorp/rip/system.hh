// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8: 
                       
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

// $XORP: xorp/rip/system.hh,v 1.16 2008/10/29 21:59:39 andrewma Exp $

#ifndef __RIP_SYSTEM_HH__
#define __RIP_SYSTEM_HH__




#include "trace.hh"
#include "route_db.hh"
#include "port_manager.hh"

#include "policy/backend/policy_filters.hh"

/**
 * @short Top Level container for XORP RIP implementation.
 */
template <typename A>
class System :
    public NONCOPYABLE
{
public:
    typedef RouteDB<A>		RouteDatabase;
    typedef PortManagerBase<A>	PortManager;

public:
    System(EventLoop& e) : _e(e), _rtdb(e,_policy_filters), _pm(0) {}
    ~System();

    /**
     * Get @ref EventLoop instance that each object in system should
     * use.
     */
    EventLoop& eventloop()			{ return _e; }

    /**
     * Get @ref EventLoop instance that each object in RIP system
     * should use.
     */
    const EventLoop& eventloop() const		{ return _e; }

    /**
     * Get the Route Database that each object in the RIP system
     * should use.
     */
    RouteDatabase& route_db()			{ return _rtdb; }

    /**
     * Get the Route Database that each object in the RIP system
     * should use.
     */
    const RouteDatabase& route_db() const	{ return _rtdb; }

    /**
     * Set the port manager object associated with the system.
     *
     * @param pointer to PortManager to be used.
     *
     * @return true if port manager has not previously been set and
     * pointer is not null, false otherwise.
     */
    bool set_port_manager(PortManager* pm);

    /**
     * Get pointer to PortManager that the RIP system is using.
     */
    PortManager* port_manager()			{ return _pm; }

    /**
     * Get pointer PortManager that the RIP system is using.
     */
    const PortManager* port_manager() const	{ return _pm; }

    /**
     * Configure a policy filter.
     *
     * @param filter id of filter to configure.
     * @param conf configuration of filter.
     */
    void configure_filter(const uint32_t& filter, const string& conf) {
	_policy_filters.configure(filter,conf);
    }

    /**
     * Reset a policy filter.
     *
     * @param filter id of filter to reset.
     */
    void reset_filter(const uint32_t& filter) {
	_policy_filters.reset(filter);
    }

    /**
     * Push routes through policy filters for re-filtering.
     */
    void push_routes() {
	_rtdb.push_routes();
    }

    /**
     * @return reference to global policy filters.
     */
    PolicyFilters& policy_filters() { return _policy_filters; }

    Trace& route_trace() { return _rtdb.trace(); }

protected:
    EventLoop&		_e;

    //
    // There should be only one instatiation per process.
    // RIP uses separate processes for IPv4 and IPv6 so we are ok.
    //
    PolicyFilters	_policy_filters;

    RouteDatabase	_rtdb;
    PortManager*	_pm;
};

// ----------------------------------------------------------------------------
// Inline System methods
//

template <typename A>
bool
System<A>::set_port_manager(PortManager* pm)
{
    if (pm != 0 && _pm == 0) {
	_pm = pm;
	return true;
    }
    return false;
}

template <typename A>
System<A>::~System()
{
    _rtdb.flush_routes();
}

#endif // __RIP_SYSTEM_HH__
