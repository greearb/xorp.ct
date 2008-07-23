// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fib2mrib/xrl_fib2mrib_node.cc,v 1.41 2008/01/04 03:16:17 pavlin Exp $"

#include "fib2mrib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "fib2mrib_node.hh"
#include "xrl_fib2mrib_node.hh"

const TimeVal XrlFib2mribNode::RETRY_TIMEVAL = TimeVal(1, 0);

XrlFib2mribNode::XrlFib2mribNode(EventLoop&	eventloop,
				 const string&	class_name,
				 const string&	finder_hostname,
				 uint16_t	finder_port,
				 const string&	finder_target,
				 const string&	fea_target,
				 const string&	rib_target)
    : Fib2mribNode(eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlFib2mribTargetBase(&xrl_router()),
      _eventloop(eventloop),
      _xrl_fea_fti_client(&xrl_router()),
      _xrl_fea_fib_client(&xrl_router()),
      _xrl_rib_client(&xrl_router()),
      _finder_target(finder_target),
      _fea_target(fea_target),
      _rib_target(rib_target),
      _ifmgr(eventloop, fea_target.c_str(), xrl_router().finder_address(),
	     xrl_router().finder_port()),
      _xrl_finder_client(&xrl_router()),
      _is_finder_alive(false),
      _is_fea_alive(false),
      _is_fea_registered(false),
      _is_fea_registering(false),
      _is_fea_deregistering(false),
      _is_fea_have_ipv4_tested(false),
      _is_fea_have_ipv6_tested(false),
      _fea_have_ipv4(false),
      _fea_have_ipv6(false),
      _is_fea_fib_client4_registered(false),
      _is_fea_fib_client6_registered(false),
      _is_rib_alive(false),
      _is_rib_registered(false),
      _is_rib_registering(false),
      _is_rib_deregistering(false),
      _is_rib_igp_table4_registered(false),
      _is_rib_igp_table6_registered(false)
{
    _ifmgr.set_observer(dynamic_cast<Fib2mribNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
}

XrlFib2mribNode::~XrlFib2mribNode()
{
    shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<Fib2mribNode*>(this));
}

int
XrlFib2mribNode::startup()
{
    return (Fib2mribNode::startup());
}

int
XrlFib2mribNode::shutdown()
{
    return (Fib2mribNode::shutdown());
}

//
// Finder-related events
//
/**
 * Called when Finder connection is established.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlFib2mribNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlFib2mribNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    Fib2mribNode::shutdown();
}

//
// Register with the FEA
//
void
XrlFib2mribNode::fea_register_startup()
{
    bool success;

    _fea_register_startup_timer.unschedule();
    _fea_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_fea_registered)
	return;		// Already registered

    if (! _is_fea_registering) {
	Fib2mribNode::incr_startup_requests_n();	// XXX: for the ifmgr

	if (! _is_fea_fib_client4_registered)
	    Fib2mribNode::incr_startup_requests_n();
	if (! _is_fea_fib_client6_registered)
	    Fib2mribNode::incr_startup_requests_n();

	_is_fea_registering = true;
    }

    //
    // Register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _fea_target,
	callback(this, &XrlFib2mribNode::finder_register_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_startup_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::fea_register_startup));
	return;
    }
}

void
XrlFib2mribNode::finder_register_interest_fea_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then the FEA birth event will startup the ifmgr
	//
	_is_fea_registering = false;
	_is_fea_registered = true;
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot register interest in Finder events: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_register_startup_timer.scheduled()) {
	    XLOG_ERROR("Failed to register interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_register_startup_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::fea_register_startup));
	}
	break;
    }
}

//
// De-register with the FEA
//
void
XrlFib2mribNode::fea_register_shutdown()
{
    bool success;

    _fea_register_startup_timer.unschedule();
    _fea_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_fea_alive)
	return;		// The FEA is not there anymore

    if (! _is_fea_registered)
	return;		// Not registered

    if (! _is_fea_deregistering) {
	Fib2mribNode::incr_shutdown_requests_n();	// XXX: for the ifmgr

	if (_is_fea_fib_client4_registered)
	    Fib2mribNode::incr_shutdown_requests_n();
	if (_is_fea_fib_client6_registered)
	    Fib2mribNode::incr_shutdown_requests_n();

	_is_fea_deregistering = true;
    }

    //
    // De-register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _fea_target,
	callback(this, &XrlFib2mribNode::finder_deregister_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_shutdown_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::fea_register_shutdown));
	return;
    }

    //
    // XXX: when the shutdown is completed, Fib2mribNode::status_change()
    // will be called.
    //
    _ifmgr.shutdown();

    send_fea_delete_fib_client();
}

void
XrlFib2mribNode::finder_deregister_interest_fea_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_deregistering = false;
	_is_fea_registered = false;
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot deregister interest in Finder events: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_fea_deregistering = false;
	_is_fea_registered = false;
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to deregister interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::fea_register_shutdown));
	}
	break;
    }
}

//
// Register with the RIB
//
void
XrlFib2mribNode::rib_register_startup()
{
    bool success;

    _rib_register_startup_timer.unschedule();
    _rib_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_rib_registered)
	return;		// Already registered

    if (! _is_rib_registering) {
	if (! _is_rib_igp_table4_registered)
	    Fib2mribNode::incr_startup_requests_n();
	if (! _is_rib_igp_table6_registered)
	    Fib2mribNode::incr_startup_requests_n();

	_is_rib_registering = true;
    }

    //
    // Register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _rib_target,
	callback(this, &XrlFib2mribNode::finder_register_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_startup_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::rib_register_startup));
	return;
    }
}

void
XrlFib2mribNode::finder_register_interest_rib_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then the RIB birth event will startup the RIB
	// registration.
	//
	_is_rib_registering = false;
	_is_rib_registered = true;
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot register interest in Finder events: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_startup_timer.scheduled()) {
	    XLOG_ERROR("Failed to register interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_startup_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_startup));
	}
	break;
    }
}

//
// De-register with the RIB
//
void
XrlFib2mribNode::rib_register_shutdown()
{
    bool success;

    _rib_register_startup_timer.unschedule();
    _rib_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_rib_alive)
	return;		// The RIB is not there anymore

    if (! _is_rib_registered)
	return;		// Not registered

    if (! _is_rib_deregistering) {
	if (_is_rib_igp_table4_registered)
	    Fib2mribNode::incr_shutdown_requests_n();
	if (_is_rib_igp_table6_registered)
	    Fib2mribNode::incr_shutdown_requests_n();

	_is_rib_deregistering = true;
    }

    //
    // De-register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _rib_target,
	callback(this, &XrlFib2mribNode::finder_deregister_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_shutdown_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::rib_register_shutdown));
	return;
    }

    send_rib_delete_tables();
}

void
XrlFib2mribNode::finder_deregister_interest_rib_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_deregistering = false;
	_is_rib_registered = false;
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot deregister interest in Finder events: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_rib_deregistering = false;
	_is_rib_registered = false;
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to deregister interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	}
	break;
    }
}

//
// Register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_add_fib_client()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    //
    // Test whether the underlying system supports IPv4
    //
    if (! _is_fea_have_ipv4_tested) {
	success = _xrl_fea_fti_client.send_have_ipv4(
	    _fea_target.c_str(),
	    callback(this, &XrlFib2mribNode::fea_fti_client_send_have_ipv4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to test using the FEA whether the system "
		   "supports IPv4. "
		   "Will try again.");
	goto start_timer_label;
    }

    //
    // Test whether the underlying system supports IPv6
    //
    if (! _is_fea_have_ipv6_tested) {
	success = _xrl_fea_fti_client.send_have_ipv6(
	    _fea_target.c_str(),
	    callback(this, &XrlFib2mribNode::fea_fti_client_send_have_ipv6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to test using the FEA whether the system "
		   "supports IPv6. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (_fea_have_ipv4 && ! _is_fea_fib_client4_registered) {
	success = _xrl_fea_fib_client.send_add_fib_client4(
	    _fea_target.c_str(),
	    xrl_router().class_name(),
	    true,		/* send_updates */
	    false,		/* send_resolves */
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_add_fib_client4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv4 FIB client with the FEA. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (_fea_have_ipv6 && ! _is_fea_fib_client6_registered) {
	success = _xrl_fea_fib_client.send_add_fib_client6(
	    _fea_target.c_str(),
	    xrl_router().class_name(),
	    true,		/* send_updates */
	    false,		/* send_resolves */
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_add_fib_client6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv6 FIB client with the FEA. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
    start_timer_label:
	_fea_fib_client_registration_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
    }
}

void
XrlFib2mribNode::fea_fti_client_send_have_ipv4_cb(const XrlError& xrl_error,
						  const bool* result)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_have_ipv4_tested = true;
	_fea_have_ipv4 = *result;
	send_fea_add_fib_client();
	// XXX: if the underying system doesn't support IPv4, then we are done
	if (! _fea_have_ipv4)
	    Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot test using the FEA whether the system "
		   "supports IPv4: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_fib_client_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to test using the FEA whether the system "
		       "supports IPv4: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_fib_client_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
	}
	break;
    }
}

