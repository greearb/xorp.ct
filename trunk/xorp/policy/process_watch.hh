// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __POLICY_PROCESS_WATCH_HH__
#define __POLICY_PROCESS_WATCH_HH__

#include "policy/common/policy_exception.hh"

#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "process_watch_base.hh"
#include "pw_notifier.hh"


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
	PWException(const string& err) : PolicyException(err) {}
    };

    /**
     * @param rtr Xrl router to use.
     */
    ProcessWatch(XrlStdRouter& rtr);

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
    set<string> _watching;
    
    set<string> _alive;

    XrlFinderEventNotifierV0p1Client _finder;

    string _instance_name;

    // do not delete, we do not own
    PWNotifier* _notifier;

    string _finder_name;
};

#endif // __POLICY_PROCESS_WATCH_HH__
