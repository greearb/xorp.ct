// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_entry_set_click.cc,v 1.6 2004/11/12 07:49:47 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FtiConfigEntrySetClick::FtiConfigEntrySetClick(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      ClickSocket(ftic.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FtiConfigEntrySetClick::~FtiConfigEntrySetClick()
{
    stop();
}

int
FtiConfigEntrySetClick::start()
{
    if (_is_running)
	return (XORP_OK);

    if (! ClickSocket::is_enabled())
	return (XORP_ERROR);	// XXX: Not enabled

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    // XXX: add myself as an observer to the NexthopPortMapper
    ftic().nexthop_port_mapper().add_observer(this);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    register_ftic_secondary();

    return (XORP_OK);
}

int
FtiConfigEntrySetClick::stop()
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    // XXX: delete myself as an observer to the NexthopPortMapper
    ftic().nexthop_port_mapper().delete_observer(this);

    ret_value = ClickSocket::stop();

    _is_running = false;

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry4(const Fte4& fte)
{
    bool ret_value;

    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);

	_fte_table4.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::delete_entry4(const Fte4& fte)
{
    bool ret_value;

    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry6(const Fte6& fte)
{
    bool ret_value;

    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);

	_fte_table6.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::delete_entry6(const Fte6& fte)
{
    bool ret_value;

    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry(const FteX& fte)
{
    int port = -1;

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return (false);
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return (false);
	    break;
	}
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = ftic().nexthop_port_mapper();
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

    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to add entry %s", fte.str().c_str());
	return (false);
    }

    //
    // Write the configuration
    //
    string config = c_format("%s %s %d\n",
			     fte.net().str().c_str(),
			     fte.nexthop().str().c_str(),
			     port);
    string handler = "_xorp_rt.add";
    string errmsg;
    if (ClickSocket::write_config(handler, config, errmsg) != XORP_OK) {
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    return (true);
}

bool
FtiConfigEntrySetClick::delete_entry(const FteX& fte)
{
    int port = -1;

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return (false);
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return (false);
	    break;
	}
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = ftic().nexthop_port_mapper();
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

    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to delete entry %s", fte.str().c_str());
	return (false);
    }

    //
    // Write the configuration
    //
    string config = c_format("%s %s %d\n",
			     fte.net().str().c_str(),
			     fte.nexthop().str().c_str(),
			     port);
    string handler = "_xorp_rt.remove";
    string errmsg;
    if (ClickSocket::write_config(handler, config, errmsg) != XORP_OK) {
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    return (true);
}

void
FtiConfigEntrySetClick::nexthop_port_mapper_event()
{
    // TODO: XXX: PAVPAVPAV: implement it!
}
