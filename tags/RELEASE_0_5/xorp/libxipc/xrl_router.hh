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

// $XORP: xorp/libxipc/xrl_router.hh,v 1.23 2003/09/10 21:42:05 hodson Exp $

#ifndef __LIBXIPC_XRL_ROUTER_HH__
#define __LIBXIPC_XRL_ROUTER_HH__

#include "config.h"
#include "libxorp/callback.hh"

#include "xrl.hh"
#include "xrl_sender.hh"
#include "xrl_dispatcher.hh"
#include "xrl_pf.hh"
#include "finder_constants.hh"
#include "finder_client_observer.hh"

class DispatchState;

class FinderClient;
class FinderClientXrlTarget;
class FinderTcpAutoConnector;
class FinderDBEntry;
class XrlRouterDispatchState;

class XrlRouter :
    public XrlDispatcher,
    public XrlSender,
    public FinderClientObserver
{
public:
    typedef XrlSender::Callback XrlCallback;
    typedef XrlRouterDispatchState DispatchState;

public:
    XrlRouter(EventLoop&	e,
	      const char*	class_name,
	      const char*	finder_address,
	      uint16_t		finder_port = FINDER_DEFAULT_PORT)
	throw (InvalidAddress);

    XrlRouter(EventLoop&	e,
	      const char*	class_name,
	      IPv4		finder_address = FINDER_DEFAULT_HOST,
	      uint16_t		finder_port = FINDER_DEFAULT_PORT)
	throw (InvalidAddress);

    virtual ~XrlRouter();

    bool add_listener(XrlPFListener* listener);

    void finalize();

    bool connected() const;

    bool ready() const;

    bool pending() const;

    bool send(const Xrl& xrl, const XrlCallback& cb);

    bool add_handler(const string& cmd, const XrlRecvCallback& rcb);

    inline EventLoop& eventloop()		{ return _e; }

    inline const string& instance_name() const	{ return _instance_name; }

    inline const string& class_name() const	{ return XrlCmdMap::name(); }

    IPv4     finder_address() const;

    uint16_t finder_port() const;

protected:
    /**
     * Called when Finder connection is established.
     *
     * Default implementation is a no-op.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Default implementation is a no-op.
     */
    virtual void finder_disconnect_event();

    /**
     * Called when an Xrl Target becomes visible to other processes.
     * Implementers of this method should check @ref tgt_name
     * corresponds to the @ref XrlRouter::instance_name as other
     * targets within same process may cause this method to be
     * invoked.
     *
     * Default implementation is a no-op.
     *
     * @param tgt_name name of Xrl Target becoming ready.
     */
    virtual void finder_ready_event(const string& tgt_name);

protected:
    XrlError dispatch_xrl(const string&	 method_name,
			  const XrlArgs& inputs,
			  XrlArgs&	 outputs) const;

protected:
    /**
     * Resolve callback (slow path).
     *
     * Called with results from asynchronous FinderClient Xrl queries.
     */
    void resolve_callback(const XrlError&	  e,
			  const FinderDBEntry*	  dbe,
			  XrlRouterDispatchState* ds);

    /**
     * Send callback (fast path).
     */
    void send_callback(const XrlError&	e,
		       XrlArgs*		reply,
		       XrlPFSender*	sender,
		       XrlCallback	user_callback);

    /**
     * Choose appropriate XrlPFSender and execute Xrl dispatch.
     *
     * @return true on success, false otherwise.
     */
    bool send_resolved(const Xrl&		xrl,
		       const FinderDBEntry*	dbe,
		       const XrlCallback&	dispatch_cb);

    void initialize(const char* class_name,
		    IPv4	finder_addr,
		    uint16_t	finder_port);

protected:
    EventLoop&			_e;
    FinderClient*		_fc;
    FinderClientXrlTarget*	_fxt;
    FinderTcpAutoConnector*	_fac;
    string			_instance_name;

    list<XrlPFListener*>	_listeners;		// listeners
    list<XrlRouterDispatchState*> _dsl;			// dispatch state
    list<XrlPFSender*>		_senders;		// active senders

    static uint32_t		_icnt;			// instance count
};

#endif // __LIBXIPC_XRL_ROUTER_HH__
