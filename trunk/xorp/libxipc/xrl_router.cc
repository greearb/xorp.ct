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

#ident "$XORP: xorp/libxipc/xrl_router.cc,v 1.14 2003/04/02 22:58:57 hodson Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/xlog.h"

#include "xrl_error.hh"
#include "xrl_router.hh"
#include "xrl_pf.hh"
#include "xrl_pf_factory.hh"

#include "finder_ng_client.hh"
#include "finder_ng_client_xrl_target.hh"
#include "finder_tcp_messenger.hh"

#include "sockutil.hh"

// ----------------------------------------------------------------------------
// Xrl Tracing central

static class TraceXrl {
public:
    TraceXrl() {
	_do_trace = !(getenv("XRLTRACE") == 0);
    }
    inline bool on() const { return _do_trace; }
    operator bool() { return _do_trace; }

protected:
    bool _do_trace;
} xrl_trace;

#define trace_xrl(p, x) 						      \
do {									      \
    if (xrl_trace.on()) XLOG_INFO(string((p) + (x).str()).c_str());	      \
} while (0)

// ----------------------------------------------------------------------------
// XrlCmdDispatcher methods (temporary class and location?)

XrlError
XrlCmdDispatcher::dispatch_xrl(const Xrl& xrl, XrlArgs& response) const
{
    const XrlCmdEntry *c = get_handler(xrl.command().c_str());
    if (c) {
	return c->callback->dispatch(xrl, &response);	
    }
    debug_msg("No handler for %s\n", xrl.command().c_str());
    return XrlError::NO_SUCH_METHOD();
}

struct XrlRouterDispatchState {
public:
    typedef XrlRouter::XrlCallback XrlCallback;

public:
    XrlRouterDispatchState(const XrlRouter*	r,
			   const Xrl&		x,
			   const XrlCallback&	xcb)
	: _rtr(r), _xrl(x), _xcb(xcb), _xrs(0)
    {
    }

    ~XrlRouterDispatchState()
    {
	delete _xrs;
    }
    
    inline const XrlRouter* router() const { return _rtr; }

    inline const Xrl& xrl() const { return _xrl; }

    inline void dispatch(const XrlError& e, XrlArgs* a = 0)
    {
	_xcb->dispatch(e, a);
    }

    inline void set_sender(XrlPFSender* s) { _xrs = s; }
    
protected:
    const XrlRouter*		_rtr;
    Xrl				_xrl;
    XrlRouter::XrlCallback	_xcb;
    XrlPFSender*		_xrs;
};

//
// This is scatty and temporary
//
static IPv4
finder_host(const char* host)
    throw (InvalidAddress)
{
    in_addr ia;
    if (address_lookup(host, ia) == false) {
	xorp_throw(InvalidAddress,
		   c_format("Could resolve finder host %s\n", host));
    }
    return IPv4(ia);
}

XrlRouter::XrlRouter(EventLoop&  e,
		     const char* entity_name,
		     const char* host,
		     uint16_t	 port)
    throw (InvalidAddress)
    : XrlCmdDispatcher(entity_name), _e(e), _rpend(0), _spend(0)
{
    _fc = new FinderNGClient();
    _fxt = new FinderNGClientXrlTarget(_fc, &_fc->commands());

    if (0 == port)
	port = FINDER_NG_TCP_DEFAULT_PORT;
    _fac = new FinderNGTcpAutoConnector(e, *_fc, _fc->commands(),
					finder_host(host), port);

    if (_fc->register_xrl_target(entity_name, "undisclosed", _id) == false) {
	XLOG_FATAL("Failed to register target %s\n", entity_name);
    }
}

XrlRouter::~XrlRouter()
{
    _fac->set_enabled(false);

    while (_dsl.empty() == false) {
	delete _dsl.front();
	_dsl.pop_front();
    }

    delete _fac;
    delete _fxt;
    delete _fc;
}

