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

// $XORP: xorp/rip/system.hh,v 1.4 2004/04/02 00:27:56 mjh Exp $

#ifndef __RIP_SYSTEM_HH__
#define __RIP_SYSTEM_HH__

#include <map>

#include "route_db.hh"
#include "port_manager.hh"

/**
 * @short Top Level container for XORP RIP implementation.
 */
template <typename A>
class System {
public:
    typedef RouteDB<A>		RouteDatabase;
    typedef PortManagerBase<A>	PortManager;

public:
    System(EventLoop& e) : _e(e), _rtdb(e), _pm(0) {}
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

protected:
    System(const System&);				// Not implemented
    System& operator=(const System&);			// Not implemented

protected:
    EventLoop&		_e;
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
