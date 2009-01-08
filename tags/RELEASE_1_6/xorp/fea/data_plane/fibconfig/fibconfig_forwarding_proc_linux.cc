// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_forwarding_proc_linux.cc,v 1.6 2008/10/02 21:56:58 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/fibconfig.hh"

#include "fibconfig_forwarding_proc_linux.hh"


const string FibConfigForwardingProcLinux::PROC_LINUX_FORWARDING_FILE_V4 = "/proc/sys/net/ipv4/ip_forward";
const string FibConfigForwardingProcLinux::PROC_LINUX_FORWARDING_FILE_V6 = "/proc/sys/net/ipv6/conf/all/forwarding";

//
// Configure unicast forwarding.
//
// The mechanism to get/set the information is Linux "/proc" file system.
//

#ifdef HAVE_PROC_LINUX

FibConfigForwardingProcLinux::FibConfigForwardingProcLinux(
    FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigForwarding(fea_data_plane_manager)
{
}

FibConfigForwardingProcLinux::~FibConfigForwardingProcLinux()
{
}

int
FibConfigForwardingProcLinux::unicast_forwarding_enabled4(bool& ret_value,
							  string& error_msg) const
{
    int enabled = 0;
    FILE* fh;
    
    if (! fea_data_plane_manager().have_ipv4()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv4 unicast forwarding "
			     "is enabled: IPv4 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Read the value from the corresponding "/proc" file system entry
    //
    fh = fopen(PROC_LINUX_FORWARDING_FILE_V4.c_str(), "r");
    if (fh == NULL) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     PROC_LINUX_FORWARDING_FILE_V4.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (fscanf(fh, "%d", &enabled) != 1) {
	error_msg = c_format("Error reading file %s: %s",
			     PROC_LINUX_FORWARDING_FILE_V4.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	fclose(fh);
	return (XORP_ERROR);
    }
    fclose(fh);

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;

    return (XORP_OK);
}

int
FibConfigForwardingProcLinux::unicast_forwarding_enabled6(bool& ret_value,
							  string& error_msg) const
{
    int enabled = 0;
    FILE* fh;
    
    if (! fea_data_plane_manager().have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			     "is enabled: IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Read the value from the corresponding "/proc" file system entry
    //
    fh = fopen(PROC_LINUX_FORWARDING_FILE_V6.c_str(), "r");
    if (fh == NULL) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     PROC_LINUX_FORWARDING_FILE_V6.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (fscanf(fh, "%d", &enabled) != 1) {
	error_msg = c_format("Error reading file %s: %s",
			     PROC_LINUX_FORWARDING_FILE_V6.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	fclose(fh);
	return (XORP_ERROR);
    }
    fclose(fh);

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingProcLinux::accept_rtadv_enabled6(bool& ret_value,
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

    // XXX: nothing to do in case of Linux

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;

    return (XORP_OK);
}

int
FibConfigForwardingProcLinux::set_unicast_forwarding_enabled4(bool v,
							      string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value;
    FILE* fh;
    
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
    // Write the value to the corresponding "/proc" file system entry
    //
    fh = fopen(PROC_LINUX_FORWARDING_FILE_V4.c_str(), "w");
    if (fh == NULL) {
	error_msg = c_format("Cannot open file %s for writing: %s",
			     PROC_LINUX_FORWARDING_FILE_V4.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (fprintf(fh, "%d", enable) != 1) {
	error_msg = c_format("Error writing %d to file %s: %s",
			     enable,
			     PROC_LINUX_FORWARDING_FILE_V4.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	fclose(fh);
	return (XORP_ERROR);
    }
    fclose(fh);

    return (XORP_OK);
}

int
FibConfigForwardingProcLinux::set_unicast_forwarding_enabled6(bool v,
							      string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value, old_value_accept_rtadv;
    FILE* fh;
    
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

    //
    // Write the value to the corresponding "/proc" file system entry
    //
    fh = fopen(PROC_LINUX_FORWARDING_FILE_V6.c_str(), "w");
    if (fh == NULL) {
	error_msg = c_format("Cannot open file %s for writing: %s",
			     PROC_LINUX_FORWARDING_FILE_V6.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (fprintf(fh, "%d", enable) != 1) {
	error_msg = c_format("Error writing %d to file %s: %s",
			     enable,
			     PROC_LINUX_FORWARDING_FILE_V6.c_str(),
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	// Restore the old accept_rtadv value
	if (old_value_accept_rtadv != !v) {
	    string dummy_error_msg;
	    set_accept_rtadv_enabled6(old_value_accept_rtadv,
				      dummy_error_msg);
	}
	fclose(fh);
	return (XORP_ERROR);
    }
    fclose(fh);

    return (XORP_OK);
}

int
FibConfigForwardingProcLinux::set_accept_rtadv_enabled6(bool v,
							string& error_msg)
{
    int enable = (v) ? 1 : 0;
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
    if (accept_rtadv_enabled6(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);
    if (old_value == v)
	return (XORP_OK);	// Nothing changed
    
    // XXX: nothing to do in case of Linux
    UNUSED(enable);

    return (XORP_OK);
}

#endif // HAVE_PROC_LINUX
