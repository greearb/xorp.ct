// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_observer_rtmv2.cc,v 1.17 2008/07/23 05:10:22 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"

#include "fea/fibconfig_table_get.hh"
#include "fea/fibconfig_table_observer.hh"
#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif

#include "fibconfig_table_get_sysctl.hh"
#include "fibconfig_table_observer_rtmv2.hh"


//
// Observe whole-table information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would NOT specify the particular entry that
// has changed.
//
// The mechanism to observe the information is Router Manager V2.
//

#ifdef HOST_OS_WINDOWS

FibConfigTableObserverRtmV2::FibConfigTableObserverRtmV2(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableObserver(fea_data_plane_manager),
      _rs4(NULL),
      _rso4(NULL),
      _rs6(NULL),
      _rso6(NULL)
{
    if (!WinSupport::is_rras_running()) {
        XLOG_WARNING("RRAS is not running; disabling FibConfigTableObserverRtmV2.");
        return;
    }

    _rs4 = new WinRtmPipe(fea_data_plane_manager.eventloop());
    _rso4 = new RtmV2Observer(*_rs4, AF_INET, *this);

#ifdef HAVE_IPV6
    _rs6 = new WinRtmPipe(fea_data_plane_manager.eventloop());
    _rso6 = new RtmV2Observer(*_rs6, AF_INET6, *this);
#endif
}

FibConfigTableObserverRtmV2::~FibConfigTableObserverRtmV2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Router Manager V2 mechanism to observe "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
#ifdef HAVE_IPV6
    if (_rso6)
	delete _rso6;
    if (_rs6)
	delete _rs6;
#endif
    if (_rso4)
	delete _rso4;
    if (_rs4)
	delete _rs4;
}

int
FibConfigTableObserverRtmV2::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (_rs4 == NULL || (_rs4->start(AF_INET, error_msg) != XORP_OK))
	return (XORP_ERROR);
#if 0
#ifdef HAVE_IPV6
    if (_rs6 == NULL || (_rs6->start(AF_INET6, error_msg) != XORP_OK))
	return (XORP_ERROR);
#endif
#endif
    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigTableObserverRtmV2::stop(string& error_msg)
{
    int result = XORP_OK;

    if (! _is_running)
	return (result);

    if (_rs4 == NULL || (_rs4->stop(error_msg) != XORP_OK))
	result = XORP_ERROR;
#if 0
#ifdef HAVE_IPV6
    if (rs6 == NULL || (_rs6->stop(error_msg) != XORP_OK))
	result = XORP_ERROR;
#endif
#endif

    _is_running = false;

    return (result);
}

void
FibConfigTableObserverRtmV2::receive_data(const vector<uint8_t>& buffer)
{
    list<FteX> fte_list;
    FibConfigTableGetSysctl::FibMsgSet filter;
    filter = FibConfigTableGetSysctl::FibMsg::UPDATES | FibConfigTableGetSysctl::FibMsg::GETS;

    //
    // Get the IPv4 routes
    //
    if (fea_data_plane_manager().have_ipv4() && _rs4->is_open()) {
	FibConfigTableGetSysctl::parse_buffer_routing_socket(AF_INET,
							     fibconfig().system_config_iftree(),
							     fte_list,
							     buffer,
							     filter);
	if (! fte_list.empty()) {
	    fibconfig().propagate_fib_changes(fte_list, this);
	    fte_list.clear();
	}
    }

#ifdef HAVE_IPV6
    //
    // Get the IPv6 routes
    //
    if (fea_data_plane_manager().have_ipv6() && _rs6->is_open()) {
	FibConfigTableGetSysctl::parse_buffer_routing_socket(AF_INET6,
							     fibconfig().system_config_iftree(),
							     fte_list,
							     buffer,
							     filter);
	if (! fte_list.empty()) {
	    fibconfig().propagate_fib_changes(fte_list, this);
	    fte_list.clear();
	}
    }
#endif // HAVE_IPV6
}

#endif // HOST_OS_WINDOWS
