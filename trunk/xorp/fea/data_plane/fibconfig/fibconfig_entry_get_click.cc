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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_click.cc,v 1.12 2008/10/02 21:56:55 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fea/fibconfig.hh"

#include "fibconfig_entry_get_click.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is Click:
//   http://www.read.cs.ucla.edu/click/
//


FibConfigEntryGetClick::FibConfigEntryGetClick(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryGet(fea_data_plane_manager),
      ClickSocket(fea_data_plane_manager.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FibConfigEntryGetClick::~FibConfigEntryGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntryGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

int
FibConfigEntryGetClick::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    list<Fte4> fte_list4;
    bool found = false;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table4(fte_list4) != XORP_OK)
	return (XORP_ERROR);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (! fte4.net().contains(dst))
	    continue;
	if ((! found) || fte.net().contains(fte4.net())) {
	    fte = fte4;
	    found = true;
	}
    }

    if (! found)
	return (XORP_ERROR);
    return (XORP_OK);
}

int
FibConfigEntryGetClick::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    list<Fte4> fte_list4;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the exact match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table4(fte_list4) != XORP_OK)
	return (XORP_ERROR);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (fte4.net() == dst) {
	    fte = fte4;
	    return (XORP_OK);
	}
    }

    return (XORP_ERROR);
}

int
FibConfigEntryGetClick::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    list<Fte6> fte_list6;
    bool found = false;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table6(fte_list6) != XORP_OK)
	return (XORP_ERROR);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (! fte6.net().contains(dst))
	    continue;
	if ((! found) || fte.net().contains(fte6.net())) {
	    fte = fte6;
	    found = true;
	}
    }

    if (! found)
	return (XORP_ERROR);
    return (XORP_OK);
}

int
FibConfigEntryGetClick::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{ 
    list<Fte6> fte_list6;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table6(fte_list6) != XORP_OK)
	return (XORP_ERROR);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (fte6.net() == dst) {
	    fte = fte6;
	    return (XORP_OK);
	}
    }

    return (XORP_ERROR);
}
