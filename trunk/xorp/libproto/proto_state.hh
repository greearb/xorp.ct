// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libproto/proto_state.hh,v 1.11 2008/07/23 05:10:39 pavlin Exp $


#ifndef __LIBPROTO_PROTO_STATE_HH__
#define __LIBPROTO_PROTO_STATE_HH__


#include "libxorp/xorp.h"
#include "libxorp/service.hh"


//
// Protocol state generic functionality
//


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short Base class for keeping state for each protocol unit (node, vif, etc).
 */
class ProtoState : public ServiceBase {
public:
    /**
     * Default Constructor.
     */
    ProtoState();
    
    /**
     * Destructor
     */
    virtual ~ProtoState();
    
    /**
     * Start the unit.
     * 
     * This operation will fail if the unit is disabled, or is already up.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the unit.
     * 
     * This operation will fail if the unit was down already.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Pending-start the unit.
     * 
     * The pending-start state is an intermediate state between down and up.
     * In this state only some operations are allowed (the allowed operations
     * are unit-specific).
     * This operation will fail if the unit is disabled, is up, or is
     * pending-up already.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		pending_start();
    
    /**
     * Pending-stop the unit.
     * 
     * The pending-stop state is an intermediate state between up and down.
     * In this state only some operations are allowed (the allowed operations
     * are unit-specific).
     * This operation will fail if the unit is not up.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		pending_stop();
    
    /**
     * Enable the unit.
     * 
     * If an unit is not enabled, it cannot be start, or pending-start.
     */
    void	enable()	{ _flags |= XORP_ENABLED;	}
    
    /**
     * Disable the unit.
     * 
     * If an unit is disabled, it cannot be start or pending-start.
     * If the unit was runnning, it will be stop first.
     */
    void	disable();
    
    /**
     * Test if the unit state is UP.
     * 
     * @return true if the unit state is UP.
     */
    bool	is_up()	const;
    
    /**
     * Test if the unit state is DOWN.
     * 
     * @return true if the unit state is DOWN.
     */
    bool	is_down() const;

    /**
     * Test if the unit state is PENDING-UP.
     * 
     * @return true if the unit state is PENDING-UP.
     */
    bool	is_pending_up()	const;

    /**
     * Test if the unit state is PENDING-DOWN.
     * 
     * @return true if the unit state is PENDING-DOWN.
     */
    bool	is_pending_down() const;
    
    /**
     * Test if the unit is enabled.
     * 
     * @return true if the unit is enabled.
     */
    bool	is_enabled()	const	{ return (_flags & XORP_ENABLED); }
    
    /**
     * Test if the unit is disabled.
     * 
     * @return true if the unit is disabled.
     */
    bool	is_disabled()	const	{ return (! is_enabled());	}
    
    /**
     * Test if debug mode is enabled.
     * 
     * @return true if debug mode is enabled.
     */
    bool	is_debug()	const	{ return (_debug_flag);		}
    
    /**
     * Set/reset debug mode.
     * 
     * @param v if true, set debug mode, otherwise reset it.
     */
    void	set_debug(bool v)	{ _debug_flag = v;		}
    
    /**
     * Get a string with the state of the unit.
     * 
     * The state string is one of the following:
     *   "DISABLED", "DOWN", "UP", "PENDING_UP", "PENDING_DOWN", "UNKNOWN"
     * 
     * @return string with the state of the unit.
     */
    string	state_str() const;
    
private:
    /**
     * Startup operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Reset service to SERVICE_READY from whichever state it is in.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int reset();

    enum {
	XORP_ENABLED	= 1 << 0	// Entity is enabled.
    };
    uint32_t	_flags;			// Misc. flags: XORP_ENABLED, etc
					// (see above).
    bool	_debug_flag;		// Enable/Disable debug messages.
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __LIBPROTO_PROTO_STATE_HH__
