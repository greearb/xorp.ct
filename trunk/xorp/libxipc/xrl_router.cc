// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/xrl_router.cc,v 1.11 2003/03/09 00:55:38 hodson Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/xlog.h"

#include "xrl_error.hh"
#include "xrl_router.hh"
#include "xrl_pf.hh"
#include "xrl_pf_factory.hh"

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

#ifdef ORIGINAL_FINDER

// ----------------------------------------------------------------------------
// Xrl Resolve and return handling

struct DispatchState {
    // DispatchState objects are created in XrlRouter::send and destroyed
    // in XrlRouter::resolve_callback or XrlRouter::send_callback depending
    // on whether resolve is successful or not.
    XrlRouter*			_router;
    Xrl				_xrl;
    XrlRouter::XrlCallback	_cb;
    XrlPFSender*		_sender;
    DispatchState(XrlRouter* r, const Xrl& x, XrlRouter::XrlCallback cb)
	: _router(r), _xrl(x), _cb(cb), _sender(0) {}
    ~DispatchState()	{ if (_sender) delete _sender; }
};

void
XrlRouter::send_callback(const XrlError& e,
			 const Xrl&	 /* xrl */,
			 XrlArgs* 	 ret,
			 DispatchState*	 ds)
{
    ds->_cb->dispatch(e, ret);

    ds->_router->_sends_pending--;
    if (e != XrlError::OKAY() && e != XrlError::COMMAND_FAILED()) {
	FinderClient& fc = ds->_router->_fc;
	fc.invalidate(ds->_xrl.target());
    }
    delete ds;
}

void
XrlRouter::resolve_callback(FinderClient::Error	err,
			    const char*		/* name - ignored */,
			    const char*		value,
			    DispatchState*	ds)
{

    ds->_router->_finder_lookups_pending--;

    switch (err) {
    case FinderClient::FC_OKAY:
	{
	    debug_msg("Resolved: %s\n", value);
	    XrlPFSender* s = XrlPFSenderFactory::create(ds->_router->_event_loop,
							value);
	    if (s) {
		ds->_sender = s;
		ds->_router->_sends_pending++;
		s->send(ds->_xrl,
			callback(&XrlRouter::send_callback, ds));
		trace_xrl("Sending ", ds->_xrl);
		return;
	    } else {
		trace_xrl("Sender construction failed on ", ds->_xrl);
		ds->_cb->dispatch(XrlError::SEND_FAILED(), 0);
	    }
	}
	break;
    case FinderClient::FC_LOOKUP_FAILED:
	trace_xrl("Resolve failed on ", ds->_xrl);
	ds->_cb->dispatch(XrlError::RESOLVE_FAILED(), 0);
	break;
    case FinderClient::FC_NO_SERVER:
	trace_xrl("Finder or Finder connection died when execution reached ",
		  ds->_xrl);
	ds->_cb->dispatch(XrlError::NO_FINDER(), 0);
	break;
    default:
	trace_xrl(c_format("FinderClient error = %d when sending ", err), 
		  ds->_xrl);
	ds->_cb->dispatch(XrlError::FAILED_UNKNOWN(), 0);
    }
    delete ds;
}

bool
XrlRouter::send(const Xrl&      	xrl,
		const XrlCallback&	cb)
{
    DispatchState* ds = new DispatchState(this, xrl, cb);

    ds->_router->_finder_lookups_pending++;

    if (xrl.is_resolved()) {
	assert(0 && "Dispatch of resolved XRL's thru XrlRouter TBD.");
    }

    _fc.lookup(xrl.target().c_str(),
	       callback(&XrlRouter::resolve_callback, ds));

    return true;	// XXX This needs correcting.
}

bool
XrlRouter::pending() const
{
    return 0 != _sends_pending || 0 != _finder_lookups_pending;
}

XrlRouter::~XrlRouter()
{
    assert(_finder_lookups_pending >= 0 && _sends_pending >= 0);

    // XXX XrlRouter's are not intended to be dynamic...
    if (_finder_lookups_pending > 0 ||
	_sends_pending > 0) {

	fprintf(stderr, "XrlRouter deleted with transactions pending.\n");

	bool pends_failed = false;
	XorpTimer t = _event_loop.set_flag_after_ms(10000, &pends_failed);

	while ((_finder_lookups_pending > 0 || _sends_pending > 0) &&
	      pends_failed == false) {
	    _event_loop.run();
	}

	if (_sends_pending > 0 || _finder_lookups_pending > 0) {
	    fprintf(stderr, "XrlRouter transactions did not complete.\n");
	    abort();
	}
	fprintf(stderr, "XrlRouter transactions completed.\n");
    }
}

// ----------------------------------------------------------------------------
// Registration with finder related

static const int FINDER_REGISTER_PENDING = 100;
static const int FINDER_REGISTER_SUCCESS = 101;
static const int FINDER_REGISTER_FAILURE = 102;

void
XrlRouter::finder_register_callback(FinderClient::Error	e,
				    const char* 	/* name */,
				    const char* 	/* value */,
				    int*		success) {
    if (e == FinderClient::FC_OKAY) {
	*success = FINDER_REGISTER_SUCCESS;
    } else {
	debug_msg("Failed to register with finder %d\n", (int)e);
	*success = FINDER_REGISTER_FAILURE;
    }
}

bool
XrlRouter::add_listener(XrlPFListener* pfl)
{

    bool timed_out = false;
    XorpTimer timeout = _event_loop.set_flag_after_ms(3000, &timed_out);

    int frs = FINDER_REGISTER_PENDING;

    string protocol_colon_address =
	string(pfl->protocol()) + string(":") + string(pfl->address());

    _fc.add(name().c_str(), protocol_colon_address.c_str(),
	    callback(&XrlRouter::finder_register_callback, &frs));

    /*
     * XXX This is really bad and esp since I've lectured people on doing
     * this!!
     */
    while (frs == FINDER_REGISTER_PENDING) {
	_event_loop.run();
    }

    if (frs == FINDER_REGISTER_SUCCESS) {
	debug_msg("Registered with finder mapping %s to %s",
		  name().c_str(), protocol_colon_address.c_str());
	_listeners.push_back(pfl);
	pfl->set_dispatcher(this);
	return true;
    } else {
	debug_msg("Failed to register finder mapping %s to %s",
		  name().c_str(), protocol_colon_address.c_str());
	return false;
    }
}

void
XrlRouter::finalize()
{
}

///////////////////////////////////////////////////////////////////////////////
#else // ndef ORIGINAL_FINDER
///////////////////////////////////////////////////////////////////////////////

#include "finder_ng_client.hh"
#include "finder_ng_client_xrl_target.hh"
#include "finder_tcp_messenger.hh"

#include "sockutil.hh"

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
	ds->dispatch(XrlError::CORRUPT_XRL());
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

#endif // ORIGINAL_FINDER
