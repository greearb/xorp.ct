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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_get_click.cc,v 1.8 2007/09/15 19:52:44 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_get.hh"
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
