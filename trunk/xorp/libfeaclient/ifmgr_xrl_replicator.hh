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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__
#define __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__

#include "ifmgr_cmd_base.hh"
#include "ifmgr_cmd_queue.hh"

#include "libxorp/eventloop.hh"
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
class IfMgrXrlReplicator : public IfMgrCommandSinkBase {
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
     * Accessor for xrl target name.
     */
    inline const string& xrl_target_name() const	{ return _tgt; }

protected:
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
    void crank();
    void xrl_cb(const XrlError& e);

protected:
    XrlSender&		  _s;
    string		  _tgt;

    IfMgrCommandFifoQueue _queue;
    bool		  _pending;
};

#endif // __LIBFEACLIENT_IFMGR_XRL_REPLICATOR_HH__
