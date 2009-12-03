// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "finder_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/random.h"
#include "libxorp/status_codes.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"

#include <list>
#include <vector>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "xrl/targets/test_finder_events_base.hh"

#include "finder_server.hh"
#include "finder_tcp_messenger.hh"
#include "finder_xrl_target.hh"
#include "permits.hh"
#include "sockutil.hh"

#include "xrl_std_router.hh"


///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_finder_events";
static const char *program_description  = "Test Finder events are "
					  "properly reported";
static const char *program_version_id   = "0.1";
static const char *program_date         = "May, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//


static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FUNCTION__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	const char *wh = strrchr(file, '/');				\
	wh = (wh) ? wh + 1 : file;					\
	TimeVal t;							\
	TimerList::system_gettimeofday(&t);				\
 	printf("From %s:%d at %ld.%06ld: ", wh, line,			\
	       static_cast<long>(t.sec()),				\
	       static_cast<long>(t.usec()));				\
	printf(x);							\
    }									\
} while(0)

static int ready = 0;
#define WAIT_FOR_CALLBACK() 						\
do {									\
    ready++;								\
    verbose_log("up ready: %d\n", ready);				\
} while(0)

#define CALLBACK_CALLED() 						\
do {									\
    ready--;								\
    verbose_log("down ready: %d\n", ready);				\
} while(0)


/**
 * @short Test class that registers for event notifications with the finder.
 */
class FinderEventObserver
{
public:
    struct Watch {
	set<string> instance_names;
    };

    typedef map<string, Watch> Watches;

public:
    FinderEventObserver(EventLoop& e, IPv4 finder_addr, uint16_t finder_port)
	: _xrl_router(e, "finder_event_observer", finder_addr, finder_port)
    {
    }

    void finder_register_interest_cb(const XrlError& e,
				     string	     class_name)
    {
	XLOG_ASSERT(e == XrlError::OKAY());
	_watches[class_name];

	CALLBACK_CALLED();
    }

    void add_class_to_watch(const string& class_name)
    {
	WAIT_FOR_CALLBACK();

	XrlFinderEventNotifierV0p1Client cl(&_xrl_router);
	bool r = cl.send_register_class_event_interest(
			 "finder", _xrl_router.instance_name(), class_name,
		callback(this,
			 &FinderEventObserver::finder_register_interest_cb,
			 class_name));
	XLOG_ASSERT(r == true);
    }

    void finder_deregister_interest_cb(const XrlError& e,
				       string	       class_name)
    {
	XLOG_ASSERT(e == XrlError::OKAY());
	map<string,Watch>::iterator wi = _watches.find(class_name);
	XLOG_ASSERT(wi != _watches.end());
	_watches.erase(wi);

	CALLBACK_CALLED();
    }

    void remove_class_to_watch(const string& class_name)
    {
	WAIT_FOR_CALLBACK();

	XrlFinderEventNotifierV0p1Client cl(&_xrl_router);
	bool r = cl.send_deregister_class_event_interest(
			 "finder", _xrl_router.instance_name(), class_name,
		callback(this,
			 &FinderEventObserver::finder_deregister_interest_cb,
			 class_name));
	XLOG_ASSERT(r == true);
    }

    void birth_event(const string& class_name, const string& instance_name)
    {
	map<string,Watch>::iterator wi = _watches.find(class_name);
	XLOG_ASSERT(wi != _watches.end());
	Watch& w = wi->second;
	set<string>::iterator si = w.instance_names.find(instance_name);
	XLOG_ASSERT(si == w.instance_names.end());
	w.instance_names.insert(instance_name);
	verbose_log("Birth in class \"%s\", it's a \"%s\"\n",
		    class_name.c_str(), instance_name.c_str());
	CALLBACK_CALLED();
    }

    void death_event(const string& class_name, const string& instance_name)
    {
	map<string,Watch>::iterator wi = _watches.find(class_name);
	XLOG_ASSERT(wi != _watches.end());
	Watch& w = wi->second;
	set<string>::iterator si = w.instance_names.find(instance_name);
	XLOG_ASSERT(si != w.instance_names.end());
	w.instance_names.erase(si);
	verbose_log("Death in class \"%s\", it's a \"%s\"\n",
		    class_name.c_str(), instance_name.c_str());

	CALLBACK_CALLED();
    }

