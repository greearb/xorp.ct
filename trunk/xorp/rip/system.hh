// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/rip/system.hh,v 1.7 2004/09/17 13:57:15 abittau Exp $

#ifndef __RIP_SYSTEM_HH__
#define __RIP_SYSTEM_HH__

#include <map>

#include "route_db.hh"
#include "port_manager.hh"

#include "policy/backend/policy_filters.hh"

/**
 * @short Top Level container for XORP RIP implementation.
 */
template <typename A>
class System {
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
    inline EventLoop& eventloop()			{ return _e; }

    /**
     * Get @ref EventLoop instance that each object in RIP system
     * should use.
     */
    inline const EventLoop& eventloop() const		{ return _e; }

    /**
     * Get the Route Database that each object in the RIP system
     * should use.
     */
    inline RouteDatabase& route_db()			{ return _rtdb; }

    /**
     * Get the Route Database that each object in the RIP system
     * should use.
     */
    inline const RouteDatabase& route_db() const	{ return _rtdb; }

    /**
     * Set the port manager object associated with the system.
     *
     * @param pointer to PortManager to be used.
     *
     * @return true if port manager has not previously been set and
     * pointer is not null, false otherwise.
     */
    inline bool set_port_manager(PortManager* pm);

    /**
     * Get pointer to PortManager that the RIP system is using.
     */
    inline PortManager* port_manager()			{ return _pm; }

    /**
     * Get pointer PortManager that the RIP system is using.
     */
    inline const PortManager* port_manager() const	{ return _pm; }

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

protected:
    System(const System&);				// Not implemented
    System& operator=(const System&);			// Not implemented

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
