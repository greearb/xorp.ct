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


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "bgp4_mib_module.h"
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
    : XrlBgpV0p2Client(&_xrl_router), 
      _xrl_router(SnmpEventLoop::the_instance(),"bgp4_mib"),
      _xrl_target(&_xrl_router, *this), _name(XORP_MODULE_NAME) 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib created\n"));
}

BgpMib::~BgpMib() 
{
    DEBUGMSGTL((XORP_MODULE_NAME, "BgpMib destroyed\n"));
}






