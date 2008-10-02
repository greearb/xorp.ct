// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libfeaclient/ifmgr_xrl_replicator.cc,v 1.17 2008/07/23 05:10:38 pavlin Exp $"

#include "libfeaclient_module.h"

#include "libxorp/xorp.h"
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
	push_manager_queue();
	crank_manager();
    } else {
	_queue.push(cmd);
	push_manager_queue();
    }
}

void
IfMgrXrlReplicator::crank_replicator()
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
	crank_manager_cb();
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

//
// XXX: note that this method may be overwritten by
// IfMgrManagedXrlReplicator::crank_manager()
//
void
IfMgrXrlReplicator::crank_manager()
{
    crank_replicator();
}

//
// XXX: note that this method may be overwritten by
// IfMgrManagedXrlReplicator::crank_manager_cb()
//
void
IfMgrXrlReplicator::crank_manager_cb()
{
    crank_replicator();
}

//
// XXX: note that this method may be overwritten by
// IfMgrManagedXrlReplicator::push_manager_queue()
//
void
IfMgrXrlReplicator::push_manager_queue()
{
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
    : IfMgrXrlReplicator(s, n), _mgr(m)
{
}

void
IfMgrManagedXrlReplicator::crank_manager()
{
    _mgr.crank_replicators_queue();
}

void
IfMgrManagedXrlReplicator::crank_manager_cb()
{
    _mgr.crank_replicators_queue_cb();
}


void
IfMgrManagedXrlReplicator::push_manager_queue()
{
    _mgr.push_manager_queue(this);
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
    Outputs::iterator i;

    // Remove all pending commands for this target
    for (i = _replicators_queue.begin(); i != _replicators_queue.end(); ) {
	IfMgrManagedXrlReplicator* r = *i;
	Outputs::iterator i2 = i;
	++i;
	if (r->xrl_target_name() == target_name)
	    _replicators_queue.erase(i2);
    }

    // Remove the target itself
    for (i = _outputs.begin(); i != _outputs.end(); ++i) {
	if ((*i)->xrl_target_name() == target_name) {
	    delete *i;
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
	XLOG_ERROR("Apply bad command. %s", cmd->str().c_str());
	return;
    }

    for (Outputs::iterator i = _outputs.begin(); _outputs.end() != i; ++i) {
	(*i)->push(cmd);
    }
}

void
IfMgrXrlReplicationManager::crank_replicators_queue()
{
    do {
	if (_replicators_queue.empty())
	    return;
	IfMgrManagedXrlReplicator* r = _replicators_queue.front();

	if (r->is_empty_queue()) {
	    _replicators_queue.pop_front();
	    continue;
	}

	//
	// XXX: we ignore the check the replicator is pending, because
	// this case should be considered by the crank_replicator() method.
	//
	r->crank_replicator();
	return;
    } while (true);
}

void
IfMgrXrlReplicationManager::crank_replicators_queue_cb()
{
    XLOG_ASSERT(_replicators_queue.empty() == false);

    _replicators_queue.pop_front();

    crank_replicators_queue();
}

void
IfMgrXrlReplicationManager::push_manager_queue(IfMgrManagedXrlReplicator* r)
{
    //
    // This is a centralized queue with the ordered replicators.
    // For each command that is to be send the replicator is listed
    // in this queue.
    // We need this centralized queue mechanish to ensure that the targets
    // receive the commands in the order they were registered.
    // Otherwise, there could be a race condition if some of the targets
    // try to communicate with each other immediately after they receive
    // the updates.
    //
    _replicators_queue.push_back(r);
}
