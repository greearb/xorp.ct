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

// $XORP: xorp/libfeaclient/ifmgr_xrl_mirror.hh,v 1.3 2003/09/10 19:21:33 hodson Exp $

#ifndef __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__
#define __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__

#include "config.h"

#include "xrl/targets/fea_ifmgr_mirror_base.hh"
#include "ifmgr_atoms.hh"
#include "ifmgr_cmd_queue.hh"

class XrlStdRouter;
class EventLoop;
class IfMgrCommandBase;
class IfMgrXrlMirrorRouter;
class IfMgrXrlMirrorTarget;

/**
 * @short Base for classes that watch for Finder events from an
 * IfMgrXrlMirrorRouter.
 */
class IfMgrXrlMirrorRouterObserver {
public:
    virtual ~IfMgrXrlMirrorRouterObserver() = 0;
    virtual void finder_disconnect_event() = 0;
    virtual void finder_ready_event() = 0;
};

/**
 * @short Base for classes that are interested in configuration event
 * hint commands.
 */
class IfMgrHintObserver {
public:
    virtual ~IfMgrHintObserver() = 0;
    virtual void tree_complete() = 0;
    virtual void updates_made() = 0;
};

/**
 * @short Maintainer of a local mirror of central IfMgr configuration
 * state via Xrls sent by the IfMgr.
 *
 * The IfMgrXrlMirror contains a copy of the central interface
 * configuration state.  Upon construction it registers itself with
 * the main configuration state maintainer.  It should then receive the
 * complete configuration tree and future updates to the tree.
 *
 * The local copy of the interface configuration state is accessible
 * through the @ref iftree() method.  The status of IfMgrXrlMirror
 * is obtainable through the @ref status() method. Only when the
 * IfMgrXrlMirror has the @ref READY status should the information
 * returned by @ref iftree() be relied upon.
 *
 * Interested parties can register as hint observers through the
 * @ref attach_hint_observer() and @ref detach_hint_observer() methods.
 * They will then receive notification of when the complete
 * IfMgrIfTree state has been received and told when they should
 * check the state again.
 */
class IfMgrXrlMirror
    : protected IfMgrXrlMirrorRouterObserver, protected IfMgrHintObserver
{
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

    enum Status {
	NO_FINDER,
	REGISTERING_WITH_FEA,
	WAITING_FOR_TREE_IMAGE,
	READY
    };

    /**
     * Default Xrl Target to register interest with.
     */
    static const char* DEFAULT_REGISTRATION_TARGET;

public:
    /**
     * Constructor
     *
     * @param eventloop to use for events.
     * @param reg_tgt name of Xrl class or target to supply interface
     * configuration updates.
     */
    IfMgrXrlMirror(EventLoop&	eventloop,
		   const char*	reg_tgt = DEFAULT_REGISTRATION_TARGET);

    /**
     * Constructor
     *
     * @param eventloop to use for events.
     * @param finder_addr address to route finder messages to.
     * @param reg_tgt name of Xrl class or target to supply interface
     * configuration updates.
     */
    IfMgrXrlMirror(EventLoop&	eventloop,
		   IPv4		finder_addr,
		   const char*	reg_tgt = DEFAULT_REGISTRATION_TARGET);

    /**
     * Constructor
     *
     * @param eventloop to use for events.
     * @param finder_addr address to route finder messages to.
     * @param finder_port port to direct finder messages to.
     * @param reg_tgt name of Xrl class or target to supply interface
     *                configuration updates.
     */
    IfMgrXrlMirror(EventLoop&	e,
		   IPv4		finder_addr,
		   uint16_t	finder_port,
		   const char*	reg_tgt = DEFAULT_REGISTRATION_TARGET);

    ~IfMgrXrlMirror();

    /**
     * @return interface configuration tree.  Should only be trusted when
     * status() is READY.
     */
    inline const IfMgrIfTree& iftree() const		{ return _iftree; }

    /**
     * @return state of current IfMgrXrlMirror.
     */
    Status status() const;

    /**
     * Attach an observer interested in receiving IfMgr hints.
     * @param o observer to be attached.
     * @return true on success, false if observer is already registered.
     */
    bool attach_hint_observer(IfMgrHintObserver* o);

    /**
     * Detach an observer interested in receiving IfMgr hints.
     * @param o observer to be detached.
     * @return true on success, false if observer was not registered.
     */
    bool detach_hint_observer(IfMgrHintObserver* o);

protected:
    void finder_ready_event();
    void finder_disconnect_event();
    void register_with_ifmgr();

protected:
    void tree_complete();
    void updates_made();

protected:
    void set_status(Status s);
    void register_cb(const XrlError& e);

protected:
    EventLoop&			_e;
    IfMgrXrlMirrorRouter*	_rtr;
    IfMgrIfTree	   		_iftree;
    IfMgrCommandDispatcher	_dispatcher;
    IfMgrXrlMirrorTarget*	_xrl_tgt;
    string			_rtarget;	// registration target (ifmgr)
    Status			_status;
    list<IfMgrHintObserver*>	_hint_observers;

    XorpTimer			_reg_timer;	// registration timer
};

#endif // __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__
