// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/service.cc,v 1.11 2008/01/04 03:16:39 pavlin Exp $"

#include "libxorp_module.h"
#include "xorp.h"

#include "service.hh"

// ----------------------------------------------------------------------------
// ServiceStatus related

const char*
service_status_name(ServiceStatus s)
{
    switch (s) {
    case SERVICE_READY:		return "Ready";
    case SERVICE_STARTING:	return "Starting";
    case SERVICE_RUNNING:	return "Running";
    case SERVICE_PAUSING:	return "Pausing";
    case SERVICE_PAUSED:	return "Paused";
    case SERVICE_RESUMING:	return "Resuming";
    case SERVICE_SHUTTING_DOWN: return "Shutting down";
    case SERVICE_SHUTDOWN:	return "Shutdown";
    case SERVICE_FAILED:	return "Failed";
    case SERVICE_ALL:		return "All";			// Invalid
    }
    return "Unknown";
}

// ----------------------------------------------------------------------------
// ServiceBase implmentation

ServiceBase::ServiceBase(const string& n)
    : _name(n),
      _status(SERVICE_READY),
      _observer(NULL)
{
}

ServiceBase::~ServiceBase()
{
}

int
ServiceBase::reset()
{
    return (XORP_ERROR);
}

int
ServiceBase::pause()
{
    return (XORP_ERROR);
}

int
ServiceBase::resume()
{
    return (XORP_ERROR);
}


const char*
ServiceBase::status_name() const
{
    return (service_status_name(_status));
}

int
ServiceBase::set_observer(ServiceChangeObserverBase* so)
{
    if (_observer != NULL)
	return (XORP_ERROR);

    _observer = so;
    return (XORP_OK);
}

int
ServiceBase::unset_observer(ServiceChangeObserverBase* so)
{
    if (_observer != so)
	return (XORP_ERROR);

    _observer = NULL;
    return (XORP_OK);
}

void
ServiceBase::set_status(ServiceStatus status, const string& note)
{
    ServiceStatus ost = _status;
    _status = status;

    bool note_changed = (_note != note);
    _note = note;

    if ((_observer != NULL) && (ost != _status || note_changed))
	_observer->status_change(this, ost, _status);
}

void
ServiceBase::set_status(ServiceStatus status)
{
    ServiceStatus ost = _status;
    _status = status;

    _note.erase();

    if ((_observer != NULL) && (ost != _status))
	_observer->status_change(this, ost, _status);
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
    : _child(child),
      _from_mask(from),
      _to_mask(to)
{
}

void
ServiceFilteredChangeObserver::status_change(ServiceBase*	service,
					     ServiceStatus 	old_status,
					     ServiceStatus 	new_status)
{
    if ((old_status & _from_mask) && (new_status & _to_mask))
	_child->status_change(service, old_status, new_status);
}
