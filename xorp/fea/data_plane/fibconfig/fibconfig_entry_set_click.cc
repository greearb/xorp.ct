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

#include "fea/fibconfig.hh"

#include "fibconfig_entry_set_click.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is Click:
//   http://www.read.cs.ucla.edu/click/
//


FibConfigEntrySetClick::FibConfigEntrySetClick(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager),
      ClickSocket(fea_data_plane_manager.eventloop()),
      _cs_reader(*(ClickSocket *)this),
      _reinstall_all_entries_time_slice(100000, 20),	// 100ms, test every 20th iter
      _start_reinstalling_fte_table4(false),
      _is_reinstalling_fte_table4(false),
      _start_reinstalling_fte_table6(false),
      _is_reinstalling_fte_table6(false),
      _reinstalling_ipv4net(IPv4::ZERO(), 0),
      _reinstalling_ipv6net(IPv6::ZERO(), 0)
{
}

FibConfigEntrySetClick::~FibConfigEntrySetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntrySetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // XXX: add myself as an observer to the NexthopPortMapper
    fibconfig().nexthop_port_mapper().add_observer(this);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntrySetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    // XXX: delete myself as an observer to the NexthopPortMapper
    fibconfig().nexthop_port_mapper().delete_observer(this);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

int
FibConfigEntrySetClick::add_entry4(const Fte4& fte)
{
    int ret_value;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }

    FteX ftex(fte);
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value == XORP_OK) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);

	_fte_table4.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

int
FibConfigEntrySetClick::delete_entry4(const Fte4& fte)
{
    int ret_value;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }

    FteX ftex(fte);
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value == XORP_OK) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);
    }

    return (ret_value);
}

int
FibConfigEntrySetClick::add_entry6(const Fte6& fte)
{
    int ret_value;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }

    FteX ftex(fte);
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value == XORP_OK) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);

	_fte_table6.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

int
FibConfigEntrySetClick::delete_entry6(const Fte6& fte)
{
    int ret_value;

    if (fte.is_connected_route()) {
	// XXX: accept directly-connected routes
    }

    FteX ftex(fte);
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value == XORP_OK) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);
    }

    return (ret_value);
}

int
FibConfigEntrySetClick::add_entry(const FteX& fte)
{
    int port = -1;
    string element;
    string handler = "add";
    string error_msg;

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    element = "_xorp_rt4";
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    element = "_xorp_rt6";
	    break;
	}
	XLOG_UNREACHABLE();
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = fibconfig().nexthop_port_mapper();
	const IPvX& nexthop = fte.nexthop();
	bool lookup_nexthop_interface_first = true;

	if (fte.is_connected_route()
	    && (IPvXNet(nexthop, nexthop.addr_bitlen()) == fte.net())) {
	    //
	    // XXX: if a directly-connected route, then check whether the
	    // next-hop address is same as the destination address. This
	    // could be the case of either:
	    // (a) the other side of a p2p link
	    // OR
	    // (b) a connected route for a /32 or /128 configured interface
	    // We are interested in discovering case (b) only, but anyway
	    // in both cases we then lookup the MextPortMapper by trying
	    // first the nexthop address.
	    //
	    // Note that we don't want to map all "connected" routes
	    // by the next-hop address, because typically the next-hop
	    // address will be local IP address, and we don't want to
	    // mis-route the traffic for all directly connected destinations
	    // to the local delivery port of the Click lookup element.
	    //
	    lookup_nexthop_interface_first = false;
	}
	if (lookup_nexthop_interface_first) {
	    port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	    if (port >= 0)
		break;
	}
	if (nexthop.is_ipv4()) {
	    port = m.lookup_nexthop_ipv4(nexthop.get_ipv4());
	    if (port >= 0)
		break;
	}
	if (nexthop.is_ipv6()) {
	    port = m.lookup_nexthop_ipv6(nexthop.get_ipv6());
	    if (port >= 0)
		break;
	}
	if (! lookup_nexthop_interface_first) {
	    port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	    if (port >= 0)
		break;
	}
	break;
    } while (false);

    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to add entry %s", fte.str().c_str());
	return (XORP_ERROR);
    }

    //
    // Write the configuration
    //
    string config;
    if (fte.is_connected_route()) {
	config = c_format("%s %d\n",
			  fte.net().str().c_str(),
			  port);
    } else {
	config = c_format("%s %s %d\n",
			  fte.net().str().c_str(),
			  fte.nexthop().str().c_str(),
			  port);
    }

    //
    // XXX: here we always write the same config to both kernel and
    // user-level Click.
    //
    bool has_kernel_config = true;
    bool has_user_config = true;
    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, config,
				  has_user_config, config,
				  error_msg)
	!= XORP_OK) {
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfigEntrySetClick::delete_entry(const FteX& fte)
{
    int port = -1;
    string element;
    string handler = "remove";
    string error_msg;

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    element = "_xorp_rt4";
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    element = "_xorp_rt6";
	    break;
	}
	XLOG_UNREACHABLE();
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = fibconfig().nexthop_port_mapper();
	port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	if (port >= 0)
	    break;
	if (fte.nexthop().is_ipv4()) {
	    port = m.lookup_nexthop_ipv4(fte.nexthop().get_ipv4());
	    if (port >= 0)
		break;
	}
	if (fte.nexthop().is_ipv6()) {
	    port = m.lookup_nexthop_ipv6(fte.nexthop().get_ipv6());
	    if (port >= 0)
		break;
	}
	break;
    } while (false);

