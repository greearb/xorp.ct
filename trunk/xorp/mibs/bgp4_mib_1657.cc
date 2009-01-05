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

#ident "$XORP: xorp/mibs/bgp4_mib_1657.cc,v 1.23 2008/10/02 21:57:40 bms Exp $"


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "fixconfigs.h"

#include "bgp4_mib_module.h"
#include "libxorp/xorp.h"

#include "xorpevents.hh"
#include "bgp4_mib_1657.hh"
#include "bgp4_mib_1657_bgpversion.hh"
#include "bgp4_mib_1657_bgplocalas.hh"
#include "bgp4_mib_1657_bgpidentifier.hh"
#include "bgp4_mib_1657_bgppeertable.hh"
#include "bgp4_mib_1657_bgp4pathattrtable.hh"


BgpMib BgpMib::_bgp_mib;

void
init_bgp4_mib_1657 (void)
{

    BgpMib & bgp_mib = BgpMib::the_instance();
    DEBUGMSGTL((bgp_mib.name(), "Initializing...\n"));

    init_bgp4_mib_1657_bgpversion();
    init_bgp4_mib_1657_bgplocalas();
    init_bgp4_mib_1657_bgppeertable();
    init_bgp4_mib_1657_bgpidentifier();
    init_bgp4_mib_1657_bgp4pathattrtable();

    static XorpUnexpectedHandler x(xorp_unexpected_handler);
    // NOTE:  these xlog calls are required by each mib module, since the
    // runtime linker seems to reset the values of xlog.c static variables
    // everytime a new mib module is loaded.  Only the last unloaded mib module
    // (xorp_if_mib_module) should do xlog cleanup.
     
    xlog_init("snmpd", NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW); 
    xlog_add_default_output();
    xlog_start();
}

void  
deinit_bgp4_mib_1657 (void)
{
    BgpMib & bgp_mib = BgpMib::the_instance();
    deinit_bgp4_mib_1657_bgp4pathattrtable();
    DEBUGMSGTL((bgp_mib.name(), "Unloading...\n"));
}



BgpMib& 
BgpMib::the_instance() 
{
    return _bgp_mib;
}

BgpMib::BgpMib() 
    : XrlBgpV0p3Client(&_xrl_router), 
      _xrl_router(SnmpEventLoop::the_instance(),"bgp4_mib"),
      _xrl_target(&_xrl_router, *this), _name(XORP_MODULE_NAME) 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib created\n"));
}

BgpMib::~BgpMib() 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib destroyed\n"));
}






