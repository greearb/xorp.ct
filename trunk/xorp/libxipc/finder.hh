// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/finder.hh,v 1.25 2008/07/23 05:10:41 pavlin Exp $

#ifndef __LIBXIPC_FINDER_HH__
#define __LIBXIPC_FINDER_HH__

#include "libxorp/xorp.h"

#include <list>
#include <map>
#include <set>

#include "xrl_cmd_map.hh"
#include "finder_messenger.hh"
#include "finder_xrl_queue.hh"


class FinderTarget;
class FinderClass;
class FinderEvent;

class Finder : public FinderMessengerManager
{
public:
    typedef list<FinderMessengerBase*> FinderMessengerList;
    typedef map<FinderMessengerBase*, FinderXrlCommandQueue> OutQueueTable;
    typedef map<string, FinderTarget> TargetTable;
    typedef map<string, FinderClass> ClassTable;
    typedef list<string> Resolveables;
    typedef list<FinderEvent> EventQueue;

public:
    Finder(EventLoop& e);
    virtual ~Finder();

protected:
    /* Methods for FinderMessengerManager interface */
    void messenger_active_event(FinderMessengerBase*);
    void messenger_inactive_event(FinderMessengerBase*);
    void messenger_stopped_event(FinderMessengerBase*);
    void messenger_birth_event(FinderMessengerBase*);
    void messenger_death_event(FinderMessengerBase*);
    bool manages(const FinderMessengerBase*) const;

public:
    XrlCmdMap& commands();

    bool add_target(const string& class_name,
		    const string& instance_name,
		    bool 	  singleton,
		    const string& cookie);

    bool active_messenger_represents_target(const string& target_name) const;

    bool remove_target(const string& target_name);

    bool remove_target_with_cookie(const string& cookie);

    bool set_target_enabled(const string& target_name, bool en);

    bool target_enabled(const string& target_name, bool& is_enabled) const;

    bool add_resolution(const string& target,
			const string& key,
			const string& value);

    bool remove_resolutions(const string& target,
			    const string& key);

    bool add_class_watch(const string& target,
			 const string& class_to_watch);

    bool remove_class_watch(const string& target,
			    const string& class_to_watch);

    bool add_instance_watch(const string& target,
			    const string& instance_to_watch);

    bool remove_instance_watch(const string& target,
			       const string& instance_to_watch);

    const string& primary_instance(const string& instance_or_class) const;

    const Resolveables* resolve(const string& target, const string& key);

    size_t messengers() const;

    bool fill_target_list(list<string>& target_list) const;
    bool fill_targets_xrl_list(const string& target,
			       list<string>& xrl_list) const;

protected:
    /**
     * Buffer event of instance becoming externally visible.
     */
    void log_arrival_event(const string& class_name,
			   const string& instance_name);

    /**
     * Buffer event of instance ceasing to be externally visible.
     */
    void log_departure_event(const string& class_name,
			     const string& instance_name);

    void announce_xrl_departure(const string& target, const string& key);

    void announce_events_externally();

    void announce_class_instances(const string& recv_instance_name,
				  const string& class_name);

    void announce_new_instance(const string& recv_instance_name,
			       FinderXrlCommandQueue& out_queue,
			       const string& class_name,
			       const string& instance_name);

    bool hello_timer_running() const { return _hello.scheduled(); }
    void start_hello_timer();
    bool send_hello();

    void remove_target(TargetTable::iterator& i);

    bool add_class_instance(const string& class_name,
			    const string& instance,
			    bool	  singleton);

    bool remove_class_instance(const string& class_name,
			       const string& instance);

    bool class_default_instance(const string& class_name,
				string&	      instance) const;

    bool class_exists(const string& class_name) const;

    EventLoop& eventloop() const { return _e; }

    Finder(const Finder&);		// Not implemented
    Finder& operator=(const Finder&);	// Not implemented

protected:
    EventLoop&		 _e;
    XrlCmdMap		 _cmds;
    FinderMessengerBase* _active_messenger;	// Currently active endpoint
    FinderMessengerList	 _messengers;		// List of Finder
    						// communication endpoints
    TargetTable		 _targets;		// Table of target instances
    ClassTable		 _classes;		// Table of known classes
    OutQueueTable	 _out_queues;		// Outbound message queues
    EventQueue		 _event_queue;		// Queue for externally
    						// advertised events
    XorpTimer		 _hello;		// Timer used to send
    						// keepalive messages to
    						// FinderMessenger instances
};

#endif // __LIBXIPC_FINDER_HH__
