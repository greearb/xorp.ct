// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_forwarding_solaris.cc,v 1.2 2007/12/28 05:12:36 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_INET_ND_H
#include <inet/nd.h>
#endif

#include "fea/fibconfig.hh"

#include "fibconfig_forwarding_solaris.hh"

#define DEV_SOLARIS_DRIVER_FORWARDING_V4 "/dev/ip"
#define DEV_SOLARIS_DRIVER_FORWARDING_V6 "/dev/ip6"
#define DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4 "ip_forwarding"
#define DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6 "ip6_forwarding"
#define DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6 "ip6_ignore_redirect"

//
// Configure unicast forwarding.
//
// The mechanism to get/set the information is Solaris /dev device driver.
//

#ifdef HOST_OS_SOLARIS

FibConfigForwardingSolaris::FibConfigForwardingSolaris(
    FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigForwarding(fea_data_plane_manager)
{
}

FibConfigForwardingSolaris::~FibConfigForwardingSolaris()
{
}

int
FibConfigForwardingSolaris::unicast_forwarding_enabled4(bool& ret_value,
							string& error_msg) const
{
    int enabled = 0;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;

    if (! fea_data_plane_manager().have_ipv4()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv4 unicast forwarding "
			     "is enabled: IPv4 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V4, O_RDONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Read the value from the device
    //
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4, sizeof(buf) - 1);
    strioctl.ic_cmd = ND_GET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Error testing whether IPv4 unicast "
			     "forwarding is enabled: %s",
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (sscanf(buf, "%d", &enabled) != 1) {
	error_msg = c_format("Error reading result %s: %s",
			     buf, strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;

    return (XORP_OK);
}

int
FibConfigForwardingSolaris::unicast_forwarding_enabled6(bool& ret_value,
							string& error_msg) const
{
    int enabled = 0;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;
    
    if (! fea_data_plane_manager().have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			     "is enabled: IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_RDONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Read the value from the device
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6, sizeof(buf) - 1);
    strioctl.ic_cmd = ND_GET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Error testing whether IPv6 unicast "
			     "forwarding is enabled: %s",
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (sscanf(buf, "%d", &enabled) != 1) {
	error_msg = c_format("Error reading result %s: %s",
			     buf, strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingSolaris::accept_rtadv_enabled6(bool& ret_value,
						  string& error_msg) const
{
    int enabled = 0;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;
    int ignore_redirect = 0;

    if (! fea_data_plane_manager().have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			     "Router Advertisement messages is enabled: "
			     "IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_RDONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Read the value from the device
    //
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6,
	    sizeof(buf) - 1);
    strioctl.ic_cmd = ND_GET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Error testing whether the acceptance of "
			     "IPv6 Router Advertisement messages is "
			     "enabled: %s",
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (sscanf(buf, "%d", &ignore_redirect) != 1) {
	error_msg = c_format("Error reading result %s: %s",
			     buf, strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);

    //
    // XXX: The logic of "Accept IPv6 Router Advertisement" is just the
    // opposite of "Ignore Redirect".
    //
    if (ignore_redirect == 0)
	enabled = 1;
    else
	enabled = 0;

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

int
FibConfigForwardingSolaris::set_unicast_forwarding_enabled4(bool v,
							    string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;

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
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V4, O_WRONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for writing: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V4);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Write the value to the device
    //
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "%s %d",
	     DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4, enable);
    strioctl.ic_cmd = ND_SET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Cannot set IPv4 unicast forwarding to %s: %s",
			     bool_c_str(v), strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);

    return (XORP_OK);
}

int
FibConfigForwardingSolaris::set_unicast_forwarding_enabled6(bool v,
							    string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value, old_value_accept_rtadv;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;

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
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_WRONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for writing: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Write the value to the device
    //
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "%s %d",
	     DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6, enable);
    strioctl.ic_cmd = ND_SET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: %s",
			     bool_c_str(v), strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);

    return (XORP_OK);
}

int
FibConfigForwardingSolaris::set_accept_rtadv_enabled6(bool v,
						      string& error_msg)
{
    int enable = (v) ? 1 : 0;
    bool old_value;
    struct strioctl strioctl;
    char buf[256];
    int fd, r;
    int ignore_redirect = 0;

    
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
    
    //
    // XXX: The logic of "Accept IPv6 Router Advertisement" is just the
    // opposite of "Ignore Redirect".
    //
    if (enable == 0)
	ignore_redirect = 1;
    else
	ignore_redirect = 0;

    //
    // Open the device
    //
    fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_WRONLY);
    if (fd < 0) {
	error_msg = c_format("Cannot open file %s for writing: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    r = isastream(fd);
    if (r < 0) {
	error_msg = c_format("Error testing whether file %s is a stream: %s",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6,
			     strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    if (r == 0) {
	error_msg = c_format("File %s is not a stream",
			     DEV_SOLARIS_DRIVER_FORWARDING_V6);
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }

    //
    // Write the value to the device
    //
    memset(&strioctl, 0, sizeof(strioctl));
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "%s %d",
	     DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6,
	     ignore_redirect);
    strioctl.ic_cmd = ND_SET;
    strioctl.ic_timout = 0;
    strioctl.ic_len = sizeof(buf);
    strioctl.ic_dp = buf;
    if (ioctl(fd, I_STR, &strioctl) < 0) {
	error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: %s",
			     bool_c_str(v), strerror(errno));
	XLOG_ERROR("%s", error_msg.c_str());
	close(fd);
	return (XORP_ERROR);
    }
    close(fd);
    
    return (XORP_OK);
}

#endif // HOST_OS_SOLARIS