void
XrlFib2mribNode::fea_fti_client_send_have_ipv6_cb(const XrlError& xrl_error,
						  const bool* result)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_have_ipv6_tested = true;
	_fea_have_ipv6 = *result;
	send_fea_add_fib_client();
	// XXX: if the underying system doesn't support IPv6, then we are done
	if (! _fea_have_ipv6)
	    Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot test using the FEA whether the system "
		   "supports IPv6: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_fib_client_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to test using the FEA whether the system "
		       "supports IPv6: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_fib_client_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
	}
	break;
    }
}

void
XrlFib2mribNode::fea_fib_client_send_add_fib_client4_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_fib_client4_registered = true;
	send_fea_add_fib_client();
	Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add IPv4 FIB client to the FEA: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_fib_client_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to add IPv4 FIB client to the FEA: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_fib_client_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
	}
	break;
    }
}

void
XrlFib2mribNode::fea_fib_client_send_add_fib_client6_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_fib_client6_registered = true;
	send_fea_add_fib_client();
	Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add IPv6 FIB client to the FEA: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _fea_fib_client_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to add IPv6 FIB client to the FEA: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _fea_fib_client_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
	}
	break;
    }
}

//
// De-register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_delete_fib_client()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_fea_fib_client4_registered) {
	bool success4;
	success4 = _xrl_fea_fib_client.send_delete_fib_client4(
	    _fea_target.c_str(),
	    xrl_router().class_name(),
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_delete_fib_client4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_fea_fib_client6_registered) {
	bool success6;
	success6 = _xrl_fea_fib_client.send_delete_fib_client6(
	    _fea_target.c_str(),
	    xrl_router().class_name(),
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_delete_fib_client6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (! success) {
	Fib2mribNode::set_status(SERVICE_FAILED);
	Fib2mribNode::update_status();
    }
}

void
XrlFib2mribNode::fea_fib_client_send_delete_fib_client4_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_have_ipv4_tested = false;
	_fea_have_ipv4 = false;
	_is_fea_fib_client4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Failed to deregister IPv4 FIB client with the FEA: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_fea_have_ipv4_tested = false;
	_fea_have_ipv4 = false;
	_is_fea_fib_client4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Cannot deregister IPv4 FIB client with the FEA: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	}
	break;
    }
}

