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

#ident "$XORP: xorp/libxipc/xrl.cc,v 1.4 2003/03/04 23:41:24 hodson Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "xrl.hh"
#include "xrl_tokens.hh"

const string Xrl::_finder_protocol = "finder";

Xrl::Xrl(const char* c_str) throw (InvalidString)
{
    const char *sep, *start;

    if (0 == c_str)
	xorp_throw0(InvalidString);

    // Extract protocol
    start = c_str;
    sep = strstr(start, XrlToken::PROTO_TGT_SEP);
    if (0 == sep) {
	// Not found, assume finder protocol
	_protocol = _finder_protocol;
    } else {
	_protocol = string(start, sep - start);
	start = sep + TOKEN_BYTES(XrlToken::PROTO_TGT_SEP) - 1;
    }

    // Extract Target
    sep = strstr(start, XrlToken::TGT_CMD_SEP);
    if (0 == sep)
	xorp_throw0(InvalidString);
    _target = string(start, sep - start);
    start = sep + TOKEN_BYTES(XrlToken::TGT_CMD_SEP) - 1;

    // Extract Command
    sep = strstr(start, XrlToken::CMD_ARGS_SEP);
    if (sep == 0) {
	_command = string(start);
	if (_command.size() == 0) {
	    xorp_throw0(InvalidString);
	}
	return;
    }
    _command = string(start, sep - start);
    start = sep + TOKEN_BYTES(XrlToken::CMD_ARGS_SEP) - 1;

    // Extract Arguments and pass to XrlArgs string constructor
    if (*start != '\0') {
	try {
	    _args = XrlArgs(start);
	} catch (const InvalidString& is) {
	    debug_msg("Failed to restore xrl args:\n\t\"%s\"", start);
	    throw is;
	}
    }
}

string
Xrl::string_no_args() const
{
    return _protocol + string(XrlToken::PROTO_TGT_SEP) + _target +
	string(XrlToken::TGT_CMD_SEP) + _command;
}

string
Xrl::str() const
{
    string s = string_no_args();
    if (_args.size()) {
	return s + string(XrlToken::CMD_ARGS_SEP) + _args.str();
    }
    return s;
}

bool
Xrl::operator==(const Xrl& x) const
{
    return ((x._protocol == _protocol) && (x._target == _target) &&
	    (x._command == _command) && (x.const_args() == const_args()));
}
