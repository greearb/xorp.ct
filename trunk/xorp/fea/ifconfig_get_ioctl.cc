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

#ident "$XORP: xorp/fea/ifconfig_get_ioctl.cc,v 1.9 2004/08/17 02:20:09 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <net/if.h>

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is ioctl(2).
//

IfConfigGetIoctl::IfConfigGetIoctl(IfConfig& ifc)
    : IfConfigGet(ifc)
{
#ifdef HAVE_IOCTL_SIOCGIFCONF
    register_ifc_primary();
#endif
}

IfConfigGetIoctl::~IfConfigGetIoctl()
{
    stop();
}

int
IfConfigGetIoctl::start()
{
    if (_is_running)
	return (XORP_OK);

    if (ifc().have_ipv4()) {
	if (_s4 < 0) {
	    _s4 = socket(AF_INET, SOCK_DGRAM, 0);
	    if (_s4 < 0) {
		XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	    }
	}
    }
    
#ifdef HAVE_IPV6
    if (ifc().have_ipv6()) {
	if (_s6 < 0) {
	    _s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (_s6 < 0) {
		XLOG_FATAL("Could not initialize IPv6 ioctl() socket");
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetIoctl::stop()
{
    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	close(_s4);
	_s4 = -1;
    }
    if (_s6 >= 0) {
	close(_s6);
	_s6 = -1;
    }

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigGetIoctl::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_IOCTL_SIOCGIFCONF
bool
IfConfigGetIoctl::read_config(IfTree& )
{
    return false;
}

#else // HAVE_IOCTL_SIOCGIFCONF

static bool ioctl_read_ifconf(int family, ifconf *ifconf);

bool
IfConfigGetIoctl::read_config(IfTree& it)
{
    struct ifconf ifconf;
    
    //
    // The IPv4 information
    //
    if (ifc().have_ipv4()) {
	if (ioctl_read_ifconf(AF_INET, &ifconf) != true)
	    return false;
	parse_buffer_ifreq(it, AF_INET, (uint8_t *)ifconf.ifc_buf,
			   ifconf.ifc_len);
	delete[] ifconf.ifc_buf;
    }
    
#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    if (ifc().have_ipv6()) {
	if (ioctl_read_ifconf(AF_INET6, &ifconf) != true)
	    return false;
	parse_buffer_ifreq(it, AF_INET6, (uint8_t *)ifconf.ifc_buf,
			   ifconf.ifc_len);
	delete[] ifconf.ifc_buf;
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
		close(s);
		return false;
	    }
	} else {
	    if (ifconf->ifc_len == lastlen)
		break;		// success, len has not changed
	    lastlen = ifconf->ifc_len;
	}
	ifnum += 10;
    }
    
    close(s);
    
    return true;
}

#endif // HAVE_IOCTL_SIOCGIFCONF
