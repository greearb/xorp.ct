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

#ident "$XORP: xorp/rtrmgr/unexpanded_xrl.cc,v 1.9 2004/05/28 22:27:59 pavlin Exp $"


#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "template_commands.hh"
#include "unexpanded_xrl.hh"


UnexpandedXrl::UnexpandedXrl(const ConfigTreeNode& node,
			     const XrlAction& action) 
    : _node(node),
      _action(action),
      _xrl(NULL)
{
}

UnexpandedXrl::~UnexpandedXrl()
{
    if (_xrl != NULL)
	delete _xrl;
}

/**
 * Expand expands the variables in the unexpanded XRL, and creates an
 * XRL that we can actually send.  
 */
Xrl*
UnexpandedXrl::expand(string& errmsg) const
{
    string request;

    // Remove the old expanded XRL, because it may be obsolete
    if (_xrl != NULL) {
	delete _xrl;
	_xrl = NULL;
    }

    if (_action.expand_xrl_variables(_node, request, errmsg) != XORP_OK) {
	debug_msg("Failed to expand XRL variables: %s\n", errmsg.c_str());
	return NULL;
    }
    debug_msg("XRL expanded to %s\n", request.c_str());
    try {
	_xrl = new Xrl(request.c_str());
    } catch (const InvalidString& e) {
	debug_msg("Failed to initialize XRL: %s\n", e.why().c_str());
	return NULL;
    }
    return _xrl;
}

/**
 * return_spec returns the return spec of the XRL as a string
 */
string
UnexpandedXrl::return_spec() const
{
    return _action.xrl_return_spec();
}

string
UnexpandedXrl::str() const
{
    return _action.str();
}
