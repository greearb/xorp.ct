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

#ident "$XORP: xorp/libfeaclient/ifmgr_xrl_replicator.cc,v 1.2 2003/09/10 19:21:33 hodson Exp $"

#include "config.h"

#include <string>

#include "libfeaclient_module.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxipc/xrl_router.hh"
#include "ifmgr_xrl_replicator.hh"

IfMgrXrlReplicator::IfMgrXrlReplicator(XrlSender&	sender,
				       const string&	xrl_target_name)
    : _s(sender), _tgt(xrl_target_name), _pending(false)
{
}

void
IfMgrXrlReplicator::push(const Cmd& cmd)
{
    if (_queue.empty()) {
	XLOG_ASSERT(_pending == false);
	_queue.push(cmd);
	crank();
    } else {
	XLOG_ASSERT(_pending == true);
	_queue.push(cmd);
    }
}

void
IfMgrXrlReplicator::crank()
{
    if (_pending)
	return;

    if (_queue.empty())
	return;

    _pending = true;

    Cmd c = _queue.front();
    if (c->forward(_s, _tgt, callback(this, &IfMgrXrlReplicator::xrl_cb))
	== false) {
	// XXX todo
	XLOG_FATAL("Send failed.");
    }
}

void
IfMgrXrlReplicator::xrl_cb(const XrlError& err)
{
    XLOG_ASSERT(_queue.empty() == false);

    _pending = false;
    Cmd c = _queue.front();
    _queue.pop_front();

    if (err == XrlError::OKAY()) {
	crank();
	return;
    }

    if (err == XrlError::COMMAND_FAILED()) {
	//
	// If command failed then we're out of sync with remote tree
	// this means we have a bug.
	//
	XLOG_FATAL("Remote and local trees out of sync.  Programming bug.");
    }
    xrl_error_event(err);
}

void
IfMgrXrlReplicator::xrl_error_event(const XrlError& err)
{
    XLOG_ERROR("%s", err.str().c_str());
}



IfMgrManagedXrlReplicator::IfMgrManagedXrlReplicator
(
 IfMgrXrlReplicationManager&	m,
 XrlSender&			s,
 const string&			n
 )
    : IfMgrXrlReplicator(s,n), _mgr(m)
{
}

void
IfMgrManagedXrlReplicator::xrl_error_event(const XrlError& /* e */)
{
    XLOG_INFO("An error occurred sending an Xrl to \"%s\".  Target is being "
	      "removed from list of interface update receivers.",
	      xrl_target_name().c_str());
    _mgr.remove_mirror(xrl_target_name());
}



IfMgrXrlReplicationManager::IfMgrXrlReplicationManager(XrlRouter& r)
    : _rtr(r)
{
}

IfMgrXrlReplicationManager::~IfMgrXrlReplicationManager()
{
    while (_outputs.empty() == false) {
	delete _outputs.front();
	_outputs.pop_front();
    }
}

bool
IfMgrXrlReplicationManager::add_mirror(const string& target_name)
{
    Outputs::const_iterator ci = _outputs.begin();
    while (ci != _outputs.end()) {
	if ((*ci)->xrl_target_name() == target_name)
	    return false;
	++ci;
    }
    _outputs.push_back(new
		       IfMgrManagedXrlReplicator(*this, _rtr, target_name));

    IfMgrIfTreeToCommands config_commands(_iftree);
    config_commands.convert(*_outputs.back());
    return true;
}

bool
IfMgrXrlReplicationManager::remove_mirror(const string& target_name)
{
    for (Outputs::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
	if ((*i)->xrl_target_name() == target_name) {
	    _outputs.erase(i);
	    return true;
	}
    }
    return false;
}

void
IfMgrXrlReplicationManager::push(const Cmd& cmd)
{
    if (cmd->execute(_iftree) == false) {
	XLOG_ERROR("Apply bad command.");
	return;
    }

    for (Outputs::iterator i = _outputs.begin(); _outputs.end() != i; ++i) {
	(*i)->push(cmd);
    }
}
