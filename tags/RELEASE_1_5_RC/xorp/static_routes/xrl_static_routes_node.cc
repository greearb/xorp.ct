// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/static_routes/xrl_static_routes_node.cc,v 1.40 2007/10/13 01:50:09 pavlin Exp $"

#include "static_routes_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "static_routes_node.hh"
#include "xrl_static_routes_node.hh"

const TimeVal XrlStaticRoutesNode::RETRY_TIMEVAL = TimeVal(1, 0);

XrlStaticRoutesNode::XrlStaticRoutesNode(EventLoop&	eventloop,
					 const string&	class_name,
					 const string&	finder_hostname,
					 uint16_t	finder_port,
					 const string&	finder_target,
					 const string&	fea_target,
					 const string&	rib_target)
    : StaticRoutesNode(eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlStaticRoutesTargetBase(&xrl_router()),
      _eventloop(eventloop),
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
      _is_rib_alive(false),
      _is_rib_registered(false),
      _is_rib_registering(false),
      _is_rib_deregistering(false),
      _is_rib_igp_table4_registered(false),
      _is_rib_igp_table6_registered(false)
{
    _ifmgr.set_observer(dynamic_cast<StaticRoutesNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<StaticRoutesNode*>(this));
}

XrlStaticRoutesNode::~XrlStaticRoutesNode()
{
    shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<StaticRoutesNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<StaticRoutesNode*>(this));
}

int
XrlStaticRoutesNode::startup()
{
    return StaticRoutesNode::startup();
}

int
XrlStaticRoutesNode::shutdown()
{
    return StaticRoutesNode::shutdown();
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
XrlStaticRoutesNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlStaticRoutesNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    StaticRoutesNode::shutdown();
}

//
// Register with the FEA
//
void
XrlStaticRoutesNode::fea_register_startup()
{
    bool success;

    _fea_register_startup_timer.unschedule();
    _fea_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_fea_registered)
	return;		// Already registered

    if (! _is_fea_registering) {
	StaticRoutesNode::incr_startup_requests_n();	// XXX: for the ifmgr

	_is_fea_registering = true;
    }

    //
    // Register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _fea_target,
	callback(this, &XrlStaticRoutesNode::finder_register_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_startup_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlStaticRoutesNode::fea_register_startup));
	return;
    }
}

void
XrlStaticRoutesNode::finder_register_interest_fea_cb(const XrlError& xrl_error)
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
		callback(this, &XrlStaticRoutesNode::fea_register_startup));
	}
	break;
    }
}

//
// De-register with the FEA
//
void
XrlStaticRoutesNode::fea_register_shutdown()
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
	StaticRoutesNode::incr_shutdown_requests_n();	// XXX: for the ifmgr

	_is_fea_deregistering = true;
    }

    //
    // De-register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _fea_target,
	callback(this, &XrlStaticRoutesNode::finder_deregister_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_shutdown_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlStaticRoutesNode::fea_register_shutdown));
	return;
    }

    //
    // XXX: when the shutdown is completed, StaticRoutesNode::status_change()
    // will be called.
    //
    _ifmgr.shutdown();
}

void
XrlStaticRoutesNode::finder_deregister_interest_fea_cb(
    const XrlError& xrl_error)
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
		callback(this, &XrlStaticRoutesNode::fea_register_shutdown));
	}
	break;
    }
}

//
// Register with the RIB
//
void
XrlStaticRoutesNode::rib_register_startup()
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
	    StaticRoutesNode::incr_startup_requests_n();
	if (! _is_rib_igp_table6_registered)
	    StaticRoutesNode::incr_startup_requests_n();

	_is_rib_registering = true;
    }

    //
    // Register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _rib_target,
	callback(this, &XrlStaticRoutesNode::finder_register_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_startup_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlStaticRoutesNode::rib_register_startup));
	return;
    }
}

void
XrlStaticRoutesNode::finder_register_interest_rib_cb(const XrlError& xrl_error)
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
		callback(this, &XrlStaticRoutesNode::rib_register_startup));
	}
	break;
    }
}

//
// De-register with the RIB
//
void
XrlStaticRoutesNode::rib_register_shutdown()
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
	    StaticRoutesNode::incr_shutdown_requests_n();
	if (_is_rib_igp_table6_registered)
	    StaticRoutesNode::incr_shutdown_requests_n();

	_is_rib_deregistering = true;
    }

    //
    // De-register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), xrl_router().instance_name(), _rib_target,
	callback(this, &XrlStaticRoutesNode::finder_deregister_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_shutdown_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlStaticRoutesNode::rib_register_shutdown));
	return;
    }

    send_rib_delete_tables();
}

