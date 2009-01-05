// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/process_watch.hh,v 1.9 2008/10/02 21:58:00 bms Exp $

#ifndef __POLICY_PROCESS_WATCH_HH__
#define __POLICY_PROCESS_WATCH_HH__

#include "policy/common/policy_exception.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "process_watch_base.hh"
#include "pw_notifier.hh"
#include "protocol_map.hh"
#include <set>
#include <string>

/**
 * @short Keeps track of which XORP processes of interest are alive.
 *
 * The VarMap will register interest in protocols for known protocols with the
 * ProcessWatch. The ProcessWatch will then register this interest with the
 * finder. 
 *
 * Very similar / identical to BGP's process watch.
 */
class ProcessWatch : public ProcessWatchBase {
public:
    /**
     * @short Exception thrown on error, such as Xrl failure.
     */
    class PWException : public PolicyException {
    public:
        PWException(const char* file, size_t line, const string& init_why = "")   
            : PolicyException("PWException", file, line, init_why) {} 
    };

    /**
     * @param rtr Xrl router to use.
     * @param pmap protocol map.
     */
    ProcessWatch(XrlStdRouter& rtr, ProtocolMap& pmap);

    /**
     * Callback for all Xrl calls.
     *
     * @param err possible Xrl error.
     */
    void register_cb(const XrlError& err);

    /**
     * Add an interest in a protocol.
     *
     * @param proc process of the protocol to add interest for.
     */
    void add_interest(const string& proc);

    /**
     * Announce birth of a protocol [process].
     *
     * @param proto protocol that came to life.
     */
    void birth(const string& proto);
    
    /**
     * Announce death of a protocol.
     *
     * @param proto protocol that died.
     */
    void death(const string& proto);
   
    /**
     * An exception is thrown if the process watch is not watching the requested
     * protocol.
     *
     * @return true if protocol is alive, false otherwise.
     * @param proto protocol for which status is requested.
     */
    bool alive(const string& proto);

    /**
     * Set an object which will receive birth/death notifications.
     *
     * If a previous object was "registered", it will be removed. Only one
     * object may receive notifications.
     *
     * @param notifier object where notifications should be sent.
     */
    void set_notifier(PWNotifier& notifier);

private:
    ProtocolMap&    _pmap;
    set<string>	    _watching;
    set<string>	    _alive;
    XrlFinderEventNotifierV0p1Client _finder;
    string	    _instance_name;

    // do not delete, we do not own
    PWNotifier*	    _notifier;

    string	    _finder_name;
};

#endif // __POLICY_PROCESS_WATCH_HH__
