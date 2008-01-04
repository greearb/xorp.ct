// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/pa_entry.cc,v 1.5 2007/02/16 22:45:48 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "pa_entry.hh"

static struct {
	PaAction	 act;
	const char	*str;
} pa_action_table[PaAction(MAX_ACTIONS)] = {
	{ ACTION_ANY, "any" },
	{ ACTION_NONE, "none" },
	{ ACTION_PASS, "pass" },
	{ ACTION_DROP, "drop" }
};

PaAction
PaActionWrapper::convert(const string& act_str)
{
	for (int i = 0; i < PaAction(MAX_ACTIONS); i++) {
		if (act_str.compare(pa_action_table[i].str) == 0) {
			return pa_action_table[i].act;
		}
	}
	return PaAction(ACTION_INVALID);
}

string
PaActionWrapper::convert(const PaAction act)
{
	if (act < 0 || act >= PaAction(MAX_ACTIONS))
		return string("invalid_action");

	return string(pa_action_table[(int)act].str);
}
