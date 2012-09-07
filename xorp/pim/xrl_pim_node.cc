// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "pim_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"
#include "libxorp/utils.hh"

#include "pim_mfc.hh"
#include "pim_node.hh"
#include "pim_node_cli.hh"
#include "pim_mrib_table.hh"
#include "pim_vif.hh"
#include "xrl_pim_node.hh"

const TimeVal XrlPimNode::RETRY_TIMEVAL = TimeVal(1, 0);

//
// XrlPimNode front-end interface
//

XrlPimNode::XrlPimNode(int		family,
		       xorp_module_id	module_id,
		       EventLoop&	eventloop,
		       const string&	class_name,
		       const string&	finder_hostname,
		       uint16_t		finder_port,
		       const string&	finder_target,
		       const string&	fea_target,
		       const string&	mfea_target,
		       const string&	rib_target,
		       const string&	mld6igmp_target)
    : PimNode(family, module_id, eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlPimTargetBase(&xrl_router()),
      PimNodeCli(*static_cast<PimNode *>(this)),
      _eventloop(eventloop),
      _finder_target(finder_target),
      _fea_target(fea_target),
      _mfea_target(mfea_target),
      _rib_target(rib_target),
      _mld6igmp_target(mld6igmp_target),
      _ifmgr(eventloop, mfea_target.c_str(), xrl_router().finder_address(),
	     xrl_router().finder_port()),
      _mrib_transaction_manager(eventloop),
      _xrl_fea_client4(&xrl_router()),
#ifdef HAVE_IPV6
      _xrl_fea_client6(&xrl_router()),
#endif
      _xrl_mfea_client(&xrl_router()),
      _xrl_rib_client(&xrl_router()),
      _xrl_mld6igmp_client(&xrl_router()),
      _xrl_cli_manager_client(&xrl_router()),
      _xrl_finder_client(&xrl_router()),
      _is_finder_alive(false),
      _is_fea_alive(false),
      _is_fea_registered(false),
      _is_mfea_alive(false),
      _is_mfea_registered(false),
      _is_rib_alive(false),
      _is_rib_registered(false),
      _is_rib_registering(false),
      _is_rib_deregistering(false),
      _is_rib_redist_transaction_enabled(false),
      _is_mld6igmp_alive(false),
      _is_mld6igmp_registered(false),
      _is_mld6igmp_registering(false),
      _is_mld6igmp_deregistering(false)
{
    _ifmgr.set_observer(dynamic_cast<PimNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<PimNode*>(this));
}

void XrlPimNode::destruct_me() {
    shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<PimNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<PimNode*>(this));

    delete_pointers_list(_xrl_tasks_queue);
    PimNode::destruct_me();
}

XrlPimNode::~XrlPimNode()
{
    destruct_me();
}

int
XrlPimNode::startup()
{
    if (start_pim() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::shutdown()
{
    int ret_value = XORP_OK;

    if (stop_cli() != XORP_OK)
	ret_value = XORP_ERROR;

    if (stop_pim() != XORP_OK)
	ret_value = XORP_ERROR;

    return (ret_value);
}

int
XrlPimNode::enable_cli()
{
    PimNodeCli::enable();
    
    return (XORP_OK);
}

int
XrlPimNode::disable_cli()
{
    PimNodeCli::disable();
    
    return (XORP_OK);
}

int
XrlPimNode::start_cli()
{
    if (PimNodeCli::start() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_cli()
{
    if (PimNodeCli::stop() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::enable_pim()
{
    PimNode::enable();

    return (XORP_OK);
}

int
XrlPimNode::disable_pim()
{
    PimNode::disable();

    return (XORP_OK);
}

int
XrlPimNode::start_pim()
{
    if (PimNode::start() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_pim()
{
    if (PimNode::stop() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::enable_bsr()
{
    PimNode::enable_bsr();

    return (XORP_OK);
}

int
XrlPimNode::disable_bsr()
{
    PimNode::disable_bsr();

    return (XORP_OK);
}

int
XrlPimNode::start_bsr()
{
    if (PimNode::start_bsr() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_bsr()
{
    if (PimNode::stop_bsr() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
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
XrlPimNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlPimNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    stop_pim();
}

//
// Task-related methods
//
void
XrlPimNode::add_task(XrlTaskBase* xrl_task)
{
    _xrl_tasks_queue.push_back(xrl_task);

    // If the queue was empty before, start sending the changes
    if (_xrl_tasks_queue.size() == 1)
	send_xrl_task();
}

void
XrlPimNode::send_xrl_task()
{
    if (_xrl_tasks_queue.empty())
	return;

    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    XLOG_ASSERT(xrl_task_base != NULL);

    xrl_task_base->dispatch();
}

void
XrlPimNode::pop_xrl_task()
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());

    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    XLOG_ASSERT(xrl_task_base != NULL);

    delete xrl_task_base;
    _xrl_tasks_queue.pop_front();
}

void
XrlPimNode::retry_xrl_task()
{
    if (_xrl_tasks_queue_timer.scheduled())
	return;		// XXX: already scheduled

    _xrl_tasks_queue_timer = _eventloop.new_oneoff_after(
	RETRY_TIMEVAL,
	callback(this, &XrlPimNode::send_xrl_task));
}

//
// Register with the FEA
//
void
XrlPimNode::fea_register_startup()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_fea_registered)
	return;		// Already registered

    PimNode::incr_startup_requests_n();		// XXX: for FEA registration
    PimNode::incr_startup_requests_n();		// XXX: for FEA birth

    //
    // Register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _fea_target, true));
}

//
// Register with the MFEA
//
void
XrlPimNode::mfea_register_startup()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_mfea_registered)
	return;		// Already registered

    PimNode::incr_startup_requests_n();		// XXX: for MFEA registration
    PimNode::incr_startup_requests_n();		// XXX: for MFEA birth
    PimNode::incr_startup_requests_n();		// XXX: for the ifmgr

    //
    // Register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _mfea_target, true));
}

//
// De-register with the FEA
//
void
XrlPimNode::fea_register_shutdown()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_fea_alive)
	return;		// The FEA is not there anymore

    if (! _is_fea_registered)
	return;		// Not registered

    PimNode::incr_shutdown_requests_n();	// XXX: for FEA deregistration

    //
    // De-register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _fea_target, false));
}

//
// De-register with the MFEA
//
void
XrlPimNode::mfea_register_shutdown()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_mfea_alive)
	return;		// The MFEA is not there anymore

    if (! _is_mfea_registered)
	return;		// Not registered

    PimNode::incr_shutdown_requests_n();	// XXX: for MFEA deregistration
    PimNode::incr_shutdown_requests_n();	// XXX: for the ifmgr

    //
    // De-register interest in the MFEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _mfea_target, false));

    //
    // XXX: when the shutdown is completed, PimNode::status_change()
    // will be called.
    //
    _ifmgr.shutdown();
}

void
XrlPimNode::send_register_unregister_interest()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterInterest* entry;

    entry = dynamic_cast<RegisterUnregisterInterest*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    string eop(entry->operation_name());
    string et(entry->target_name());

    if (entry->is_register()) {
	// Register interest
	success = _xrl_finder_client.send_register_class_event_interest(
	    _finder_target.c_str(), _instance_name, entry->target_name(),
	    callback(this, &XrlPimNode::finder_send_register_unregister_interest_cb));
    } else {
	// Unregister interest
	success = _xrl_finder_client.send_deregister_class_event_interest(
	    _finder_target.c_str(), _instance_name, entry->target_name(),
	    callback(this, &XrlPimNode::finder_send_register_unregister_interest_cb));
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s interest in %s with the Finder. Will try again.",
		   eop.c_str(), et.c_str());
	retry_xrl_task();
	return;
    }
    else {
	XLOG_INFO("Successfully sent %s interest in %s with the Finder.",
		  eop.c_str(), et.c_str());
    }
}

void
XrlPimNode::finder_send_register_unregister_interest_cb(const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterInterest* entry;

    entry = dynamic_cast<RegisterUnregisterInterest*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	if (entry->is_register()) {
	    //
	    // Register interest
	    //
	    if (entry->target_name() == _fea_target) {
		//
		// If success, then the FEA birth event will startup the FEA
		// registration.
		//
		_is_fea_registered = true;
		PimNode::decr_startup_requests_n();	// XXX: for FEA registration
	    }
	    if (entry->target_name() == _mfea_target) {
		//
		// If success, then the MFEA birth event will startup the MFEA
		// registration and the ifmgr.
		//
		_is_mfea_registered = true;
		PimNode::decr_startup_requests_n();	// XXX: for MFEA registration
	    }
	} else {
	    //
	    // Unregister interest
	    //
	    if (entry->target_name() == _fea_target) {
		_is_fea_registered = false;
		PimNode::decr_shutdown_requests_n();	// XXX: for the FEA
	    }
	    if (entry->target_name() == _mfea_target) {
		_is_mfea_registered = false;
		PimNode::decr_shutdown_requests_n();	// XXX: for the MFEA
	    }
	}
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot %s interest in Finder events: %s",
		   entry->operation_name(), xrl_error.str().c_str());
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
	if (entry->is_register()) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    if (entry->target_name() == _fea_target) {
		_is_fea_registered = false;
	    }
	    if (entry->target_name() == _mfea_target) {
		_is_mfea_registered = false;
	    }
	}
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to %s interest in Finder envents: %s. "
		   "Will try again.",
		   entry->operation_name(), xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }

    pop_xrl_task();
    send_xrl_task();
}

//
// Register with the RIB
//
void
XrlPimNode::rib_register_startup()
{
    bool success;

    _rib_register_startup_timer.unschedule();
    _rib_register_shutdown_timer.unschedule();

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_rib_registered)
	return;		// Already registered

    if (! _is_rib_registering) {
	if (! _is_rib_redist_transaction_enabled)
	    PimNode::incr_startup_requests_n();		// XXX: for the RIB
	_is_rib_registering = true;
    }

    //
    // Register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), _instance_name, _rib_target,
	callback(this, &XrlPimNode::finder_register_interest_rib_cb));

    if (! success) {
	//
	// If an error, then try again
	//
	_rib_register_startup_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlPimNode::rib_register_startup));
	return;
    }
}

void
XrlPimNode::finder_register_interest_rib_cb(const XrlError& xrl_error)
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
	// If a transient error, then try again
	//
	if (! _rib_register_startup_timer.scheduled()) {
	    XLOG_ERROR("Failed to register interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_startup_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlPimNode::rib_register_startup));
	}
	break;
    }
}

//
// De-register with the RIB
//
void
XrlPimNode::rib_register_shutdown()
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
	if (_is_rib_redist_transaction_enabled) {
	    PimNode::incr_shutdown_requests_n();	// XXX: for the RIB
	}
	_is_rib_deregistering = true;
    }

    //
    // De-register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), _instance_name, _rib_target,
	callback(this, &XrlPimNode::finder_deregister_interest_rib_cb));

    if (! success) {
	//
	// If an error, then try again
	//
	_rib_register_shutdown_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlPimNode::rib_register_shutdown));
	return;
    }

    send_rib_redist_transaction_disable();
}

void
XrlPimNode::finder_deregister_interest_rib_cb(const XrlError& xrl_error)
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
	// If a transient error, then try again
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to deregister interest in Finder events: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlPimNode::rib_register_shutdown));
	}
	break;
    }
}

