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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_get_dummy.cc,v 1.7 2007/07/11 22:18:08 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_get.hh"

#include "fibconfig_table_get_dummy.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is Dummy (for testing purpose).
//


FibConfigTableGetDummy::FibConfigTableGetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableGet(fea_data_plane_manager)
{
}

FibConfigTableGetDummy::~FibConfigTableGetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigTableGetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
FibConfigTableGetDummy::get_table4(list<Fte4>& fte_list)
{
    Trie4::iterator ti;
    for (ti = fibconfig().trie4().begin(); ti != fibconfig().trie4().end(); ++ti) {
	const Fte4& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return true;
}

bool
FibConfigTableGetDummy::get_table6(list<Fte6>& fte_list)
{
    Trie6::iterator ti;
    for (ti = fibconfig().trie6().begin(); ti != fibconfig().trie6().end(); ++ti) {
	const Fte6& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return true;
}
