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

// $XORP: xorp/libxipc/finder_ng.hh,v 1.2 2003/01/28 00:42:24 hodson Exp $

#ifndef __LIBXIPC_FINDER_NG_HH__
#define __LIBXIPC_FINDER_NG_HH__

#include "config.h"

#include <list>
#include <map>

#include "xrl_cmd_map.hh"
#include "finder_messenger.hh"
#include "finder_ng_xrl_queue.hh"

class FinderNGTarget;

class FinderNG : public FinderMessengerManager {
public:
    typedef list<FinderMessengerBase*> FinderMessengerList;
    typedef map<FinderMessengerBase*, FinderNGXrlCommandQueue> OutQueueTable;
    typedef map<string, FinderNGTarget> TargetTable;
    typedef list<string> Resolveables;
    
public:
    FinderNG();
    virtual ~FinderNG();

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

    bool add_target(const string& target_name,
		    const string& class_name,
		    const string& cookie);

    bool active_messenger_represents_target(const string& target_name) const;
    
    bool remove_target(const string& target_name);
    bool remove_target_with_cookie(const string& cookie);
    
    bool add_resolution(const string& target,
			const string& key,
			const string& value);

    bool remove_resolutions(const string& target,
			    const string& key);

    const Resolveables* resolve(const string& target, const string& key);

    size_t messengers() const;
    
protected:
    void announce_departure(const string& target);
    void announce_departure(const string& target, const string& key);
    
protected:
    XrlCmdMap		 _cmds;
    FinderMessengerBase* _active_messenger;
    FinderMessengerList	 _messengers;
    TargetTable		 _targets;
    OutQueueTable	 _out_queues;
};

#endif // __LIBXIPC_FINDER_NG_HH__
