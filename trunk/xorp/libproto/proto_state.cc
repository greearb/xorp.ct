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

#ident "$XORP: xorp/libproto/proto_state.cc,v 1.1 2003/03/19 23:38:22 pavlin Exp $"


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
    disable();			// XXX: default is to disable.
}

ProtoState::~ProtoState()
{
    
}

const char *
ProtoState::state_string() const
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

int
ProtoState::start()
{
    if (is_disabled())
	return (XORP_ERROR);
    if (is_up())
	return (XORP_ERROR);		// Already running
    _flags &= ~(XORP_UP | XORP_DOWN | XORP_PENDING_UP | XORP_PENDING_DOWN);
    _flags |= XORP_UP;
    
    return (XORP_OK);
}

int
ProtoState::stop()
{
    if (is_down())
	return (XORP_ERROR);		// Wasn't running
    _flags &= ~(XORP_UP | XORP_DOWN | XORP_PENDING_UP | XORP_PENDING_DOWN);
    _flags |= XORP_DOWN;
    
    return (XORP_OK);
}

int
ProtoState::pending_start()
{
    if (is_disabled())
	return (XORP_ERROR);
    if (is_up())
	return (XORP_ERROR);		// Already running
    if (is_pending_up())
	return (XORP_ERROR);		// Already pending UP
    _flags &= ~(XORP_UP | XORP_DOWN | XORP_PENDING_UP | XORP_PENDING_DOWN);
    _flags |= XORP_PENDING_UP;
    
    return (XORP_OK);
}

int
ProtoState::pending_stop()
{
    if (! is_up())
	return (XORP_ERROR);		// Wasn't running
    if (is_pending_down())
	return (XORP_ERROR);		// Already pending DOWN
    _flags &= ~(XORP_UP | XORP_DOWN | XORP_PENDING_UP | XORP_PENDING_DOWN);
    _flags |= XORP_PENDING_DOWN;
    
    return (XORP_OK);
}
