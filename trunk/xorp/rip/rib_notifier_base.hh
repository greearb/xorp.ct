// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rip/rib_notifier_base.hh,v 1.8 2008/10/02 21:58:17 bms Exp $

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
