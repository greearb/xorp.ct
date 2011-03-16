// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#endif

#include "libcomm/comm_api.h"

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/data_plane/control_socket/windows_rras_support.hh"

#include "fibconfig_forwarding_windows.hh"

#ifdef __MINGW32__
#define MIB_IP_FORWARDING	1
#define MIB_IP_NOT_FORWARDING	2
#endif

//
// Configure unicast forwarding.
//
// The mechanism to get/set the information is Windows.
//

#ifdef HOST_OS_WINDOWS

FibConfigForwardingWindows::FibConfigForwardingWindows(
    FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigForwarding(fea_data_plane_manager),
      _event(NULL),
      _enablecnt(0)
{
    memset(&_overlapped, 0, sizeof(_overlapped));
}

FibConfigForwardingWindows::~FibConfigForwardingWindows()
{
}

int
FibConfigForwardingWindows::start(string& error_msg)
{
    int ret_value;

    if (_event != NULL)
	return (XORP_OK);		// Already started

    _event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (_event == NULL)
	XLOG_FATAL("Could not create Win32 event object.");
    memset(&_overlapped, 0, sizeof(_overlapped));
    _overlapped.hEvent = _event;
    _enablecnt = 0;

    ret_value = FibConfigForwarding::start(error_msg);
    if (ret_value != XORP_OK) {
	string dummy_error_msg;
	stop(dummy_error_msg);
    }

    return (ret_value);
}

int
FibConfigForwardingWindows::stop(string& error_msg)
{
    if (_event == NULL)
	return (XORP_OK);		// Not running

    if (_enablecnt > 0) {
	XLOG_WARNING("EnableRouter() without %d matching "
		     "UnenableRouter() calls.", _enablecnt);
    }
    CloseHandle(_event);
    _event = NULL;

    return (FibConfigForwarding::stop(error_msg));
}

int
FibConfigForwardingWindows::unicast_forwarding_enabled4(bool& ret_value,
							string& error_msg) const
{
    int enabled = 0;
    MIB_IPSTATS ipstats;
    DWORD error;

    if (! fea_data_plane_manager().have_ipv4()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv4 unicast forwarding "
			     "is enabled: IPv4 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Read the value
    //
    error = GetIpStatistics(&ipstats);
    if (error != NO_ERROR) {
	error_msg = c_format("GetIpStatistics() failed: %s",
			     win_strerror(GetLastError()));
	return (XORP_ERROR);
    }
    enabled = (int)(ipstats.dwForwarding == MIB_IP_FORWARDING);

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingWindows::unicast_forwarding_enabled6(bool& ret_value,
							string& error_msg) const
{
    int enabled = 0;
    MIB_IPSTATS ipstats;
    DWORD error;

    if (! fea_data_plane_manager().have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			     "is enabled: IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    UNUSED(ipstats);
    UNUSED(error);
#if 0
    // XXX: Not in MinGW w32api yet.
    error = GetIpStatisticsEx(&ipstats, AF_INET6);
    if (error != NO_ERROR) {
	error_msg = c_format("GetIpStatisticsEx() failed: %s",
			     win_strerror(GetLastError()));
	return (XORP_ERROR);
    }
    enabled = (int)(ipstats.dwForwarding == MIB_IP_FORWARDING);
#endif // 0

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingWindows::accept_rtadv_enabled6(bool& ret_value,
						  string& error_msg) const
{
    int enabled = 0;

    if (! fea_data_plane_manager().have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			     "Router Advertisement messages is enabled: "
			     "IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

#ifdef HAVE_IPV6    
// TODO:  Implement this (Windows)
#warning "Don't know how to test whether the acceptance of IPv6 "
#warning "Router Advertisement messages is enabled"
#endif // HAVE_IPV6
    
    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingWindows::set_unicast_forwarding_enabled4(bool v,
							    string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value;
    
    if (! fea_data_plane_manager().have_ipv4()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}
	error_msg = c_format("Cannot set IPv4 unicast forwarding to %s: "
			     "IPv4 is not supported", bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Get the old value
    //
    if (unicast_forwarding_enabled4(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);
    if (old_value == v)
	return (XORP_OK);	// Nothing changed

    //
    // Set the value
    //
    if (enable) {
	if (WinSupport::is_rras_running()) {
	    XLOG_WARNING("RRAS is running; ignoring request to enable "
			 "IPv4 forwarding.");
	    return (XORP_OK);
	}
	HANDLE hFwd;
	DWORD result = EnableRouter(&hFwd, &_overlapped);
	if (result != ERROR_IO_PENDING) {
	    error_msg = c_format("Error '%s' from EnableRouter",
				 win_strerror(GetLastError()));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}
	++_enablecnt;
    } else {
	if (WinSupport::is_rras_running()) {
	    XLOG_WARNING("RRAS is running; ignoring request to disable "
			 "IPv4 forwarding.");
	    return (XORP_OK);
	}
	if (_enablecnt == 0) {
	    error_msg = c_format("UnenableRouter() called without any previous "
				 "call to EnableRouter()");
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}

	DWORD result = UnenableRouter(&_overlapped, NULL);
	if (result != NO_ERROR) {
	    error_msg = c_format("Error '%s' from UnenableRouter",
				 win_strerror(GetLastError()));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}
	--_enablecnt;
    }
    
    return (XORP_OK);
}

int
FibConfigForwardingWindows::set_unicast_forwarding_enabled6(bool v,
							    string& error_msg)
{
    bool old_value, old_value_accept_rtadv;
    MIB_IPSTATS ipstats;
    DWORD error;

    if (! fea_data_plane_manager().have_ipv6()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}

	error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: "
			     "IPv6 is not supported", bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Get the old value
    //
    if (unicast_forwarding_enabled6(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);
    if (accept_rtadv_enabled6(old_value_accept_rtadv, error_msg) != XORP_OK)
	return (XORP_ERROR);
    if ((old_value == v) && (old_value_accept_rtadv == !v))
	return (XORP_OK);	// Nothing changed

    //
    // Set the IPv6 Router Advertisement value
    //
    if (set_accept_rtadv_enabled6(!v, error_msg) != XORP_OK)
	return (XORP_ERROR);

    UNUSED(ipstats);
    UNUSED(error);
#if 0
    // XXX: Not yet in MinGW w32api
    error = GetIpStatisticsEx(&ipstats, AF_INET6);
    if (error != NO_ERROR) {
	error_msg = c_format("GetIpStatisticsEx() failed: %s",
			     win_strerror(GetLastError()));
	return (XORP_ERROR);
    }
    ipstats.dwForwarding = (enable != 0) ? 1 : 0;
    ipstats.dwDefaultTTL = MIB_USE_CURRENT_TTL;
    error = SetIpStatisticsEx(&ipstats, AF_INET6);
    if (error != NO_ERROR) {
	error_msg = c_format("SetIpStatisticsEx() failed: %s",
			     win_strerror(GetLastError()));
	return (XORP_ERROR);
    }
#endif // 0
    
    return (XORP_OK);
}

int
FibConfigForwardingWindows::set_accept_rtadv_enabled6(bool v,
						      string& error_msg)
{
    bool old_value;

    if (! fea_data_plane_manager().have_ipv6()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}

	error_msg = c_format("Cannot set the acceptance of IPv6 "
			     "Router Advertisement messages to %s: "
			     "IPv6 is not supported",
			     bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Get the old value
    //
    if (accept_rtadv_enabled6(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);
    if (old_value == v)
	return (XORP_OK);	// Nothing changed

#ifdef HAVE_IPV6
// TODO:  Implement this (Windows)
#warning "Don't know how to enable/disable the acceptance of IPv6 "
#warning "Router Advertisement messages"
#endif // HAVE_IPV6
    
    return (XORP_OK);
}

#endif // HOST_OS_WINDOWS
