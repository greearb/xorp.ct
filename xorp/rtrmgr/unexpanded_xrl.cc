// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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




#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "template_commands.hh"
#include "unexpanded_xrl.hh"


UnexpandedXrl::UnexpandedXrl(const MasterConfigTreeNode& node,
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

#if 0
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
#endif
    _xrl = _action.expand_xrl_variables(_node, errmsg);
    if (_xrl == NULL) {
	debug_msg("Failed to expand XRL variables: %s\n", errmsg.c_str());
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
