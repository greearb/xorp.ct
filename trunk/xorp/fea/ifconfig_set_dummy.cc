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

#ident "$XORP: xorp/fea/ifconfig_set_ioctl.cc,v 1.1 2003/05/02 07:50:48 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_set.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is dummy (for testing purpose).
//

IfConfigSetDummy::IfConfigSetDummy(IfConfig& ifc)
    : IfConfigSet(ifc)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ifc();
#endif
}

IfConfigSetDummy::~IfConfigSetDummy()
{
    stop();
}

int
IfConfigSetDummy::start()
{
    return (XORP_OK);
}

int
IfConfigSetDummy::stop()
{
    return (XORP_OK);
}

int
IfConfigSetDummy::push_config(const IfTree& it)
{
    ifc().set_live_config(it);
    
    return XORP_OK;
}
