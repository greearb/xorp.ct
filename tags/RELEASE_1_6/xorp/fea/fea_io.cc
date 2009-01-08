// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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

#ident "$XORP: xorp/fea/fea_io.cc,v 1.7 2008/10/02 21:56:45 bms Exp $"


//
// FEA (Forwarding Engine Abstraction) I/O interface.
//


#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "fea_io.hh"


FeaIo::FeaIo(EventLoop& eventloop)
    : _eventloop(eventloop),
      _is_running(false)
{
}

FeaIo::~FeaIo()
{
    shutdown();
}

int
FeaIo::startup()
{
    _is_running = true;

    return (XORP_OK);
}

int
FeaIo::shutdown()
{
    _is_running = false;

    return (XORP_OK);
}

bool
FeaIo::is_running() const
{
    return (_is_running);
}

int
FeaIo::add_instance_watch(const string& instance_name,
			  InstanceWatcher* instance_watcher,
			  string& error_msg)
{
    list<pair<string, InstanceWatcher *> >::iterator iter;
    bool is_watched = false;

    for (iter = _instance_watchers.begin();
	 iter != _instance_watchers.end();
	 ++iter) {
	const string& name = iter->first;
	InstanceWatcher* watcher = iter->second;

	if (name != instance_name)
	    continue;

	if (watcher == instance_watcher)
	    return (XORP_OK);		// Exact match found

	// The instance is watched by somebody else
	is_watched = true;
    }

    // Add the new watcher
    _instance_watchers.push_back(make_pair(instance_name, instance_watcher));

    if (is_watched)
	return (XORP_OK);	// Somebody else registered for the instance

    if (register_instance_event_interest(instance_name, error_msg)
	!= XORP_OK) {
	_instance_watchers.pop_back();
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FeaIo::delete_instance_watch(const string& instance_name,
			     InstanceWatcher* instance_watcher,
			     string& error_msg)
{
    list<pair<string, InstanceWatcher *> >::iterator iter, delete_iter;
    bool is_watched = false;

    delete_iter = _instance_watchers.end();
    for (iter = _instance_watchers.begin();
	 iter != _instance_watchers.end();
	++iter) {
	const string& name = iter->first;
	InstanceWatcher* watcher = iter->second;

	if (name != instance_name)
	    continue;

	if (watcher == instance_watcher) {
	    delete_iter = iter;		// Exact match found
	    continue;
	}

	// The instance is watched by somebody else
	is_watched = true;
    }

    if (delete_iter == _instance_watchers.end()) {
	// Entry not found
	error_msg = c_format("Instance watcher for %s not found",
			     instance_name.c_str());
	return (XORP_ERROR);
    }

    // Delete the watcher
    _instance_watchers.erase(delete_iter);

    if (is_watched)
	return (XORP_OK);	// Somebody else registered for the instance

    return (deregister_instance_event_interest(instance_name, error_msg));
}

void
FeaIo::instance_birth(const string& instance_name)
{
    list<pair<string, InstanceWatcher *> >::iterator iter;

    for (iter = _instance_watchers.begin();
	 iter != _instance_watchers.end();
	 ) {
	const string& name = iter->first;
	InstanceWatcher* watcher = iter->second;

	//
	// XXX: We need to increment the iterator in advance, in case
	// the watcher that is informed about the change decides to delete
	// the entry the iterator points to.
	//
	++iter;

	if (name == instance_name)
	    watcher->instance_birth(instance_name);
    }
}

void
FeaIo::instance_death(const string& instance_name)
{
    list<pair<string, InstanceWatcher *> >::iterator iter;

    for (iter = _instance_watchers.begin();
	 iter != _instance_watchers.end();
	 ) {
	const string& name = iter->first;
	InstanceWatcher* watcher = iter->second;

	//
	// XXX: We need to increment the iterator in advance, in case
	// the watcher that is informed about the change decides to delete
	// the entry the iterator points to.
	//
	++iter;

	if (name == instance_name)
	    watcher->instance_death(instance_name);
    }
}
