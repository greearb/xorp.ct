// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008 International Computer Science Institute
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

#include "firewall_entry.hh"

static struct {
    FirewallEntry::Action	action;
    const string		name;
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
