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

#ident "$XORP: xorp/fea/ifconfig_observer_rtsock.cc,v 1.7 2004/08/17 02:20:10 pavlin Exp $"

#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_observer.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information change is routing sockets.
//


IfConfigObserverRtsock::IfConfigObserverRtsock(IfConfig& ifc)
    : IfConfigObserver(ifc),
      RoutingSocket(ifc.eventloop()),
      RoutingSocketObserver(*(RoutingSocket *)this)
{
#ifdef HAVE_ROUTING_SOCKETS
    register_ifc_primary();
#endif
}

IfConfigObserverRtsock::~IfConfigObserverRtsock()
{
    stop();
}

int
IfConfigObserverRtsock::start()
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverRtsock::stop()
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop() < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverRtsock::receive_data(const uint8_t* data, size_t nbytes)
{
    if (ifc().ifc_get_primary().parse_buffer_rtm(ifc().live_config(),
						 data, nbytes)
	!= true)
	return;
    ifc().report_updates(ifc().live_config(), true);
    ifc().live_config().finalize_state();
}

void
IfConfigObserverRtsock::rtsock_data(const uint8_t* data, size_t nbytes)
{
    receive_data(data, nbytes);
}