#if 0	// TODO: XXX: currently, we don't have the next-hop info, hence
	// we allow to delete an entry without the port number.
    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to delete entry %s", fte.str().c_str());
	return (XORP_ERROR);
    }
#endif // 0

    //
    // Write the configuration
    //
    string config;
    if (port < 0) {
	// XXX: remove all routes for a given prefix
	//
	// TODO: XXX: currently, we don't have the next-hop info,
	// hence we delete all routes for a given prefix. After all,
	// in case of XORP+Click we always install no more than
	// a single route per prefix.
	//
	config = c_format("%s\n",
			  fte.net().str().c_str());
    } else {
	// XXX: remove a specific route
	if (fte.is_connected_route()) {
	    config = c_format("%s %d\n",
			      fte.net().str().c_str(),
			      port);
	} else {
	    config = c_format("%s %s %d\n",
			      fte.net().str().c_str(),
			      fte.nexthop().str().c_str(),
			      port);
	}
    }

    //
    // XXX: here we always write the same config to both kernel and
    // user-level Click.
    //
    bool has_kernel_config = true;
    bool has_user_config = true;
    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, config,
				  has_user_config, config,
				  error_msg)
	!= XORP_OK) {
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
FibConfigEntrySetClick::nexthop_port_mapper_event(bool is_mapping_changed)
{
    UNUSED(is_mapping_changed);

    //
    // XXX: always reinstall all entries, because they were lost
    // when the new Click config was written.
    //
    start_task_reinstall_all_entries();
}

void
FibConfigEntrySetClick::start_task_reinstall_all_entries()
{
    // Initialize the reinstalling from the beginning
    _start_reinstalling_fte_table4 = true;
    _is_reinstalling_fte_table4 = false;
    _start_reinstalling_fte_table6 = true;
    _is_reinstalling_fte_table6 = false;

    run_task_reinstall_all_entries();
}

void
FibConfigEntrySetClick::run_task_reinstall_all_entries()
{
    _reinstall_all_entries_time_slice.reset();

    //
    // Reinstall all IPv4 entries. If the time slice expires, then schedule
    // a timer to continue later.
    //
    if (_start_reinstalling_fte_table4 || _is_reinstalling_fte_table4) {
	if (reinstall_all_entries4() == true) {
	    _reinstall_all_entries_timer = fibconfig().eventloop().new_oneoff_after(
		TimeVal(0, 1),
		callback(this, &FibConfigEntrySetClick::run_task_reinstall_all_entries));
	    return;
	}
    }

    //
    // Reinstall all IPv6 entries. If the time slice expires, then schedule
    // a timer to continue later.
    //
    if (_start_reinstalling_fte_table6 || _is_reinstalling_fte_table6) {
	if (reinstall_all_entries6() == true) {
	    _reinstall_all_entries_timer = fibconfig().eventloop().new_oneoff_after(
		TimeVal(0, 1),
		callback(this, &FibConfigEntrySetClick::run_task_reinstall_all_entries));
	    return;
	}
    }

    return;
}

bool
FibConfigEntrySetClick::reinstall_all_entries4()
{
    map<IPv4Net, Fte4>::const_iterator iter4, iter4_begin, iter4_end;

    // Set the begin and end iterators
    if (_start_reinstalling_fte_table4) {
	iter4_begin = _fte_table4.begin();
    } else if (_is_reinstalling_fte_table4) {
	iter4_begin = _fte_table4.lower_bound(_reinstalling_ipv4net);
    } else {
	return (false);		// XXX: nothing to do
    }
    iter4_end = _fte_table4.end();

    _start_reinstalling_fte_table4 = false;
    _is_reinstalling_fte_table4 = true;

    for (iter4 = iter4_begin; iter4 != iter4_end; ) {
	const FteX& ftex = FteX(iter4->second);
	++iter4;
	add_entry(ftex);
	if (_reinstall_all_entries_time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    if (iter4 != iter4_end) {
		const Fte4& fte4_next = iter4->second;
		_reinstalling_ipv4net = fte4_next.net();
	    } else {
		_is_reinstalling_fte_table4 = false;
	    }
	    return (true);
	}
    }
    _is_reinstalling_fte_table4 = false;

    return (false);
}

bool
FibConfigEntrySetClick::reinstall_all_entries6()
{
    map<IPv6Net, Fte6>::const_iterator iter6, iter6_begin, iter6_end;

    // Set the begin and end iterators
    if (_start_reinstalling_fte_table6) {
	iter6_begin = _fte_table6.begin();
    } else if (_is_reinstalling_fte_table6) {
	iter6_begin = _fte_table6.lower_bound(_reinstalling_ipv6net);
    } else {
	return (false);		// XXX: nothing to do
    }
    iter6_end = _fte_table6.end();

    _start_reinstalling_fte_table6 = false;
    _is_reinstalling_fte_table6 = true;

    for (iter6 = iter6_begin; iter6 != iter6_end; ) {
	const FteX& ftex = FteX(iter6->second);
	++iter6;
	add_entry(ftex);
	if (_reinstall_all_entries_time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    if (iter6 != iter6_end) {
		const Fte6& fte6_next = iter6->second;
		_reinstalling_ipv6net = fte6_next.net();
	    } else {
		_is_reinstalling_fte_table6 = false;
	    }
	    return (true);
	}
    }
    _is_reinstalling_fte_table6 = false;

    return (false);
}
