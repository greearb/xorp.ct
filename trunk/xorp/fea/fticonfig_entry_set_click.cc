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

#ident "$XORP$"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


// TODO: XXX: PAVPAVPAV: temporary here
// #define DEBUG_CLICK

//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FtiConfigEntrySetClick::FtiConfigEntrySetClick(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      ClickSocket(ftic.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
#ifdef DEBUG_CLICK      // TODO: XXX: PAVPAVPAV
    register_ftic_secondary();
    ClickSocket::enable_user_click(true);
#endif
}

FtiConfigEntrySetClick::~FtiConfigEntrySetClick()
{
    stop();
}

int
FtiConfigEntrySetClick::start()
{
    if (_is_running)
	return (XORP_OK);

#ifndef DEBUG_CLICK     // TODO: XXX: PAVPAVPAV
    register_ftic_secondary();
#endif

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetClick::stop()
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop();

    _is_running = false;

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetClick::delete_entry4(const Fte4& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetClick::add_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetClick::delete_entry6(const Fte6& fte)
{
    FteX ftex(IPvXNet(fte.net()), IPvX(fte.nexthop()), fte.ifname(),
	      fte.vifname(), fte.metric(), fte.admin_distance(),
	      fte.xorp_route());
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetClick::add_entry(const FteX& fte)
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    UNUSED(fte);

    return (false);
}

bool
FtiConfigEntrySetClick::delete_entry(const FteX& fte)
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    UNUSED(fte);

    return (false);
}
