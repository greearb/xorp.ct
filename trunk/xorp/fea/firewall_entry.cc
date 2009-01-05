// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008-2009 XORP, Inc.
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

#ident "$XORP: xorp/fea/firewall_entry.cc,v 1.4 2008/10/02 21:56:46 bms Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "firewall_entry.hh"

static struct {
    FirewallEntry::Action	action;
    string			name;
} firewall_action_table[FirewallEntry::ACTION_MAX] = {
    { FirewallEntry::ACTION_ANY,	"any" },	// XXX: not used
    { FirewallEntry::ACTION_NONE,	"none" },
    { FirewallEntry::ACTION_PASS,	"pass" },
    { FirewallEntry::ACTION_DROP,	"drop" },
    { FirewallEntry::ACTION_REJECT,	"reject" }
};

string
FirewallEntry::action2str(FirewallEntry::Action action)
{
    if ((action < ACTION_MIN) || (action >= ACTION_MAX))
	return (string("InvalidAction"));

    return (firewall_action_table[action].name);
}

FirewallEntry::Action
FirewallEntry::str2action(const string& name)
{
    size_t i;

    for (i = ACTION_MIN; i < ACTION_MAX; i++) {
	if (firewall_action_table[i].name == name)
	    return (firewall_action_table[i].action);
    }

    return (ACTION_INVALID);
}
