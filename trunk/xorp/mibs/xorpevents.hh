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

// $XORP: xorp/mibs/xorpevents.hh,v 1.8 2003/06/16 19:50:01 jcardona Exp $

#ifndef __MIBS_XORPEVENTLOOP_HH__
#define __MIBS_XORPEVENTLOOP_HH__

#include "fixconfigs.h"
#include <set>
#include <map>
#include "libxorp/timeval.hh"
#include "libxorp/eventloop.hh"

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
    typedef std::set<int> FdSet;
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
     * XorpTimer xt = e.new_periodic(100, callback);
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

    void notify_added(int, const SelectorMask&);
    void notify_removed(int, const SelectorMask&);

    void notify_scheduled(const TimeVal&);
    void notify_unscheduled(const TimeVal&);

    friend void run_fd_callbacks (int, void *);
    friend void run_timer_callbacks(u_int, void *);

    static const char * _log_name;
    static SnmpEventLoop _sel;
};


#endif // __MIBS_XORPEVENTLOOP_HH__
