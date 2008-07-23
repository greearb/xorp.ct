// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_dummy.cc,v 1.12 2008/05/14 06:59:54 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"

#include "ifconfig_set_dummy.hh"
#include "ifconfig_get_dummy.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is Dummy (for testing purpose).
//


IfConfigGetDummy::IfConfigGetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager)
{
}

IfConfigGetDummy::~IfConfigGetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigGetDummy::pull_config(IfTree& iftree)
{
    //
    // XXX: Get the tree from the IfConfigSetDummy instance.
    //
    IfConfigSet* ifconfig_set = fea_data_plane_manager().ifconfig_set();
    if ((ifconfig_set == NULL) || (! ifconfig_set->is_running()))
	return (XORP_ERROR);

    IfConfigSetDummy* ifconfig_set_dummy;
    ifconfig_set_dummy = dynamic_cast<IfConfigSetDummy*>(ifconfig_set);
    if (ifconfig_set_dummy == NULL) {
	//
	// XXX: The IfConfigSet plugin was probably changed to something else
	// which we don't know how to deal with.
	//
	return (XORP_ERROR);
    }

    iftree = ifconfig_set_dummy->iftree();

    return (XORP_OK);
}
