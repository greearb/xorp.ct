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

// $XORP: xorp/libfeaclient/ifmgr_xrl_mirror.hh,v 1.5 2004/04/22 01:11:51 pavlin Exp $

#ifndef __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__
#define __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__

#include "config.h"

#include "libxorp/ipv4.hh"
#include "libxorp/timer.hh"
#include "libxorp/service.hh"
#include "libxipc/finder_constants.hh"

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
 * configuration state.  The IfMgrXrlMirror class implements the @ref
 * ServiceBase interface.  When @ref startup() is called it attempts
 * to register with the @ref IfMgrXrlReplicationManager instance
 * running within the FEA.  If registration succeeds it will receive
 * the complete configuration tree and receive future configuration
 * tree.  Once the configuration tree is received it transitions into
 * the RUNNING state and is considered operational.
 *
 * When the status of the IfMgrXrlMirror is RUNNING, then a copy of
 * the interface configuration state is accessible through the
 * @ref iftree() method.  If the instance is another state then
 * configuration tree available through the IfMgrXrlMirror will be
 * empty.
 *
 * For parties that are interested in receiving synchronous
 * configuration tree change notifications, they can register as hint
 * observers.  Hints provide coarse indication that a change has
 * occured - they announce the which item in the tree has changed and
 * should be checked.  Hint observers express interest through the
 * @ref attach_hint_observer() and @ref detach_hint_observer()
 * methods.
 */
class IfMgrXrlMirror
    : public	ServiceBase,
      protected	IfMgrXrlMirrorRouterObserver,
      protected	IfMgrHintObserver
{
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

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
     *                configuration updates.
     * @param finder_host address to route finder messages to.
     * @param finder_port port to direct finder messages to.
     */
    IfMgrXrlMirror(EventLoop&	e,
		   const char*	reg_tgt     = DEFAULT_REGISTRATION_TARGET,
		   IPv4		finder_host = FINDER_DEFAULT_HOST,
		   uint16_t	finder_port = FINDER_DEFAULT_PORT);

    ~IfMgrXrlMirror();

    /**
     * Start running.  Attempt to register instance with the
     * registration target supplied in the constructor and await
     * interface configuration tree data.  When data is received
     * transition into the RUNNING state (see @ref ServiceBase for
     * states).
     *
     * @return true on success, false on failure.
     */
    bool startup();

    /**
     * Stop running and shutdown.  Deregister with the registration
     * target and transition to SHUTDOWN state when complete.
     *
     * @return true on success, false on failure.
     */
    bool shutdown();

    /**
     * @return interface configuration tree.  Should only be trusted when
     * status() is READY.
     */
    inline const IfMgrIfTree& iftree() const		{ return _iftree; }

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
    void unregister_with_ifmgr();

protected:
    void tree_complete();
    void updates_made();

protected:
    void register_cb(const XrlError& e);
    void unregister_cb(const XrlError& e);

protected:
    EventLoop&			_e;
    IPv4			_finder_host;
    uint16_t			_finder_port;
    IfMgrIfTree	   		_iftree;
    IfMgrCommandDispatcher	_dispatcher;
    string			_rtarget;	// registration target (ifmgr)

    IfMgrXrlMirrorRouter*	_rtr;
    IfMgrXrlMirrorTarget*	_xrl_tgt;

    list<IfMgrHintObserver*>	_hint_observers;

    XorpTimer			_reg_timer;	// registration timer
};

#endif // __LIBFEACLIENT_XRL_IFMGR_MIRROR_HH__
