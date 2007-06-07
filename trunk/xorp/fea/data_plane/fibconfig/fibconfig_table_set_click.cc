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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_set_click.cc,v 1.5 2007/04/30 23:40:32 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_set.hh"

#include "fibconfig_table_set_click.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FibConfigTableSetClick::FibConfigTableSetClick(FibConfig& fibconfig)
    : FibConfigTableSet(fibconfig),
      ClickSocket(fibconfig.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FibConfigTableSetClick::~FibConfigTableSetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to set "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableSetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    // Cleanup any leftover entries from previously run XORP instance
    if (! fibconfig().unicast_forwarding_entries_retain_on_startup4())
	delete_all_entries4();
    if (! fibconfig().unicast_forwarding_entries_retain_on_startup6())
	delete_all_entries6();

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    if (ClickSocket::is_duplicate_routes_to_kernel_enabled())
	fibconfig().register_fibconfig_table_set_secondary(this);
    else
	fibconfig().register_fibconfig_table_set_primary(this);

    return (XORP_OK);
}

int
FibConfigTableSetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    // Delete the XORP entries
    if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown4())
	delete_all_entries4();
    if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown6())
	delete_all_entries6();

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
FibConfigTableSetClick::set_table4(const list<Fte4>& fte_list)
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
FibConfigTableSetClick::delete_all_entries4()
{
    list<Fte4> fte_list;
    list<Fte4>::const_iterator iter;
    
    // Get the list of all entries
    fibconfig().get_table4(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	if (fte.xorp_route())
	    fibconfig().delete_entry4(fte);
    }
    
    return true;
}

bool
FibConfigTableSetClick::set_table6(const list<Fte6>& fte_list)
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
FibConfigTableSetClick::delete_all_entries6()
{
    list<Fte6> fte_list;
    list<Fte6>::const_iterator iter;
    
    // Get the list of all entries
    fibconfig().get_table6(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	if (fte.xorp_route())
	    fibconfig().delete_entry6(fte);
    }
    
    return true;
}
