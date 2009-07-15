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
#include "fea/fea_data_plane_manager.hh"

#include "fibconfig_entry_set_click.hh"
#include "fibconfig_table_get_click.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is Click:
//   http://www.read.cs.ucla.edu/click/
//


FibConfigTableGetClick::FibConfigTableGetClick(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigTableGet(fea_data_plane_manager),
      ClickSocket(fea_data_plane_manager.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FibConfigTableGetClick::~FibConfigTableGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetClick::start(string& error_msg)
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
FibConfigTableGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

int
FibConfigTableGetClick::get_table4(list<Fte4>& fte_list)
{
    //
    // XXX: Get the table from the FibConfigEntrySetClick instance.
    // The reason for that is because it is practically
    // impossible to read the Click configuration and parse it to restore
    // the original table.
    //
    FibConfigEntrySet* fibconfig_entry_set = fea_data_plane_manager().fibconfig_entry_set();
    if ((fibconfig_entry_set == NULL) || (! fibconfig_entry_set->is_running()))
	return (XORP_ERROR);

    FibConfigEntrySetClick* fibconfig_entry_set_click;
    fibconfig_entry_set_click = dynamic_cast<FibConfigEntrySetClick*>(fibconfig_entry_set);
    if (fibconfig_entry_set_click == NULL) {
	//
	// XXX: The FibConfigEntrySet plugin was probably changed to
	// something else which we don't know how to deal with.
	//
	return (XORP_ERROR);
    }

    const map<IPv4Net, Fte4>& fte_table4 = fibconfig_entry_set_click->fte_table4();
    map<IPv4Net, Fte4>::const_iterator iter;

    for (iter = fte_table4.begin(); iter != fte_table4.end(); ++iter) {
	fte_list.push_back(iter->second);
    }
    
    return (XORP_OK);
}

int
FibConfigTableGetClick::get_table6(list<Fte6>& fte_list)
{
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return (XORP_ERROR);
#else

    //
    // XXX: Get the table from the FibConfigEntrySetClick instance.
    // The reason for that is because it is practically
    // impossible to read the Click configuration and parse it to restore
    // the original table.
    //
    FibConfigEntrySet* fibconfig_entry_set = fea_data_plane_manager().fibconfig_entry_set();
    if ((fibconfig_entry_set == NULL) || (! fibconfig_entry_set->is_running()))
	return (XORP_ERROR);

    FibConfigEntrySetClick* fibconfig_entry_set_click;
    fibconfig_entry_set_click = dynamic_cast<FibConfigEntrySetClick*>(fibconfig_entry_set);
    if (fibconfig_entry_set_click == NULL) {
	//
	// XXX: The FibConfigEntrySet plugin was probably changed to
	// something else which we don't know how to deal with.
	//
	return (XORP_ERROR);
    }

    const map<IPv6Net, Fte6>& fte_table6 = fibconfig_entry_set_click->fte_table6();
    map<IPv6Net, Fte6>::const_iterator iter;

    for (iter = fte_table6.begin(); iter != fte_table6.end(); ++iter) {
	fte_list.push_back(iter->second);
    }
    
    return (XORP_OK);
#endif // HAVE_IPV6
}
