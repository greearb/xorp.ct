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

#ident "$Header$"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "xorpevents.hh"
#include "libxorp/xlog.h"

static const char * mib_mod_name = "xorp_if_mib_module";

extern "C" {
void   init_xorp_if_mib_module(void);
void deinit_xorp_if_mib_module(void);
}

void
init_xorp_if_mib_module(void)
{
    DEBUGMSGTL((mib_mod_name, "Initialized...\n"));
    SnmpEventLoop& eventloop = SnmpEventLoop::the_instance();
    eventloop.set_log_name(mib_mod_name);  
    xlog_init("snmpd", NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW); 
    xlog_add_default_output();
    xlog_start();
}


void
deinit_xorp_if_mib_module(void)
{
    DEBUGMSGTL((mib_mod_name, "Unloaded...\n"));
    xlog_stop();
    xlog_exit();
}


