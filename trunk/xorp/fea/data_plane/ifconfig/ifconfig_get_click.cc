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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_click.cc,v 1.5 2007/06/05 10:30:29 greenhal Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"

#include "ifconfig_get_click.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//

IfConfigGetClick::IfConfigGetClick(IfConfig& ifconfig)
    : IfConfigGet(ifconfig),
      ClickSocket(ifconfig.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

IfConfigGetClick::~IfConfigGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetClick::start(string& error_msg)
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
    ifconfig().register_ifconfig_get_secondary(this);

    return (XORP_OK);
}

int
IfConfigGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
IfConfigGetClick::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

bool
IfConfigGetClick::read_config(IfTree& it)
{
    //
    // XXX: get the tree from the IfconfigSetClick instance.
    // The reason for that is because it is practically
    // impossible to read the Click configuration and parse it to restore
    // the original IfTree state.
    //
    if (! ifconfig().ifconfig_set_click().is_running())
	return (false);

    it = ifconfig().ifconfig_set_click().iftree();
    return (true);
}
