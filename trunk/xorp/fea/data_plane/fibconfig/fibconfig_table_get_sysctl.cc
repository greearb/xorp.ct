// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "fea/fibconfig.hh"

#include "fibconfig_table_get_sysctl.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is sysctl(3).
//

#ifdef HAVE_SYSCTL_NET_RT_DUMP

FibConfigTableGetSysctl::FibConfigTableGetSysctl(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableGet(fea_data_plane_manager)
{
}

FibConfigTableGetSysctl::~FibConfigTableGetSysctl()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the sysctl(3) mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetSysctl::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigTableGetSysctl::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigTableGetSysctl::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET, ftex_list) != XORP_OK)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte4());
    }
    
    return (XORP_OK);
}

int
FibConfigTableGetSysctl::get_table6(list<Fte6>& fte_list)
{
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return (XORP_ERROR);
#else
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET6, ftex_list) != XORP_OK)
	return (XORP_ERROR);
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(ftex.get_fte6());
    }
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

int
FibConfigTableGetSysctl::get_table(int family, list<FteX>& fte_list)
{
    int mib[6];

    // Check that the family is supported
    switch(family) {
    case AF_INET:
	if (! fea_data_plane_manager().have_ipv4())
	    return (XORP_ERROR);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (! fea_data_plane_manager().have_ipv6())
	    return (XORP_ERROR);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;				// protocol number - always 0
    mib[3] = family;
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;				// no flags
    
    // Get the table size
    size_t sz;
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &sz, NULL, 0) != 0) {
	XLOG_ERROR("sysctl(NET_RT_DUMP) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // XXX: we need to fetch the data in a loop, because its size
    // may increase between the two sysctl() calls.
    //
    for ( ; ; ) {
	vector<uint8_t> buffer(sz);
	
	// Get the data
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &buffer[0], &sz, NULL, 0)
	    == 0) {
	    // Check the new size
	    if (buffer.size() < sz)
		continue;	// XXX: shouldn't happen, but just in case
	    if (buffer.size() > sz)
		buffer.resize(sz);
	    // Parse the result
	    return (parse_buffer_routing_socket(family,
						fibconfig().system_config_iftree(),
						fte_list, buffer,
						FibMsg::GETS));
	}
	
	// Error
	if (errno == ENOMEM) {
	    // Buffer is too small. Try again.
	    continue;
	}
	XLOG_ERROR("sysctl(NET_RT_DUMP) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
}

#endif // HAVE_SYSCTL_NET_RT_DUMP
