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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_get_dummy.cc,v 1.12 2008/10/02 21:56:58 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"

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

int
FibConfigTableGetDummy::get_table4(list<Fte4>& fte_list)
{
    Trie4::iterator ti;
    for (ti = fibconfig().trie4().begin(); ti != fibconfig().trie4().end(); ++ti) {
	const Fte4& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return (XORP_OK);
}

int
FibConfigTableGetDummy::get_table6(list<Fte6>& fte_list)
{
    Trie6::iterator ti;
    for (ti = fibconfig().trie6().begin(); ti != fibconfig().trie6().end(); ++ti) {
	const Fte6& fte = ti.payload();
	fte_list.push_back(fte);
    }
    
    return (XORP_OK);
}
