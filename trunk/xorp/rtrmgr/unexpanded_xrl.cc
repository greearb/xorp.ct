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

#ident "$XORP: xorp/rtrmgr/unexpanded_xrl.cc,v 1.3 2003/03/10 23:21:02 hodson Exp $"

#include "rtrmgr_module.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "unexpanded_xrl.hh"
#include "template_commands.hh"

UnexpandedXrl::UnexpandedXrl(const ConfigTreeNode* node,
			     const XrlAction* action) 
    : _node(node),
      _action(action),
      _xrl(NULL)
{
}

Xrl* UnexpandedXrl::xrl() {
    
    if (_xrl==NULL) {
	string request;
	if (_node)
	    request = _action->expand_xrl_variables(*_node);
	else
	    request = _action->request();
	debug_msg("XRL expanded to %s\n", request.c_str());
	try {
	    _xrl = new Xrl(request.c_str());
	} catch (const InvalidString& e) {
	    debug_msg("Failed to initialize XRL: %s\n", e.why().c_str());
	    return NULL;
	}
    }
    return _xrl;
}

/*
 * xrl_return_spec returns the return spec of the XRL as a string
 */
string UnexpandedXrl::xrl_return_spec() const {
    return _action->xrl_return_spec();
}

UnexpandedXrl::~UnexpandedXrl() {
    if (_xrl != NULL)
	delete _xrl;
}

string UnexpandedXrl::str() const {
    string s = _action->str();
    return s;
}
