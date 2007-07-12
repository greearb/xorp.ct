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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_observer_rtmv2.cc,v 1.7 2007/07/11 22:18:07 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_observer.hh"

#include "fibconfig_entry_observer_rtmv2.hh"


//
// Observe single-entry information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would specify the particular entry that
// has changed.
//
// The mechanism to observe the information is Router Manager V2.
//

#ifdef HOST_OS_WINDOWS

FibConfigEntryObserverRtmV2::FibConfigEntryObserverRtmV2(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryObserver(fea_data_plane_manager)
#if 0
      WinRtmPipe(fea_data_plane_manager.eventloop()),
      WinRtmPipeObserver(*(WinRtmPipe *)this)
#endif
{
}

FibConfigEntryObserverRtmV2::~FibConfigEntryObserverRtmV2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Router Manager V2 mechanism to observe "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryObserverRtmV2::start(string& error_msg)
{
#if 0
    if (_is_running)
	return (XORP_OK);

    if (WinRtmPipe::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;
#endif
    return (XORP_OK);
    UNUSED(error_msg);
}

int
FibConfigEntryObserverRtmV2::stop(string& error_msg)
{
#if 0
    if (! _is_running)
	return (XORP_OK);

    if (WinRtmPipe::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;
#endif

    return (XORP_OK);
    UNUSED(error_msg);
}

#if 0
void
FibConfigEntryObserverRtmV2::receive_data(const vector<uint8_t>& buffer)
{
    // TODO: XXX: PAVPAVPAV: use it?
    UNUSED(buffer);
}

void
FibConfigEntryObserverRtmV2::routing_socket_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
#endif // 0

#endif // HOST_OS_WINDOWS