void
XrlStaticRoutesNode::finder_deregister_interest_rib_cb(
    const XrlError& xrl_error)
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
		callback(this, &XrlStaticRoutesNode::rib_register_shutdown));
	}
	break;
    }
}

//
// Add tables with the RIB
//
void
XrlStaticRoutesNode::send_rib_add_tables()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_rib_igp_table4_registered) {
	success = _xrl_rib_client.send_add_igp_table4(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this,
		     &XrlStaticRoutesNode::rib_client_send_add_igp_table4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv4 IGP table with the RIB. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! _is_rib_igp_table6_registered) {
	success = _xrl_rib_client.send_add_igp_table6(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this,
		     &XrlStaticRoutesNode::rib_client_send_add_igp_table6_cb));
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
	    callback(this, &XrlStaticRoutesNode::send_rib_add_tables));
    }
}

void
XrlStaticRoutesNode::rib_client_send_add_igp_table4_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table4_registered = true;
	send_rib_add_tables();
	StaticRoutesNode::decr_startup_requests_n();
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
		callback(this, &XrlStaticRoutesNode::send_rib_add_tables));
	}
	break;
    }
}

void
XrlStaticRoutesNode::rib_client_send_add_igp_table6_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table6_registered = true;
	send_rib_add_tables();
	StaticRoutesNode::decr_startup_requests_n();
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
		callback(this, &XrlStaticRoutesNode::send_rib_add_tables));
	}
	break;
    }
}

//
// Delete tables with the RIB
//
void
XrlStaticRoutesNode::send_rib_delete_tables()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_delete_igp_table4(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::rib_client_send_delete_igp_table4_cb));
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
	    StaticRoutesNode::protocol_name(),
	    xrl_router().class_name(),
	    xrl_router().instance_name(),
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::rib_client_send_delete_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (! success) {
	StaticRoutesNode::set_status(SERVICE_FAILED);
	StaticRoutesNode::update_status();
    }
}

void
XrlStaticRoutesNode::rib_client_send_delete_igp_table4_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table4_registered = false;
	StaticRoutesNode::decr_shutdown_requests_n();
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
	StaticRoutesNode::decr_shutdown_requests_n();
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
		callback(this, &XrlStaticRoutesNode::rib_register_shutdown));
	}
	break;
    }
}