void
XrlRouter::dispose(XrlRouterDispatchState* ds)
{
    list<XrlRouterDispatchState*>::iterator i;
    i = find(_dsl.begin(), _dsl.end(), ds);
    XLOG_ASSERT(_dsl.end() != i);
    _dsl.erase(i);
}

bool
XrlRouter::connected() const
{
    return _fc->connected();
}

bool
XrlRouter::pending() const
{
    return _rpend != 0 || _spend != 0;
}

bool
XrlRouter::add_listener(XrlPFListener* l)
{
    _listeners.push_back(l);
    l->set_dispatcher(this);
    
    // Walk list of Xrl in command map and register them with finder client
    XrlCmdMap::CmdMap::const_iterator ci = _cmd_map.begin();
    while (ci != _cmd_map.end()) {
	Xrl x("finder", name(), ci->first);
	debug_msg("adding handler for %s protocol %s address %s\n",
		  x.str().c_str(), l->protocol(), l->address());
	_fc->register_xrl(_id, x.str(), l->protocol(), l->address());
	++ci;
    }
    
    return true;
}

void
XrlRouter::finalize()
{
    // XXX should set a variable here to signal finalize set and no
    // further registrations of commands or xrls will be accepted.
    _fc->enable_xrls(_id);
}

bool
XrlRouter::add_handler(const string& cmd, const XrlRecvCallback& rcb)
{
    if (XrlCmdMap::add_handler(cmd, rcb) == false) {
	return false;
    }
    // Walk list of listeners and register xrl corresponding to listener with
    // finder client.
    list<XrlPFListener*>::const_iterator pli = _listeners.begin();

    while (pli != _listeners.end()) {
	Xrl x("finder", name(), cmd);
	debug_msg("adding handler for %s protocol %s address %s\n",
		  x.str().c_str(), (*pli)->protocol(), (*pli)->address());
	_fc->register_xrl(_id, x.str(), (*pli)->protocol(), (*pli)->address());
	++pli;
    }

    return true;
}

void
XrlRouter::send_callback(const XrlError&	 e,
			 const Xrl&		 /* xrl */,
			 XrlArgs*	 	 args,
			 XrlRouterDispatchState* ds)
{
    _spend--;
    ds->dispatch(e, args);
    dispose(ds);
}

void
XrlRouter::resolve_callback(const XrlError&	 	e,
			    const FinderDBEntry*	dbe,
			    XrlRouterDispatchState*	ds)
{
    _rpend--;
    if (e != XrlError::OKAY()) {
	ds->dispatch(e, 0);
	dispose(ds);
	return;
    }

    if (dbe->values().size() > 1) {
	XLOG_WARNING("Xrl resolves multiple times, using first.");
    }

    try {
	Xrl x(dbe->values().front().c_str());
	XrlPFSender* s = XrlPFSenderFactory::create(_e,
						    x.protocol().c_str(),
						    x.target().c_str());
	ds->set_sender(s);
	if (s) {
	    trace_xrl("Sending ", x);
	    _spend++;
	    s->send(ds->xrl(),
		    callback(this, &XrlRouter::send_callback, ds));
	    return;
	}
	ds->dispatch(XrlError::SEND_FAILED(), 0);
    } catch (const InvalidString&) {
	ds->dispatch(XrlError(XrlError::INTERNAL_ERROR().error_code(),
			      "bad factory arguments"), 0);
    }
    dispose(ds);

    return;    
}

bool
XrlRouter::send(const Xrl& xrl, const XrlCallback& xcb)
{
    trace_xrl("Resolving xrl:", xrl);
    _rpend++;
    
    DispatchState *ds = new XrlRouterDispatchState(this, xrl, xcb);
    _dsl.push_back(ds);

    if (xrl.protocol() == "finder" && xrl.target().substr(0,6) == "finder") {
	_fc->forward_finder_xrl(ds->xrl(),
				callback(this, &XrlRouter::send_callback, ds));
    } else {
	_fc->query(xrl.string_no_args(),
		   callback(this, &XrlRouter::resolve_callback, ds));
    }
    return true;
}


