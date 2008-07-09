// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/ioevents.hh,v 1.4 2007/02/16 22:46:19 pavlin Exp $

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
