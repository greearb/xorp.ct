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

#ident "$XORP: xorp/fea/fticonfig_entry_set_click.cc,v 1.2 2004/10/27 21:45:42 pavlin Exp $"


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

    register_ftic_secondary();

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    // XXX: add myself as an observer to the NexthopPortMapper
    ftic().nexthop_port_mapper().add_observer(this);

    _is_running = true;

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
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetClick::delete_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetClick::add_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetClick::delete_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetClick::add_entry(const FteX& fte)
{
    string config;
    string errmsg;
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

    config = c_format("_xorp_rt.add %s %s %d\n",
		      fte.net().str().c_str(),
		      fte.nexthop().str().c_str(),
		      port);

    //
    // TODO: XXX: PAVPAVPAV: the code for WRITEDATA below is a copy
    // from IfConfigSetClick::config_end(). Add a new method
    // to ClickSocket that eliminates the need for such replication.
    //
    string write_config = c_format("WRITEDATA hotconfig %u\n",
				   static_cast<uint32_t>(config.size()));
    write_config += config;
    if (ClickSocket::write(write_config.c_str(), write_config.size())
	!= static_cast<ssize_t>(write_config.size())) {
	errmsg = c_format("Error writing to Click socket: %s",
			  strerror(errno));
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    //
    // Check the command status
    //
    bool is_warning, is_error;
    string command_warning, command_error;
    if (ClickSocket::check_command_status(is_warning, command_warning,
					  is_error, command_error,
					  errmsg) != XORP_OK) {
	errmsg = c_format("Error verifying the command status after writing to Click socket: %s",
			  errmsg.c_str());
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    if (is_warning) {
	XLOG_WARNING("Click command warning: %s", command_warning.c_str());
    }
    if (is_error) {
	XLOG_ERROR("Click command error: %s", command_error.c_str());
	return (false);
    }

    return (true);
}

bool
FtiConfigEntrySetClick::delete_entry(const FteX& fte)
{
    string config;
    string errmsg;
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

    config = c_format("_xorp_rt.remove %s %s %d\n",
		      fte.net().str().c_str(),
		      fte.nexthop().str().c_str(),
		      port);

    //
    // TODO: XXX: PAVPAVPAV: the code for WRITEDATA below is a copy
    // from IfConfigSetClick::config_end(). Add a new method
    // to ClickSocket that eliminates the need for such replication.
    //
    string write_config = c_format("WRITEDATA hotconfig %u\n",
				   static_cast<uint32_t>(config.size()));
    write_config += config;
    if (ClickSocket::write(write_config.c_str(), write_config.size())
	!= static_cast<ssize_t>(write_config.size())) {
	errmsg = c_format("Error writing to Click socket: %s",
			  strerror(errno));
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    //
    // Check the command status
    //
    bool is_warning, is_error;
    string command_warning, command_error;
    if (ClickSocket::check_command_status(is_warning, command_warning,
					  is_error, command_error,
					  errmsg) != XORP_OK) {
	errmsg = c_format("Error verifying the command status after writing to Click socket: %s",
			  errmsg.c_str());
	XLOG_ERROR("%s", errmsg.c_str());
	return (false);
    }

    if (is_warning) {
	XLOG_WARNING("Click command warning: %s", command_warning.c_str());
    }
    if (is_error) {
	XLOG_ERROR("Click command error: %s", command_error.c_str());
	return (false);
    }

    return (true);
}

void
FtiConfigEntrySetClick::nexthop_port_mapper_event()
{
    // TODO: XXX: PAVPAVPAV: implement it!
}
