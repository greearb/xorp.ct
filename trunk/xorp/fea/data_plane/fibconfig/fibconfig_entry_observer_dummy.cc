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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_observer_dummy.cc,v 1.5 2007/04/30 23:40:30 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_observer.hh"

#include "fibconfig_entry_observer_dummy.hh"


//
// Observe single-entry information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would specify the particular entry that
// has changed.
//
// The mechanism to observe the information is dummy (for testing purpose).
//


FibConfigEntryObserverDummy::FibConfigEntryObserverDummy(FibConfig& fibconfig)
    : FibConfigEntryObserver(fibconfig)
{
#if 0	// XXX: by default Dummy is never registering by itself
    fibconfig.register_fibconfig_entry_observer_primary(this);
#endif
}

FibConfigEntryObserverDummy::~FibConfigEntryObserverDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the dummy mechanism to observe "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryObserverDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    // TODO: XXX: PAVPAVPAV: implement it!

    _is_running = true;

    return (XORP_OK);
}
    
int
FibConfigEntryObserverDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    // TODO: XXX: PAVPAVPAV: implement it!

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

void
FibConfigEntryObserverDummy::receive_data(const vector<uint8_t>& buffer)
{
    // TODO: use it?
    UNUSED(buffer);
}

