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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"


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
