// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_observer_dummy.cc,v 1.2 2003/05/28 21:50:55 pavlin Exp $"

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


IfConfigObserverDummy::IfConfigObserverDummy(IfConfig& ifc)
    : IfConfigObserver(ifc)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ifc();
#endif
}

IfConfigObserverDummy::~IfConfigObserverDummy()
{
    stop();
}

int
IfConfigObserverDummy::start()
{
    // TODO: XXX: PAVPAVPAV: implement it!

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigObserverDummy::stop()
{
    // TODO: XXX: PAVPAVPAV: implement it!

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

void
IfConfigObserverDummy::receive_data(const uint8_t* data, size_t nbytes)
{
    // TODO: use it?
    UNUSED(data);
    UNUSED(nbytes);
}