void
XrlFib2mribNode::fea_fib_client_send_delete_fib_client6_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_fea_have_ipv6_tested = false;
	_fea_have_ipv6 = false;
	_is_fea_fib_client6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Failed to deregister IPv6 FIB client with the FEA: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_fea_have_ipv6_tested = false;
	_fea_have_ipv6 = false;
	_is_fea_fib_client6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Cannot deregister IPv6 FIB client with the FEA: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	}
	break;
    }
}

//
// Add tables with the RIB
//
void
XrlFib2mribNode::send_rib_add_tables()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_rib_igp_table4_registered) {
	success = _xrl_rib_client.send_add_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_add_igp_table4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv4 IGP table with the RIB. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! _is_rib_igp_table6_registered) {
	success = _xrl_rib_client.send_add_igp_table6(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_add_igp_table6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv6 IGP table with the RIB. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
    start_timer_label:
	_rib_igp_table_registration_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_rib_add_tables));
    }
}

void
XrlFib2mribNode::rib_client_send_add_igp_table4_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table4_registered = true;
	send_rib_add_tables();
	Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add IPv4 IGP table to the RIB: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_igp_table_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to add IPv4 IGP table to the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_igp_table_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_rib_add_tables));
	}
	break;
    }
}

void
XrlFib2mribNode::rib_client_send_add_igp_table6_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table6_registered = true;
	send_rib_add_tables();
	Fib2mribNode::decr_startup_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add IPv6 IGP table to the RIB: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_igp_table_registration_timer.scheduled()) {
	    XLOG_ERROR("Failed to add IPv6 IGP table to the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_igp_table_registration_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_rib_add_tables));
	}
	break;
    }
}

