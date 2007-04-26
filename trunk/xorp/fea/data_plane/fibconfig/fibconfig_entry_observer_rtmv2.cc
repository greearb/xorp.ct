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

#ident "$XORP: xorp/fea/fticonfig_entry_observer_rtmv2.cc,v 1.3 2007/02/16 22:45:38 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_observer.hh"


//
// Observe single-entry information change about the unicast forwarding table.
//
// E.g., if the forwarding table has changed, then the information
// received by the observer would specify the particular entry that
// has changed.
//
// The mechanism to observe the information is Router Manager V2.
//


FtiConfigEntryObserverRtmV2::FtiConfigEntryObserverRtmV2(FtiConfig& ftic)
    : FtiConfigEntryObserver(ftic)
#if 0
      WinRtmPipe(ftic.eventloop()),
      WinRtmPipeObserver(*(WinRtmPipe *)this)
#endif
{
#if 0
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();
#endif
#endif
}

FtiConfigEntryObserverRtmV2::~FtiConfigEntryObserverRtmV2()
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
FtiConfigEntryObserverRtmV2::start(string& error_msg)
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
FtiConfigEntryObserverRtmV2::stop(string& error_msg)
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
FtiConfigEntryObserverRtmV2::receive_data(const vector<uint8_t>& buffer)
{
    // TODO: XXX: PAVPAVPAV: use it?
    UNUSED(buffer);
}

void
FtiConfigEntryObserverRtmV2::rtsock_data(const vector<uint8_t>& buffer)
{
    receive_data(buffer);
}
#endif // 0
