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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "fea_module.h"

#include "libxorp/callback.hh"
#include "libxorp/c_format.hh"
#include "libxorp/xlog.h"

#include "libxipc/xrl_sender.hh"

#include "xrl_socket_cmds.hh"
#include "xrl_socket_server.hh"

#include "xrl/interfaces/socket4_user_xif.hh"

XrlSocketCommandBase::XrlSocketCommandBase(const string& target)
    : _t(target)
{
}

XrlSocketCommandBase::~XrlSocketCommandBase()
{
}

// ----------------------------------------------------------------------------
// Socket4UserSendRecvEvent

bool
Socket4UserSendRecvEvent::execute(XrlSender&			xs,
				  const CommandCallback&	cb)
{
    XrlSocket4UserV0p1Client c(&xs);
    return c.send_recv_event(target(),
			     _sockid, _src_host, _src_port, _data, cb);
}

string
Socket4UserSendRecvEvent::str() const
{
    return c_format("SendRecvEvent(%s, %s, %u, %d bytes)",
		    _sockid.c_str(), _src_host.str().c_str(), _src_port,
		    static_cast<int>(_data.size()));
}

// ----------------------------------------------------------------------------
// Socket4UserSendConnectEvent

//
// ... Socket4UserSendConnectEvent ...
//

// ----------------------------------------------------------------------------
// Socket4UserSendErrorEvent

bool
Socket4UserSendErrorEvent::execute(XrlSender&			xs,
				   const CommandCallback&	cb)
{
    XrlSocket4UserV0p1Client c(&xs);
    return c.send_error_event(target(), _sockid, _error, _fatal, cb);
}

string
Socket4UserSendErrorEvent::str() const
{
    return c_format("SendErrorEvent(%s, \"%s\", fatal = %d)",
		    _sockid.c_str(), _error.c_str(), _fatal);
}

// ----------------------------------------------------------------------------
// Socket4UserSendCloseEvent

bool
Socket4UserSendCloseEvent::execute(XrlSender& xs, const CommandCallback& cb)
{
    XrlSocket4UserV0p1Client c(&xs);
    return c.send_close_event(target(), _sockid, _reason, cb);
}

string
Socket4UserSendCloseEvent::str() const
{
    return c_format("SendCloseEvent(%s, \"%s\")",
		    _sockid.c_str(), _reason.c_str());
}


// ----------------------------------------------------------------------------
// XrlSocketCommandDispatcher

XrlSocketCommandDispatcher::XrlSocketCommandDispatcher(XrlSender& xs)
    : _xs(xs)
{
}

XrlSocketCommandDispatcher::~XrlSocketCommandDispatcher()
{
}

inline bool
XrlSocketCommandDispatcher::dispatch_head()
{
    const Command& cmd = _cmds.front();

    bool r = cmd->execute(_xs,
			  callback(this, &XrlSocketCommandDispatcher::xrl_cb));
    if (r == false) {
	XLOG_ERROR("Command failed discarding: %s", cmd->str().c_str());
	_cmds.pop_front();
    }
    return r;
}

void
XrlSocketCommandDispatcher::enqueue(const Command& cmd)
{
    bool only_cmd = _cmds.empty();
    _cmds.push_back(cmd);
    if (only_cmd == false)
	return;
    dispatch_head();
}

bool
XrlSocketCommandDispatcher::send_next()
{
    if (_cmds.empty())
	return false;
    _cmds.pop_front();
    if (_cmds.empty() == false)
	return dispatch_head();
    return false;
}
