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

#ident "$XORP: xorp/libxipc/xrl_router.cc,v 1.5 2003/01/10 00:30:24 hodson Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/xlog.h"

#include "xrl_router.hh"
#include "xrl_pf.hh"
#include "xrl_pf_factory.hh"

inline void
trace_xrl(const string& preamble, const Xrl& xrl)
{
    static const char* do_trace = getenv("XRLTRACE");

    if (do_trace) {
	string s(preamble + xrl.str());
	XLOG_INFO(s.c_str());
    }
}

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
	pfl->set_command_map(this);
	return true;
    } else {
	debug_msg("Failed to register finder mapping %s to %s",
		  name().c_str(), protocol_colon_address.c_str());
	return false;
    }
}









