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

#ident "$XORP: xorp/libxipc/xrl_router.cc,v 1.27 2003/09/10 18:06:03 hodson Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/xlog.h"

#include "hmac_md5.h"
#include "xrl_error.hh"
#include "xrl_router.hh"
#include "xrl_pf.hh"
#include "xrl_pf_factory.hh"

#include "finder_client.hh"
#include "finder_client_xrl_target.hh"
#include "finder_tcp_messenger.hh"

#include "sockutil.hh"

//
// Enable this macro to enable Xrl callback checker that checks each Xrl
// callback is dispatched just once.
//
// #define USE_XRL_CALLBACK_CHECKER


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


/**
 * Slow-path dispatch state.  Contains information that needs to be held
 * whilst waiting for a finder resolution.
 */
struct XrlRouterDispatchState
{
public:
    typedef XrlRouter::XrlCallback XrlCallback;

public:
    XrlRouterDispatchState(const Xrl&		x,
			   const XrlCallback&	xcb)
	: _xrl(x), _xcb(xcb)
    {}

    inline const Xrl& xrl() const		{ return _xrl; }
    inline XrlCallback& cb()			{ return _xcb; }

protected:
    Xrl				_xrl;
    XrlRouter::XrlCallback	_xcb;
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

static string
mk_instance_name(EventLoop& e, const char* classname)
{
    static uint32_t sp = (uint32_t)getpid();
    static uint32_t sa = if_get_preferred().s_addr;
    static uint32_t sc;

    TimeVal now;
    e.current_time(now);
    sc++;

    uint32_t data[5];
    data[0] = sa;
    data[1] = sp;
    data[2] = sc;
    data[3] = now.sec();
    data[4] = now.usec();

    const char* key = "hubble bubble toil and trouble";
    uint8_t digest[16];
    hmac_md5(reinterpret_cast<const uint8_t*>(data), sizeof(data),
	     reinterpret_cast<const uint8_t*>(key), sizeof(key), digest);

    char asc_digest[33];
    if (hmac_md5_digest_to_ascii(digest, asc_digest, sizeof(asc_digest)) == 0)
	XLOG_FATAL("Could not make ascii md5 digest representation");

    return c_format("%s-%s@", classname, asc_digest) + IPv4(sa).str();
}

// ----------------------------------------------------------------------------
// Debug code (check callbacks only invoked once)




// ----------------------------------------------------------------------------
// XrlRouter code

uint32_t XrlRouter::_icnt = 0;

void
XrlRouter::initialize(const char* class_name,
		      IPv4	  finder_addr,
		      uint16_t	  finder_port)
{
    _fc = new FinderClient();

    _fxt = new FinderClientXrlTarget(_fc, &_fc->commands());

    _fac = new FinderTcpAutoConnector(_e, *_fc, _fc->commands(),
				      finder_addr, finder_port);

    _instance_name = mk_instance_name(_e, class_name);
    _fc->attach_observer(this);
    if (_fc->register_xrl_target(_instance_name, class_name, this) == false) {
	XLOG_FATAL("Failed to register target %s\n", class_name);
    }
    if (_icnt == 0)
	XrlPFSenderFactory::startup();
    _icnt++;
}

XrlRouter::XrlRouter(EventLoop&  e,
		     const char* class_name,
		     const char* finder_addr,
		     uint16_t	 finder_port)
    throw (InvalidAddress)
    : XrlDispatcher(class_name), _e(e)
{
    IPv4 finder_ip = finder_addr ? finder_host(finder_addr)
	: FINDER_DEFAULT_HOST;
    if (0 == finder_port)
	finder_port = FINDER_DEFAULT_PORT;

    initialize(class_name, finder_ip, finder_port);
}

XrlRouter::XrlRouter(EventLoop&  e,
		     const char* class_name,
		     IPv4 	 finder_ip,
		     uint16_t	 finder_port)
    throw (InvalidAddress)
    : XrlDispatcher(class_name), _e(e)
{
    if (0 == finder_port)
	finder_port = FINDER_DEFAULT_PORT;

    initialize(class_name, finder_ip, finder_port);
}

XrlRouter::~XrlRouter()
{
    _fc->detach_observer(this);
    _fac->set_enabled(false);

    while (_senders.empty() == false) {
	XrlPFSenderFactory::destroy_sender(_senders.front());
	_senders.pop_front();
    }

    while (_dsl.empty() == false) {
	delete _dsl.front();
	_dsl.pop_front();
    }

    delete _fac;
    delete _fxt;
    delete _fc;
    _icnt--;
    if (_icnt == 0)
	XrlPFSenderFactory::shutdown();

}

bool
XrlRouter::connected() const
{
    return _fc && _fc->connected();
}

bool
XrlRouter::ready() const
{
    return _fc && _fc->ready();
}

bool
XrlRouter::pending() const
{
    return _senders.empty() == false && _dsl.empty() == false;
}

bool
XrlRouter::add_listener(XrlPFListener* l)
{
    _listeners.push_back(l);
    l->set_dispatcher(this);

    // Walk list of Xrl in command map and register them with finder client
    XrlCmdMap::CmdMap::const_iterator ci = _cmd_map.begin();
    while (ci != _cmd_map.end()) {
	Xrl x("finder", _instance_name, ci->first);
	debug_msg("adding handler for %s protocol %s address %s\n",
		  x.str().c_str(), l->protocol(), l->address());
	_fc->register_xrl(instance_name(), x.str(),
			  l->protocol(), l->address());
	++ci;
    }

    return true;
}

void
XrlRouter::finalize()
{
    // XXX should set a variable here to signal finalize set and no
    // further registrations of commands or xrls will be accepted.
    _fc->enable_xrls(instance_name());
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
	Xrl x("finder", _instance_name, cmd);
	debug_msg("adding handler for %s protocol %s address %s\n",
		  x.str().c_str(), (*pli)->protocol(), (*pli)->address());
	_fc->register_xrl(instance_name(), x.str(),
			  (*pli)->protocol(), (*pli)->address());
	++pli;
    }