//
// Enable receiving MRIB information from the RIB
//
void
XrlPimNode::send_rib_redist_transaction_enable()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_rib_redist_transaction_enabled) {
	if (PimNode::is_ipv4()) {
	    success = _xrl_rib_client.send_redist_transaction_enable4(
		_rib_target.c_str(),
		xrl_router().class_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		IPv4Net(IPv4::ZERO(), 0), // XXX: get the whole table
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_enable_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_rib_client.send_redist_transaction_enable6(
		_rib_target.c_str(),
		xrl_router().class_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		IPv6Net(IPv6::ZERO(), 0), // XXX: get the whole table
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_enable_cb));
	    if (success)
		return;
	}
#endif
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to enable receiving MRIB information from the RIB. "
		   "Will try again.");
	_rib_redist_transaction_enable_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlPimNode::send_rib_redist_transaction_enable));
	return;
    }
}

void
XrlPimNode::rib_client_send_redist_transaction_enable_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_redist_transaction_enabled = true;
	PimNode::decr_startup_requests_n();	// XXX: for the RIB
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot enable receiving MRIB information from the RIB: %s",
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
	// If a transient error, then try again
	//
	if (! _rib_redist_transaction_enable_timer.scheduled()) {
	    XLOG_ERROR("Failed to enable receiving MRIB information from the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_redist_transaction_enable_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlPimNode::send_rib_redist_transaction_enable));
	}
	break;
    }
}

//
// Disable receiving MRIB information from the RIB
//
void
XrlPimNode::send_rib_redist_transaction_disable()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_rib_redist_transaction_enabled) {
	if (PimNode::is_ipv4()) {
	    bool success4;
	    success4 = _xrl_rib_client.send_redist_transaction_disable4(
		_rib_target.c_str(),
		xrl_router().class_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_disable_cb));
	    if (success4 != true)
		success = false;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    bool success6;
	    success6 = _xrl_rib_client.send_redist_transaction_disable6(
		_rib_target.c_str(),
		xrl_router().class_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_disable_cb));
	    if (success6 != true)
		success = false;
	}
#endif
    }

    if (! success) {
	XLOG_ERROR("Failed to disable receiving MRIB information from the RIB. "
		   "Will give up.");
	PimNode::set_status(SERVICE_FAILED);
	PimNode::update_status();
    }
}

void
XrlPimNode::rib_client_send_redist_transaction_disable_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	_is_rib_redist_transaction_enabled = false;
	PimNode::decr_shutdown_requests_n();		// XXX: for the RIB
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot disable receiving MRIB information from the RIB: %s",
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
	_is_rib_redist_transaction_enabled = false;
	PimNode::decr_shutdown_requests_n();		// XXX: for the RIB
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
	// If a transient error, then try again
	//
	if (! _rib_register_shutdown_timer.scheduled()) {
	    XLOG_ERROR("Failed to disable receiving MRIB information from the RIB: %s. "
		       "Will try again.",
		       xrl_error.str().c_str());
	    _rib_register_shutdown_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlPimNode::rib_register_shutdown));
	}
	break;
    }
}

int
XrlPimNode::register_receiver(const string& if_name,
			      const string& vif_name,
			      uint8_t ip_protocol,
			      bool enable_multicast_loopback)
{
    PimNode::incr_startup_requests_n();		// XXX: for FEA-receiver
    add_task(new RegisterUnregisterReceiver(*this, if_name, vif_name,
					    ip_protocol,
					    enable_multicast_loopback,
					    true));

    return (XORP_OK);
}

int
XrlPimNode::unregister_receiver(const string& if_name,
				const string& vif_name,
				uint8_t ip_protocol)
{
    PimNode::incr_shutdown_requests_n();	// XXX: for FEA-non-receiver
    add_task(new RegisterUnregisterReceiver(*this, if_name, vif_name,
					    ip_protocol,
					    false,	// XXX: ignored
					    false));

    return (XORP_OK);
}

void
XrlPimNode::send_register_unregister_receiver()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterReceiver* entry;

    entry = dynamic_cast<RegisterUnregisterReceiver*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //
    // Check whether we have already registered with the FEA
    //
    if (! _is_fea_registered) {
	retry_xrl_task();
	return;
    }

    if (entry->is_register()) {
	// Register a receiver with the FEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_register_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->enable_multicast_loopback(),
		callback(this, &XrlPimNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_register_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->enable_multicast_loopback(),
		callback(this, &XrlPimNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Unregister a receiver with the FEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_unregister_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlPimNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_unregister_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlPimNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}
#endif
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s register receiver on interface %s vif %s "
		   "IP protocol %u with the FEA. "
		   "Will try again.",
		   entry->operation_name(),
		   entry->if_name().c_str(),
		   entry->vif_name().c_str(),
		   entry->ip_protocol());
	retry_xrl_task();
	return;
    }
}

void
XrlPimNode::fea_client_send_register_unregister_receiver_cb(
    const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterReceiver* entry;

    entry = dynamic_cast<RegisterUnregisterReceiver*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	if (entry->is_register())
	    PimNode::decr_startup_requests_n();	 // XXX: for FEA-receiver
	else {
	    PimNode::decr_shutdown_requests_n(); // XXX: for FEA-non-receiver
	}
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot %s receiver with the FEA: %s",
		   entry->operation_name(), xrl_error.str().c_str());
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
	if (entry->is_register()) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    PimNode::decr_shutdown_requests_n();  // XXX: for FEA-non-receiver
	}
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to %s receiver with the FEA: %s. "
		   "Will try again.",
		   entry->operation_name(), xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }
    pop_xrl_task();
    send_xrl_task();
}

int
XrlPimNode::register_protocol(const string& if_name,
			      const string& vif_name,
			      uint8_t ip_protocol)
{
    PimNode::incr_startup_requests_n();		// XXX: for MFEA-protocol
    add_task(new RegisterUnregisterProtocol(*this, if_name, vif_name,
					    ip_protocol,
					    true));

    return (XORP_OK);
}

int
XrlPimNode::unregister_protocol(const string& if_name,
				const string& vif_name)
{
    PimNode::incr_shutdown_requests_n();	// XXX: for MFEA-non-protocol
    add_task(new RegisterUnregisterProtocol(*this, if_name, vif_name,
					    0,		// XXX: ignored
					    false));

    return (XORP_OK);
}

void
XrlPimNode::send_register_unregister_protocol()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterProtocol* entry;

    entry = dynamic_cast<RegisterUnregisterProtocol*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_registered) {
	retry_xrl_task();
	return;
    }

    if (entry->is_register()) {
	// Register a protocol with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_register_protocol4(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlPimNode::mfea_client_send_register_unregister_protocol_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_register_protocol6(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlPimNode::mfea_client_send_register_unregister_protocol_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Unregister a protocol with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_unregister_protocol4(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->if_name(),
		entry->vif_name(),
		callback(this, &XrlPimNode::mfea_client_send_register_unregister_protocol_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_unregister_protocol6(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->if_name(),
		entry->vif_name(),
		callback(this, &XrlPimNode::mfea_client_send_register_unregister_protocol_cb));
	    if (success)
		return;
	}
#endif
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s register protocol on interface %s vif %s "
		   "IP protocol %u with the MFEA. "
		   "Will try again.",
		   entry->operation_name(),
		   entry->if_name().c_str(),
		   entry->vif_name().c_str(),
		   entry->ip_protocol());
	retry_xrl_task();
	return;
    }
}

void
XrlPimNode::mfea_client_send_register_unregister_protocol_cb(
    const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterProtocol* entry;

    entry = dynamic_cast<RegisterUnregisterProtocol*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	if (entry->is_register())
	    PimNode::decr_startup_requests_n();	 // XXX: for MFEA-protocol
	else
	    PimNode::decr_shutdown_requests_n(); // XXX: for MFEA-non-protocol
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	// Shouldn't be fatal, network device can suddenly dissappear, for instance.
	//
	XLOG_ERROR("Cannot %s protocol with the MFEA: %s",
		   entry->operation_name(), xrl_error.str().c_str());
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
	if (entry->is_register()) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    PimNode::decr_shutdown_requests_n(); // XXX: for MFEA-non-protocol
	}
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to %s protocol with the MFEA: %s. "
		   "Will try again.",
		   entry->operation_name(), xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }
    pop_xrl_task();
    send_xrl_task();
}

int
XrlPimNode::join_multicast_group(const string& if_name,
				 const string& vif_name,
				 uint8_t ip_protocol,
				 const IPvX& group_address)
{
    PimNode::incr_startup_requests_n();		// XXX: for FEA-join
    add_task(new JoinLeaveMulticastGroup(*this, if_name, vif_name, ip_protocol,
					 group_address, true));

    return (XORP_OK);
}

int
XrlPimNode::leave_multicast_group(const string& if_name,
				  const string& vif_name,
				  uint8_t ip_protocol,
				  const IPvX& group_address)
{
    PimNode::incr_shutdown_requests_n();		// XXX: for FEA-leave
    add_task(new JoinLeaveMulticastGroup(*this, if_name, vif_name, ip_protocol,
					 group_address, false));

    return (XORP_OK);
}

void
XrlPimNode::send_join_leave_multicast_group()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    JoinLeaveMulticastGroup* entry;

    entry = dynamic_cast<JoinLeaveMulticastGroup*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //
    // Check whether we have already registered with the FEA
    //
    if (! _is_fea_registered) {
	retry_xrl_task();
	return;
    }

    if (entry->is_join()) {
	// Join a multicast group with the FEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_join_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv4(),
		callback(this, &XrlPimNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_join_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv6(),
		callback(this, &XrlPimNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Leave a multicast group with the FEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_leave_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv4(),
		callback(this, &XrlPimNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_leave_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv6(),
		callback(this, &XrlPimNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}
#endif
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s group %s on interface/vif %s/%s with the FEA. "
		   "Will try again.",
		   entry->operation_name(),
		   entry->group_address().str().c_str(),
		   entry->if_name().c_str(),
		   entry->vif_name().c_str());
	retry_xrl_task();
	return;
    }
}

void
XrlPimNode::fea_client_send_join_leave_multicast_group_cb(
    const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    JoinLeaveMulticastGroup* entry;

    entry = dynamic_cast<JoinLeaveMulticastGroup*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	if (entry->is_join())
	    PimNode::decr_startup_requests_n();		// XXX: for FEA-join
	else
	    PimNode::decr_shutdown_requests_n();	// XXX: for FEA-leave
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_WARNING("Cannot %s a multicast group with the FEA: %s",
		   entry->operation_name(),
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
	if (entry->is_join()) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    PimNode::decr_shutdown_requests_n();	// XXX: for FEA-leave
	}
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to %s group %s on interface/vif %s/%s "
		   "with the FEA: %s. "
		   "Will try again.",
		   entry->operation_name(),
		   entry->group_address().str().c_str(),
		   entry->if_name().c_str(),
		   entry->vif_name().c_str(),
		   xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }
    pop_xrl_task();
    send_xrl_task();
}

int
XrlPimNode::add_mfc_to_kernel(const PimMfc& pim_mfc)
{
    add_task(new AddDeleteMfc(*this, pim_mfc, true));

    return (XORP_OK);
}

int
XrlPimNode::delete_mfc_from_kernel(const PimMfc& pim_mfc)
{
    add_task(new AddDeleteMfc(*this, pim_mfc, false));

    return (XORP_OK);
}

