// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_dummy.cc,v 1.12 2008/10/02 21:56:57 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"

#include "fibconfig_entry_set_dummy.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is Dummy (for testing purpose).
//


FibConfigEntrySetDummy::FibConfigEntrySetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager)
{
}

FibConfigEntrySetDummy::~FibConfigEntrySetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntrySetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigEntrySetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}


int
FibConfigEntrySetDummy::add_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return (XORP_ERROR);

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    int rc = fibconfig().trie4().route_count();
    XLOG_ASSERT(rc >= 0);
    
    fibconfig().trie4().insert(fte.net(), fte);
    if (fibconfig().trie4().route_count() == rc) {
	XLOG_WARNING("add_entry4 is overriding old entry for %s (%d %d)",
		     fte.net().str().c_str(), rc, fibconfig().trie4().route_count());
    }
    
    return (XORP_OK);
}

int
FibConfigEntrySetDummy::delete_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return (XORP_ERROR);

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    Trie4::iterator ti = fibconfig().trie4().find(fte.net());
    if (ti == fibconfig().trie4().end())
	return (XORP_ERROR);
    fibconfig().trie4().erase(ti);
    
    return (XORP_OK);
}

int
FibConfigEntrySetDummy::add_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return (XORP_ERROR);

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    int rc = fibconfig().trie6().route_count();
    XLOG_ASSERT(rc >= 0);
    
    fibconfig().trie6().insert(fte.net(), fte);
    if (fibconfig().trie6().route_count() == rc) {
	XLOG_WARNING("add_entry6 is overriding old entry for %s (%d %d)",
		     fte.net().str().c_str(), rc,
		     fibconfig().trie6().route_count());
    }
    
    return (XORP_OK);
}

int
FibConfigEntrySetDummy::delete_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return (XORP_ERROR);

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    Trie6::iterator ti = fibconfig().trie6().find(fte.net());
    if (ti == fibconfig().trie6().end())
	return (XORP_ERROR);
    fibconfig().trie6().erase(ti);
    
    return (XORP_OK);
}