void
XrlStaticRoutesNode::rib_client_send_delete_igp_table6_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_igp_table6_registered = false;
	StaticRoutesNode::decr_shutdown_requests_n();
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
	StaticRoutesNode::decr_shutdown_requests_n();
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
		callback(this, &XrlStaticRoutesNode::rib_register_shutdown));
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
XrlStaticRoutesNode::common_0_1_get_target_name(
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
XrlStaticRoutesNode::common_0_1_get_version(
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
XrlStaticRoutesNode::common_0_1_get_status(
    // Output values, 
    uint32_t&	status, 
    string&	reason)
{
    status = StaticRoutesNode::node_status(reason);

    return XrlCmdError::OKAY();
}

/**
 *  Request clean shutdown of Xrl Target
 */
XrlCmdError
XrlStaticRoutesNode::common_0_1_shutdown()
{
    string error_msg;

    if (shutdown() != XORP_OK) {
	error_msg = c_format("Failed to shutdown StaticRoutes");
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
XrlStaticRoutesNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&   target_class,
    const string&   target_instance)
{
    if (target_class == _fea_target) {
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	_is_fea_alive = true;
	if (_ifmgr.startup() != XORP_OK) {
	    StaticRoutesNode::ServiceBase::set_status(SERVICE_FAILED);
	    StaticRoutesNode::update_status();
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
XrlStaticRoutesNode::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&   target_class,
    const string&   target_instance)
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
	StaticRoutesNode::shutdown();

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable/start/stop StaticRoutes.
 *
 *  @param enable if true, then enable StaticRoutes, otherwise disable it.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_enable_static_routes(
    // Input values,
    const bool&	enable)
{
    StaticRoutesNode::set_enabled(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_start_static_routes()
{
    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_stop_static_routes()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

/**
 *  Add/replace/delete a static route.
 *
 *  @param unicast if true, then the route would be used for unicast
 *  routing.
 *
 *  @param multicast if true, then the route would be used in the MRIB
 *  (Multicast Routing Information Base) for multicast purpose (e.g.,
 *  computing the Reverse-Path Forwarding information).
 *
 *  @param network the network address prefix this route applies to.
 *
 *  @param nexthop the address of the next-hop router for this route.
 *
 *  @param metric the metric distance for this route.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::add_route4(unicast, multicast, network, nexthop,
				     "", "", metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::add_route6(unicast, multicast, network, nexthop,
				     "", "", metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::replace_route4(unicast, multicast, network, nexthop,
					 "", "", metric, is_backup_route,
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::replace_route6(unicast, multicast, network, nexthop,
					 "", "", metric, is_backup_route,
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::delete_route4(unicast, multicast, network, nexthop,
					"", "", is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::delete_route6(unicast, multicast, network, nexthop,
					"", "", is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Add/replace/delete a backup static route.
 *
 *  @param unicast if true, then the route would be used for unicast
 *  routing.
 *
 *  @param multicast if true, then the route would be used in the MRIB
 *  (Multicast Routing Information Base) for multicast purpose (e.g.,
 *  computing the Reverse-Path Forwarding information).
 *
 *  @param network the network address prefix this route applies to.
 *
 *  @param nexthop the address of the next-hop router for this route.
 *
 *  @param metric the metric distance for this route.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_backup_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::add_route4(unicast, multicast, network, nexthop,
				     "", "", metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_backup_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::add_route6(unicast, multicast, network, nexthop,
				     "", "", metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_backup_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::replace_route4(unicast, multicast, network, nexthop,
					 "", "", metric, is_backup_route,
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_backup_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::replace_route6(unicast, multicast, network, nexthop,
					 "", "", metric, is_backup_route,
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_backup_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::delete_route4(unicast, multicast, network, nexthop,
					"", "", is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_backup_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::delete_route6(unicast, multicast, network, nexthop,
					"", "", is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Add/replace a static route by explicitly specifying the network
 *  interface toward the destination.
 *
 *  @param unicast if true, then the route would be used for unicast
 *  routing.
 *
 *  @param multicast if true, then the route would be used in the MRIB
 *  (Multicast Routing Information Base) for multicast purpose (e.g.,
 *  computing the Reverse-Path Forwarding information).
 *
 *  @param network the network address prefix this route applies to.
 *
 *  @param nexthop the address of the next-hop router for this route.
 *
 *  @param ifname of the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname of the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the metric distance for this route.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::add_route4(unicast, multicast, network, nexthop,
				     ifname, vifname, metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::add_route6(unicast, multicast, network, nexthop,
				     ifname, vifname, metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::replace_route4(unicast, multicast, network, nexthop,
					 ifname, vifname, metric,
					 is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::replace_route6(unicast, multicast, network, nexthop,
					 ifname, vifname, metric,
					 is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::delete_route4(unicast, multicast, network, nexthop,
					ifname, vifname, is_backup_route,
					error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname)
{
    bool is_backup_route = false;
    string error_msg;

    if (StaticRoutesNode::delete_route6(unicast, multicast, network, nexthop,
					ifname, vifname, is_backup_route,
					error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Add/replace/delete a backup static route by explicitly specifying the
 *  network interface toward the destination.
 *
 *  @param unicast if true, then the route would be used for unicast
 *  routing.
 *
 *  @param multicast if true, then the route would be used in the MRIB
 *  (Multicast Routing Information Base) for multicast purpose (e.g.,
 *  computing the Reverse-Path Forwarding information).
 *
 *  @param network the network address prefix this route applies to.
 *
 *  @param nexthop the address of the next-hop router for this route.
 *
 *  @param ifname of the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname of the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the metric distance for this route.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_backup_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::add_route4(unicast, multicast, network, nexthop,
				     ifname, vifname, metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_backup_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::add_route6(unicast, multicast, network, nexthop,
				     ifname, vifname, metric, is_backup_route,
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_backup_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::replace_route4(unicast, multicast, network, nexthop,
					 ifname, vifname, metric,
					 is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_backup_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::replace_route6(unicast, multicast, network, nexthop,
					 ifname, vifname, metric,
					 is_backup_route, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_backup_interface_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::delete_route4(unicast, multicast, network, nexthop,
					ifname, vifname, is_backup_route,
					error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_backup_interface_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname)
{
    bool is_backup_route = true;
    string error_msg;

    if (StaticRoutesNode::delete_route6(unicast, multicast, network, nexthop,
					ifname, vifname, is_backup_route,
					error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable the StaticRoutes trace log for all operations.
 *
 *  @param enable if true, then enable the trace log, otherwise disable it.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_enable_log_trace_all(
    // Input values,
    const bool&	enable)
{
    StaticRoutesNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

/**
 * Inform the RIB about a route change.
 *
 * @param static_route the route with the information about the change.
 */
void
XrlStaticRoutesNode::inform_rib_route_change(const StaticRoute& static_route)
{
    // Add the request to the queue
    _inform_rib_queue.push_back(static_route);

    // If the queue was empty before, start sending the routes
    if (_inform_rib_queue.size() == 1) {
	send_rib_route_change();
    }
}

/**
 * Cancel a pending request to inform the RIB about a route change.
 *
 * @param static_route the route with the request that would be canceled.
 */
void
XrlStaticRoutesNode::cancel_rib_route_change(const StaticRoute& static_route)
{
    list<StaticRoute>::iterator iter;

    for (iter = _inform_rib_queue.begin();
	 iter != _inform_rib_queue.end();
	 ++iter) {
	StaticRoute& tmp_static_route = *iter;
	if (tmp_static_route == static_route)
	    tmp_static_route.set_ignored(true);
    }
}

void
XrlStaticRoutesNode::send_rib_route_change()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    do {
	// Pop-up all routes that are to be ignored
	if (_inform_rib_queue.empty())
	    return;		// No more route changes to send

	StaticRoute& tmp_static_route = _inform_rib_queue.front();
	if (tmp_static_route.is_ignored()) {
	    _inform_rib_queue.pop_front();
	    continue;
	}
	break;
    } while (true);

    StaticRoute& static_route = _inform_rib_queue.front();

    //
    // Check whether we have already registered with the RIB
    //
    if (static_route.is_ipv4() && (! _is_rib_igp_table4_registered)) {
	success = false;
	goto start_timer_label;
    }

    if (static_route.is_ipv6() && (! _is_rib_igp_table6_registered)) {
	success = false;
	goto start_timer_label;
    }

    //
    // Send the appropriate XRL
    //
    if (static_route.is_add_route()) {
	if (static_route.is_ipv4()) {
	    if (static_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route4(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv4net(),
		    static_route.nexthop().get_ipv4(),
		    static_route.ifname(),
		    static_route.vifname(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route4(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv4net(),
		    static_route.nexthop().get_ipv4(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (static_route.is_ipv6()) {
	    if (static_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route6(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv6net(),
		    static_route.nexthop().get_ipv6(),
		    static_route.ifname(),
		    static_route.vifname(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route6(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv6net(),
		    static_route.nexthop().get_ipv6(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }
    
    if (static_route.is_replace_route()) {
	if (static_route.is_ipv4()) {
	    if (static_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route4(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv4net(),
		    static_route.nexthop().get_ipv4(),
		    static_route.ifname(),
		    static_route.vifname(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route4(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv4net(),
		    static_route.nexthop().get_ipv4(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (static_route.is_ipv6()) {
	    if (static_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route6(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv6net(),
		    static_route.nexthop().get_ipv6(),
		    static_route.ifname(),
		    static_route.vifname(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route6(
		    _rib_target.c_str(),
		    StaticRoutesNode::protocol_name(),
		    static_route.unicast(),
		    static_route.multicast(),
		    static_route.network().get_ipv6net(),
		    static_route.nexthop().get_ipv6(),
		    static_route.metric(),
		    static_route.policytags().xrl_atomlist(),
		    callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }

    if (static_route.is_delete_route()) {
	if (static_route.is_ipv4()) {
	    success = _xrl_rib_client.send_delete_route4(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv4net(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
		if (success)
		    return;
	}
	if (static_route.is_ipv6()) {
	    success = _xrl_rib_client.send_delete_route6(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv6net(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
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
		   (static_route.is_add_route())? "add"
		   : (static_route.is_replace_route())? "replace"
		   : "delete",
		   static_route.network().str().c_str());
    start_timer_label:
	_inform_rib_queue_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlStaticRoutesNode::send_rib_route_change));
    }
}

void
XrlStaticRoutesNode::send_rib_route_change_cb(const XrlError& xrl_error)
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
		callback(this, &XrlStaticRoutesNode::send_rib_route_change));
	}
	break;
    }
}

XrlCmdError
XrlStaticRoutesNode::policy_backend_0_1_configure(const uint32_t& filter,
						  const string& conf)
{
    try {
	StaticRoutesNode::configure_filter(filter, conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::policy_backend_0_1_reset(const uint32_t& filter)
{
    try {
	StaticRoutesNode::reset_filter(filter);
    } catch(const PolicyException& e) {
	// Will never happen... but for the future...
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " + e.str());
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::policy_backend_0_1_push_routes()
{
    StaticRoutesNode::push_routes(); 
    return XrlCmdError::OKAY();
}
