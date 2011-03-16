// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __LIBXORP_IOEVENTS_HH__
#define __LIBXORP_IOEVENTS_HH__

#include "xorpfd.hh"
#include "callback.hh"

/**
 * @short I/O event type.
 *
 * Enumeration of various event types supported by the I/O callback facade.
 */
enum IoEventType {
    IOT_READ,		// Object is ready to read
    IOT_WRITE,		// Object is ready to write
    IOT_EXCEPTION,	// Object has exceptional condition
    IOT_ACCEPT,		// Socket: Inbound connection available for accept()
    IOT_CONNECT,	// Socket: Outgoing connect() has completed
    IOT_DISCONNECT,	// Socket: Peer disconnected
    IOT_ANY		// Match any kind of event (for removal)
};

typedef XorpCallback2<void,XorpFd,IoEventType>::RefPtr IoEventCb;

#endif // __LIBXORP_IOEVENTS_HH__