//
// Delete tables with the RIB
//
void
XrlFib2mribNode::send_rib_delete_tables()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_delete_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_delete_igp_table4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_rib_igp_table6_registered) {
	bool success6;
	success6 = _xrl_rib_client.send_delete_igp_table6(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_delete_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (! success) {
	Fib2mribNode::set_status(SERVICE_FAILED);
	Fib2mribNode::update_status();
    }
}

void
XrlFib2mribNode::rib_client_send_delete_igp_table4_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot deregister IPv4 IGP table with the RIB: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_rib_igp_table4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	}
	break;
    }
}

void
XrlFib2mribNode::rib_client_send_delete_igp_table6_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot deregister IPv6 IGP table with the RIB: %s",
		   xrl_error.str().c_str());
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	_is_rib_igp_table6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	}
	break;
    }
}


//
// XRL target methods
//

/**
 *  Get name of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_target_name(
    // Output values, 
    string&	name)
{
    name = xrl_router().class_name();

    return XrlCmdError::OKAY();
}

/**
 *  Get version string from Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_version(
    // Output values, 
    string&	version)
{
    version = XORP_MODULE_VERSION;

    return XrlCmdError::OKAY();
}

/**
 *  Get status of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_status(
    // Output values, 
    uint32_t&	status, 
    string&	reason)
{
    status = Fib2mribNode::node_status(reason);

    return XrlCmdError::OKAY();
}

/**
 *  Request clean shutdown of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_shutdown()
{
    string error_msg;

    if (shutdown() != XORP_OK) {
	error_msg = c_format("Failed to shutdown Fib2mrib");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Announce target birth to observer.
 *
 *  @param target_class the target class name.
 *
 *  @param target_instance the target instance name.
 */
XrlCmdError
XrlFib2mribNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    if (target_class == _fea_target) {
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	_is_fea_alive = true;
	if (_ifmgr.startup() != XORP_OK) {
	    Fib2mribNode::ServiceBase::set_status(SERVICE_FAILED);
	    Fib2mribNode::update_status();
	} else {
	    send_fea_add_fib_client();
	}
    }

    if (target_class == _rib_target) {
	_is_rib_alive = true;
	send_rib_add_tables();
    }

    return XrlCmdError::OKAY();
    UNUSED(target_instance);
}

/**
 *  Announce target death to observer.
 *
 *  @param target_class the target class name.
 *
 *  @param target_instance the target instance name.
 */