    return true;
}

void
XrlRouter::send_callback(const XrlError& e,
			 XrlArgs*	 reply,
			 XrlPFSender*	 s,
			 XrlCallback	 user_callback)
{
    list<XrlPFSender*>::iterator i = find(_senders.begin(), _senders.end(), s);
    XLOG_ASSERT(i != _senders.end());
    _senders.erase(i);

    XrlPFSenderFactory::destroy_sender(s);

    user_callback->dispatch(e, reply);
}

bool
XrlRouter::send_resolved(const Xrl&		xrl,
			 const FinderDBEntry*	dbe,
			 const XrlCallback&	cb)
{
    try {
	Xrl x(dbe->values().front().c_str());
	XrlPFSender* s = XrlPFSenderFactory::create_sender(
				_e, x.protocol().c_str(), x.target().c_str()
				);
	Xrl tmp(xrl);
	x.args().swap(tmp.args());
	if (s) {
	    trace_xrl("Sending ", x);
	    _senders.push_back(s);
	    s->send(x, callback(this, &XrlRouter::send_callback, s, cb));
	    return true;
	}
	cb->dispatch(XrlError(SEND_FAILED, "sender not instantiated"), 0);
    } catch (const InvalidString&) {
	cb->dispatch(XrlError(INTERNAL_ERROR, "bad factory arguments"), 0);
    }
    return false;
}

void
XrlRouter::resolve_callback(const XrlError&	 	e,
			    const FinderDBEntry*	dbe,
			    XrlRouterDispatchState*	ds)
{
    list<XrlRouterDispatchState*>::iterator i;
    i = find(_dsl.begin(), _dsl.end(), ds);
    _dsl.erase(i);

    if (e == XrlError::OKAY()) {
	send_resolved(ds->xrl(), dbe, ds->cb());
    } else {
	ds->cb()->dispatch(e, 0);
    }
    delete ds;

    return;
}