void
XrlPimNode::send_add_delete_mfc()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    AddDeleteMfc* entry;

    entry = dynamic_cast<AddDeleteMfc*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    size_t max_vifs_oiflist = entry->olist().size();
    const IPvX& source_addr = entry->source_addr();
    const IPvX& group_addr = entry->group_addr();
    uint32_t iif_vif_index = entry->iif_vif_index();
    const IPvX& rp_addr = entry->rp_addr();

    vector<uint8_t> oiflist_vector(max_vifs_oiflist);
    vector<uint8_t> oiflist_disable_wrongvif_vector(max_vifs_oiflist);
    mifset_to_vector(entry->olist(), oiflist_vector);
    mifset_to_vector(entry->olist_disable_wrongvif(),
		     oiflist_disable_wrongvif_vector);
    
    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_registered) {
	retry_xrl_task();
	return;
    }

    if (entry->is_add()) {
	// Add a MFC with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_add_mfc4(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		iif_vif_index,
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		rp_addr.get_ipv4(),
		1, /* default distance is 1 for PIM */
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_add_mfc6(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		iif_vif_index,
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		rp_addr.get_ipv6(),
		1, /* default distance is 1 for PIM */
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Delete a MFC with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_delete_mfc4(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_delete_mfc6(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_cb));
	    if (success)
		return;
	}
#endif
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s MFC entry for (%s, %s) with the MFEA. "
		   "Will try again.",
		   entry->operation_name(),
		   cstring(source_addr),
		   cstring(group_addr));
	retry_xrl_task();
    }
}

void
XrlPimNode::mfea_client_send_add_delete_mfc_cb(
    const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    AddDeleteMfc* entry;

    entry = dynamic_cast<AddDeleteMfc*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it,
	// then print an error and send the next one.
	//
	XLOG_ERROR("Cannot %s a multicast forwarding entry with the MFEA: %s",
		   entry->operation_name(),
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to add/delete a multicast forwarding entry "
		   "with the MFEA: %s. "
		   "Will try again.",
		   xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }
    pop_xrl_task();
    send_xrl_task();
}

int
XrlPimNode::add_dataflow_monitor(const IPvX& source_addr,
				 const IPvX& group_addr,
				 uint32_t threshold_interval_sec,
				 uint32_t threshold_interval_usec,
				 uint32_t threshold_packets,
				 uint32_t threshold_bytes,
				 bool is_threshold_in_packets,
				 bool is_threshold_in_bytes,
				 bool is_geq_upcall,
				 bool is_leq_upcall)
{
    add_task(new AddDeleteDataflowMonitor(*this,
					  source_addr,
					  group_addr,
					  threshold_interval_sec,
					  threshold_interval_usec,
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall,
					  true));

    return (XORP_OK);
}

int
XrlPimNode::delete_dataflow_monitor(const IPvX& source_addr,
				    const IPvX& group_addr,
				    uint32_t threshold_interval_sec,
				    uint32_t threshold_interval_usec,
				    uint32_t threshold_packets,
				    uint32_t threshold_bytes,
				    bool is_threshold_in_packets,
				    bool is_threshold_in_bytes,
				    bool is_geq_upcall,
				    bool is_leq_upcall)
{
    add_task(new AddDeleteDataflowMonitor(*this,
					  source_addr,
					  group_addr,
					  threshold_interval_sec,
					  threshold_interval_usec,
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall,
					  false));

    return (XORP_OK);
}

int
XrlPimNode::delete_all_dataflow_monitor(const IPvX& source_addr,
					const IPvX& group_addr)
{
    add_task(new AddDeleteDataflowMonitor(*this, source_addr, group_addr));

    return (XORP_OK);
}

void
XrlPimNode::send_add_delete_dataflow_monitor()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    AddDeleteDataflowMonitor* entry;

    entry = dynamic_cast<AddDeleteDataflowMonitor*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_registered) {
	retry_xrl_task();
	return;
    }

    if (entry->is_delete_all()) {
	// Delete all dataflow monitors for a source and a group addresses
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_delete_all_dataflow_monitor4(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->source_addr().get_ipv4(),
		entry->group_addr().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_delete_all_dataflow_monitor6(
		_mfea_target.c_str(),
		xrl_router().class_name(),
		entry->source_addr().get_ipv6(),
		entry->group_addr().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
	    if (success)
		return;
	}
#endif
    } else {
	if (entry->is_add()) {
	    // Add a dataflow monitor with the MFEA
	    if (PimNode::is_ipv4()) {
		success = _xrl_mfea_client.send_add_dataflow_monitor4(
		    _mfea_target.c_str(),
		    xrl_router().class_name(),
		    entry->source_addr().get_ipv4(),
		    entry->group_addr().get_ipv4(),
		    entry->threshold_interval_sec(),
		    entry->threshold_interval_usec(),
		    entry->threshold_packets(),
		    entry->threshold_bytes(),
		    entry->is_threshold_in_packets(),
		    entry->is_threshold_in_bytes(),
		    entry->is_geq_upcall(),
		    entry->is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
		if (success)
		    return;
	    }

#ifdef HAVE_IPV6
	    if (PimNode::is_ipv6()) {
		success = _xrl_mfea_client.send_add_dataflow_monitor6(
		    _mfea_target.c_str(),
		    xrl_router().class_name(),
		    entry->source_addr().get_ipv6(),
		    entry->group_addr().get_ipv6(),
		    entry->threshold_interval_sec(),
		    entry->threshold_interval_usec(),
		    entry->threshold_packets(),
		    entry->threshold_bytes(),
		    entry->is_threshold_in_packets(),
		    entry->is_threshold_in_bytes(),
		    entry->is_geq_upcall(),
		    entry->is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
		if (success)
		    return;
	    }
#endif
	} else {
	    // Delete a dataflow monitor with the MFEA
	    if (PimNode::is_ipv4()) {
		success = _xrl_mfea_client.send_delete_dataflow_monitor4(
		    _mfea_target.c_str(),
		    xrl_router().class_name(),
		    entry->source_addr().get_ipv4(),
		    entry->group_addr().get_ipv4(),
		    entry->threshold_interval_sec(),
		    entry->threshold_interval_usec(),
		    entry->threshold_packets(),
		    entry->threshold_bytes(),
		    entry->is_threshold_in_packets(),
		    entry->is_threshold_in_bytes(),
		    entry->is_geq_upcall(),
		    entry->is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
		if (success)
		    return;
	    }

#ifdef HAVE_IPV6
	    if (PimNode::is_ipv6()) {
		success = _xrl_mfea_client.send_delete_dataflow_monitor6(
		    _mfea_target.c_str(),
		    xrl_router().class_name(),
		    entry->source_addr().get_ipv6(),
		    entry->group_addr().get_ipv6(),
		    entry->threshold_interval_sec(),
		    entry->threshold_interval_usec(),
		    entry->threshold_packets(),
		    entry->threshold_bytes(),
		    entry->is_threshold_in_packets(),
		    entry->is_threshold_in_bytes(),
		    entry->is_geq_upcall(),
		    entry->is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb));
		if (success)
		    return;
	    }
#endif
	}
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s dataflow monitor entry for (%s, %s) "
		   "with the MFEA. "
		   "Will try again.",
		   entry->operation_name(),
		   cstring(entry->source_addr()),
		   cstring(entry->group_addr()));
	retry_xrl_task();
    }
}

void
XrlPimNode::mfea_client_send_add_delete_dataflow_monitor_cb(
    const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    AddDeleteDataflowMonitor* entry;

    entry = dynamic_cast<AddDeleteDataflowMonitor*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	break;

    case COMMAND_FAILED:
	XLOG_ERROR("Cannot %s dataflow monitor entry with the MFEA: %s",
		   entry->operation_name(),
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
	// If a transient error, then try again
	//
	XLOG_ERROR("Failed to %s dataflow monitor entry with the MFEA: %s. "
		   "Will try again.",
		   entry->operation_name(),
		   xrl_error.str().c_str());
	retry_xrl_task();
	return;
    }
    pop_xrl_task();
    send_xrl_task();
}

int
XrlPimNode::add_protocol_mld6igmp(uint32_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot add protocol with MLD6IGMP "
		   "for vif with vif_index %u: "
		   "no such vif", XORP_UINT_CAST(vif_index));
	return (XORP_ERROR);
    }

    PimNode::incr_startup_requests_n();		// XXX: for MLD6IGMP-add
    _add_delete_protocol_mld6igmp_queue.push_back(make_pair(vif_index, true));
    _add_protocol_mld6igmp_vif_index_set.insert(vif_index);

    // If the queue was empty before, start sending the changes
    if (_add_delete_protocol_mld6igmp_queue.size() == 1) {
	send_add_delete_protocol_mld6igmp();
    }

    return (XORP_OK);
}

int
XrlPimNode::delete_protocol_mld6igmp(uint32_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot delete protocol with MLD6IGMP "
		   "for vif with vif_index %u: "
		   "no such vif", XORP_UINT_CAST(vif_index));
	return (XORP_ERROR);
    }

    PimNode::incr_shutdown_requests_n();	// XXX: for MLD6IGMP-delete
    _add_delete_protocol_mld6igmp_queue.push_back(make_pair(vif_index, false));
    _add_protocol_mld6igmp_vif_index_set.erase(vif_index);

    // If the queue was empty before, start sending the changes
    if (_add_delete_protocol_mld6igmp_queue.size() == 1) {
	send_add_delete_protocol_mld6igmp();
    }

    return (XORP_OK);
}

void
XrlPimNode::send_add_delete_protocol_mld6igmp()
{
    bool success = true;
    PimVif *pim_vif = NULL;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_add_delete_protocol_mld6igmp_queue.empty())
	return;			// No more changes

    uint32_t vif_index = _add_delete_protocol_mld6igmp_queue.front().first;
    bool is_add = _add_delete_protocol_mld6igmp_queue.front().second;

    pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot %s vif with vif_index %u with the MLD6IGMP: "
		   "no such vif",
		   (is_add)? "add" : "delete",
		   XORP_UINT_CAST(vif_index));
	_add_delete_protocol_mld6igmp_queue.pop_front();
	goto start_timer_label;
    }

    if (is_add) {
	//
	// Register the protocol with the MLD6IGMP for membership
	// change on this interface.
	//
	if (PimNode::is_ipv4()) {
	    success = _xrl_mld6igmp_client.send_add_protocol4(
		_mld6igmp_target.c_str(),
		xrl_router().class_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mld6igmp_client.send_add_protocol6(
		_mld6igmp_target.c_str(),
		xrl_router().class_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb));
	    if (success)
		return;
	}
    } else {
	//
	// De-register the protocol with the MLD6IGMP for membership
	// change on this interface.
	//
	if (PimNode::is_ipv4()) {
	    success = _xrl_mld6igmp_client.send_delete_protocol4(
		_mld6igmp_target.c_str(),
		xrl_router().class_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mld6igmp_client.send_delete_protocol6(
		_mld6igmp_target.c_str(),
		xrl_router().class_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Cannot %s vif %s with the MLD6IGMP. "
		   "Will try again.",
		   (is_add)? "add" : "delete",
		   pim_vif->name().c_str());
    start_timer_label:
	_add_delete_protocol_mld6igmp_queue_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlPimNode::send_add_delete_protocol_mld6igmp));
    }
}

