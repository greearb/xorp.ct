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

#ifndef __POLICY_FILTER_MANAGER_HH__
#define __POLICY_FILTER_MANAGER_HH__

#include "policy/common/policy_exception.hh"
#include "policy/common/filter.hh"

#include "xrl/interfaces/policy_backend_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "libxorp/eventloop.hh"

#include "code.hh"
#include "set_map.hh"
#include "process_watch.hh"
#include "filter_manager_base.hh"
#include "pw_notifier.hh"

#include <map>


/**
 * @short Deals with sending code to policy filters.
 *
 * It manages sending the code and sets to the filters in the protocols. It also
 * updates the policy-tag-map in the RIB.
 *
 * It also keeps track if protocols die / come back to life, throw the process
 * watch in order to re-send code to filters which were dead and are now back.
 */
class FilterManager : public FilterManagerBase, public PWNotifier {
public:
    // XXX: pull this out ?
    typedef map<string,Code*> CodeMap;

    typedef map<string,string> ConfQueue;
    
    typedef set<uint32_t> TagSet;
    typedef map<string,TagSet*> TagMap;


    /**
     * @short Exception thrown on error. Such as xrl failure.
     */
    class FMException : public PolicyException {
    public:
	FMException(const string& err) : PolicyException(err) {}
    };

    /**
     * The FilterManager closely works with the Configuration class. Maybe in
     * the future holding a reference the Configuration class, instead of the
     * internal components may be a better solution.
     *
     * @param imp import filter CodeMap to use.
     * @param sm source match filter CodeMap to use.
     * @param exp export filter CodeMap to use.
     * @param sets SetMap to use.
     * @param tagmap TagMap to use.
     * @param rtr the XRL router used by the policy process.
     * @param pw the process watcher.
     */
    FilterManager(const CodeMap& imp, const CodeMap& sm, 
		 const CodeMap& exp, const SetMap& sets, 
		 const TagMap& tagmap, XrlStdRouter& rtr, 
		 ProcessWatch& pw);

    /**
     * Update the filter for a specific target. This will normally queue a
     * filter configuration request.
     *
     * @param t target which should be updated.
     */
    void update_filter(const Code::Target& t);

    /**
     * Xrl callback for all XRL requests.
     *
     * @param e possible XRL error.
     */
    void policy_backend_cb(const XrlError& e);

    /**
     * Flushes the route pushing queue.
     */
     void push_routes_now();

    /**
     * Flush all queues now
     */
    void flush_updates_now();

    /**
     * Flush all updates after msec milliseconds.
     *
     * If a new update comes in before msec expires, only the new update will be
     * performed.
     *
     * @param msec milliseconds after which all queues should be flushed.
     */
    void flush_updates(uint32_t msec);


    // PWNotifier interface:
    /**
     * A protocol just came back to life.
     *
     * @param protocol name of protocol which is alive.
     */
    void birth(const string& protocol);

    /**
     * A protocol just died.
     *
     * @param protocol name of protocol which died.
     */
    void death(const string& protocol);

private:
    /**
     * Update the import filter for a specific protocol.
     *
     * @param protocol protocol of which the import filter must be updated.
     */
    void update_import_filter(const string& protocol);
    
    /**
     * Update the source-match filter for a specific protocol.
     *
     * @param protocol protocol of which the sourcematch filter must be updated.
     */
    void update_sourcematch_filter(const string& protocol);
    
    /**
     * Update the export filter for a specific protocol.
     *
     * @param protocol protocol of which the export filter must be updated.
     */
    void update_export_filter(const string& protocol);

    /**
     * Update the policy-tag map in the RIB for a specific protocol.
     *
     * @param protocol protocol for which tags should be updated.
     */
    void update_tagmap(const string& protocol);

    /**
     * Flush all updates for export filters. Also keep track which protocols
     * must have their routes pushed.
     */
    void flush_export_queue();

    /**
     * Flush all updates for a specific filter-type queue. Record which
     * protocols need to be pushed.
     *
     * @param queue queue for which updates need to be flushed.
     * @param f filter for which updates should be flushed.
     */
    void flush_queue(ConfQueue& queue, filter::Filter f);


    void delete_queue_protocol(ConfQueue& queue, const string& protocol);

    void update_queue(const string& protocol, const CodeMap& cm, 
		      ConfQueue& queue);


    const CodeMap& _import;
    const CodeMap& _sourcematch;
    const CodeMap& _export;
    const SetMap& _sets;
    const TagMap& _tagmap;

    ConfQueue _import_queue;
    ConfQueue _sourcematch_queue;
    ConfQueue _export_queue;
    set<string> _push_queue;


    EventLoop& _eventloop;

    // we should have a timer per protocol.
    XorpTimer _flush_timer;
    XorpTimer _push_timer;
    unsigned _push_timeout;
    

    ProcessWatch& _process_watch;

    XrlPolicyBackendV0p1Client _policy_backend;
    XrlRibV0p1Client _rib;

    string _rib_name;
};


#endif // __POLICY_FILTER_MANAGER_HH__
