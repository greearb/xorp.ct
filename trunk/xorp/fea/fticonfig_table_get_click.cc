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

#ident "$XORP: xorp/fea/fticonfig_table_get_click.cc,v 1.4 2004/12/01 03:28:08 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FtiConfigTableGetClick::FtiConfigTableGetClick(FtiConfig& ftic)
    : FtiConfigTableGet(ftic),
      ClickSocket(ftic.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FtiConfigTableGetClick::~FtiConfigTableGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigTableGetClick::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (! ClickSocket::is_enabled()) {
	error_msg = c_format("Click is not enabled");
	return (XORP_ERROR);	// XXX: Not enabled
    }

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    register_ftic_primary();

    return (XORP_OK);
}

int
FtiConfigTableGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
FtiConfigTableGetClick::get_table4(list<Fte4>& fte_list)
{
    list<FteX> ftex_list;

    // Get the table
    if (get_table(AF_INET, ftex_list) != true)
	return false;
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte4(ftex.net().get_ipv4net(),
				ftex.nexthop().get_ipv4(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance(),
				ftex.xorp_route()));
    }
    
    return true;
}

bool
FtiConfigTableGetClick::get_table6(list<Fte6>& fte_list)
{
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return false;
#else
    list<FteX> ftex_list;
    
    // Get the table
    if (get_table(AF_INET6, ftex_list) != true)
	return false;
    
    // Copy the result back to the original list
    list<FteX>::iterator iter;
    for (iter = ftex_list.begin(); iter != ftex_list.end(); ++iter) {
	FteX& ftex = *iter;
	fte_list.push_back(Fte6(ftex.net().get_ipv6net(),
				ftex.nexthop().get_ipv6(),
				ftex.ifname(), ftex.vifname(),
				ftex.metric(), ftex.admin_distance(),
				ftex.xorp_route()));
    }
    
    return true;
#endif // HAVE_IPV6
}

bool
FtiConfigTableGetClick::get_table(int family, list<FteX>& fte_list)
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    UNUSED(family);
    UNUSED(fte_list);

    return (false);
}
