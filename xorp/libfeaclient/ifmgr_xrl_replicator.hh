// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/libfeaclient/ifmgr_xrl_replicator.hh,v 1.15 2008/10/02 21:57:16 bms Exp $

#ifndef __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__
#define __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__

#include "ifmgr_atoms.hh"
#include "ifmgr_cmd_base.hh"
#include "ifmgr_cmd_queue.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/safe_callback_obj.hh"
#include "libxipc/xrl_error.hh"

/**
 * @short Class that sends a configuration information to a remote
 * mirror of IfMgr configuration state.
 *
 * The IfMgrXrlReplicator contains an @ref IfMgrCommandFifoQueue and
 * adds commands to it when @ref IfMgrXrlReplicator::push is called.
 * Invoking push also cranks the queue if an Xrl dispatch is not in
 * progress.  Cranking causes the Xrl at the head of the queue to be
 * dispatched.
 *
 * On the successful dispatch of an Xrl, the next command ready for
 * dispatching as an Xrl is taken from the queue and dispatched if
 * available.  If no command is available, processing stops.  If an
 * Xrl dispatch fails, the overrideable method @ref
 * IfMgrXrlReplicator::xrl_error_event is called.  After an error, the
 * queue processing stops and the IfMgrXrlReplicator instance should in
 * most cases be destructed.
 */
class IfMgrXrlReplicator :
    public IfMgrCommandSinkBase, public CallbackSafeObject  {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

public:
    /**
     * Constructor
     */
    IfMgrXrlReplicator(XrlSender&	sender,
		       const string&	xrl_target_name);

    /**
     * Add a command to be sent as an Xrl to the remote target.
     */
    void push(const Cmd& cmd);

    /**
     * Schedule the next Xrl dispatch.
     */
    void crank_replicator();

    /**
     * Accessor for xrl target name.
     */
    const string& xrl_target_name() const	{ return _tgt; }

    /**
     * Test whether the queue with the commands is empty.
     *
     * @return trie if the queue with the commans is empty, otherwise false.
     */
    bool is_empty_queue() const { return (_queue.empty() == true); }

protected:
    /**
     * Method invoked when it is time to schedule the next Xrl dispatch.
     */
    virtual void crank_manager();

    /**
     * Method invoked when the previous Xrl dispatch has completed.
     */
    virtual void crank_manager_cb();

    /**
     * Method invoked when a command should be added to the manager's queue.
     */
    virtual void push_manager_queue();

    /**
     * Method invoked when an Xrl dispatch fails.
     */
    virtual void xrl_error_event(const XrlError& e);

protected:
    /**
     * Not implemented
     */
    IfMgrXrlReplicator();

    /**
     * Not implemented
     */
    IfMgrXrlReplicator(const IfMgrXrlReplicator&);

    /**
     * Not implemented
     */
    IfMgrXrlReplicator& operator=(const IfMgrXrlReplicator&);

private:
    void xrl_cb(const XrlError& e);

protected:
    XrlSender&		  _s;
    string		  _tgt;

    IfMgrCommandFifoQueue _queue;
    bool		  _pending;
};


class IfMgrXrlReplicationManager;

/**
 * @short An IfMgrXrlReplicator managed by an IfMgrXrlReplicationManager.
 *
 * This class implements the functionality of an IfMgrXrlReplicator,
 * and is used by an IfMgrXrlReplicationManager.  Instances of
 * IfMgrXrlReplicatorManager contain a set of these objects.  When an
 * error occurs with IPC the objects request removal from the manager,
 * which causes their destruction.
 */
class IfMgrManagedXrlReplicator :
    public IfMgrXrlReplicator {
public:
    IfMgrManagedXrlReplicator(IfMgrXrlReplicationManager& manager,
			      XrlSender&		 sender,
			      const string&		 target_name);

protected:
    /**
     * Method invoked when it is time to schedule the next Xrl dispatch.
     */
    void crank_manager();

    /**
     * Method invoked when the previous Xrl dispatch has completed.
     */
    void crank_manager_cb();

    /**
     * Method invoked when a command should be added to the manager's queue.
     */
    void push_manager_queue();

    void xrl_error_event(const XrlError& e);

private:
    IfMgrXrlReplicationManager&	_mgr;
};


class XrlRouter;

/**
 * @short Class that builds and maintains replicator state for
 * multiple remote targets.
 */
class IfMgrXrlReplicationManager : public IfMgrCommandSinkBase {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

public:
    IfMgrXrlReplicationManager(XrlRouter& rtr);

    ~IfMgrXrlReplicationManager();

    /**
     * Add a remote mirror.  The name of the mirror is added to list
     * of known targets and immediately sent a copy of the
     * configuration tree.
     *
     * @param xrl_target_name target to be added.
     * @return true on success, false if target already exists.
     */
    bool add_mirror(const string& xrl_target_name);

    /**
     * Remove remote mirror.
     * @param xrl_target_name target to be removed.
     */
    bool remove_mirror(const string& xrl_target_name);

    /**
     * Apply command to local configuration tree and forward Xrls to
     * targets to replicate state remotely if application to local tree
     * succeeds.
     */
    void push(const Cmd& c);

    /**
     * Method invoked when it is time to schedule the next Xrl dispatch.
     */
    void crank_replicators_queue();

    /**
     * Method invoked when the previous Xrl dispatch has completed.
     */
    void crank_replicators_queue_cb();

    /**
     * Method invoked when a command should be added to the manager's queue.
     */
    void push_manager_queue(IfMgrManagedXrlReplicator* r);

    const IfMgrIfTree& iftree() const		{ return _iftree; }

private:
    typedef list<IfMgrManagedXrlReplicator*> Outputs;

    IfMgrIfTree _iftree;
    XrlRouter&	_rtr;
    Outputs	_outputs;
    Outputs	_replicators_queue;	// Cmd queue with ordered replicators
};

#endif // __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__
