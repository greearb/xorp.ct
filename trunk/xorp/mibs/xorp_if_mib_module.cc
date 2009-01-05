// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/mibs/xorp_if_mib_module.cc,v 1.15 2008/10/02 21:57:41 bms Exp $"


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "fixconfigs.h"

#include "xorp_if_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "xorpevents.hh"
#include "xorp_if_mib_module.hh"


void
init_xorp_if_mib_module(void)
{
    XorpIfMib & xorp_if_mib = XorpIfMib::the_instance();
    DEBUGMSGTL((xorp_if_mib.name(), "Initialized...\n"));

    xlog_init("snmpd", NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW); 
    xlog_add_default_output();
    xlog_start();
}


void
deinit_xorp_if_mib_module(void)
{
    DEBUGMSGTL((XorpIfMib::the_instance().name(), "Unloaded...\n"));

    // since this is the last XORP mib module that will be unloaded, it must do
    // the clean up 
    xlog_stop();
    xlog_exit();
}


XorpIfMib XorpIfMib::_xorp_if_mib;


XorpIfMib&
XorpIfMib::the_instance()
{
    return _xorp_if_mib;
}

XorpIfMib::XorpIfMib()
    : _xrl_router(SnmpEventLoop::the_instance(),"xorp_if_mib"),
      _xrl_target(&_xrl_router, *this) 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "XorpIfMib created\n"));
}

XorpIfMib::~XorpIfMib()
{
    DEBUGMSGTL((XORP_MODULE_NAME, "XorpIfMib destroyed\n"));
    while(_xrl_router.pending()) {
	SnmpEventLoop::the_instance().run();
	DEBUGMSGTL((XORP_MODULE_NAME, "flushing _xrl_router "
	    "operations...\n"));
    }
}
