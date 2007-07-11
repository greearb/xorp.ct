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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_dummy.cc,v 1.6 2007/06/07 01:28:38 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_set.hh"

#include "fibconfig_entry_set_dummy.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is dummy (for testing purpose).
//


FibConfigEntrySetDummy::FibConfigEntrySetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager)
{
}

FibConfigEntrySetDummy::~FibConfigEntrySetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the dummy mechanism to set "
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


bool
FibConfigEntrySetDummy::add_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return false;

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
    
    return true;
}

bool
FibConfigEntrySetDummy::delete_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    Trie4::iterator ti = fibconfig().trie4().find(fte.net());
    if (ti == fibconfig().trie4().end())
	return false;
    fibconfig().trie4().erase(ti);
    
    return true;
}

bool
FibConfigEntrySetDummy::add_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return false;

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
    
    return true;
}

bool
FibConfigEntrySetDummy::delete_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }
    
    Trie6::iterator ti = fibconfig().trie6().find(fte.net());
    if (ti == fibconfig().trie6().end())
	return false;
    fibconfig().trie6().erase(ti);
    
    return true;
}
