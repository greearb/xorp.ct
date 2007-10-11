// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fea_io.cc,v 1.3 2007/08/15 23:09:51 pavlin Exp $"


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
