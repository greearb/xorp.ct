// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP$"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_forwarding.hh"

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

bool
FibConfigForwardingProcLinux::test_have_ipv4() const
{
    XorpFd s = comm_sock_open(AF_INET, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);

    comm_close(s);

    return (true);
}

bool
FibConfigForwardingProcLinux::test_have_ipv6() const
{
#ifndef HAVE_IPV6
    return (false);
#else
    XorpFd s = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);

    comm_close(s);
    return (true);
#endif // HAVE_IPV6
}

int
FibConfigForwardingProcLinux::unicast_forwarding_enabled4(bool& ret_value,
							  string& error_msg) const
{
    int enabled = 0;
    FILE* fh;
    
    if (! have_ipv4()) {
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
    
    if (! have_ipv6()) {
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
    
    if (! have_ipv6()) {
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
    
    if (! have_ipv4()) {
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
			     PROC_LINUX_FORWARDING_FILE_V4.c_srt(),
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
    
    if (! have_ipv6()) {
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
    
    if (! have_ipv6()) {
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
