// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/forwarding_plane/fibconfig/fibconfig_entry_observer_routing_socket.cc,v 1.1 2007/04/26 01:23:48 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig.hh"
#include "fibconfig_entry_observer.hh"


//
// Observe single-entry information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would specify the particular entry that
// has changed.
//
// The mechanism to observe the information is routing sockets.
//


FtiConfigEntryObserverRtsock::FtiConfigEntryObserverRtsock(FtiConfig& ftic)
    : FtiConfigEntryObserver(ftic),
      RoutingSocket(ftic.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigEntryObserverRtsock::~FtiConfigEntryObserverRtsock()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing sockets mechanism to observe "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntryObserverRtsock::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntryObserverRtsock::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
FtiConfigEntryObserverRtsock::receive_data(const vector<uint8_t>& buffer)
{
    // TODO: XXX: PAVPAVPAV: use it?
    UNUSED(buffer);
}

void
FtiConfigEntryObserverRtsock::rtsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
