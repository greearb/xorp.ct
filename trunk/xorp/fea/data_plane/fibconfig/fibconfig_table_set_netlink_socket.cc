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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_set_netlink_socket.cc,v 1.5 2007/04/30 23:40:32 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_set.hh"

#include "fibconfig_table_set_netlink_socket.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is netlink(7) sockets.
//


FibConfigTableSetNetlink::FibConfigTableSetNetlink(FibConfig& fibconfig)
    : FibConfigTableSet(fibconfig)
{
#ifdef HAVE_NETLINK_SOCKETS
    fibconfig.register_fibconfig_table_set_primary(this);
#endif
}

FibConfigTableSetNetlink::~FibConfigTableSetNetlink()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableSetNetlink::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    // Cleanup any leftover entries from previously run XORP instance
    if (! fibconfig().unicast_forwarding_entries_retain_on_startup4())
	delete_all_entries4();
    if (! fibconfig().unicast_forwarding_entries_retain_on_startup6())
	delete_all_entries6();

    //
    // XXX: This mechanism relies on the FibConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigTableSetNetlink::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    // Delete the XORP entries
    if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown4())
	delete_all_entries4();
    if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown6())
	delete_all_entries6();

    //
    // XXX: This mechanism relies on the FibConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = false;

    return (XORP_OK);
}

bool
FibConfigTableSetNetlink::set_table4(const list<Fte4>& fte_list)
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
FibConfigTableSetNetlink::delete_all_entries4()
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
FibConfigTableSetNetlink::set_table6(const list<Fte6>& fte_list)
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
FibConfigTableSetNetlink::delete_all_entries6()
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
