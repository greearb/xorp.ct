// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_click.cc,v 1.9 2007/10/12 07:53:49 pavlin Exp $"

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