void
XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb(
    const XrlError& xrl_error)
{
    bool is_add = _add_delete_protocol_mld6igmp_queue.front().second;

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	if (is_add)
	    PimNode::decr_startup_requests_n();	  // XXX: for MLD6IGMP-add
	else
	    PimNode::decr_shutdown_requests_n();  // XXX: for MLD6IGMP-delete
	_add_delete_protocol_mld6igmp_queue.pop_front();
	send_add_delete_protocol_mld6igmp();
	break;

    case COMMAND_FAILED:
	//
	// These errors can be caused by VIFs suddenly dissappearing (or
	// not existing to begin with).
	if (is_add) {
	    XLOG_WARNING("Cannot register with the MLD6IGMP: %s",
			 xrl_error.str().c_str());
	}
	else {
	    XLOG_WARNING("Cannot deregister with the MLD6IGMP: %s",
			 xrl_error.str().c_str());
	}
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
	if (is_add) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    PimNode::decr_shutdown_requests_n();  // XXX: for MLD6IGMP-delete
	    _add_delete_protocol_mld6igmp_queue.pop_front();
	    send_add_delete_protocol_mld6igmp();
	}
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
	// If a transient error, then try again
	//
	if (! _add_delete_protocol_mld6igmp_queue_timer.scheduled()) {
	    XLOG_ERROR("Failed to %s with the MLD6IGMP: %s. "
		       "Will try again.",
		       (is_add)? "register" : "deregister",
		       xrl_error.str().c_str());
	    _add_delete_protocol_mld6igmp_queue_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlPimNode::send_add_delete_protocol_mld6igmp));
	}
	break;
    }
}

//
// Schedule protocol registration right after MLD/IGMP becomes alive
//
void
XrlPimNode::schedule_add_protocol_mld6igmp()
{
    set<uint32_t>::const_iterator set_iter;
    list<pair<uint32_t, bool> >::const_iterator list_iter;

    //
    // Create a copy of the set with the vifs to add, so we can edit it later
    //
    set<uint32_t> tmp_set = _add_protocol_mld6igmp_vif_index_set;

    //
    // Remove from the copy those vifs that are already scheduled to be added.
    //
    for (list_iter = _add_delete_protocol_mld6igmp_queue.begin();
	 list_iter != _add_delete_protocol_mld6igmp_queue.end();
	 ++list_iter) {
	uint32_t vif_index = list_iter->first;
	bool is_add = list_iter->second;
	if (! is_add)
	    continue;
	tmp_set.erase(vif_index);
    }

    //
    // Add the remaining vifs
    //
    for (set_iter = tmp_set.begin(); set_iter != tmp_set.end(); ++set_iter) {
	uint32_t vif_index = *set_iter;
	add_protocol_mld6igmp(vif_index);
    }
}

//
// Protocol node methods
//

int
XrlPimNode::proto_send(const string& if_name,
		       const string& vif_name,
		       const IPvX& src_address,
		       const IPvX& dst_address,
		       uint8_t ip_protocol,
		       int32_t ip_ttl,
		       int32_t ip_tos,
		       bool ip_router_alert,
		       bool ip_internet_control,
		       const uint8_t* sndbuf,
		       size_t sndlen,
		       string& error_msg)
{
    add_task(new SendProtocolMessage(*this,
				     if_name,
				     vif_name,
				     src_address,
				     dst_address,
				     ip_protocol,
				     ip_ttl,
				     ip_tos,
				     ip_router_alert,
				     ip_internet_control,
				     sndbuf,
				     sndlen));
    error_msg = "";

    return (XORP_OK);
}

/**
 * Send a protocol message through the FEA.
 **/
void
XrlPimNode::send_protocol_message()
{
    bool success = true;

    if (! _is_finder_alive) {
	XLOG_ERROR("ERROR: XrlPimNode::send_protocol_message, finder is NOT alive!\n");
	return;		// The Finder is dead
    }

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    SendProtocolMessage* entry;

    entry = dynamic_cast<SendProtocolMessage*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //XLOG_INFO("XrlPimNode::send_protocol_message interface/vif %s/%s. from: %s to: %s",
    //      entry->if_name().c_str(), entry->vif_name().c_str(),
    //      entry->src_address().str().c_str(), entry->dst_address().str().c_str());

    //
    // Check whether we have already registered with the FEA
    //
    if (! _is_fea_registered) {
	XLOG_ERROR("ERROR: XrlPimNode::send_protocol_message, finder is NOT registered!\n");
	retry_xrl_task();
	return;
    }

    //
    // Send the protocol message
    //
    do {
	if (PimNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_send(
		_fea_target.c_str(),
		entry->if_name(),
		entry->vif_name(),
		entry->src_address().get_ipv4(),
		entry->dst_address().get_ipv4(),
		entry->ip_protocol(),
		entry->ip_ttl(),
		entry->ip_tos(),
		entry->ip_router_alert(),
		entry->ip_internet_control(),
		entry->payload(),
		callback(this, &XrlPimNode::fea_client_send_protocol_message_cb));
	    if (success)
		return;
	    break;
	}

#ifdef HAVE_IPV6
	if (PimNode::is_ipv6()) {
	    // XXX: no Extention headers
	    XrlAtomList ext_headers_type;
	    XrlAtomList ext_headers_payload;
	    success = _xrl_fea_client6.send_send(
		_fea_target.c_str(),
		entry->if_name(),
		entry->vif_name(),
		entry->src_address().get_ipv6(),
		entry->dst_address().get_ipv6(),
		entry->ip_protocol(),
		entry->ip_ttl(),
		entry->ip_tos(),
		entry->ip_router_alert(),
		entry->ip_internet_control(),
		ext_headers_type,
		ext_headers_payload,
		entry->payload(),
		callback(this, &XrlPimNode::fea_client_send_protocol_message_cb));
	    if (success)
		return;
	    break;
	}
	XLOG_UNREACHABLE();
#endif
	break;
    } while (false);

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to send a protocol message on interface/vif %s/%s. "
		   "Will try again.",
		   entry->if_name().c_str(),
		   entry->vif_name().c_str());
	retry_xrl_task();
	return;
    }
}

void
XrlPimNode::fea_client_send_protocol_message_cb(const XrlError& xrl_error)
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    SendProtocolMessage* entry;

    entry = dynamic_cast<SendProtocolMessage*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it,
	// then print an error and send the next one.
	//
	// XXX: The FEA may fail to send a protocol message, therefore
	// we don't call XLOG_FATAL() here. For example, the transimssion
	// by the FEA it may fail if there is no buffer space or if an
	// unicast destination is not reachable.
	// Furthermore, all protocol messages are soft-state (i.e., they are
	// retransmitted periodically by the protocol),
	// hence we don't retransmit them here if there was an error.
	//
	XLOG_ERROR("Cannot send a protocol message: %s",
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
	XLOG_ERROR("Cannot send a protocol message: %s",
		   xrl_error.str().c_str());
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
	// XXX: if a transient error, then don't try again.
	// All protocol messages are soft-state (i.e., they are
	// retransmitted periodically by the protocol),
	// hence we don't retransmit them here if there was an error.
	//
	XLOG_ERROR("Failed to send a protocol message: %s",
		   xrl_error.str().c_str());
	break;
    }

    // In all cases, do the next task.
    pop_xrl_task();
    send_xrl_task();
}

