// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP$"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_observer.hh"


//
// Observe information change about network interface configuration from
// the underlying system.
//
// The mechanism to observe the information change is dummy (for testing
// purpose).
//


IfConfigObserverIPHelper::IfConfigObserverIPHelper(IfConfig& ifc)
    : IfConfigObserver(ifc)
{
#ifdef HOST_OS_WINDOWS
    register_ifc_primary();
#endif
}

IfConfigObserverIPHelper::~IfConfigObserverIPHelper()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IP Helper mechanism to observe "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigObserverIPHelper::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    // XXX: Dummy.
    ifc().report_updates(ifc().live_config(), true);
    ifc().live_config().finalize_state();

    return (XORP_OK);
    UNUSED(error_msg);
}

int
IfConfigObserverIPHelper::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
    UNUSED(error_msg);
}

void
IfConfigObserverIPHelper::receive_data(const uint8_t* data, size_t nbytes)
{
    debug_msg("called\n");
    UNUSED(data);
    UNUSED(nbytes);
}