    uint32_t instance_count(const string& class_name)
    {
	map<string,Watch>::const_iterator wi = _watches.find(class_name);
	if (wi == _watches.end())
	    return 0;
	return wi->second.instance_names.size();
    }

    const Watches& watches() const	{ return _watches; }

    XrlRouter* router()			{ return &_xrl_router; }
    const XrlRouter* router() const	{ return &_xrl_router; }

protected:
    Watches	 _watches;
    XrlStdRouter _xrl_router;
};


/**
 * @short Xrl Handler interface for FinderEventObserver.
 */
class FinderEventObserverXrlTarget : public XrlTestFinderEventsTargetBase
{
public:
    FinderEventObserverXrlTarget(FinderEventObserver& feo)
	: XrlTestFinderEventsTargetBase(feo.router()), _feo(feo)
    {
    }

    XrlCmdError common_0_1_get_target_name(string& name)
    {
	name = _feo.router()->name();
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_version(string& version)
    {
	version = "0.0";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_status(uint32_t& status_enum,
				      string&	status_txt)
    {
	status_enum = PROC_READY;
	status_txt = "Ready.";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_startup()
    {
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_shutdown()
    {
	//We don't normally want to shutdown a process through this
	//particular Xrl target.
	return XrlCmdError::COMMAND_FAILED();
    }

    XrlCmdError finder_event_observer_0_1_xrl_target_birth(const string& cls,
							   const string& ins)
    {
	_feo.birth_event(cls, ins);
	return XrlCmdError::OKAY();
    }

    XrlCmdError finder_event_observer_0_1_xrl_target_death(const string& cls,
							   const string& ins)
    {
	_feo.death_event(cls, ins);
	return XrlCmdError::OKAY();
    }

private:
    FinderEventObserver& _feo;
};


/**
 * @short Class containing components for Finder event observation.
 */
class FinderEventObserverPackage
{
public:
    FinderEventObserverPackage(EventLoop& e, IPv4 addr, uint16_t port)
	: _feo(e, addr, port), _feoxt(_feo)
    {
	_feo.router()->finalize();
    }

    FinderEventObserver& observer()	{ return _feo; }

    bool xrl_router_connected() const	{ return _feo.router()->connected(); }
    bool xrl_router_ready() const	{ return _feo.router()->ready(); }
    bool xrl_router_failed() const	{ return _feo.router()->failed(); }

protected:
    FinderEventObserver 	 _feo;
    FinderEventObserverXrlTarget _feoxt;
};


///////////////////////////////////////////////////////////////////////////////
//
// Test Class (to be watched)
//

class AnXrlTarget
{
public:
    AnXrlTarget(EventLoop& e, const char* class_name,
		IPv4 finder_addr, uint16_t finder_port)
	: _router(e, class_name, finder_addr, finder_port)
    {
	_router.finalize();
    }
    const string& instance_name() const { return _router.instance_name(); }
    const string& class_name() const { return _router.class_name(); }
private:
    XrlStdRouter _router;
};


///////////////////////////////////////////////////////////////////////////////
//
// Timed command requests
//

static void
create_observer(EventLoop*		     e,
		FinderEventObserverPackage** pp,
		IPv4			     addr,
		uint16_t		     port)
{
    verbose_log("Creating observer.\n");
    *pp = new FinderEventObserverPackage(*e, addr, port);
    FinderEventObserverPackage* p = *pp;
    p->observer().router()->finalize();
}

static void
destroy_observer(FinderEventObserverPackage** pp)
{
    verbose_log("Destroying observer.\n");
    delete *pp;
    *pp = 0;
}

static void
assert_observer_ready(FinderEventObserverPackage** pp)
{
    verbose_log("Checking observer is ready.\n");
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    XLOG_ASSERT(p->xrl_router_ready());
}

static void
add_watch_to_observer(FinderEventObserverPackage** pp,
		      const char*		   class_name)
{
    verbose_log("Adding watch to observer\n");
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    XLOG_ASSERT((p)->observer().router());
    XLOG_ASSERT((p)->observer().router()->connected());
    p->observer().add_class_to_watch(class_name);
}

static void
assert_observer_watching(FinderEventObserverPackage** pp,
			 const char*		      class_name)
{
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    XLOG_ASSERT(p->observer().watches().find(class_name) !=
		p->observer().watches().end());
}

static void
remove_watch_from_observer(FinderEventObserverPackage** pp,
			   const char*			class_name)
{
    verbose_log("Removing watch from observer\n");
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    XLOG_ASSERT((p)->observer().router());
    XLOG_ASSERT((p)->observer().router()->connected());
    p->observer().remove_class_to_watch(class_name);
}

static void
assert_observer_not_watching(FinderEventObserverPackage** pp,
			     const char*		  class_name)
{
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    XLOG_ASSERT(p->observer().watches().find(class_name) ==
		p->observer().watches().end());
}

static void
assert_observer_class_instance_count(FinderEventObserverPackage** pp,
				     const char*		  class_name,
				     uint32_t			  cnt)
{
    FinderEventObserverPackage* p = *pp;
    XLOG_ASSERT(p != 0);
    uint32_t icnt = p->observer().instance_count(class_name);

    if (icnt != cnt)
	XLOG_FATAL("icnt %d != cnt %d", icnt, cnt);
}

static void
create_xrl_target(EventLoop*		e,
		  IPv4			finder_addr,
		  uint16_t		finder_port,
		  const char*		class_name,
		  list<AnXrlTarget*>* store,
		  bool observing)
{
    if (observing) {
	WAIT_FOR_CALLBACK();
    }

    AnXrlTarget* a = new AnXrlTarget(*e, class_name, finder_addr, finder_port);
    store->push_back(a);
    verbose_log("Creating Xrl target \"%s\" in class \"%s\"\n",
		a->instance_name().c_str(), a->class_name().c_str());
}

static void
remove_oldest_xrl_target(list<AnXrlTarget*>* store, bool observing)
{
    if (observing) {
	WAIT_FOR_CALLBACK();
    }

    XLOG_ASSERT(store != 0 && store->empty() == false);
    AnXrlTarget* a = store->front();
    store->pop_front();
    verbose_log("Kill Xrl target \"%s\" in class \"%s\"\n",
		a->instance_name().c_str(), a->class_name().c_str());
    delete a;
}

static void
remove_any_xrl_target(list<AnXrlTarget*>* store, bool observing)
{
    if (observing) {
	WAIT_FOR_CALLBACK();
    }

    uint32_t n = store->size();
    XLOG_ASSERT(n > 0);

    n = (uint32_t)xorp_random() % n;
    list<AnXrlTarget*>::iterator i = store->begin();
    while (n > 0) {
	i++;
	n--;
    }
    XLOG_ASSERT(i != store->end());

    AnXrlTarget* a = *i;
    store->erase(i);
    verbose_log("Kill Xrl target \"%s\" in class \"%s\"\n",
		a->instance_name().c_str(), a->class_name().c_str());
    delete a;
}

static void
assert_xrl_target_count(list<AnXrlTarget*>* store,
			uint32_t cnt)
{
    uint32_t n = store->size();
    XLOG_ASSERT(n == cnt);
}

// On a slow machine even allowed may not be sufficient. If a test
// fails try reducing the burst count and increasing the allowed time.
static const uint32_t allowed = 60 * 1000;	// Allowed time for test in ms

static
void
toolong()
{
    XLOG_FATAL("The test has taken too long allowed %dms", allowed);
}

// A list of functions to call is passed in via locb and any function
// that requires synchronisation with the next function should
// increment "ready". The callback should decrement "ready" and the
// next function in the list will be called. If the tests take too long
// the timeout will fire and the test will fail.
static void
drip_run(EventLoop& e, list<OneoffTimerCallback>& locb)
{
    XorpTimer timeout = e.new_oneoff_after_ms(allowed, callback(&toolong));

    int pos = 0;
    while (locb.empty() == false) {
	while(0 != ready) {
	    e.run();
	}
	verbose_log("Events: %u ready: %d cnt %d\n",
		    XORP_UINT_CAST(e.timer_list_length()), ready, pos++);
	XLOG_ASSERT(0 == ready);
	locb.front()->dispatch();
	locb.pop_front();
    }

    //
    // XXX: Wait for 3 seconds so that the wrapper shell script that
    // executes this test program can detect that the program was
    // actually executed and still running (the script itself
    // waits for 2 seconds).
    //
    e.timer_list().system_sleep(TimeVal(3,0));
}


static void
test1(IPv4		finder_addr,
      uint16_t	 	finder_port,
      const uint32_t	/* burst_cnt */,
      bool		use_internal_finder)
{
    EventLoop e;

    FinderServer* fs = 0;
    if (use_internal_finder) {
	fs = new FinderServer(e, finder_addr, finder_port);
    } else {
	verbose_log("Using external Finder\n");
    }

    FinderEventObserverPackage* pfeo = 0;
    list<AnXrlTarget*> tgt_store;

    // -- End cut-and-paste header --

    //
    // Test 1
    //
    // Create observer, clean up observer
    //
    create_observer(&e, &pfeo, finder_addr, finder_port);

    while (pfeo->xrl_router_ready() == false &&
	   pfeo->xrl_router_failed() == false) {
	e.run();
    }

    list<OneoffTimerCallback> locb;
    locb.push_back(callback(assert_observer_ready, &pfeo));
    locb.push_back(callback(add_watch_to_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_watching, &pfeo, "class_a"));
    locb.push_back(callback(remove_watch_from_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_not_watching, &pfeo, "class_a"));
    locb.push_back(callback(destroy_observer, &pfeo));
    locb.push_back(callback(assert_xrl_target_count, &tgt_store,
			    static_cast<uint32_t>(0u)));

    drip_run(e, locb);

    // -- Begin cut-and-paste footer --
    if (fs) delete fs;
}

static void
test2(IPv4		finder_addr,
      uint16_t	 	finder_port,
      const uint32_t	burst_cnt,
      bool		use_internal_finder)
{
    EventLoop e;

    FinderServer* fs = 0;
    if (use_internal_finder) {
	fs = new FinderServer(e, finder_addr, finder_port);
    } else {
	verbose_log("Using external Finder\n");
    }

    FinderEventObserverPackage* pfeo = 0;
    list<AnXrlTarget*> tgt_store;

    // -- End cut-and-paste header --

    //
    // Test 2
    //
    // Start an observer, spawn an observable, clean up an
    // observables, spawn an observable, clean it up, repeat over,
    // over then clean up observer.
    //
    list<OneoffTimerCallback> locb;

    create_observer(&e, &pfeo, finder_addr, finder_port);
    while (pfeo->xrl_router_ready() == false &&
	   pfeo->xrl_router_failed() == false) {
	e.run();
    }

    locb.push_back(callback(assert_observer_ready, &pfeo));
    locb.push_back(callback(add_watch_to_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_watching,&pfeo, "class_a"));

    for (uint32_t i = 0; i < burst_cnt; i++) {
	locb.push_back(callback(create_xrl_target, &e,
				finder_addr, finder_port,
				"class_a", &tgt_store, true));
	locb.push_back(callback(assert_observer_class_instance_count,
				&pfeo, "class_a", static_cast<uint32_t>(1u)));
	locb.push_back(callback(remove_oldest_xrl_target, &tgt_store, true));
	locb.push_back(callback(assert_observer_class_instance_count,
				&pfeo, "class_a", static_cast<uint32_t>(0u)));
    }

    locb.push_back(callback(create_xrl_target, &e, finder_addr, finder_port,
			    "class_b", &tgt_store, false));
    locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
			    "class_b", static_cast<uint32_t>(0u)));
    locb.push_back(callback(remove_oldest_xrl_target, &tgt_store, false));
    locb.push_back(callback(remove_watch_from_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_not_watching, &pfeo, "class_a"));
    locb.push_back(callback(destroy_observer, &pfeo));
    locb.push_back(callback(assert_xrl_target_count, &tgt_store,
			    static_cast<uint32_t>(0u)));

    drip_run(e, locb);

    // -- Begin cut-and-paste footer --
    if (fs) delete fs;
}

static void
test3(IPv4		finder_addr,
      uint16_t	 	finder_port,
      const uint32_t	burst_cnt,
      bool		use_internal_finder)
{
    EventLoop e;

    FinderServer* fs = 0;
    if (use_internal_finder) {
	fs = new FinderServer(e, finder_addr, finder_port);
    } else {
	verbose_log("Using external Finder\n");
    }

    FinderEventObserverPackage* pfeo = 0;
    list<AnXrlTarget*> tgt_store;

    // -- End cut-and-paste header --

    //
    // Test 3
    //
    // Start an observer, spawn a group of observables, clean up
    // group of observables, clean up observer.
    //
    list<OneoffTimerCallback> locb;

    create_observer(&e, &pfeo, finder_addr, finder_port);
    while (pfeo->xrl_router_ready() == false &&
	   pfeo->xrl_router_failed() == false) {
	e.run();
    }

    locb.push_back(callback(assert_observer_ready, &pfeo));
    locb.push_back(callback(add_watch_to_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_watching, &pfeo, "class_a"));

    for (uint32_t i = 1; i <= burst_cnt; i++) {
	locb.push_back(callback(create_xrl_target, &e, finder_addr,
				finder_port, "class_a", &tgt_store, true));
	locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
		     "class_a", i));
    }

