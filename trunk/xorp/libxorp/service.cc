// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/service.cc,v 1.2 2004/01/09 23:18:25 hodson Exp $"

#include "config.h"
#include <string>

#include "service.hh"

// ----------------------------------------------------------------------------
// ServiceStatus related

const char*
service_status_name(ServiceStatus s)
{
    switch (s) {
    case READY:		return "Ready";
    case STARTING:	return "Starting";
    case RUNNING:	return "Running";
    case PAUSING:	return "Pausing";
    case PAUSED:	return "Paused";
    case RESUMING:	return "Resuming";
    case SHUTTING_DOWN: return "Shutting down";
    case SHUTDOWN:	return "Shutdown";
    case FAILED:	return "Failed";
    case ALL:		return "All";				// Invalid
    }
    return "Unknown";
}

// ----------------------------------------------------------------------------
// ServiceBase implmentation

ServiceBase::ServiceBase(const string& n)
    : _name(n), _status(READY), _observer(0)
{
}

ServiceBase::~ServiceBase()
{
}

bool
ServiceBase::reset()
{
    return false;
}

bool
ServiceBase::pause()
{
    return false;
}

bool
ServiceBase::resume()
{
    return false;
}


const char*
ServiceBase::status_name() const
{
    return service_status_name(_status);
}

bool
ServiceBase::set_observer(ServiceChangeObserverBase* so)
{
    if (_observer) {
	return false;
    }
    _observer = so;
    return true;
}

bool
ServiceBase::unset_observer(ServiceChangeObserverBase* so)
{
    if (_observer == so) {
	_observer = 0;
	return true;
    }
    return false;
}

void
ServiceBase::set_status(ServiceStatus status, const string& note)
{
    ServiceStatus ost = _status;
    _status = status;

    bool note_changed = (_note != note);
    _note = note;

    if (_observer && (ost != _status || note_changed)) {
	_observer->status_change(this, ost, _status);
    }
}

void
ServiceBase::set_status(ServiceStatus status)
{
    ServiceStatus ost = _status;
    _status = status;

    _note.erase();

    if (_observer && ost != _status) {
	_observer->status_change(this, ost, _status);
    }
}


// ----------------------------------------------------------------------------
// ServiceChangeObserverBase

ServiceChangeObserverBase::~ServiceChangeObserverBase()
{
}

// ----------------------------------------------------------------------------
// ServiceFilteredChangeObserver

ServiceFilteredChangeObserver::ServiceFilteredChangeObserver(
					ServiceChangeObserverBase* child,
					ServiceStatus		   from,
					ServiceStatus		   to
					)
    : _child(child), _from_mask(from), _to_mask(to)
{
}

void
ServiceFilteredChangeObserver::status_change(ServiceBase*	service,
					     ServiceStatus 	old_status,
					     ServiceStatus 	new_status)
{
    if (old_status & _from_mask &&
	new_status & _to_mask) {
	_child->status_change(service, old_status, new_status);
    }
}
