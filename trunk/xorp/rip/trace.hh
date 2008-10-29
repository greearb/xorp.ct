// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/rip/trace.hh,v 1.21 2008/10/02 21:57:50 bms Exp $

#ifndef __RIP_TRACE_HH__
#define __RIP_TRACE_HH__

/**
 * All the trace variables in one place.
 */
struct Trace {
    Trace() :
	      _routes(false),
	      _packets(false)
	      // Don't forget to add new variables to the all() method.
	{}

    /*
     * Set all flags
     */
    void all(bool val) {
	 _routes = _packets = val;
    }

    bool _routes;	// add,replace,delete route.
    bool _packets;	// Incoming and outgoing packets.
};

#endif // __RIP_TRACE_HH__