    for (uint32_t i = 1; i <= burst_cnt ; i++) {
	locb.push_back(callback(remove_any_xrl_target, &tgt_store, true));
	locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
		     "class_a", burst_cnt - i));
    }
    locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
			    "class_a", static_cast<uint32_t>(0u)));
    locb.push_back(callback(remove_watch_from_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_not_watching, &pfeo, "class_a"));
    locb.push_back(callback(destroy_observer, &pfeo));
    locb.push_back(callback(assert_xrl_target_count, &tgt_store,
			    static_cast<uint32_t>(0u)));

    drip_run(e, locb);

    // -- Begin cut-and-paste footer --
    if (fs) delete fs;
}

#if	0
static void
test4(IPv4		finder_addr,
      uint16_t	 	finder_port,
      const uint32_t	burst_cnt,
      bool		use_internal_finder)
{
    EventLoop e;

    FinderServer* fs = 0;
    if (use_internal_finder) {
	fs = new FinderServer(e, finder_addr, finder_port);
    } else {
	verbose_log("Using external Finder\n");
    }

    FinderEventObserverPackage* pfeo = 0;
    list<AnXrlTarget*> tgt_store;
    // -- End cut-and-paste header --

    list<OneoffTimerCallback> locb;
    //
    // Test 4
    //
    // Spawn a group of observables, spawn an observer, spawn some
    // more observables, delete some, delete the observer, clean up
    // observables.
    //

    for (uint32_t i = 1; i <= burst_cnt; i++) {
	locb.push_back(callback(create_xrl_target, &e, finder_addr,
				finder_port, "class_a", &tgt_store));
    }

    observing = false;
    drip_run(e, locb, true);
    observing = true;

    create_observer(&e, &pfeo, finder_addr, finder_port);
    while (pfeo->xrl_router_ready() == false &&
	   pfeo->xrl_router_failed() == false) {
	e.run();
    }

    locb.push_back(callback(assert_observer_ready, &pfeo));
    locb.push_back(callback(add_watch_to_observer, &pfeo, "class_a"));
    locb.push_back(callback(assert_observer_watching,&pfeo, "class_a"));
    locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
		 "class_a", burst_cnt));
    locb.push_back(callback(add_watch_to_observer, &pfeo, "class_b"));
    locb.push_back(callback(assert_observer_watching,&pfeo, "class_b"));
    for (uint32_t i = 1; i <= burst_cnt; i++) {
	locb.push_back(callback(create_xrl_target, &e, finder_addr,
				finder_port, "class_b", &tgt_store));
    }

    locb.push_back(callback(assert_observer_class_instance_count, &pfeo,
			    "class_b", burst_cnt));

    for (uint32_t i = 1; i <= burst_cnt; i++) {
	locb.push_back(callback(remove_any_xrl_target, &tgt_store));
    }

    locb.push_back(callback(destroy_observer, &pfeo));

    for (uint32_t i = 1; i <= burst_cnt ; i++) {
	locb.push_back(callback(remove_any_xrl_target, &tgt_store));
    }
    locb.push_back(callback(assert_xrl_target_count, &tgt_store,
			    static_cast<uint32_t>(0u)));

    drip_run(e, locb);

    // -- Begin cut-and-paste footer --
    if (fs) delete fs;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// test main
