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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_click.cc,v 1.5 2007/04/30 23:40:30 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fea/fibconfig.hh"

#include "fibconfig_entry_get_click.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FibConfigEntryGetClick::FibConfigEntryGetClick(FibConfig& fibconfig)
    : FibConfigEntryGet(fibconfig),
      ClickSocket(fibconfig.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FibConfigEntryGetClick::~FibConfigEntryGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    // XXX: we register ourselves as the primary "get" method, because
    // Click should be the ultimate place to read the route info from.
    // The kernel itself may contain some left-over stuff.
    //
    fibconfig().register_fibconfig_entry_get_primary(this);

    return (XORP_OK);
}

int
FibConfigEntryGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetClick::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    list<Fte4> fte_list4;
    bool found = false;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table4(fte_list4) != true)
	return (false);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (! fte4.net().contains(dst))
	    continue;
	if ((! found) || fte.net().contains(fte4.net())) {
	    fte = fte4;
	    found = true;
	}
    }

    return (found);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetClick::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    list<Fte4> fte_list4;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the exact match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table4(fte_list4) != true)
	return (false);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (fte4.net() == dst) {
	    fte = fte4;
	    return (true);
	}
    }

    return (false);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetClick::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    list<Fte6> fte_list6;
    bool found = false;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table6(fte_list6) != true)
	return (false);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (! fte6.net().contains(dst))
	    continue;
	if ((! found) || fte.net().contains(fte6.net())) {
	    fte = fte6;
	    found = true;
	}
    }

    return (found);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FibConfigEntryGetClick::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{ 
    list<Fte6> fte_list6;

    //
    // XXX: Get the whole table, and then scan it entry-by-entry
    // for the longest-prefix match.
    // TODO: This implementation is very inefficient.
    //
    if (fibconfig().get_table6(fte_list6) != true)
	return (false);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (fte6.net() == dst) {
	    fte = fte6;
	    return (true);
	}
    }

    return (false);
}
