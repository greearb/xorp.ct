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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_ioctl.cc,v 1.10 2007/07/18 01:30:26 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "fea/ifconfig.hh"

#include "ifconfig_get_ioctl.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is ioctl(2).
//

#ifdef HAVE_IOCTL_SIOCGIFCONF

static bool ioctl_read_ifconf(int family, ifconf *ifconf);

IfConfigGetIoctl::IfConfigGetIoctl(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager),
      _s4(-1),
      _s6(-1)
{
}

IfConfigGetIoctl::~IfConfigGetIoctl()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the ioctl(2) mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetIoctl::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (fea_data_plane_manager().have_ipv4()) {
	if (_s4 < 0) {
	    _s4 = socket(AF_INET, SOCK_DGRAM, 0);
	    if (_s4 < 0) {
		error_msg = c_format("Could not initialize IPv4 ioctl() "
				     "socket: %s", strerror(errno));
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
    }
    
#ifdef HAVE_IPV6
    if (fea_data_plane_manager().have_ipv6()) {
	if (_s6 < 0) {
	    _s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (_s6 < 0) {
		error_msg = c_format("Could not initialize IPv6 ioctl() "
				     "socket: %s", strerror(errno));
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetIoctl::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	ret_value4 = comm_close(_s4);
	_s4 = -1;
	if (ret_value4 < 0) {
	    error_msg = c_format("Could not close IPv4 ioctl() socket: %s",
				 strerror(errno));
	}
    }
    if (_s6 >= 0) {
	ret_value6 = comm_close(_s6);
	_s6 = -1;
	if ((ret_value6 < 0) && (ret_value4 >= 0)) {
	    error_msg = c_format("Could not close IPv6 ioctl() socket: %s",
				 strerror(errno));
	}
    }

    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigGetIoctl::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

bool
IfConfigGetIoctl::read_config(IfTree& iftree)
{
    struct ifconf ifconf;
    
    //
    // The IPv4 information
    //
    if (fea_data_plane_manager().have_ipv4()) {
	if (ioctl_read_ifconf(AF_INET, &ifconf) != true)
	    return false;
	vector<uint8_t> buffer(ifconf.ifc_len);
	memcpy(&buffer[0], ifconf.ifc_buf, ifconf.ifc_len);
	delete[] ifconf.ifc_buf;

	parse_buffer_ioctl(ifconfig(), iftree, AF_INET, buffer);
    }
    
#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    if (fea_data_plane_manager().have_ipv6()) {
	if (ioctl_read_ifconf(AF_INET6, &ifconf) != true)
	    return false;
	vector<uint8_t> buffer(ifconf.ifc_len);
	memcpy(&buffer[0], ifconf.ifc_buf, ifconf.ifc_len);
	delete[] ifconf.ifc_buf;

	parse_buffer_ioctl(ifconfig(), iftree, AF_INET6, buffer);
    }
#endif // HAVE_IPV6
    
    return true;
}

static bool
ioctl_read_ifconf(int family, ifconf *ifconf)
{
    int s, ifnum, lastlen;
    
    s = socket(family, SOCK_DGRAM, 0);
    if (s < 0) {
	XLOG_FATAL("Could not initialize ioctl() socket");
    }
    
    ifnum = 32;			// XXX: initial guess
    ifconf->ifc_buf = NULL;
    lastlen = 0;
    // Loop until SIOCGIFCONF success.
    for ( ; ; ) {
	ifconf->ifc_len = ifnum * sizeof(struct ifreq);
	if (ifconf->ifc_buf != NULL)
	    delete[] ifconf->ifc_buf;
	ifconf->ifc_buf = new char[ifconf->ifc_len];
	if (ioctl(s, SIOCGIFCONF, ifconf) < 0) {
	    // Check UNPv1, 2e, pp 435 for an explanation why we need this
	    if ((errno != EINVAL) || (lastlen != 0)) {
		XLOG_ERROR("ioctl(SIOCGIFCONF) failed: %s", strerror(errno));
		delete[] ifconf->ifc_buf;
		comm_close(s);
		return false;
	    }
	} else {
	    if (ifconf->ifc_len == lastlen)
		break;		// success, len has not changed
	    lastlen = ifconf->ifc_len;
	}
	ifnum += 10;
    }
    
    comm_close(s);
    
    return true;
}

#endif // HAVE_IOCTL_SIOCGIFCONF