//
// Protocol node CLI methods
//
int
XrlPimNode::add_cli_command_to_cli_manager(const char *command_name,
					   const char *command_help,
					   bool is_command_cd,
					   const char *command_cd_prompt,
					   bool is_command_processor
    )
{
    bool success = true;

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    success = _xrl_cli_manager_client.send_add_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	xrl_router().class_name(),
	string(command_name),
	string(command_help),
	is_command_cd,
	string(command_cd_prompt),
	is_command_processor,
	callback(this, &XrlPimNode::cli_manager_client_send_add_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to add CLI command '%s' to the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlPimNode::cli_manager_client_send_add_cli_command_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot add a command to CLI manager: %s",
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
	XLOG_ERROR("Cannot add a command to CLI manager: %s",
		   xrl_error.str().c_str());
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
	// If a transient error, then try again
	//
	//
	// TODO: if the command failed, then we should retransmit it
	//
	XLOG_ERROR("Failed to add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	break;
    }
}

int
XrlPimNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    bool success = true;

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    success = _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	xrl_router().class_name(),
	string(command_name),
	callback(this, &XrlPimNode::cli_manager_client_send_delete_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to delete CLI command '%s' with the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlPimNode::cli_manager_client_send_delete_cli_command_cb(
    const XrlError& xrl_error)
{
    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then we are done
	//
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it, this is
	// fatal.
	//
	XLOG_FATAL("Cannot delete a command from CLI manager: %s",
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
	XLOG_ERROR("Cannot delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
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
	// If a transient error, then try again
	//
	//
	// TODO: if the command failed, then we should retransmit it
	//
	XLOG_ERROR("Failed to delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	break;
    }
}


//
// XRL target methods
//

XrlCmdError
XrlPimNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = xrl_router().class_name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::common_0_1_get_status(// Output values, 
				  uint32_t& status,
				  string& reason)
{
    status = PimNode::node_status(reason);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::common_0_1_shutdown()
{
    if (shutdown() != XORP_OK) {
	string error_msg = c_format("Failed to shutdown PIM");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::common_0_1_startup()
{
    if (startup() != XORP_OK) {
	string error_msg = c_format("Failed to startup PIM");
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
XrlPimNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    if (target_class == _fea_target) {
	_is_fea_alive = true;
	PimNode::decr_startup_requests_n();	// XXX: for FEA birth
    }

    if (target_class == _mfea_target) {
	_is_mfea_alive = true;
	PimNode::decr_startup_requests_n();	// XXX: for MFEA birth
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	if (_ifmgr.startup() != XORP_OK) {
	    PimNode::set_status(SERVICE_FAILED);
	    PimNode::update_status();
	}
    }

    if (target_class == _rib_target) {
	_is_rib_alive = true;
	send_rib_redist_transaction_enable();
    }

    if (target_class == _mld6igmp_target) {
	_is_mld6igmp_alive = true;

	send_add_delete_protocol_mld6igmp();
	schedule_add_protocol_mld6igmp();
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
XrlPimNode::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    bool do_shutdown = false;
    UNUSED(target_instance);

    if (target_class == _fea_target) {
	XLOG_ERROR("FEA (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_fea_alive = false;
	do_shutdown = true;
    }

    if (target_class == _mfea_target) {
	XLOG_ERROR("MFEA (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_mfea_alive = false;
	do_shutdown = true;
    }

    if (target_class == _rib_target) {
	XLOG_ERROR("RIB (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_rib_alive = false;
	do_shutdown = true;
    }

    if (target_class == _mld6igmp_target) {
	XLOG_INFO("MLD/IGMP (instance %s) has died.",
		   target_instance.c_str());
	_is_mld6igmp_alive = false;
    }

    if (do_shutdown)
	stop_pim();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::cli_processor_0_1_process_command(
    // Input values, 
    const string&	processor_name, 
    const string&	cli_term_name, 
    const uint32_t&	cli_session_id,
    const string&	command_name, 
    const string&	command_args, 
    // Output values, 
    string&		ret_processor_name, 
    string&		ret_cli_term_name, 
    uint32_t&		ret_cli_session_id,
    string&		ret_command_output)
{
    PimNodeCli::cli_process_command(processor_name,
				    cli_term_name,
				    cli_session_id,
				    command_name,
				    command_args,
				    ret_processor_name,
				    ret_cli_term_name,
				    ret_cli_session_id,
				    ret_command_output);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::raw_packet4_client_0_1_recv(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv4&		src_address,
    const IPv4&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const bool&		ip_internet_control,
    const vector<uint8_t>&	payload)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the message
    //
    PimNode::proto_recv(if_name,
			vif_name,
			IPvX(src_address),
			IPvX(dst_address),
			ip_protocol,
			ip_ttl,
			ip_tos,
			ip_router_alert,
			ip_internet_control,
			payload,
			error_msg);
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the FEA shoudn't care about it.
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::raw_packet6_client_0_1_recv(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv6&		src_address,
    const IPv6&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const bool&		ip_internet_control,
    const XrlAtomList&	ext_headers_type,
    const XrlAtomList&	ext_headers_payload,
    const vector<uint8_t>&	payload)
{
    string error_msg;

    UNUSED(ext_headers_type);
    UNUSED(ext_headers_payload);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the message
    //
    PimNode::proto_recv(if_name,
			vif_name,
			IPvX(src_address),
			IPvX(dst_address),
			ip_protocol,
			ip_ttl,
			ip_tos,
			ip_router_alert,
			ip_internet_control,
			payload,
			error_msg);
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the FEA shoudn't care about it.
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#endif
XrlCmdError
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message4(
    // Input values, 
    const string&		xrl_sender_name, 
    const uint32_t&		message_type, 
    const string&		, // vif_name, 
    const uint32_t&		vif_index, 
    const IPv4&			source_address, 
    const IPv4&			dest_address, 
    const vector<uint8_t>&	protocol_message)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 message_type,
				 vif_index,
				 IPvX(source_address),
				 IPvX(dest_address),
				 &protocol_message[0],
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message6(
    // Input values, 
    const string&		xrl_sender_name, 
    const uint32_t&		message_type, 
    const string&		, //  vif_name, 
    const uint32_t&		vif_index, 
    const IPv6&			source_address, 
    const IPv6&			dest_address, 
    const vector<uint8_t>&	protocol_message)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 message_type,
				 vif_index,
				 IPvX(source_address),
				 IPvX(dest_address),
				 &protocol_message[0],
				 protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#endif
XrlCmdError
XrlPimNode::mfea_client_0_1_recv_dataflow_signal4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	measured_interval_sec, 
    const uint32_t&	measured_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const uint32_t&	measured_packets, 
    const uint32_t&	measured_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Deliver a signal that a dataflow-related pre-condition is true.
    //
    PimNode::pim_mrt().signal_dataflow_recv(
	IPvX(source_address),
	IPvX(group_address),
	threshold_interval_sec,
	threshold_interval_usec,
	measured_interval_sec,
	measured_interval_usec,
	threshold_packets,
	threshold_bytes,
	measured_packets,
	measured_bytes,
	is_threshold_in_packets,
	is_threshold_in_bytes,
	is_geq_upcall,
	is_leq_upcall);
    
    // XXX: we don't care if the signal delivery failed
    return XrlCmdError::OKAY();
}
#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::mfea_client_0_1_recv_dataflow_signal6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	measured_interval_sec, 
    const uint32_t&	measured_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const uint32_t&	measured_packets, 
    const uint32_t&	measured_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Deliver a signal that a dataflow-related pre-condition is true.
    //
    PimNode::pim_mrt().signal_dataflow_recv(
	IPvX(source_address),
	IPvX(group_address),
	threshold_interval_sec,
	threshold_interval_usec,
	measured_interval_sec,
	measured_interval_usec,
	threshold_packets,
	threshold_bytes,
	measured_packets,
	measured_bytes,
	is_threshold_in_packets,
	is_threshold_in_bytes,
	is_geq_upcall,
	is_leq_upcall);
    
    // XXX: we don't care if the signal delivery failed
    return XrlCmdError::OKAY();
}
#endif
XrlCmdError
XrlPimNode::redist_transaction4_0_1_start_transaction(
    // Output values, 
    uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending "
			     "transactions hit");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_commit_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.commit(tid) != true) {
	error_msg = c_format("Cannot commit MRIB transaction for tid %u",
			     XORP_UINT_CAST(tid));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    PimNode::pim_mrib_table().commit_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_abort_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.abort(tid) != true) {
	error_msg = c_format("Cannot abort MRIB transaction for tid %u",
			     XORP_UINT_CAST(tid));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    PimNode::pim_mrib_table().abort_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_add_route(
    // Input values, 
    const uint32_t&	tid,
    const IPv4Net&	dst,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    string error_msg;
    uint32_t vif_index = Vif::VIF_INDEX_INVALID;
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);

    UNUSED(ifname);
    UNUSED(cookie);
    UNUSED(protocol_origin);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (pim_vif != NULL)
	vif_index = pim_vif->vif_index();

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    mrib.set_next_hop_router_addr(IPvX(nexthop));
    mrib.set_next_hop_vif_index(vif_index);
    mrib.set_metric_preference(admin_distance);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(tid, mrib, vifname);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_delete_route(
    // Input values, 
    const uint32_t&	tid,
    const IPv4Net&	dst,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    string error_msg;

    UNUSED(nexthop);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(metric);
    UNUSED(admin_distance);
    UNUSED(cookie);
    UNUSED(protocol_origin);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);

    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_delete_all_routes(
    // Input values, 
    const uint32_t&	tid, 
    const string&	cookie)
{
    string error_msg;

    UNUSED(cookie);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Add a transaction item to remove all entries
    //
    PimNode::pim_mrib_table().add_pending_remove_all_entries(tid);

    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::redist_transaction6_0_1_start_transaction(
    // Output values, 
    uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending "
			     "transactions hit");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
XrlCmdError
XrlPimNode::redist_transaction6_0_1_commit_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.commit(tid) != true) {
	error_msg = c_format("Cannot commit MRIB transaction for tid %u",
			     XORP_UINT_CAST(tid));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    PimNode::pim_mrib_table().commit_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_abort_transaction(
    // Input values, 
    const uint32_t&	tid)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.abort(tid) != true) {
	error_msg = c_format("Cannot abort MRIB transaction for tid %u",
			     XORP_UINT_CAST(tid));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    PimNode::pim_mrib_table().abort_pending_transactions(tid);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_add_route(
    // Input values, 
    const uint32_t&	tid,
    const IPv6Net&	dst,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    string error_msg;
    uint32_t vif_index = Vif::VIF_INDEX_INVALID;
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);

    UNUSED(ifname);
    UNUSED(cookie);
    UNUSED(protocol_origin);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (pim_vif != NULL)
	vif_index = pim_vif->vif_index();

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    mrib.set_next_hop_router_addr(IPvX(nexthop));
    mrib.set_next_hop_vif_index(vif_index);
    mrib.set_metric_preference(admin_distance);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(tid, mrib, vifname);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_delete_route(
    // Input values, 
    const uint32_t&	tid,
    const IPv6Net&	dst,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    string error_msg;

    UNUSED(nexthop);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(metric);
    UNUSED(admin_distance);
    UNUSED(cookie);
    UNUSED(protocol_origin);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dst));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);

    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_delete_all_routes(
    // Input values, 
    const uint32_t&	tid, 
    const string&	cookie)
{
    string error_msg;

    UNUSED(cookie);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Add a transaction item to remove all entries
    //
    PimNode::pim_mrib_table().add_pending_remove_all_entries(tid);

    //
    // Success
    //
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	vif_name,
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	error_msg = c_format("Failed to add membership for (%s, %s)"
			     "on vif %s: %s",
			     cstring(source),
			     cstring(group),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	vif_name,
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	error_msg = c_format("Failed to add membership for (%s, %s)"
			     "on vif %s: %s",
			     cstring(source),
			     cstring(group),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	vif_name,
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete membership for (%s, %s)"
			     "on vif %s: %s",
			     cstring(source),
			     cstring(group),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
    
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	vif_name,
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	error_msg = c_format("Failed to delete membership for (%s, %s)"
			     "on vif %s: %s",
			     cstring(source),
			     cstring(group),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_vif(
    // Input values,
    const string&	vif_name,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = PimNode::enable_vif(vif_name, error_msg);
    else
	ret_value = PimNode::disable_vif(vif_name, error_msg);

    if (ret_value != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::start_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::stop_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_all_vifs(
	// Input values,
	const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = PimNode::enable_all_vifs();
    else
	ret_value = PimNode::enable_all_vifs();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable all vifs");
	else
	    error_msg = c_format("Failed to disable all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_all_vifs()
{
    string error_msg;

    if (PimNode::start_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_all_vifs()
{
    string error_msg;

    if (PimNode::stop_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to stop all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_pim(
	// Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_pim();
    else
	ret_value = disable_pim();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable PIM");
	else
	    error_msg = c_format("Failed to disable PIM");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_pim()
{
    string error_msg;

    if (start_pim() != XORP_OK) {
	error_msg = c_format("Failed to start PIM");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_pim()
{
    string error_msg;

    if (stop_pim() != XORP_OK) {
	error_msg = c_format("Failed to stop PIM");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_cli(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_cli();
    else
	ret_value = disable_cli();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable PIM CLI");
	else
	    error_msg = c_format("Failed to disable PIM CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_cli()
{
    string error_msg;

    if (start_cli() != XORP_OK) {
	error_msg = c_format("Failed to start PIM CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_cli()
{
    string error_msg;

    if (stop_cli() != XORP_OK) {
	error_msg = c_format("Failed to stop PIM CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_enable_bsr(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_bsr();
    else
	ret_value = disable_bsr();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable PIM BSR");
	else
	    error_msg = c_format("Failed to disable PIM BSR");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_start_bsr()
{
    string error_msg;

    if (start_bsr() != XORP_OK) {
	error_msg = c_format("Failed to start PIM BSR");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_bsr()
{
    string error_msg;

    if (stop_bsr() != XORP_OK) {
	error_msg = c_format("Failed to stop PIM BSR");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_apply_bsr_changes()
{
    string error_msg;
    
    if (PimNode::apply_bsr_changes(error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

//
// PIM configuration
//
XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const IPv6&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const IPv4&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const string&	vif_name)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const IPv4&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr),
						      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const IPv6&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr),
						      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv6&		vif_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %u",
			     XORP_UINT_CAST(bsr_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr(IPvXNet(scope_zone_id),
				     is_scope_zone,
				     vif_name,
				     IPvX(vif_addr),
				     (uint8_t)(bsr_priority),
				     (uint8_t)(hash_mask_len),
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv4&		vif_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %u",
			     XORP_UINT_CAST(bsr_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr(IPvXNet(scope_zone_id),
				     is_scope_zone,
				     vif_name,
				     IPvX(vif_addr),
				     (uint8_t)(bsr_priority),
				     (uint8_t)(hash_mask_len),
				     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_bsr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv4&		vif_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %u",
			     XORP_UINT_CAST(rp_holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp(IPvXNet(group_prefix),
				    is_scope_zone,
				    vif_name,
				    vif_addr,
				    (uint8_t)(rp_priority),
				    (uint16_t)(rp_holdtime),
				    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv6&		vif_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %u",
			     XORP_UINT_CAST(rp_holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp(IPvXNet(group_prefix),
				    is_scope_zone,
				    vif_name,
				    vif_addr,
				    (uint8_t)(rp_priority),
				    (uint16_t)(rp_holdtime),
				    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv6&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_cand_rp(IPvXNet(group_prefix),
				       is_scope_zone,
				       vif_name,
				       IPvX(vif_addr),
				       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const IPv4&		vif_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_cand_rp(IPvXNet(group_prefix),
				       is_scope_zone,
				       vif_name,
				       IPvX(vif_addr),
				       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_static_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_static_rp(IPvXNet(group_prefix),
				      IPvX(rp_addr),
				      (uint8_t)(rp_priority),
				      (uint8_t)(hash_mask_len),
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_config_static_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_static_rp(IPvXNet(group_prefix),
				      IPvX(rp_addr),
				      (uint8_t)(rp_priority),
				      (uint8_t)(hash_mask_len),
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_static_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_static_rp(IPvXNet(group_prefix),
					 IPvX(rp_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_all_static_group_prefixes_rp6(
    // Input values, 
    const IPv6&		rp_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_all_static_group_prefixes_rp(IPvX(rp_addr),
							    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_delete_config_static_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_static_rp(IPvXNet(group_prefix),
					 IPvX(rp_addr),
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_all_static_group_prefixes_rp4(
    // Input values, 
    const IPv4&		rp_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_config_all_static_group_prefixes_rp(IPvX(rp_addr),
							    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_all_static_rps()
{
    string error_msg;
    
    if (PimNode::delete_config_all_static_rps(error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_config_static_rp_done()
{
    string error_msg;
    
    if (PimNode::config_static_rp_done(error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		proto_version)
{
    string error_msg;
    
    int v;
    if (PimNode::get_vif_proto_version(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    proto_version = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	proto_version)
{
    string error_msg;
    
    if (PimNode::set_vif_proto_version(vif_name, proto_version, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_proto_version(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_proto_version(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_triggered_delay(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	hello_triggered_delay)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_hello_triggered_delay(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    hello_triggered_delay = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_triggered_delay)
{
    string error_msg;
    
    if (hello_triggered_delay > 0xffff) {
	error_msg = c_format("Invalid Hello triggered delay value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(hello_triggered_delay),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_triggered_delay(vif_name,
					       hello_triggered_delay,
					       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_triggered_delay(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_hello_triggered_delay(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_period)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_hello_period(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    hello_period = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_period)
{
    string error_msg;
    
    if (hello_period > 0xffff) {
	error_msg = c_format("Invalid Hello period value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(hello_period),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_period(vif_name, hello_period, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_period(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_hello_period(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		hello_holdtime)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_hello_holdtime(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    hello_holdtime = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	hello_holdtime)
{
    string error_msg;
    
    if (hello_holdtime > 0xffff) {
	error_msg = c_format("Invalid Hello holdtime value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(hello_holdtime),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_holdtime(vif_name, hello_holdtime, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_holdtime(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_hello_holdtime(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		dr_priority)
{
    string error_msg;
    
    uint32_t v;
    if (PimNode::get_vif_dr_priority(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    dr_priority = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_dr_priority(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	dr_priority)
{
    string error_msg;
    
    if (dr_priority > 0xffffffffU) {
	error_msg = c_format("Invalid DR priority value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(dr_priority),
			     0xffffffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_dr_priority(vif_name, dr_priority, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_dr_priority(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_dr_priority(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_propagation_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		propagation_delay)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_propagation_delay(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    propagation_delay = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_propagation_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	propagation_delay)
{
    string error_msg;
    
    if (propagation_delay > 0xffff) {
	error_msg = c_format("Invalid Propagation delay value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(propagation_delay),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_propagation_delay(vif_name, propagation_delay,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_propagation_delay(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_propagation_delay(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		override_interval)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_override_interval(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    override_interval = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_override_interval(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	override_interval)
{
    string error_msg;
    
    if (override_interval > 0xffff) {
	error_msg = c_format("Invalid Override interval value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(override_interval),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_override_interval(vif_name, override_interval,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_override_interval(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_override_interval(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		is_tracking_support_disabled)
{
    string error_msg;
    
    bool v;
    if (PimNode::get_vif_is_tracking_support_disabled(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    is_tracking_support_disabled = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name, 
    const bool&		is_tracking_support_disabled)
{
    string error_msg;
    
    if (PimNode::set_vif_is_tracking_support_disabled(
	vif_name,
	is_tracking_support_disabled, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_is_tracking_support_disabled(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_is_tracking_support_disabled(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    bool&		accept_nohello_neighbors)
{
    string error_msg;
    
    bool v;
    if (PimNode::get_vif_accept_nohello_neighbors(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    accept_nohello_neighbors = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name, 
    const bool&		accept_nohello_neighbors)
{
    string error_msg;
    
    if (PimNode::set_vif_accept_nohello_neighbors(vif_name,
						  accept_nohello_neighbors,
						  error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_accept_nohello_neighbors(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_accept_nohello_neighbors(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		join_prune_period)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_join_prune_period(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    join_prune_period = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_join_prune_period(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	join_prune_period)
{
    string error_msg;
    
    if (join_prune_period > 0xffff) {
	error_msg = c_format("Invalid Join/Prune period value %u: "
			     "max allowed is %u",
			     XORP_UINT_CAST(join_prune_period),
			     0xffffU);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_join_prune_period(vif_name, join_prune_period,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_join_prune_period(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_join_prune_period(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_switch_to_spt_threshold(
    // Output values, 
    bool&	is_enabled, 
    uint32_t&	interval_sec, 
    uint32_t&	bytes)
{
    string error_msg;
    
    bool v1;
    uint32_t v2, v3;
    if (PimNode::get_switch_to_spt_threshold(v1, v2, v3, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    is_enabled = v1;
    interval_sec = v2;
    bytes = v3;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_switch_to_spt_threshold(
    // Input values, 
    const bool&		is_enabled, 
    const uint32_t&	interval_sec, 
    const uint32_t&	bytes)
{
    string error_msg;
    
    if (PimNode::set_switch_to_spt_threshold(is_enabled,
					     interval_sec,
					     bytes,
					     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_switch_to_spt_threshold()
{
    string error_msg;
    
    if (PimNode::reset_switch_to_spt_threshold(error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_alternative_subnet4(
    // Input values,
    const string&	vif_name,
    const IPv4Net&	subnet)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_alternative_subnet(vif_name, IPvXNet(subnet), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_alternative_subnet6(
    // Input values,
    const string&	vif_name,
    const IPv6Net&	subnet)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_alternative_subnet(vif_name, IPvXNet(subnet), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_alternative_subnet6(
    // Input values,
    const string&	vif_name,
    const IPv6Net&	subnet)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_alternative_subnet(vif_name, IPvXNet(subnet),
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_delete_alternative_subnet4(
    // Input values,
    const string&	vif_name,
    const IPv4Net&	subnet)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_alternative_subnet(vif_name, IPvXNet(subnet),
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_remove_all_alternative_subnets(
    // Input values,
    const string&	vif_name)
{
    string error_msg;

    if (PimNode::remove_all_alternative_subnets(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_log_trace_all(
    // Input values,
    const bool&	enable)
{
    PimNode::set_log_trace(enable);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_jp_entry4(
    // Input values, 
    const IPv4&		source_addr, 
    const IPv4&		group_addr, 
    const uint32_t&	group_mask_len, 
    const string&	mrt_entry_type, 
    const string&	action_jp, 
    const uint32_t&	holdtime, 
    const bool&		is_new_group)
{
    string error_msg;
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Find the entry type
    //
    do {
	if (mrt_entry_type == "SG") {
	    entry_type = MRT_ENTRY_SG;
	    break;
	}
	if (mrt_entry_type == "SG_RPT") {
	    entry_type = MRT_ENTRY_SG_RPT;
	    break;
	}
	if (mrt_entry_type == "WC") {
	    entry_type = MRT_ENTRY_WC;
	    break;
	}
	if (mrt_entry_type == "RP") {
	    entry_type = MRT_ENTRY_RP;
	    break;
	}
	// Invalid entry
	error_msg = c_format("Invalid entry type = %s",
			     mrt_entry_type.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    //
    // Find the action type
    //
    do {
	if (action_jp == "JOIN") {
	    action_type = ACTION_JOIN;
	    break;
	}
	if (action_jp == "PRUNE") {
	    action_type = ACTION_PRUNE;
	    break;
	}
	// Invalid action
	error_msg = c_format("Invalid action = %s",
			     action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    if (group_mask_len > 0xff) {
	error_msg = c_format("Invalid group mask length = %u",
			     XORP_UINT_CAST(group_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // XXX: Dupe?
    if (group_mask_len > 0xff) {
	error_msg = c_format("Invalid group mask length = %u",
			     XORP_UINT_CAST(group_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (holdtime > 0xffff) {
	error_msg = c_format("Invalid holdtime = %u",
			     XORP_UINT_CAST(holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   (uint8_t)(group_mask_len),
				   entry_type, action_type,
				   (uint16_t)(holdtime),
				   is_new_group)
	!= XORP_OK) {
	error_msg = c_format("Failed to add Join/Prune test entry "
			     "for (%s, %s)",
			     cstring(source_addr),
			     cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_test_jp_entry6(
    // Input values, 
    const IPv6&		source_addr, 
    const IPv6&		group_addr, 
    const uint32_t&	group_mask_len, 
    const string&	mrt_entry_type, 
    const string&	action_jp, 
    const uint32_t&	holdtime, 
    const bool&		is_new_group)
{
    string error_msg;
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Find the entry type
    //
    do {
	if (mrt_entry_type == "SG") {
	    entry_type = MRT_ENTRY_SG;
	    break;
	}
	if (mrt_entry_type == "SG_RPT") {
	    entry_type = MRT_ENTRY_SG_RPT;
	    break;
	}
	if (mrt_entry_type == "WC") {
	    entry_type = MRT_ENTRY_WC;
	    break;
	}
	if (mrt_entry_type == "RP") {
	    entry_type = MRT_ENTRY_RP;
	    break;
	}
	// Invalid entry
	error_msg = c_format("Invalid entry type = %s",
			     mrt_entry_type.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    //
    // Find the action type
    //
    do {
	if (action_jp == "JOIN") {
	    action_type = ACTION_JOIN;
	    break;
	}
	if (action_jp == "PRUNE") {
	    action_type = ACTION_PRUNE;
	    break;
	}
	// Invalid action
	error_msg = c_format("Invalid action = %s",
			     action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    if (group_mask_len > 0xff) {
	error_msg = c_format("Invalid group mask length = %u",
			     XORP_UINT_CAST(group_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (holdtime > 0xffff) {
	error_msg = c_format("Invalid holdtime = %u",
			     XORP_UINT_CAST(holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   (uint8_t)(group_mask_len),
				   entry_type, action_type,
				   (uint16_t)(holdtime),
				   is_new_group)
	!= XORP_OK) {
	error_msg = c_format("Failed to add Join/Prune test entry "
			     "for (%s, %s)",
			     cstring(source_addr),
			     cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		nbr_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_jp_entry(vif_name, IPvX(nbr_addr), error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Join/Prune test message to %s "
			     "on vif %s: %s",
			     cstring(nbr_addr), vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_assert6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		source_addr, 
    const IPv6&		group_addr, 
    const bool&		rpt_bit, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric,
				  error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Assert test message "
			     "for (%s, %s) on vif %s: %s",
			     cstring(source_addr),
			     cstring(group_addr),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		nbr_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_jp_entry(vif_name, IPvX(nbr_addr), error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Join/Prune test message to %s "
			     "on vif %s: %s",
			     cstring(nbr_addr), vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_assert4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		source_addr, 
    const IPv4&		group_addr, 
    const bool&		rpt_bit, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric,
				  error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Assert test message "
			     "for (%s, %s) on vif %s: %s",
			     cstring(source_addr),
			     cstring(group_addr),
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_zone4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4&		bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len, 
    const uint32_t&	fragment_tag)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %u",
			     XORP_UINT_CAST(bsr_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (fragment_tag > 0xffff) {
	error_msg = c_format("Invalid fragment tag = %u",
			     XORP_UINT_CAST(fragment_tag));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   (uint8_t)(bsr_priority),
				   (uint8_t)(hash_mask_len),
				   (uint16_t)(fragment_tag))
	!= XORP_OK) {
	error_msg = c_format("Failed to add BSR test zone %s "
			     "with BSR address %s",
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)),
			     cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_zone6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6&		bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len, 
    const uint32_t&	fragment_tag)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %u",
			     XORP_UINT_CAST(bsr_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %u",
			     XORP_UINT_CAST(hash_mask_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (fragment_tag > 0xffff) {
	error_msg = c_format("Invalid fragment tag = %u",
			     XORP_UINT_CAST(fragment_tag));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   (uint8_t)(bsr_priority),
				   (uint8_t)(hash_mask_len),
				   (uint16_t)(fragment_tag))
	!= XORP_OK) {
	error_msg = c_format("Failed to add BSR test zone %s "
			     "with BSR address %s",
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)),
			     cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_group_prefix6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const uint32_t&	expected_rp_count)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (expected_rp_count > 0xff) {
	error_msg = c_format("Invalid expected RP count = %u",
			     XORP_UINT_CAST(expected_rp_count));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	    PimScopeZoneId(zone_id_scope_zone_prefix,
			   zone_id_is_scope_zone),
	    IPvXNet(group_prefix),
	    is_scope_zone,
	    (uint8_t)(expected_rp_count))
	!= XORP_OK) {
	error_msg = c_format("Failed to add group prefix %s "
			     "for BSR test zone %s",
			     cstring(group_prefix),
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_group_prefix4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const uint32_t&	expected_rp_count)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (expected_rp_count > 0xff) {
	error_msg = c_format("Invalid expected RP count = %u",
			     XORP_UINT_CAST(expected_rp_count));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	    PimScopeZoneId(zone_id_scope_zone_prefix,
			   zone_id_is_scope_zone),
	    IPvXNet(group_prefix),
	    is_scope_zone,
	    (uint8_t)(expected_rp_count))
	!= XORP_OK) {
	error_msg = c_format("Failed to add group prefix %s "
			     "for BSR test zone %s",
			     cstring(group_prefix),
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_rp4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %u",
			     XORP_UINT_CAST(rp_holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 (uint8_t)(rp_priority),
				 (uint16_t)(rp_holdtime))
	!= XORP_OK) {
	error_msg = c_format("Failed to add test Cand-RP %s "
			     "for group prefix %s for BSR zone %s",
			     cstring(rp_addr),
			     cstring(group_prefix),
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_rp6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %u",
			     XORP_UINT_CAST(rp_priority));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %u",
			     XORP_UINT_CAST(rp_holdtime));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 (uint8_t)(rp_priority),
				 (uint16_t)(rp_holdtime))
	!= XORP_OK) {
	error_msg = c_format("Failed to add test Cand-RP %s "
			     "for group prefix %s for BSR zone %s",
			     cstring(rp_addr),
			     cstring(group_prefix),
			     cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
						    zone_id_is_scope_zone)));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap_by_dest6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		dest_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr),
					     error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Bootstrap test message "
			     "on vif %s to address %s: %s",
			     vif_name.c_str(),
			     cstring(dest_addr),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_pimstat_interface6(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		pim_version, 
    bool&		is_dr, 
    uint32_t&		dr_priority, 
    IPv6&		dr_address, 
    uint32_t&		pim_nbrs_number)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get information about vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    pim_version = pim_vif->proto_version();
    is_dr = pim_vif->i_am_dr();
    dr_priority = pim_vif->dr_priority().get();
    dr_address = pim_vif->dr_addr().get_ipv6();
    pim_nbrs_number = pim_vif->pim_nbrs_number();
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;

    if (PimNode::send_test_bootstrap(vif_name, error_msg) != XORP_OK) {
	error_msg = c_format("Failed to send Bootstrap test message "
			     "on vif %s: %s",
			     vif_name.c_str(),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_bootstrap_by_dest4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		dest_addr)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr),
					     error_msg)
	!= XORP_OK) {
	error_msg = c_format("Failed to send Bootstrap test message "
			     "on vif %s to address %s: %s",
			     vif_name.c_str(),
			     cstring(dest_addr),
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_cand_rp_adv()
{
    string error_msg;

    if (PimNode::send_test_cand_rp_adv() != XORP_OK) {
	error_msg = c_format("Failed to send Cand-RP-Adv test message");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_pimstat_neighbors4(
    // Output values, 
    uint32_t&		nbrs_number, 
    XrlAtomList&	vifs, 
    XrlAtomList&	addresses, 
    XrlAtomList&	pim_versions, 
    XrlAtomList&	dr_priorities, 
    XrlAtomList&	holdtimes, 
    XrlAtomList&	timeouts, 
    XrlAtomList&	uptimes)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    TimeVal now;
    TimerList::system_gettimeofday(&now);
    
    nbrs_number = 0;
    
    for (uint32_t i = 0; i < PimNode::maxvifs(); i++) {
	PimVif *pim_vif = PimNode::vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	
	if (pim_vif->primary_addr() == IPvX::ZERO(family()))
	    continue;		// XXX: ignore vifs with no address
	
	list<PimNbr *>::iterator iter;
	for (iter = pim_vif->pim_nbrs().begin();
	     iter != pim_vif->pim_nbrs().end();
	     ++iter) {
	    PimNbr *pim_nbr = *iter;
	    
	    nbrs_number++;
	    vifs.append(XrlAtom(pim_vif->name()));
	    addresses.append(XrlAtom(pim_vif->primary_addr().get_ipv4()));
	    pim_versions.append(XrlAtom((int32_t)pim_nbr->proto_version()));
	    if (pim_nbr->is_dr_priority_present())
		dr_priorities.append(XrlAtom((int32_t)pim_nbr->dr_priority()));
	    else
		dr_priorities.append(XrlAtom((int32_t)-1));
	    holdtimes.append(XrlAtom((int32_t)pim_nbr->hello_holdtime()));
	    if (pim_nbr->const_neighbor_liveness_timer().scheduled()) {
		TimeVal tv_left;
		pim_nbr->const_neighbor_liveness_timer().time_remaining(tv_left);
		timeouts.append(XrlAtom((int32_t)tv_left.sec()));
	    } else {
		timeouts.append(XrlAtom((int32_t)-1));
	    }
	    
	    TimeVal startup_time = pim_nbr->startup_time();
	    TimeVal delta_time = now - startup_time;
	    
	    uptimes.append(XrlAtom((int32_t)delta_time.sec()));
	}
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_pimstat_neighbors6(
    // Output values, 
    uint32_t&		nbrs_number, 
    XrlAtomList&	vifs, 
    XrlAtomList&	addresses, 
    XrlAtomList&	pim_versions, 
    XrlAtomList&	dr_priorities, 
    XrlAtomList&	holdtimes, 
    XrlAtomList&	timeouts, 
    XrlAtomList&	uptimes)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    TimeVal now;
    TimerList::system_gettimeofday(&now);
    
    nbrs_number = 0;
    
    for (uint32_t i = 0; i < PimNode::maxvifs(); i++) {
	PimVif *pim_vif = PimNode::vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	
	if (pim_vif->primary_addr() == IPvX::ZERO(family()))
	    continue;		// XXX: ignore vifs with no address
	
	list<PimNbr *>::iterator iter;
	for (iter = pim_vif->pim_nbrs().begin();
	     iter != pim_vif->pim_nbrs().end();
	     ++iter) {
	    PimNbr *pim_nbr = *iter;
	    
	    nbrs_number++;
	    vifs.append(XrlAtom(pim_vif->name()));
	    addresses.append(XrlAtom(pim_vif->primary_addr().get_ipv6()));
	    pim_versions.append(XrlAtom((int32_t)pim_nbr->proto_version()));
	    if (pim_nbr->is_dr_priority_present())
		dr_priorities.append(XrlAtom((int32_t)pim_nbr->dr_priority()));
	    else
		dr_priorities.append(XrlAtom((int32_t)-1));
	    holdtimes.append(XrlAtom((int32_t)pim_nbr->hello_holdtime()));
	    if (pim_nbr->const_neighbor_liveness_timer().scheduled()) {
		TimeVal tv_left;
		pim_nbr->const_neighbor_liveness_timer().time_remaining(tv_left);
		timeouts.append(XrlAtom((int32_t)tv_left.sec()));
	    } else {
		timeouts.append(XrlAtom((int32_t)-1));
	    }
	    
	    TimeVal startup_time = pim_nbr->startup_time();
	    TimeVal delta_time = now - startup_time;
	    
	    uptimes.append(XrlAtom((int32_t)delta_time.sec()));
	}
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_pimstat_interface4(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		pim_version, 
    bool&		is_dr, 
    uint32_t&		dr_priority, 
    IPv4&		dr_address, 
    uint32_t&		pim_nbrs_number)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    PimVif *pim_vif = PimNode::vif_find_by_name(vif_name);
    if (pim_vif == NULL) {
	error_msg = c_format("Cannot get information about vif %s: "
			     "no such vif",
			     vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    pim_version = pim_vif->proto_version();
    is_dr = pim_vif->i_am_dr();
    dr_priority = pim_vif->dr_priority().get();
    dr_address = pim_vif->dr_addr().get_ipv4();
    pim_nbrs_number = pim_vif->pim_nbrs_number();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_pimstat_rps4(
    // Output values, 
    uint32_t&		rps_number, 
    XrlAtomList&	addresses, 
    XrlAtomList&	types, 
    XrlAtomList&	priorities, 
    XrlAtomList&	holdtimes, 
    XrlAtomList&	timeouts, 
    XrlAtomList&	group_prefixes)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    rps_number = 0;
    
    list<PimRp *>::const_iterator iter;
    for (iter = PimNode::rp_table().rp_list().begin();
	 iter != PimNode::rp_table().rp_list().end();
	 ++iter) {
	PimRp *pim_rp = *iter;
	
	string rp_type;
	int holdtime = -1;
	int left_sec = -1;
	switch (pim_rp->rp_learned_method()) {
        case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	    rp_type = "bootstrap";
	    do {
		// Try first a scoped zone, then a non-scoped zone
		BsrRp *bsr_rp;
		bsr_rp = PimNode::pim_bsr().find_rp(pim_rp->group_prefix(),
						    true,
						    pim_rp->rp_addr());
		if (bsr_rp == NULL) {
		    bsr_rp = PimNode::pim_bsr().find_rp(pim_rp->group_prefix(),
							false,
							pim_rp->rp_addr());
		}
		if (bsr_rp == NULL)
		    break;
		holdtime = bsr_rp->rp_holdtime();
		if (bsr_rp->const_candidate_rp_expiry_timer().scheduled()) {
		    TimeVal tv_left;
		    bsr_rp->const_candidate_rp_expiry_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
	    } while (false);
	    
	    break;
        case PimRp::RP_LEARNED_METHOD_STATIC:
	    rp_type = "static";
	    break;
        default:
	    rp_type = "unknown";
	    break;
        }
	
	addresses.append(XrlAtom(pim_rp->rp_addr().get_ipv4()));
	types.append(XrlAtom(rp_type));
	priorities.append(XrlAtom((int32_t)pim_rp->rp_priority()));
	holdtimes.append(XrlAtom((int32_t)holdtime));
	timeouts.append(XrlAtom((int32_t)left_sec));
	group_prefixes.append(XrlAtom(pim_rp->group_prefix().get_ipv4net()));
    }
	 
    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlPimNode::pim_0_1_pimstat_rps6(
    // Output values, 
    uint32_t&		rps_number, 
    XrlAtomList&	addresses, 
    XrlAtomList&	types, 
    XrlAtomList&	priorities, 
    XrlAtomList&	holdtimes, 
    XrlAtomList&	timeouts, 
    XrlAtomList&	group_prefixes)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    rps_number = 0;
    
    list<PimRp *>::const_iterator iter;
    for (iter = PimNode::rp_table().rp_list().begin();
	 iter != PimNode::rp_table().rp_list().end();
	 ++iter) {
	PimRp *pim_rp = *iter;
	
	string rp_type;
	int holdtime = -1;
	int left_sec = -1;
	switch (pim_rp->rp_learned_method()) {
        case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	    rp_type = "bootstrap";
	    do {
		// Try first a scoped zone, then a non-scoped zone
		BsrRp *bsr_rp;
		bsr_rp = PimNode::pim_bsr().find_rp(pim_rp->group_prefix(),
						    true,
						    pim_rp->rp_addr());
		if (bsr_rp == NULL) {
		    bsr_rp = PimNode::pim_bsr().find_rp(pim_rp->group_prefix(),
							false,
							pim_rp->rp_addr());
		}
		if (bsr_rp == NULL)
		    break;
		holdtime = bsr_rp->rp_holdtime();
		if (bsr_rp->const_candidate_rp_expiry_timer().scheduled()) {
		    TimeVal tv_left;
		    bsr_rp->const_candidate_rp_expiry_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
	    } while (false);
	    
	    break;
        case PimRp::RP_LEARNED_METHOD_STATIC:
	    rp_type = "static";
	    break;
        default:
	    rp_type = "unknown";
	    break;
        }
	
	addresses.append(XrlAtom(pim_rp->rp_addr().get_ipv6()));
	types.append(XrlAtom(rp_type));
	priorities.append(XrlAtom((int32_t)pim_rp->rp_priority()));
	holdtimes.append(XrlAtom((int32_t)holdtime));
	timeouts.append(XrlAtom((int32_t)left_sec));
	group_prefixes.append(XrlAtom(pim_rp->group_prefix().get_ipv6net()));
    }
    
    return XrlCmdError::OKAY();
}
#endif

XrlCmdError
XrlPimNode::pim_0_1_clear_pim_statistics()
{
    PimNode::clear_pim_statistics();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_clear_pim_statistics_per_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::clear_pim_statistics_per_vif(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}


//
// Statistics-related counters and values
//

#define XRL_GET_PIMSTAT_PER_NODE(stat_name)			\
XrlCmdError							\
XrlPimNode::pim_0_1_pimstat_##stat_name(			\
    /* Output values , */ 					\
    uint32_t&		value)					\
{								\
    value = PimNode::pimstat_##stat_name();			\
								\
    return XrlCmdError::OKAY();					\
}

XRL_GET_PIMSTAT_PER_NODE(hello_messages_received)
XRL_GET_PIMSTAT_PER_NODE(hello_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(hello_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(register_messages_received)
XRL_GET_PIMSTAT_PER_NODE(register_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(register_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(register_stop_messages_received)
XRL_GET_PIMSTAT_PER_NODE(register_stop_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(register_stop_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(join_prune_messages_received)
XRL_GET_PIMSTAT_PER_NODE(join_prune_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(join_prune_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(bootstrap_messages_received)
XRL_GET_PIMSTAT_PER_NODE(bootstrap_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(bootstrap_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(assert_messages_received)
XRL_GET_PIMSTAT_PER_NODE(assert_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(assert_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(graft_messages_received)
XRL_GET_PIMSTAT_PER_NODE(graft_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(graft_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(graft_ack_messages_received)
XRL_GET_PIMSTAT_PER_NODE(graft_ack_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(graft_ack_messages_rx_errors)
XRL_GET_PIMSTAT_PER_NODE(candidate_rp_messages_received)
XRL_GET_PIMSTAT_PER_NODE(candidate_rp_messages_sent)
XRL_GET_PIMSTAT_PER_NODE(candidate_rp_messages_rx_errors)
//
XRL_GET_PIMSTAT_PER_NODE(unknown_type_messages)
XRL_GET_PIMSTAT_PER_NODE(unknown_version_messages)
XRL_GET_PIMSTAT_PER_NODE(neighbor_unknown_messages)
XRL_GET_PIMSTAT_PER_NODE(bad_length_messages)
XRL_GET_PIMSTAT_PER_NODE(bad_checksum_messages)
XRL_GET_PIMSTAT_PER_NODE(bad_receive_interface_messages)
XRL_GET_PIMSTAT_PER_NODE(rx_interface_disabled_messages)
XRL_GET_PIMSTAT_PER_NODE(rx_register_not_rp)
XRL_GET_PIMSTAT_PER_NODE(rp_filtered_source)
XRL_GET_PIMSTAT_PER_NODE(unknown_register_stop)
XRL_GET_PIMSTAT_PER_NODE(rx_join_prune_no_state)
XRL_GET_PIMSTAT_PER_NODE(rx_graft_graft_ack_no_state)
XRL_GET_PIMSTAT_PER_NODE(rx_graft_on_upstream_interface)
XRL_GET_PIMSTAT_PER_NODE(rx_candidate_rp_not_bsr)
XRL_GET_PIMSTAT_PER_NODE(rx_bsr_when_bsr)
XRL_GET_PIMSTAT_PER_NODE(rx_bsr_not_rpf_interface)
XRL_GET_PIMSTAT_PER_NODE(rx_unknown_hello_option)
XRL_GET_PIMSTAT_PER_NODE(rx_data_no_state)
XRL_GET_PIMSTAT_PER_NODE(rx_rp_no_state)
XRL_GET_PIMSTAT_PER_NODE(rx_aggregate)
XRL_GET_PIMSTAT_PER_NODE(rx_malformed_packet)
XRL_GET_PIMSTAT_PER_NODE(no_rp)
XRL_GET_PIMSTAT_PER_NODE(no_route_upstream)
XRL_GET_PIMSTAT_PER_NODE(rp_mismatch)
XRL_GET_PIMSTAT_PER_NODE(rpf_neighbor_unknown)
//
XRL_GET_PIMSTAT_PER_NODE(rx_join_rp)
XRL_GET_PIMSTAT_PER_NODE(rx_prune_rp)
XRL_GET_PIMSTAT_PER_NODE(rx_join_wc)
XRL_GET_PIMSTAT_PER_NODE(rx_prune_wc)
XRL_GET_PIMSTAT_PER_NODE(rx_join_sg)
XRL_GET_PIMSTAT_PER_NODE(rx_prune_sg)
XRL_GET_PIMSTAT_PER_NODE(rx_join_sg_rpt)
XRL_GET_PIMSTAT_PER_NODE(rx_prune_sg_rpt)

#undef XRL_GET_PIMSTAT_PER_NODE

#define XRL_GET_PIMSTAT_PER_VIF(stat_name)			\
XrlCmdError							\
XrlPimNode::pim_0_1_pimstat_##stat_name##_per_vif(		\
    /* Input values, */						\
    const string&	vif_name,				\
    /* Output values, */					\
    uint32_t&		value)					\
{								\
    string error_msg;						\
								\
    if (PimNode::pimstat_##stat_name##_per_vif(vif_name, value, error_msg) != XORP_OK) {	\
	return XrlCmdError::COMMAND_FAILED(error_msg);		\
    }								\
								\
    return XrlCmdError::OKAY();					\
}

XRL_GET_PIMSTAT_PER_VIF(hello_messages_received)
XRL_GET_PIMSTAT_PER_VIF(hello_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(hello_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(register_messages_received)
XRL_GET_PIMSTAT_PER_VIF(register_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(register_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(register_stop_messages_received)
XRL_GET_PIMSTAT_PER_VIF(register_stop_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(register_stop_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(join_prune_messages_received)
XRL_GET_PIMSTAT_PER_VIF(join_prune_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(join_prune_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(bootstrap_messages_received)
XRL_GET_PIMSTAT_PER_VIF(bootstrap_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(bootstrap_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(assert_messages_received)
XRL_GET_PIMSTAT_PER_VIF(assert_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(assert_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(graft_messages_received)
XRL_GET_PIMSTAT_PER_VIF(graft_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(graft_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(graft_ack_messages_received)
XRL_GET_PIMSTAT_PER_VIF(graft_ack_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(graft_ack_messages_rx_errors)
XRL_GET_PIMSTAT_PER_VIF(candidate_rp_messages_received)
XRL_GET_PIMSTAT_PER_VIF(candidate_rp_messages_sent)
XRL_GET_PIMSTAT_PER_VIF(candidate_rp_messages_rx_errors)
//
XRL_GET_PIMSTAT_PER_VIF(unknown_type_messages)
XRL_GET_PIMSTAT_PER_VIF(unknown_version_messages)
XRL_GET_PIMSTAT_PER_VIF(neighbor_unknown_messages)
XRL_GET_PIMSTAT_PER_VIF(bad_length_messages)
XRL_GET_PIMSTAT_PER_VIF(bad_checksum_messages)
XRL_GET_PIMSTAT_PER_VIF(bad_receive_interface_messages)
XRL_GET_PIMSTAT_PER_VIF(rx_interface_disabled_messages)
XRL_GET_PIMSTAT_PER_VIF(rx_register_not_rp)
XRL_GET_PIMSTAT_PER_VIF(rp_filtered_source)
XRL_GET_PIMSTAT_PER_VIF(unknown_register_stop)
XRL_GET_PIMSTAT_PER_VIF(rx_join_prune_no_state)
XRL_GET_PIMSTAT_PER_VIF(rx_graft_graft_ack_no_state)
XRL_GET_PIMSTAT_PER_VIF(rx_graft_on_upstream_interface)
XRL_GET_PIMSTAT_PER_VIF(rx_candidate_rp_not_bsr)
XRL_GET_PIMSTAT_PER_VIF(rx_bsr_when_bsr)
XRL_GET_PIMSTAT_PER_VIF(rx_bsr_not_rpf_interface)
XRL_GET_PIMSTAT_PER_VIF(rx_unknown_hello_option)
XRL_GET_PIMSTAT_PER_VIF(rx_data_no_state)
XRL_GET_PIMSTAT_PER_VIF(rx_rp_no_state)
XRL_GET_PIMSTAT_PER_VIF(rx_aggregate)
XRL_GET_PIMSTAT_PER_VIF(rx_malformed_packet)
XRL_GET_PIMSTAT_PER_VIF(no_rp)
XRL_GET_PIMSTAT_PER_VIF(no_route_upstream)
XRL_GET_PIMSTAT_PER_VIF(rp_mismatch)
XRL_GET_PIMSTAT_PER_VIF(rpf_neighbor_unknown)
//
XRL_GET_PIMSTAT_PER_VIF(rx_join_rp)
XRL_GET_PIMSTAT_PER_VIF(rx_prune_rp)
XRL_GET_PIMSTAT_PER_VIF(rx_join_wc)
XRL_GET_PIMSTAT_PER_VIF(rx_prune_wc)
XRL_GET_PIMSTAT_PER_VIF(rx_join_sg)
XRL_GET_PIMSTAT_PER_VIF(rx_prune_sg)
XRL_GET_PIMSTAT_PER_VIF(rx_join_sg_rpt)
XRL_GET_PIMSTAT_PER_VIF(rx_prune_sg_rpt)

#undef XRL_GET_PIMSTAT_PER_VIF
