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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_click.cc,v 1.12 2008/10/02 21:57:04 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"
#include "fea/fea_data_plane_manager.hh"

#include "ifconfig_set_click.hh"
#include "ifconfig_get_click.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is Click:
//   http://www.read.cs.ucla.edu/click/
//

IfConfigGetClick::IfConfigGetClick(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager),
      ClickSocket(fea_data_plane_manager.eventloop()),
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

    if (ClickSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

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

int
IfConfigGetClick::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

int
IfConfigGetClick::read_config(IfTree& iftree)
{
    //
    // XXX: Get the tree from the IfConfigSetClick instance.
    // The reason for that is because it is practically
    // impossible to read the Click configuration and parse it to restore
    // the original IfTree state.
    //
    IfConfigSet* ifconfig_set = fea_data_plane_manager().ifconfig_set();
    if ((ifconfig_set == NULL) || (! ifconfig_set->is_running()))
	return (XORP_ERROR);

    IfConfigSetClick* ifconfig_set_click;
    ifconfig_set_click = dynamic_cast<IfConfigSetClick*>(ifconfig_set);
    if (ifconfig_set_click == NULL) {
	//
	// XXX: The IfConfigSet plugin was probably changed to something else
	// which we don't know how to deal with.
	//
	return (XORP_ERROR);
    }

    iftree = ifconfig_set_click->iftree();

    return (XORP_OK);
}