#ifdef USE_XRL_CALLBACK_CHECKER

/**
 * @short Class to maonitor Xrl callbacks.
 *
 * At present this class just checks that each XrlCallback is executed
 * just once.
 */
static class
XrlCallbackChecker
{
public:
    typedef XrlRouter::XrlCallback XrlCallback;

public:
    XrlCallbackChecker()
	: _seqno(0)
    {}
    
    void process_callback(const XrlError& e, XrlArgs* a, uint32_t seqno)
    {
	map<uint32_t, XrlCallback>::iterator i = _cbs.find(seqno);
	XLOG_ASSERT(i != _cbs.end());
	XrlCallback ucb = i->second;
	_cbs.erase(i);
	if (e != XrlError::OKAY()) {
	    fprintf(stderr, "Seqno %u Failed %s\n",
		    seqno, e.str().c_str());
	}
	ucb->dispatch(e, a);
    }
    
    XrlCallback add_callback(const XrlCallback& ucb)
    {
	_cbs[_seqno] = ucb;
	return callback(this, &XrlCallbackChecker::process_callback, _seqno++);
    }
    
protected:
    map<uint32_t, XrlCallback> _cbs;
    uint32_t _seqno;
} cb_checker;

#endif

bool
XrlRouter::send(const Xrl& xrl, const XrlCallback& user_cb)
{
    trace_xrl("Resolving xrl:", xrl);

#ifdef USE_XRL_CALLBACK_CHECKER
    // Callback checker wrappers user callback with callback that
    // performs completion checking operation and then dispatches the
    // users callback.
    XrlCallback xcb = cb_checker.add_callback(user_cb);
#else
    const XrlCallback& xcb = user_cb;
#endif

    //
    // Finder directed Xrl - takes custom path through FinderClient.
    //
    if (xrl.protocol() == "finder" && xrl.target().substr(0,6) == "finder") {
	_fc->forward_finder_xrl(xrl, xcb);
	// XXX check for error.
	return true;
    }
    
    //
    // Fast path - Xrl resolution in cache and no Xrls ahead blocked on
    // on response from Finder.  Fast path cannot be taken if earlier Xrls
    // are blocked on Finder as re-ordering may occur.  We don't necessarily
    // care about re-ordering between protocol families (at this time), but
    // we do within protocol families.
    //
    string xrl_no_args = xrl.string_no_args();
    const FinderDBEntry* fdbe = _fc->query_cache(xrl_no_args);
    if (_dsl.empty() && fdbe) {
	send_resolved(xrl, fdbe, xcb);
	// XXX check for error
	return true;
    }

    //
    // Slow path - involves more state copying
    //
    DispatchState *ds = new XrlRouterDispatchState(xrl, xcb);
    _dsl.push_back(ds);
    _fc->query(xrl_no_args,
	       callback(this, &XrlRouter::resolve_callback, ds));

    return true;
}

XrlError
XrlRouter::dispatch_xrl(const string&	method_name,
			const XrlArgs&	inputs,
			XrlArgs&	outputs) const
{
    string resolved_method;
    if (_fc->query_self(method_name, resolved_method) == true) {
	return XrlDispatcher::dispatch_xrl(resolved_method, inputs, outputs);
    }
    debug_msg("Could not find mapping for %s\n", method_name.c_str());
    return XrlError::NO_SUCH_METHOD();
}

IPv4
XrlRouter::finder_address() const
{
    return _fac->finder_address();
}

uint16_t
XrlRouter::finder_port() const
{
    return _fac->finder_port();
}

void
XrlRouter::finder_connect_event()
{
    debug_msg("Finder connect event\n");
}

void
XrlRouter::finder_disconnect_event()
{
    debug_msg("Finder disconnect event\n");
}

void
XrlRouter::finder_ready_event(const string& tgt_name)
{
    debug_msg("Finder target ready event: \"%s\"\n", tgt_name.c_str());
}
