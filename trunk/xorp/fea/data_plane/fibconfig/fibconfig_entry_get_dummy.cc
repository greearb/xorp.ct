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
#include "libxorp/ipvxnet.hh"

#include "fea/fibconfig.hh"

#include "fibconfig_entry_get_dummy.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is Dummy (for testing purpose).
//


FibConfigEntryGetDummy::FibConfigEntryGetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryGet(fea_data_plane_manager)
{
}

FibConfigEntryGetDummy::~FibConfigEntryGetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigEntryGetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntryGetDummy::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    Trie4::iterator ti = fibconfig().trie4().find(dst);
    if (ti != fibconfig().trie4().end()) {
	fte = ti.payload();
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}

int
FibConfigEntryGetDummy::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    Trie4::iterator ti = fibconfig().trie4().find(dst);
    if (ti != fibconfig().trie4().end()) {
	fte = ti.payload();
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}

int
FibConfigEntryGetDummy::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    Trie6::iterator ti = fibconfig().trie6().find(dst);
    if (ti != fibconfig().trie6().end()) {
	fte = ti.payload();
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}

int
FibConfigEntryGetDummy::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{ 
    Trie6::iterator ti = fibconfig().trie6().find(dst);
    if (ti != fibconfig().trie6().end()) {
	fte = ti.payload();
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}