XrlCmdError
XrlFib2mribNode::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    bool do_shutdown = false;

    if (target_class == _fea_target) {
	XLOG_ERROR("FEA (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_fea_alive = false;
	do_shutdown = true;
    }

    if (target_class == _rib_target) {
	XLOG_ERROR("RIB (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_rib_alive = false;
	do_shutdown = true;
    }

    if (do_shutdown)
	Fib2mribNode::shutdown();

    return XrlCmdError::OKAY();
}

/**
 *  Add a route.
 *
 *  @param network the network address prefix of the route to add.
 *
 *  @param nexthop the address of the next-hop router toward the
 *  destination.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the routing metric toward the destination.
 *
 *  @param admin_distance the administratively defined distance toward the
 *  destination.
 *
 *  @param protocol_origin the name of the protocol that originated this
 *  route.
 *
 *  @param xorp_route true if this route was installed by XORP.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_add_route4(): "
	      "network = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s "
	      "xorp_route = %s\n",
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str(),
	      bool_c_str(xorp_route));

    if (Fib2mribNode::add_route4(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_add_route6(): "
	      "network = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s "
	      "xorp_route = %s\n",
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str(),
	      bool_c_str(xorp_route));

    if (Fib2mribNode::add_route6(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Replace a route.
 *
 *  @param network the network address prefix of the route to replace.
 *
 *  @param nexthop the address of the next-hop router toward the
 *  destination.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the routing metric toward the destination.
 *
 *  @param admin_distance the administratively defined distance toward the
 *  destination.
 *
 *  @param protocol_origin the name of the protocol that originated this
 *  route.
 *
 *  @param xorp_route true if this route was installed by XORP.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_replace_route4(): "
	      "network = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s "
	      "xorp_route = %s\n",
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str(),
	      bool_c_str(xorp_route));

    if (Fib2mribNode::replace_route4(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_replace_route6(): "
	      "network = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s "
	      "xorp_route = %s\n",
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str(),
	      bool_c_str(xorp_route));

    if (Fib2mribNode::replace_route6(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Delete a route.
 *
 *  @param network the network address prefix of the route to delete.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route4(
    // Input values,
    const IPv4Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_delete_route4(): "
	      "network = %s ifname = %s vifname = %s\n",
	      network.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str());

    if (Fib2mribNode::delete_route4(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route6(
    // Input values,
    const IPv6Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    debug_msg("fea_fib_client_0_1_delete_route6(): "
	      "network = %s ifname = %s vifname = %s\n",
	      network.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str());

    if (Fib2mribNode::delete_route6(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Route resolve notification.
 *
 *  @param network the network address prefix of the lookup
 *  which failed or for which upper layer intervention is
 *  requested from the FIB.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_resolve_route4(
    // Input values,
    const IPv4Net&	network)
{
    UNUSED(network);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_resolve_route6(
    // Input values,
    const IPv6Net&	network)
{
    UNUSED(network);
    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable/start/stop Fib2mrib.
 *
 *  @param enable if true, then enable Fib2mrib, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_fib2mrib(
    // Input values,
    const bool&	enable)
{
    Fib2mribNode::set_enabled(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_start_fib2mrib()
{
    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_stop_fib2mrib()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable the Fib2mrib trace log for all operations.
 *
 *  @param enable if true, then enable the trace log, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_log_trace_all(
    // Input values,
    const bool&	enable)
{
    Fib2mribNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

/**
 * Inform the RIB about a route change.
 *
 * @param fib2mrib_route the route with the information about the change.
 */
void
XrlFib2mribNode::inform_rib_route_change(const Fib2mribRoute& fib2mrib_route)
{
    // Add the request to the queue
    _inform_rib_queue.push_back(fib2mrib_route);

    // If the queue was empty before, start sending the routes
    if (_inform_rib_queue.size() == 1) {
	send_rib_route_change();
    }
}

/**
 * Cancel a pending request to inform the RIB about a route change.
 *
 * @param fib2mrib_route the route with the request that would be canceled.
 */
void
XrlFib2mribNode::cancel_rib_route_change(const Fib2mribRoute& fib2mrib_route)
{
    list<Fib2mribRoute>::iterator iter;

    for (iter = _inform_rib_queue.begin();
	 iter != _inform_rib_queue.end();
	 ++iter) {
	Fib2mribRoute& tmp_fib2mrib_route = *iter;
	if (tmp_fib2mrib_route == fib2mrib_route)
	    tmp_fib2mrib_route.set_ignored(true);
    }
}

void
XrlFib2mribNode::send_rib_route_change()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    do {
	// Pop-up all routes that are to be ignored
	if (_inform_rib_queue.empty())
	    return;		// No more route changes to send

	Fib2mribRoute& tmp_fib2mrib_route = _inform_rib_queue.front();
	if (tmp_fib2mrib_route.is_ignored()) {
	    _inform_rib_queue.pop_front();
	    continue;
	}
	break;
    } while (true);

    Fib2mribRoute& fib2mrib_route = _inform_rib_queue.front();

    //
    // Check whether we have already registered with the RIB
    //
    if (fib2mrib_route.is_ipv4() && (! _is_rib_igp_table4_registered)) {
	success = false;
	goto start_timer_label;
    }

    if (fib2mrib_route.is_ipv6() && (! _is_rib_igp_table6_registered)) {
	success = false;
	goto start_timer_label;
    }

    //
    // Send the appropriate XRL
    //
    if (fib2mrib_route.is_add_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (fib2mrib_route.is_ipv6()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }

    if (fib2mrib_route.is_replace_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (fib2mrib_route.is_ipv6()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.metric(),
		    fib2mrib_route.policytags().xrl_atomlist(),
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }

    if (fib2mrib_route.is_delete_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    success = _xrl_rib_client.send_delete_route4(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv4net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	    if (success)
		return;
	}
	if (fib2mrib_route.is_ipv6()) {
	    success = _xrl_rib_client.send_delete_route6(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv6net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	XLOG_ERROR("Failed to %s route for %s with the RIB. "
		   "Will try again.",
		   (fib2mrib_route.is_add_route())? "add"
		   : (fib2mrib_route.is_replace_route())? "replace"
		   : "delete",
		   fib2mrib_route.network().str().c_str());
    start_timer_label:
	_inform_rib_queue_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_rib_route_change));
    }
}

void
XrlFib2mribNode::send_rib_route_change_cb(const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then send the next route change
	//
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it,
	// then print an error and send the next one.
	//
	XLOG_ERROR("Cannot %s a routing entry with the RIB: %s",
		   (_inform_rib_queue.front().is_add_route())? "add"
		   : (_inform_rib_queue.front().is_replace_route())? "replace"
		   : "delete",
		   xrl_error.str().c_str());
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	break;

    case NO_FINDER:
    case RESOLVE_FAILED:
    case SEND_FAILED:
	//
	// A communication error that should have been caught elsewhere
	// (e.g., by tracking the status of the Finder and the other targets).
	// Probably we caught it here because of event reordering.
	// In some cases we print an error. In other cases our job is done.
	//
	XLOG_ERROR("Cannot %s a routing entry with the RIB: %s",
		   (_inform_rib_queue.front().is_add_route())? "add"
		   : (_inform_rib_queue.front().is_replace_route())? "replace"
		   : "delete",
		   xrl_error.str().c_str());
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:
	//
	// An error that should happen only if there is something unusual:
	// e.g., there is XRL mismatch, no enough internal resources, etc.
	// We don't try to recover from such errors, hence this is fatal.
	//
	XLOG_FATAL("Fatal XRL error: %s", xrl_error.str().c_str());
	break;

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then start a timer to try again
	// (unless the timer is already running).
	//
	if (! _inform_rib_queue_timer.scheduled()) {
	    XLOG_ERROR("Failed to %s a routing entry with the RIB: %s. "
		       "Will try again.",
		       (_inform_rib_queue.front().is_add_route())? "add"
		       : (_inform_rib_queue.front().is_replace_route())? "replace"
		       : "delete",
		       xrl_error.str().c_str());
	    _inform_rib_queue_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_rib_route_change));
	}
	break;
    }
}

XrlCmdError
XrlFib2mribNode::policy_backend_0_1_configure(const uint32_t& filter,
					      const string& conf)
{
    try {
	Fib2mribNode::configure_filter(filter, conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::policy_backend_0_1_reset(const uint32_t& filter)
{
    try {
	Fib2mribNode::reset_filter(filter);
    } catch(const PolicyException& e) {
	// Will never happen... but for the future...
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " + e.str());
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::policy_backend_0_1_push_routes()
{
    Fib2mribNode::push_routes(); 
    return XrlCmdError::OKAY();
}
