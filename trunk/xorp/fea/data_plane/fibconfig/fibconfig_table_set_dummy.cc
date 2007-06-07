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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_set_dummy.cc,v 1.5 2007/04/30 23:40:32 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_set.hh"

#include "fibconfig_table_set_dummy.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is dummy (for testing purpose).
//


FibConfigTableSetDummy::FibConfigTableSetDummy(FibConfig& fibconfig)
    : FibConfigTableSet(fibconfig)
{
#if 0	// XXX: by default Dummy is never registering by itself
    fibconfig.register_fibconfig_table_set_primary(this);
#endif
}

FibConfigTableSetDummy::~FibConfigTableSetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the dummy mechanism to set "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableSetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigTableSetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
FibConfigTableSetDummy::set_table4(const list<Fte4>& fte_list)
{
    list<Fte4>::const_iterator iter;

    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	fibconfig().add_entry4(fte);
    }
    
    return true;
}

bool
FibConfigTableSetDummy::delete_all_entries4()
{
    if (in_configuration() == false)
	return false;
    
    fibconfig().trie4().delete_all_nodes();
    
    return true;
}

bool
FibConfigTableSetDummy::set_table6(const list<Fte6>& fte_list)
{
    list<Fte6>::const_iterator iter;
    
    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	fibconfig().add_entry6(fte);
    }
    
    return true;
}

bool
FibConfigTableSetDummy::delete_all_entries6()
{
    if (in_configuration() == false)
	return false;
    
    fibconfig().trie6().delete_all_nodes();
    
    return true;
}