//

static int
test_main(IPv4		 finder_addr,
	  uint16_t	 finder_port,
	  const uint32_t burst_cnt,
	  bool		 use_internal_finder, int which)
{
    switch(which) {
    case 0:
	test1(finder_addr, finder_port, burst_cnt, use_internal_finder);
	ready = 0;
	test2(finder_addr, finder_port, burst_cnt, use_internal_finder);
	ready = 0;
	test3(finder_addr, finder_port, burst_cnt, use_internal_finder);
#if	0
	ready = 0;
	test4(finder_addr, finder_port, burst_cnt, use_internal_finder);
#endif
	break;
    case 1:
	test1(finder_addr, finder_port, burst_cnt, use_internal_finder);
	break;
    case 2:
	test2(finder_addr, finder_port, burst_cnt, use_internal_finder);
	break;
    case 3:
	test3(finder_addr, finder_port, burst_cnt, use_internal_finder);
	break;
#if	0
    case 4:
	test4(finder_addr, finder_port, burst_cnt, use_internal_finder);
	break;
#endif
    default:
	XLOG_FATAL("Unknown test");
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// Standard test program rubble.
//

/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -F <host>[:<port>]     "
	    "Specify arguments for an external Finder instance\n");
    fprintf(stderr, "  -b <n>                 "
	    "Set the number of simulated clients\n");
    fprintf(stderr, "  -r <n>                 "
	    "Set the number of test runs\n");
    fprintf(stderr, "  -t <n>                 "
	    "Test number to run\n");
    fprintf(stderr, "  -h                     Display this information\n");
    fprintf(stderr, "  -v                     Verbose output\n");
}


