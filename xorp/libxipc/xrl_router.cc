// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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
    bool on() const { return _do_trace; }
    operator bool() { return _do_trace; }

protected:
    bool _do_trace;
} xrl_trace;

#define trace_xrl(p, x) 						      \
do {									      \
    if (xrl_trace.on()) XLOG_INFO("%s", string(string(p) + (x).str()).c_str());     \
} while (0)


/**
 * Slow-path dispatch state.  Contains information that needs to be held
 * whilst waiting for a finder resolution.
 */
class XrlRouterDispatchState {
public:
    typedef XrlRouter::XrlCallback XrlCallback;

    XrlRouterDispatchState(const Xrl&		x,
			   const XrlCallback&	xcb)
	: _xrl(x), _xcb(xcb)
    {}

    const Xrl& xrl() const		{ return _xrl; }
    XrlCallback& cb()			{ return _xcb; }

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
    static uint32_t sa = get_preferred_ipv4_addr().s_addr;
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

static const uint32_t DEFAULT_FINDER_CONNECT_TIMEOUT_MS = 30 * 1000;

uint32_t XrlRouter::_icnt = 0;

void
XrlRouter::initialize(const char* class_name,
		      IPv4	  finder_addr,
		      uint16_t	  finder_port)
{
    char* value;

    // Set the finder client address from the environment variable if it is set
    value = getenv("XORP_FINDER_CLIENT_ADDRESS");
    if (value != NULL) {
	try {
	    struct in_addr addr;
	    IPv4 ipv4(value);
	    ipv4.copy_out(addr);
	    if (set_preferred_ipv4_addr(addr) != true) {
		XLOG_ERROR("Failed to change the Finder client address to %s",
			   ipv4.str().c_str());
	    }
	} catch (const InvalidString& e) {
	    UNUSED(e);
	    XLOG_ERROR("Invalid \"XORP_FINDER_CLIENT_ADDRESS\": %s",
		       e.str().c_str());
	}
    }

    // Set the finder server address from the environment variable if it is set
    value = getenv("XORP_FINDER_SERVER_ADDRESS");
    if (value != NULL) {
	try {
	    IPv4 ipv4(value);
	    if (! ipv4.is_unicast()) {
		XLOG_ERROR("Failed to change the Finder server address to %s",
			   ipv4.str().c_str());
	    } else {
		finder_addr = ipv4;
	    }
	} catch (const InvalidString& e) {
	    UNUSED(e);
	    XLOG_ERROR("Invalid \"XORP_FINDER_SERVER_ADDRESS\": %s",
		       e.str().c_str());
	}
    }

    // Set the finder server port from the environment variable if it is set
    value = getenv("XORP_FINDER_SERVER_PORT");
    if (value != NULL) {
	int port = atoi(value);
	if (port <= 0 || port > 65535) {
	    XLOG_ERROR("Invalid \"XORP_FINDER_SERVER_PORT\": %s", value);
	} else {
	    finder_port = port;
	}
    }

    // Set the finder connect timeout from environment variable if it is set.
    uint32_t timeout_ms = DEFAULT_FINDER_CONNECT_TIMEOUT_MS;
    value = getenv("XORP_FINDER_CONNECT_TIMEOUT_MS");
    if (value != NULL) {
	char *ep = NULL;
	timeout_ms = strtoul(value, &ep, 10);
	if ( !(*value != '\0' && *ep == '\0') &&
	      (timeout_ms <= 0 || timeout_ms > 120000)) {
	    XLOG_ERROR("Out of bounds \"XORP_FINDER_CONNECT_TIMEOUT_MS\": %s (must be 0..120000",
		       value);
	    timeout_ms = DEFAULT_FINDER_CONNECT_TIMEOUT_MS;
	}
    }

    _fc = new FinderClient();

    _fxt = new FinderClientXrlTarget(_fc, &_fc->commands());

    _fac = new FinderTcpAutoConnector(_e, *_fc, _fc->commands(),
				      finder_addr, finder_port,
				      true, timeout_ms);

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
    : XrlDispatcher(class_name), _e(e), _finalized(false)
{
    IPv4 finder_ip;
    if (0 == finder_addr) {
	finder_ip = FinderConstants::FINDER_DEFAULT_HOST();
    } else {
	finder_ip = finder_host(finder_addr);
    }

    if (0 == finder_port)
	finder_port = FinderConstants::FINDER_DEFAULT_PORT();

    initialize(class_name, finder_ip, finder_port);
}

XrlRouter::XrlRouter(EventLoop&  e,
		     const char* class_name,
		     IPv4 	 finder_ip,
		     uint16_t	 finder_port)
    throw (InvalidAddress)
    : XrlDispatcher(class_name), _e(e), _finalized(false)
{
    if (0 == finder_port)
	finder_port = FinderConstants::FINDER_DEFAULT_PORT();

    initialize(class_name, finder_ip, finder_port);
}

XrlRouter::~XrlRouter()
{
    _fc->detach_observer(this);
    _fac->set_enabled(false);

    while (_senders.empty() == false)
	_senders.pop_front();

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

    for (XIM::iterator i = _xi_cache.begin(); i != _xi_cache.end(); ++i)
	delete i->second;
}

bool
XrlRouter::connected() const
{
    return _fc && _fc->connected();
}

bool
XrlRouter::connect_failed() const
{
    return _fac && _fac->connect_failed();
}

bool
XrlRouter::ready() const
{
    return _fc && _fc->ready();
}

bool
XrlRouter::failed() const
{
    return _fac->enabled() == false && ready() == false;
}

bool
XrlRouter::pending() const
{
    list<XrlPFListener*>::const_iterator ci;
    for (ci = _listeners.begin(); ci != _listeners.end(); ++ci) {
	XrlPFListener* l = *ci;
	if (l->response_pending())
	    return true;
    }

    if (_dsl.size()) {
	// Return true if we have any alive senders.
	for (list< ref_ptr<XrlPFSender> >::const_iterator si = _senders.begin();
	     si != _senders.end(); ++si) {
	    ref_ptr<XrlPFSender> s = *si;
	    if (s->alive()) {
		return true;
	    }
	}
    }

    // No alive senders it seems.
    return false;
}

bool
XrlRouter::add_listener(XrlPFListener* l)
{
    _listeners.push_back(l);
    l->set_dispatcher(this);

    return true;
}

void
XrlRouter::finalize()
{
    list<XrlPFListener*>::iterator li = _listeners.begin();
    while (li != _listeners.end()) {
	const XrlPFListener* l = *li;
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
	++li;
    }
    _fc->enable_xrls(instance_name());
    _finalized = true;
}

bool
XrlRouter::add_handler_internal(const string& cmd,
				const XrlRecvAsyncCallback& rcb)
{
    if (finalized()) {
	XLOG_ERROR("Attempting to add handler after XrlRouter finalized.  Handler = \"%s\"", cmd.c_str());
	return false;
    }

    return XrlCmdMap::add_handler_internal(cmd, rcb);
}

void
XrlRouter::send_callback(const XrlError& e,
			 XrlArgs*	 reply,
			 XrlPFSender* /* s */, // Don't use, should be a ref-ptr if it is ever used.
			 XrlCallback	 user_callback)
{
    user_callback->dispatch(e, reply);
}

bool
XrlRouter::send_resolved(const Xrl&		xrl,
			 const FinderDBEntry*	dbe,
			 const XrlCallback&	cb,
			 bool  direct_call)
{
    try {
	ref_ptr<XrlPFSender> s = lookup_sender(xrl, const_cast<FinderDBEntry*>(dbe));
	if (!s.get()) {
	    // Notify Finder client that result was bad.
	    _fc->uncache_result(dbe);

	    // Coerce finder client to check with Finder.
	    return send(xrl, cb);
	}

	const Xrl& x = dbe->xrls().front();
    	x.set_args(xrl);

	trace_xrl("Sending ", x);
	// NOTE:  using s.get below breaks ref counting, but can't figure out WTF the
	// callback template magic is to make it work with a ref-ptr.  Either way, it appears
	// the ptr may not be used anyway (it's not in send_callback, for instance)
	return s->send(x, direct_call,
		       callback(this, &XrlRouter::send_callback,
				s.get(), cb));

	cb->dispatch(XrlError(SEND_FAILED, "sender not instantiated"), 0);
    } catch (const InvalidString&) {
	cb->dispatch(XrlError(INTERNAL_ERROR, "bad factory arguments"), 0);
    }
    return false;
}

ref_ptr<XrlPFSender>
XrlRouter::lookup_sender(const Xrl& xrl, FinderDBEntry* dbe)
{
    const Xrl& x = dbe->xrls().front();
    ref_ptr<XrlPFSender> s;

    // Try to use the cached pointer to the sender.
    if (xrl.resolved()) {
        s = xrl.resolved_sender();

        if (!s.is_empty())
	    return s;

	xrl.set_resolved(false);
    }

    // Find a new sender.
    for (list< ref_ptr<XrlPFSender> >::iterator i = _senders.begin();
	 i != _senders.end(); ++i) {
	s = *i;

	if (s->protocol() != x.protocol() || s->address()  != x.target())
	    continue;

	if (s->alive()) {
	    xrl.set_resolved(true);
	    xrl.set_resolved_sender(s);
	    return s;
	}

	XLOG_INFO("Sender died (protocol = \"%s\", address = \"%s\")",
		  s->protocol(), s->address().c_str());

	_senders.erase(i);
	break;
    }

    s.reset();

    // create sender
    while (dbe->xrls().size()) {
	const Xrl& x = dbe->xrls().front();
	s = XrlPFSenderFactory::create_sender(dbe->key(), _e, x.protocol().c_str(),
					      x.target().c_str());
	if (s.get() != 0)
	    break;

	XLOG_ERROR("Could not create XrlPFSender for protocol = \"%s\" "
		   "address = \"%s\" ",
		   x.protocol().c_str(), x.target().c_str());

	dbe->pop_front();
    }

    // Unable to instantiate.
    if (!s.get())
	return s;

    // New sender instantiated, take lock and record state.
    const Xrl& front = dbe->xrls().front();

    XLOG_ASSERT(s->protocol() == front.protocol());
    XLOG_ASSERT(s->address()  == front.target());
    _senders.push_back(s);

    return s;
}

void
XrlRouter::resolve_callback(const XrlError&	 	e,
			    const FinderDBEntry*	dbe,
			    XrlRouterDispatchState*	ds)
{
    list<XrlRouterDispatchState*>::iterator i;
    i = find(_dsl.begin(), _dsl.end(), ds);

    // It seems that the finder_client force_failure logic can cause
    // callbacks to be called out of order, so this assert fails.
    // Test case is to 'killall -r xorp' when lots of things are running.
    // --Ben
    //XLOG_ASSERT(i == _dsl.begin());

    _dsl.erase(i);

    if (e == XrlError::OKAY()) {
	const Xrl& xrl = ds->xrl();
	xrl.set_resolved(false);
	ref_ptr<XrlPFSender> nl;
	xrl.set_resolved_sender(nl);
	if (send_resolved(xrl, dbe, ds->cb(), false) == false) {
	    // We tried to force sender to send xrl and it declined the
	    // opportunity.  This should only happen when it's out of buffer
	    // space
	    ds->cb()->dispatch(XrlError::SEND_FAILED_TRANSIENT(), 0);
	}
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

    uint32_t last_seqno() const { return _seqno - 1; }

protected:
    map<uint32_t, XrlCallback> _cbs;
    uint32_t _seqno;
} cb_checker;

#endif

bool
XrlRouter::send(const Xrl& xrl, const XrlCallback& user_cb)
{
    trace_xrl("Resolving xrl:", xrl);

    if (_fc->connected() == false) {
#if	0
 	user_cb->dispatch(XrlError::NO_FINDER(), 0);
#endif
	XLOG_ERROR("NO FINDER");
	return false;
    }

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
    if (xrl.to_finder()) {
	if (_fc->forward_finder_xrl(xrl, xcb)) {
	    return true;
	}
#if	0
#ifdef USE_XRL_CALLBACK_CHECKER
	cb_checker.process_callback(XrlError::NO_FINDER(), 0,
				    cb_checker.last_seqno());
#else
 	user_cb->dispatch(XrlError::NO_FINDER(), 0);
#endif
#endif
	XLOG_ERROR("NO FINDER");
	return false;
    }

#if 0
    // XXX This stops us getting stuck with everything queued up on
    // finder client responses.  It's likely to be a cause of pain for
    // existing code.
    if (_fc->queries_pending() > 0)
	return false;
#endif

    //
    // Fast path - Xrl resolution in cache and no Xrls ahead blocked on
    // on response from Finder.  Fast path cannot be taken if earlier Xrls
    // are blocked on Finder as re-ordering may occur.  We don't necessarily
    // care about re-ordering between protocol families (at this time), but
    // we do within protocol families.
    //
    const string& xrl_no_args = xrl.string_no_args();
    // We can't cache this object.
    const FinderDBEntry* fdbe = _fc->query_cache(xrl_no_args);
    if (_dsl.empty() && fdbe) {
	return send_resolved(xrl, fdbe, xcb, true);
    }

    //
    // Slow path - involves more state copying
    //
    DispatchState *ds = new XrlRouterDispatchState(xrl, xcb);
    _dsl.push_back(ds);
    _fc->query(eventloop(), xrl_no_args,
	       callback(this, &XrlRouter::resolve_callback, ds));

    return true;
}

void
XrlRouter::dispatch_xrl(const string&	      method_name,
			const XrlArgs&	      inputs,
			XrlDispatcherCallback outputs) const
{
    string resolved_method;
    if (_fc->query_self(method_name, resolved_method) == true) {
	return
	    XrlDispatcher::dispatch_xrl(resolved_method, inputs, outputs);
    }
    debug_msg("Could not find mapping for %s\n", method_name.c_str());
    return outputs->dispatch(XrlError::NO_SUCH_METHOD(), NULL);
}

XrlDispatcher::XI*
XrlRouter::lookup_xrl(const string& name) const
{
    // fast path
    XIM::const_iterator i = _xi_cache.find(name);

    if (i != _xi_cache.end())
	return i->second;

    // slow path
    string resolved;
    if (_fc->query_self(name, resolved) != true)
	return NULL;

    XI* xi = XrlDispatcher::lookup_xrl(resolved);
    if (!xi)
	return NULL;

    _xi_cache[name] = xi;

    return xi;
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
    //    _failed = true;
}

void
XrlRouter::finder_ready_event(const string& tgt_name)
{
    UNUSED(tgt_name);
    debug_msg("Finder target ready event: \"%s\"\n", tgt_name.c_str());
}


string XrlRouter::toString() const {
    ostringstream oss;
    if (_fac) {
	oss << " fac enabled: " << _fac->enabled() << " fac connect failed: "
	    << _fac->connect_failed() << " fac connected: " << _fac->connected()
	    << " ready: " << ready() << " pending: " << pending() << endl;
    }
    else {
	oss << " fac NULL, ready: " << ready() << endl;
    }

    int i = 0;
    list<XrlPFListener*>::const_iterator ci;
    for (ci = _listeners.begin(); ci != _listeners.end(); ++ci) {
	XrlPFListener* l = *ci;
	oss << " Listener [" << i << "]  " << l->toString() << endl;
	i++;
    }

    i = 0;
    for (list< ref_ptr<XrlPFSender> >::const_iterator si = _senders.begin();
	 si != _senders.end(); ++si) {
	ref_ptr<XrlPFSender> s = *si;
	oss << " Sender [" << i << "]  " << s->toString() << endl;
    }

    oss << " dispatch-state size: " << _dsl.size() << endl;

    return oss.str();
}



// ----------------------------------------------------------------------------
// wait_until_xrl_router_is_ready

void
wait_until_xrl_router_is_ready(EventLoop& eventloop, XrlRouter& xrl_router)
{
    while (! xrl_router.failed()) {
	eventloop.run();
	if (xrl_router.ready())
	    return;
    }

    ostringstream msg;
    msg << "XrlRouter failed.  No Finder?  xrl_router debug: " << xrl_router.toString() << endl;
    if (xlog_is_running()) {
	XLOG_ERROR("%s", msg.str().c_str());
	xlog_stop();
	xlog_exit();
    } else {
	fprintf(stderr, "%s", msg.str().c_str());
    }
    exit(-1);
}
