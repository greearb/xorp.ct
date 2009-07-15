// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net




//
// Protocol unit generic functionality implementation
//


#include "libproto_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "proto_state.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * ProtoState::ProtoState:
 * 
 * Proto state default constructor.
 **/
ProtoState::ProtoState()
{
    _flags		= 0;
    _debug_flag		= false;
    _flags &= ~XORP_ENABLED;	// XXX: default is to disable.
}

ProtoState::~ProtoState()
{
    
}

int
ProtoState::start()
{
    if (is_disabled())
	return (XORP_ERROR);
    if (is_up())
	return (XORP_OK);		// Already running

    ProtoState::reset();

    if (ProtoState::startup() != XORP_OK)
	return (XORP_ERROR);

    ServiceBase::set_status(SERVICE_RUNNING);

    return (XORP_OK);
}

int
ProtoState::stop()
{
    if (is_down())
	return (XORP_OK);		// Already down

    if (ProtoState::shutdown() != XORP_OK)
	return (XORP_ERROR);

    ServiceBase::set_status(SERVICE_SHUTDOWN);

    return (XORP_OK);
}

int
ProtoState::pending_start()
{
    if (is_disabled())
	return (XORP_ERROR);
    if (is_up())
	return (XORP_OK);		// Already running
    if (is_pending_up())
	return (XORP_OK);		// Already pending UP

    ServiceBase::set_status(SERVICE_STARTING);
    
    return (XORP_OK);
}

int
ProtoState::pending_stop()
{
    if (is_down())
	return (XORP_OK);		// Already down
    if (is_pending_down())
	return (XORP_OK);		// Already pending DOWN

    ServiceBase::set_status(SERVICE_SHUTTING_DOWN);

    return (XORP_OK);
}

int
ProtoState::startup()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_STARTING)
	|| (ServiceBase::status() == SERVICE_RUNNING))
	return (XORP_OK);

    if (ServiceBase::status() != SERVICE_READY)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
ProtoState::reset()
{
    if (ServiceBase::status() != SERVICE_READY)
	ServiceBase::set_status(SERVICE_READY);

    return (XORP_OK);
}

int
ProtoState::shutdown()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_SHUTDOWN)
	|| (ServiceBase::status() == SERVICE_SHUTTING_DOWN)
	|| (ServiceBase::status() == SERVICE_FAILED)) {
	return (XORP_OK);
    }

    if ((ServiceBase::status() != SERVICE_RUNNING)
	&& (ServiceBase::status() != SERVICE_STARTING)
	&& (ServiceBase::status() != SERVICE_PAUSING)
	&& (ServiceBase::status() != SERVICE_PAUSED)
	&& (ServiceBase::status() != SERVICE_RESUMING)) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
ProtoState::disable()
{
    (void)ProtoState::shutdown();
    _flags &= ~XORP_ENABLED;
}

string
ProtoState::state_str() const
{
    if (is_disabled())
	return ("DISABLED");
    if (is_down())
	return ("DOWN");
    if (is_up())
	return ("UP");
    if (is_pending_up())
	return ("PENDING_UP");
    if (is_pending_down())
	return ("PENDING_DOWN");
    
    return ("UNKNOWN");
}

/**
 * Test if the unit state is UP.
 * 
 * @return true if the unit state is UP.
 */
bool
ProtoState::is_up() const
{
    return (ServiceBase::status() == SERVICE_RUNNING);
}

/**
 * Test if the unit state is DOWN.
 * 
 * @return true if the unit state is DOWN.
 */
bool
ProtoState::is_down() const
{
    return ((ServiceBase::status() == SERVICE_READY)
	    || (ServiceBase::status() == SERVICE_SHUTDOWN));
}

/**
 * Test if the unit state is PENDING-UP.
 * 
 * @return true if the unit state is PENDING-UP.
 */
bool
ProtoState::is_pending_up() const
{
    return (ServiceBase::status() == SERVICE_STARTING);
}

/**
 * Test if the unit state is PENDING-DOWN.
 * 
 * @return true if the unit state is PENDING-DOWN.
 */
bool
ProtoState::is_pending_down() const
{
    return (ServiceBase::status() == SERVICE_SHUTTING_DOWN);
}