static bool
parse_finder_arg(const char* host_colon_port,
		 IPv4&	     finder_addr,
		 uint16_t&   finder_port)
{
    string finder_host;

    const char* p = strstr(host_colon_port, ":");

    if (p) {
	finder_host = string(host_colon_port, p);
	finder_port = atoi(p + 1);
	if (finder_port == 0) {
	    fprintf(stderr, "Invalid port \"%s\"\n", p + 1);
	    return false;
	}
    } else {
	finder_host = string(host_colon_port);
	finder_port = FinderConstants::FINDER_DEFAULT_PORT();
    }

    try {
	finder_addr = IPv4(finder_host.c_str());
    } catch (const InvalidString& ) {
	// host string may need resolving
	in_addr ia;
	if (address_lookup(finder_host, ia) == false) {
	    fprintf(stderr, "Invalid host \"%s\"\n", finder_host.c_str());
	    return false;
	}
	finder_addr.copy_in(ia);
    }
    return true;
}


int
main(int argc, char * const argv[])
{
    int ret_value = 0;
    const char* const argv0 = argv[0];

    //
    // Defaults
    //
    IPv4	finder_addr = FinderConstants::FINDER_DEFAULT_HOST();
    uint16_t	finder_port = 16600;	// over-ridden for external finder
    bool	use_internal_finder = true;
    int		reps = 1;
    uint32_t	burst_cnt = 25;
    uint32_t 	dtablesize;

    dtablesize = getdtablesize();

    //
    // For systems with small default dtable sizes.
    //
    if (burst_cnt > dtablesize / 20) {
	burst_cnt = dtablesize / 20;
	verbose_log("Cropped maximum burst count to %u "
		    "(descriptor table constraint).\n",
		    XORP_UINT_CAST(burst_cnt));
    }

    int ch;
    char* bp = NULL;
    int which = 0;

    while ((ch = getopt(argc, argv, "F:b:r:t:hv")) != -1) {
        switch (ch) {
	case 'F':
	    if (parse_finder_arg(optarg, finder_addr, finder_port) == false) {
		usage(argv[0]);
		return 1;
	    }
	    use_internal_finder = false;
	    break;
	case 'b':
	    burst_cnt = (uint32_t)strtoul(optarg, &bp, 10);
	    if (bp != NULL && *bp != '\0') {
		usage(argv[0]);
		return 1;
	    }
	    break;
	case 'r':
	    reps = atoi(optarg);
	    if (reps < 0) {
		usage(argv[0]);
		return 1;
	    }
	    break;
	case 't':
	    which = atoi(optarg);
	    break;
        case 'v':
            set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            if (ch == 'h')
                return 0;
            else
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    //
    // Initialize and start xlog
    //
    xlog_init(argv0, NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	int iter = 0;
	while (iter < reps) {
	    verbose_log("============================\n");
	    verbose_log("Iteration %d\n", iter);
	    verbose_log("============================\n");
	    ret_value = test_main(finder_addr, finder_port,
				  burst_cnt,
				  use_internal_finder, which);
	    if (ret_value != 0)
		break;
	    iter++;
	}
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        ret_value = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return (ret_value);
}
