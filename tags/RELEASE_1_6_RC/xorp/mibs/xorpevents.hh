// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/mibs/xorpevents.hh,v 1.20 2008/07/23 05:11:02 pavlin Exp $

#ifndef __MIBS_XORPEVENTS_HH__
#define __MIBS_XORPEVENTS_HH__

#include <set>
#include <map>
#include "libxorp/timeval.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xorpfd.hh"

class SnmpEventLoop;

/**
 * @short A singleton @ref EventLoop capable of exporting events to
 * the snmp agent.
 *
 * The @ref SnmpEventLoop class provides an EventLoop to all the MIB modules
 * that need to communicate with XORP.  The modules will use the EventLoop to
 * instanciate listeners required for asyncronous inter-process communication
 * with XORP.
 */

class SnmpEventLoop : public EventLoop, public SelectorListObserverBase, 
    public TimerListObserverBase 
{
public:
    typedef std::set<XorpFd> FdSet;
    typedef std::map<TimeVal, unsigned int> AlarmMap;

public:
    /**
     * @return reference to the @ref SnmpEventLoop used by all XORP MIB
     * modules.
     *
     * This function provides the only way to instantiate this class, since
     * all other constructors are private.  Use it like this:
    <pre>
     * SnmpEventLoop& e = SnmpEventLoop::the_instance();
    </pre>
     *
     * Now use 'e' anywhere you would use an event loop in Xorp modules,
     * for instance:
    <pre>
     * XorpTimer xt = e.new_periodic_ms(100, callback);
    </pre>
     *
     * NOTE:  typically you would never need to call e.run().  If you do, you
     * should set a short timeout, since while your MIB module is blocked, you
     * cannot process any other SNMP requests. 
     */
    static SnmpEventLoop& the_instance();

    /**
    * This will free the one and only instance
    */
    void destroy();

     /**
     * This is the name this class will use to identify itself in the
     * snmpd log.  Only used for debugging
     */
    static void set_log_name(const char * n) { _log_name = n; }
    static const char *  get_log_name() { return _log_name; }

protected:
    AlarmMap _pending_alarms;
    FdSet _exported_readfds;
    FdSet _exported_writefds;
    FdSet _exported_exceptfds;

private:
    SnmpEventLoop(SnmpEventLoop& o);		    // Not implemented
    SnmpEventLoop& operator= (SnmpEventLoop& o);    // Not implemented
    SnmpEventLoop();
    virtual ~SnmpEventLoop();

    void clear_pending_alarms ();
    void clear_monitored_fds ();

    void notify_added(XorpFd, const SelectorMask&);
    void notify_removed(XorpFd, const SelectorMask&);

    void notify_scheduled(const TimeVal&);
    void notify_unscheduled(const TimeVal&);

    friend void run_fd_callbacks(int, void *);
    friend void run_timer_callbacks(u_int, void *);

    static const char * _log_name;
    static SnmpEventLoop _sel;
};

//
// run_fd_callbacks and run_timer_callbacks must be callable from C modules,
// this is why they are not members of SnmpEventLoop.  In principle they are
// only needed in this module, but not declared static so they can be invoked
// from test programs.
//
extern void run_fd_callbacks(int, void *);
extern void run_timer_callbacks(u_int, void *);

#endif // __MIBS_XORPEVENTS_HH__
