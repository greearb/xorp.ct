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

// $XORP: xorp/libproto/proto_state.hh,v 1.1 2003/03/19 23:38:22 pavlin Exp $


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
     * @return C-style string with the state of the unit.
     */
    const char	*state_string() const;
    
private:
    /**
     * Startup operation.
     * 
     * @return true on success, false on failure.
     */
    bool	startup();

    /**
     * Shutdown operation.
     * 
     * @return true on success, false on failure.
     */
    bool	shutdown();

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
