// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rip/port_manager.hh,v 1.5 2004/03/02 00:47:04 hodson Exp $

#ifndef __RIP_PORT_MANAGER_HH__
#define __RIP_PORT_MANAGER_HH__

#include <list>

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
    PortManagerBase(SystemType& system) : _system(system) {}

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
    inline SystemType& system()			{ return _system; }

    /**
     * Get parent @ref System<A> instance.
     */
    inline const SystemType& system() const	{ return _system; }

    /**
     * Get list of managed RIP Ports.
     */
    inline const PortList& const_ports() const	{ return _ports; }

    /**
     * Get EventLoop.
     */
    inline EventLoop& eventloop() 		{ return _system.eventloop(); }

    /**
     * Get EventLoop.
     */
    inline const EventLoop& eventloop() const	{ return _system.eventloop(); }

protected:
    /**
     * Get list of managed RIP Ports.
     */
    inline PortList& ports()			{ return _ports; }

    /**
     * Get list of managed RIP Ports.
     */
    inline const PortList& ports() const	{ return _ports; }

protected:
    SystemType&	_system;
    PortList	_ports;
};

// ----------------------------------------------------------------------------
// Inline PortManagerBase methods
//

template <typename A>
PortManagerBase<A>::~PortManagerBase()
{
}

#endif // __RIP_PORT_MANAGER_HH__
