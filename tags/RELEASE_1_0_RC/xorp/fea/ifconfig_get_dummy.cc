// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_get_dummy.cc,v 1.4 2004/06/02 22:52:39 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about the network interfaces from the underlying system.
//
// The mechanism to obtain the information is dummy (for testing purpose).
//


IfConfigGetDummy::IfConfigGetDummy(IfConfig& ifc)
    : IfConfigGet(ifc)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ifc();
#endif
}

IfConfigGetDummy::~IfConfigGetDummy()
{
    
}

int
IfConfigGetDummy::start()
{
    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetDummy::stop()
{
    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigGetDummy::pull_config(IfTree& iftree)
{
    iftree = ifc().live_config();
    
    return true;
}
