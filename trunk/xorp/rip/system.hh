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

// $XORP: xorp/rip/system.hh,v 1.2 2003/07/09 00:06:34 hodson Exp $

#ifndef __RIP_SYSTEM_HH__
#define __RIP_SYSTEM_HH__

#include <map>

#include "route_db.hh"
#include "port_manager.hh"
#include "redist.hh"

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

    /**
     * Add route redistributor.
     *
     * @param protocol name associated with routing protocol
     *        originating redistribution routes.
     * @param tag identifier to be placed in RIP packets.
     *
     * @return true on success, false if redistributor already exists
     *         and has a different tag.
     */
    inline bool add_route_redistributor(const string& protocol, uint16_t tag);

     /**
     * Remove route distributor.
     *
     * @param protocol name associated with routing protocol
     *        originating redistribution routes.
     *
     * @return true on success, false if redistributor does not exist.
     */
    inline bool remove_route_redistributor(const string& protocol);

    /**
     * Add route for redistribution.
     *
     * @param protocol name associated with routing protocol
     *        originating redistribution routes.
     * @param net network route to be added.
     * @param nh  nexthop to advertise with network.
     * @param cost cost to adverise route with [1--16].
     *
     * @return true on success, false on failure.  Failure may occur
     * if route already exists or a lower cost route exists.
     */
    inline bool redistributor_add_route(const string& 	protocol,
					const IPNet<A>&	net,
					const A& 	nh,
					uint32_t 	cost);

    /**
     * Remove route from redistribution.
     * @param protocol name associated with routing protocol
     *        originating redistribution routes.
     * @param net network route to be removed.
     */
    inline bool redistributor_remove_route(const string& 	protocol,
					   const IPNet<A>&	net);

protected:
    System(const System&);				// Not implemented
    System& operator=(const System&);			// Not implemented

protected:
    EventLoop&		_e;
    RouteDatabase	_rtdb;
    PortManager*	_pm;
    map<string,RouteRedistributor<A>*> _redists;
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

    while (_redists.empty() == false) {
	typename map<string,RouteRedistributor<A>*>::iterator i
	    = _redists.begin();
	delete i->second;
	_redists.erase(i);
    }
}

template <typename A>
inline bool
System<A>::add_route_redistributor(const string& protocol, uint16_t tag)
{
    typename map<string,RouteRedistributor<A>*>::const_iterator ci;
    ci = _redists.find(protocol);
    if (ci == _redists.end()) {
	_redists.insert(
			make_pair(protocol,
				  new RouteRedistributor<A>(_rtdb,
							    protocol,
							    tag)
				  )
			);
	return true;
    }
    const RouteRedistributor<A>* prr = ci->second;
    if (prr->tag() == tag) {
	return true;	// Already exists and has correct tag
    }
    return false;
}

template <typename A>
inline bool
System<A>::remove_route_redistributor(const string& protocol)
{
    typename map<string,RouteRedistributor<A>*>::iterator i;
    i = _redists.find(protocol);
    if (ci == _redists.end()) {
	return false;
    }
    delete i->second;
    _redists.erase(i);

    return true;
}

template <typename A>
inline bool
System<A>::redistributor_add_route(const string&	protocol,
				   const IPNet<A>&	net,
				   const A&		nh,
				   uint32_t		cost)
{
    typename map<string,RouteRedistributor<A>*>::iterator i;
    i = _redists.find(protocol);
    if (i == _redists.end())
	return false;

    if (cost > RIP_INFINITY)
	return false;

    RouteRedistributor<A>* prr = i->second;
    return prr->add_route(net, nh, cost);
}

template <typename A>
inline bool
System<A>::redistributor_remove_route(const string&	protocol,
				      const IPNet<A>&	net)
{
    typename map<string,RouteRedistributor<A>*>::iterator i;
    i = _redists.find(protocol);
    if (i == _redists.end()) {
	return false;
    }

    RouteRedistributor<A>* prr = i->second;
    return prr->expire_route(net);
}

#endif // __RIP_SYSTEM_HH__
