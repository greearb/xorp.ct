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

#ident "$XORP: xorp/fea/ifconfig_get_click.cc,v 1.2 2004/10/22 23:07:18 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//

IfConfigGetClick::IfConfigGetClick(IfConfig& ifc)
    : IfConfigGet(ifc),
      ClickSocket(ifc.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

IfConfigGetClick::~IfConfigGetClick()
{
    stop();
}

int
IfConfigGetClick::start()
{
    if (_is_running)
	return (XORP_OK);

    if (! ClickSocket::is_enabled())
	return (XORP_ERROR);	// XXX: Not enabled

    register_ifc_secondary();

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetClick::stop()
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop();

    _is_running = false;

    return (ret_value);
}

bool
IfConfigGetClick::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

bool
IfConfigGetClick::read_config(IfTree& it)
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    UNUSED(it);
    
    //
    // The IPv4 information
    //
    if (ifc().have_ipv4()) {

    }
    
#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    if (ifc().have_ipv6()) {

    }
#endif // HAVE_IPV6
    
    return true;
}
