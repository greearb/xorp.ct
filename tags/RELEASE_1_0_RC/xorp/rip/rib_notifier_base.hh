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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_RIB_NOTIFIER_BASE_HH__
#define __RIP_RIB_NOTIFIER_BASE_HH__

#include "libxorp/eventloop.hh"
#include "update_queue.hh"

class EventLoop;

/**
 * @short Base class for RIB notificatiers.
 *
 * This class acts as a base class for RIB notifiers.  RIB notifiers
 * are classes that convey RIP routes to the RIB.  The base class
 * implements update queue polling.  When polling is enabled, it polls
 * the UpdateQueue periodically for updates to be send to the RIB.  If
 * updates are available available it calls @ref updates_available()
 * which derived classes implement according to their needs.
 */
template <typename A>
class RibNotifierBase {
public:
    static const uint32_t DEFAULT_POLL_MS = 1000;

public:
    RibNotifierBase(EventLoop&	    eventloop,
		    UpdateQueue<A>& update_queue,
		    uint32_t	    poll_ms = DEFAULT_POLL_MS);
    virtual ~RibNotifierBase();

protected:
    virtual void updates_available() = 0;

protected:
    void start_polling();
    void stop_polling();
    bool poll_updates();

protected:
    EventLoop&					_e;
    UpdateQueue<A>&		  		_uq;
    typename UpdateQueue<A>::ReadIterator	_ri;
    uint32_t					_poll_ms;
    XorpTimer					_t;
};

#endif // __RIP_RIB_NOTIFIER_BASE_HH__
