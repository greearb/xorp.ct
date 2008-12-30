// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/port_manager.hh,v 1.14 2008/07/23 05:11:36 pavlin Exp $

#ifndef __RIP_PORT_MANAGER_HH__
#define __RIP_PORT_MANAGER_HH__

#include <list>

#include "libfeaclient/ifmgr_atoms.hh"

template <typename A>
class Port;

template <typename A>
class System;

/**
 * @short Base for RIP Port container and factory classes.
 *
 * Classes derived from the PortManagerBase are expected to create
 * and manage RIP Port instances.  The created Port instances should have
 * associated IO systems attached.
 */
template <typename A>
class PortManagerBase {
public:
    typedef list<Port<A>*>	PortList;
    typedef System<A>		SystemType;

public:
    PortManagerBase(SystemType& system, const IfMgrIfTree& iftree)
	: _system(system), _iftree(iftree) {}

    /**
     * Destructor
     *
     * It is important that all the routes stored in the associated
     * @ref System<A> Route database and it's update queue are flushed
     * before destructor is invoked.
     */
    virtual ~PortManagerBase();

    /**
     * Get parent @ref System<A> instance.
     */
    SystemType& system()		{ return _system; }

    /**
     * Get parent @ref System<A> instance.
     */
    const SystemType& system() const	{ return _system; }

    /**
     * Get list of managed RIP Ports.
     */
    const PortList& const_ports() const	{ return _ports; }

    /**
     * Get EventLoop.
     */
    EventLoop& eventloop() 		{ return _system.eventloop(); }

    /**
     * Get EventLoop.
     */
    const EventLoop& eventloop() const	{ return _system.eventloop(); }

    /**
     * Get IfMgrIfTree.
     */
    const IfMgrIfTree& iftree() const	{ return _iftree; }

protected:
    /**
     * Get list of managed RIP Ports.
     */
    PortList& ports()			{ return _ports; }

    /**
     * Get list of managed RIP Ports.
     */
    const PortList& ports() const	{ return _ports; }

protected:
    SystemType&	_system;
    PortList	_ports;
    const IfMgrIfTree& _iftree;
};

// ----------------------------------------------------------------------------
// Inline PortManagerBase methods
//

template <typename A>
PortManagerBase<A>::~PortManagerBase()
{
}

#endif // __RIP_PORT_MANAGER_HH__
