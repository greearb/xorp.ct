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

// $XORP: xorp/libxipc/xrl_router.hh,v 1.18 2003/05/21 23:04:42 hodson Exp $

#ifndef __LIBXIPC_XRL_ROUTER_HH__
#define __LIBXIPC_XRL_ROUTER_HH__

#include "config.h"
#include "libxorp/callback.hh"

#include "xrl.hh"
#include "xrl_sender.hh"
#include "xrl_dispatcher.hh"
#include "xrl_pf.hh"
#include "finder_constants.hh"

class DispatchState;

class FinderClient;
class FinderClientXrlTarget;
class FinderTcpAutoConnector;
class FinderDBEntry;
class XrlRouterDispatchState;

class XrlRouter : public XrlDispatcher, public XrlSender {
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

protected:
    void resolve_callback(const XrlError&		e,
			  const FinderDBEntry*		dbe,
			  XrlRouterDispatchState*	ds);

    void send_callback(const XrlError&		e,
		       const Xrl&		xrl,
		       XrlArgs*			reply,
		       XrlRouterDispatchState*	ds);

    void dispose(XrlRouterDispatchState*);

    void initialize(const char* class_name,
		    IPv4	finder_addr,
		    uint16_t	finder_port);

protected:
    EventLoop&			_e;
    FinderClient*		_fc;
    FinderClientXrlTarget*	_fxt;
    FinderTcpAutoConnector*	_fac;
    string			_instance_name;

    uint32_t			_rpend, _spend;

    list<XrlPFListener*>	_listeners;
    list<XrlRouterDispatchState*> _dsl;
};

#endif // __LIBXIPC_XRL_ROUTER_HH__
