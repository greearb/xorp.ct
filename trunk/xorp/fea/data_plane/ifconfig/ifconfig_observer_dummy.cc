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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_observer_dummy.cc,v 1.7 2007/07/11 22:18:14 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"

#include "ifconfig_observer_dummy.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information is Dummy (for testing
// purpose).
//


IfConfigObserverDummy::IfConfigObserverDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigObserver(fea_data_plane_manager)
{
}

IfConfigObserverDummy::~IfConfigObserverDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to observe "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigObserverDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    // TODO: XXX: PAVPAVPAV: implement it!

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    // TODO: XXX: PAVPAVPAV: implement it!

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverDummy::receive_data(const vector<uint8_t>& buffer)
{
    // TODO: use it?
    UNUSED(buffer);
}

