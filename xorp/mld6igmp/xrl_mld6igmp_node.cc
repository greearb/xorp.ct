// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "mld6igmp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"
#include "libxorp/utils.hh"

#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"
#include "mld6igmp_vif.hh"
#include "xrl_mld6igmp_node.hh"

const TimeVal XrlMld6igmpNode::RETRY_TIMEVAL = TimeVal(1, 0);

//
// XrlMld6igmpNode front-end interface
//

XrlMld6igmpNode::XrlMld6igmpNode(int		family,
				 xorp_module_id	module_id, 
				 EventLoop&	eventloop,
				 const string&	class_name,
				 const string&	finder_hostname,
				 uint16_t	finder_port,
				 const string&	finder_target,
				 const string&	fea_target,
				 const string&	mfea_target)
    : Mld6igmpNode(family, module_id, eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlMld6igmpTargetBase(&xrl_router()),
      Mld6igmpNodeCli(*static_cast<Mld6igmpNode *>(this)),
      _eventloop(eventloop),
      _finder_target(finder_target),
      _fea_target(fea_target),
      _mfea_target(mfea_target),
      _ifmgr(eventloop, mfea_target.c_str(), xrl_router().finder_address(),
	     xrl_router().finder_port()),
      _xrl_fea_client4(&xrl_router()),
#ifdef HAVE_IPV6
      _xrl_fea_client6(&xrl_router()),
#endif
      _xrl_mld6igmp_client_client(&xrl_router()),
      _xrl_cli_manager_client(&xrl_router()),
      _xrl_finder_client(&xrl_router()),
      _is_finder_alive(false),
      _is_fea_alive(false),
      _is_fea_registered(false),
      _is_mfea_alive(false),
      _is_mfea_registered(false)
{
    _ifmgr.set_observer(dynamic_cast<Mld6igmpNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<Mld6igmpNode*>(this));
}

XrlMld6igmpNode::~XrlMld6igmpNode()
{
    shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<Mld6igmpNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<Mld6igmpNode*>(this));

    delete_pointers_list(_xrl_tasks_queue);
}

int
XrlMld6igmpNode::startup()
{
    if (start_mld6igmp() != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlMld6igmpNode::shutdown()
{
   int ret_value = XORP_OK;

    if (stop_cli() != XORP_OK)
	ret_value = XORP_ERROR;

    if (stop_mld6igmp() != XORP_OK)
	ret_value = XORP_ERROR;

    return (ret_value);
}

int
XrlMld6igmpNode::enable_cli()
{
    Mld6igmpNodeCli::enable();
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::disable_cli()
{
    Mld6igmpNodeCli::disable();
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::start_cli()
{
    if (Mld6igmpNodeCli::start() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_cli()
{
    if (Mld6igmpNodeCli::stop() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::enable_mld6igmp()
{
    Mld6igmpNode::enable();
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::disable_mld6igmp()
{
    Mld6igmpNode::disable();
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::start_mld6igmp()
{
    if (Mld6igmpNode::start() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_mld6igmp()
{
    if (Mld6igmpNode::stop() != XORP_OK)
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
XrlMld6igmpNode::finder_connect_event()
{
    _is_finder_alive = true;
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XrlMld6igmpNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    _is_finder_alive = false;

    stop_mld6igmp();
}

//
// Task-related methods
//
void
XrlMld6igmpNode::add_task(XrlTaskBase* xrl_task)
{
    _xrl_tasks_queue.push_back(xrl_task);

    // If the queue was empty before, start sending the changes
    if (_xrl_tasks_queue.size() == 1)
	send_xrl_task();
}

void
XrlMld6igmpNode::send_xrl_task()
{
    if (_xrl_tasks_queue.empty())
	return;

    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    XLOG_ASSERT(xrl_task_base != NULL);

    xrl_task_base->dispatch();
}

void
XrlMld6igmpNode::pop_xrl_task()
{
    XLOG_ASSERT(! _xrl_tasks_queue.empty());

    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    XLOG_ASSERT(xrl_task_base != NULL);

    delete xrl_task_base;
    _xrl_tasks_queue.pop_front();
}

void
XrlMld6igmpNode::retry_xrl_task()
{
    if (_xrl_tasks_queue_timer.scheduled())
	return;		// XXX: already scheduled

    _xrl_tasks_queue_timer = _eventloop.new_oneoff_after(
	RETRY_TIMEVAL,
	callback(this, &XrlMld6igmpNode::send_xrl_task));
}

//
// Register with the FEA
//
void
XrlMld6igmpNode::fea_register_startup()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_fea_registered)
	return;		// Already registered

    Mld6igmpNode::incr_startup_requests_n();	// XXX: for FEA registration
    Mld6igmpNode::incr_startup_requests_n();	// XXX: for FEA birth

    //
    // Register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _fea_target, true));
}

//
// Register with the MFEA
//
void
XrlMld6igmpNode::mfea_register_startup()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_is_mfea_registered)
	return;		// Already registered

    Mld6igmpNode::incr_startup_requests_n();	// XXX: for MFEA registration
    Mld6igmpNode::incr_startup_requests_n();	// XXX: for MFEA birth
    Mld6igmpNode::incr_startup_requests_n();	// XXX: for the ifmgr

    //
    // Register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _mfea_target, true));
}

//
// De-register with the FEA
//
void
XrlMld6igmpNode::fea_register_shutdown()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_fea_alive)
	return;		// The FEA is not there anymore

    if (! _is_fea_registered)
	return;		// Not registered

    Mld6igmpNode::incr_shutdown_requests_n();	// XXX: for FEA deregistration

    //
    // De-register interest in the FEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _fea_target, false));
}

//
// De-register with the MFEA
//
void
XrlMld6igmpNode::mfea_register_shutdown()
{
    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (! _is_mfea_alive)
	return;		// The MFEA is not there anymore

    if (! _is_mfea_registered)
	return;		// Not registered

    Mld6igmpNode::incr_shutdown_requests_n();	// XXX: for MFEA deregistration
    Mld6igmpNode::incr_shutdown_requests_n();	// XXX: for the ifmgr

    //
    // De-register interest in the MFEA with the Finder
    //
    add_task(new RegisterUnregisterInterest(*this, _mfea_target, false));

    //
    // XXX: when the shutdown is completed, Mld6igmpNode::status_change()
    // will be called.
    //
    _ifmgr.shutdown();
}

void
XrlMld6igmpNode::send_register_unregister_interest()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    RegisterUnregisterInterest* entry;

    entry = dynamic_cast<RegisterUnregisterInterest*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    if (entry->is_register()) {
	// Register interest
	success = _xrl_finder_client.send_register_class_event_interest(
	    _finder_target.c_str(), xrl_router().instance_name(),
	    entry->target_name(),
	    callback(this, &XrlMld6igmpNode::finder_send_register_unregister_interest_cb));
    } else {
	// Unregister interest
	success = _xrl_finder_client.send_deregister_class_event_interest(
	    _finder_target.c_str(), xrl_router().instance_name(),
	    entry->target_name(),
	    callback(this, &XrlMld6igmpNode::finder_send_register_unregister_interest_cb));
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to %s register interest in %s with the Finder. "
		   "Will try again.",
		   entry->operation_name(),
		   entry->target_name().c_str());
	retry_xrl_task();
	return;
    }
}

void
XrlMld6igmpNode::finder_send_register_unregister_interest_cb(const XrlError& xrl_error)
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
		Mld6igmpNode::decr_startup_requests_n(); // XXX: for FEA registration
	    }
	    if (entry->target_name() == _mfea_target) {
		//
		// If success, then the MFEA birth event will startup the MFEA
		// registration and the ifmgr.
		//
		_is_mfea_registered = true;
		Mld6igmpNode::decr_startup_requests_n(); // XXX: for MFEA registration
	    }
	} else {
	    //
	    // Unregister interest
	    //
	    if (entry->target_name() == _fea_target) {
		_is_fea_registered = false;
		Mld6igmpNode::decr_shutdown_requests_n(); // XXX: for the FEA
	    }
	    if (entry->target_name() == _mfea_target) {
		_is_mfea_registered = false;
		Mld6igmpNode::decr_shutdown_requests_n(); // XXX: for the MFEA
	    }
	}
	pop_xrl_task();
	send_xrl_task();
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
	    pop_xrl_task();
	    send_xrl_task();
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
	break;
    }
}

int
XrlMld6igmpNode::register_receiver(const string& if_name,
				   const string& vif_name,
				   uint8_t ip_protocol,
				   bool enable_multicast_loopback)
{
    Mld6igmpNode::incr_startup_requests_n();	// XXX: for FEA-receiver
    add_task(new RegisterUnregisterReceiver(*this, if_name, vif_name,
					    ip_protocol,
					    enable_multicast_loopback,
					    true));

    return (XORP_OK);
}

int
XrlMld6igmpNode::unregister_receiver(const string& if_name,
				     const string& vif_name,
				     uint8_t ip_protocol)
{
    Mld6igmpNode::incr_shutdown_requests_n();	// XXX: for FEA-non-receiver
    add_task(new RegisterUnregisterReceiver(*this, if_name, vif_name,
					    ip_protocol,
					    false,	// XXX: ignored
					    false));

    return (XORP_OK);
}

void
XrlMld6igmpNode::send_register_unregister_receiver()
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
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_register_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->enable_multicast_loopback(),
		callback(this, &XrlMld6igmpNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_register_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->enable_multicast_loopback(),
		callback(this, &XrlMld6igmpNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Unregister a receiver with the FEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_unregister_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlMld6igmpNode::fea_client_send_register_unregister_receiver_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_unregister_receiver(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		callback(this, &XrlMld6igmpNode::fea_client_send_register_unregister_receiver_cb));
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
XrlMld6igmpNode::fea_client_send_register_unregister_receiver_cb(
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
	    Mld6igmpNode::decr_startup_requests_n();  // XXX: for FEA-receiver
	else
	    Mld6igmpNode::decr_shutdown_requests_n(); // XXX: for FEA-non-receiver
	pop_xrl_task();
	send_xrl_task();
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
	    Mld6igmpNode::decr_shutdown_requests_n();  // XXX: for FEA-non-receiver
	    pop_xrl_task();
	    send_xrl_task();
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
	break;
    }
}

int
XrlMld6igmpNode::join_multicast_group(const string& if_name,
				      const string& vif_name,
				      uint8_t ip_protocol,
				      const IPvX& group_address)
{
    Mld6igmpNode::incr_startup_requests_n();		// XXX: for FEA-join
    add_task(new JoinLeaveMulticastGroup(*this, if_name, vif_name, ip_protocol,
					 group_address, true));

    return (XORP_OK);
}

int
XrlMld6igmpNode::leave_multicast_group(const string& if_name,
				       const string& vif_name,
				       uint8_t ip_protocol,
				       const IPvX& group_address)
{
    Mld6igmpNode::incr_shutdown_requests_n();		// XXX: for FEA-leave
    add_task(new JoinLeaveMulticastGroup(*this, if_name, vif_name, ip_protocol,
					 group_address, false));

    return (XORP_OK);
}

void
XrlMld6igmpNode::send_join_leave_multicast_group()
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
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_join_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv4(),
		callback(this, &XrlMld6igmpNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_join_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv6(),
		callback(this, &XrlMld6igmpNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}
#endif
    } else {
	// Leave a multicast group with the FEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_fea_client4.send_leave_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv4(),
		callback(this, &XrlMld6igmpNode::fea_client_send_join_leave_multicast_group_cb));
	    if (success)
		return;
	}

#ifdef HAVE_IPV6
	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_fea_client6.send_leave_multicast_group(
		_fea_target.c_str(),
		xrl_router().instance_name(),
		entry->if_name(),
		entry->vif_name(),
		entry->ip_protocol(),
		entry->group_address().get_ipv6(),
		callback(this, &XrlMld6igmpNode::fea_client_send_join_leave_multicast_group_cb));
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
XrlMld6igmpNode::fea_client_send_join_leave_multicast_group_cb(
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
	    Mld6igmpNode::decr_startup_requests_n();	// XXX: for FEA-join
	else
	    Mld6igmpNode::decr_shutdown_requests_n();	// XXX: for FEA-leave
	pop_xrl_task();
	send_xrl_task();
	break;

    case COMMAND_FAILED:

	XLOG_WARNING("Cannot %s a multicast group with the FEA: %s",
		     entry->operation_name(),
		     xrl_error.str().c_str());
	// If it was a join, try to leave it to clean things up as best as possible.
	// Currently, this doesn't seem to really do anything locally, but might
	// help FEA clean out it's logic.
	if (entry->is_join()) {
	    leave_multicast_group(entry->if_name(), entry->vif_name(),
				  entry->ip_protocol(), entry->group_address());
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
	if (entry->is_join()) {
	    XLOG_ERROR("XRL communication error: %s", xrl_error.str().c_str());
	} else {
	    Mld6igmpNode::decr_shutdown_requests_n();	// XXX: for FEA-leave
	    pop_xrl_task();
	    send_xrl_task();
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
	break;
    }
}

int
XrlMld6igmpNode::send_add_membership(const string& dst_module_instance_name,
				     xorp_module_id dst_module_id,
				     uint32_t vif_index,
				     const IPvX& source,
				     const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send add_membership to %s for (%s, %s) on vif "
		   "with vif_index %d: no such vif",
		   dst_module_instance_name.c_str(),
		   cstring(source),
		   cstring(group),
		   vif_index);
	return (XORP_ERROR);
    }

    _send_add_delete_membership_queue.push_back(SendAddDeleteMembership(
						    dst_module_instance_name,
						    dst_module_id,
						    vif_index,
						    source,
						    group,
						    true));

    // If the queue was empty before, start sending the changes
    if (_send_add_delete_membership_queue.size() == 1) {
	send_add_delete_membership();
    }

    return (XORP_OK);
}

int
XrlMld6igmpNode::send_delete_membership(const string& dst_module_instance_name,
					xorp_module_id dst_module_id,
					uint32_t vif_index,
					const IPvX& source,
					const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send delete_membership to %s for (%s, %s) on vif "
		   "with vif_index %d: no such vif",
		   dst_module_instance_name.c_str(),
		   cstring(source),
		   cstring(group),
		   vif_index);
	return (XORP_ERROR);
    }

    _send_add_delete_membership_queue.push_back(SendAddDeleteMembership(
						    dst_module_instance_name,
						    dst_module_id,
						    vif_index,
						    source,
						    group,
						    false));

    // If the queue was empty before, start sending the changes
    if (_send_add_delete_membership_queue.size() == 1) {
	send_add_delete_membership();
    }

    return (XORP_OK);
}

void
XrlMld6igmpNode::send_add_delete_membership()
{
    bool success = true;
    Mld6igmpVif *mld6igmp_vif = NULL;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    if (_send_add_delete_membership_queue.empty())
	return;			// No more changes

    const SendAddDeleteMembership& membership = _send_add_delete_membership_queue.front();

    mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(membership.vif_index());
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send %s for (%s, %s) on vif "
		   "with vif_index %d to %s: no such vif",
		   membership.operation_name(),
		   cstring(membership.source()),
		   cstring(membership.group()),
		   membership.vif_index(),
		   membership.dst_module_instance_name().c_str());
	_send_add_delete_membership_queue.pop_front();
	goto start_timer_label;
    }

    if (membership.is_add()) {
	// Send add_membership to the client protocol
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mld6igmp_client_client.send_add_membership4(
		membership.dst_module_instance_name().c_str(),
		xrl_router().class_name(),
		mld6igmp_vif->name(),
		membership.vif_index(),
		membership.source().get_ipv4(),
		membership.group().get_ipv4(),
		callback(this, &XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb));
	    if (success)
		return;
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mld6igmp_client_client.send_add_membership6(
		membership.dst_module_instance_name().c_str(),
		xrl_router().class_name(),
		mld6igmp_vif->name(),
		membership.vif_index(),
		membership.source().get_ipv6(),
		membership.group().get_ipv6(),
		callback(this, &XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb));
	    if (success)
		return;
	}
    } else {
	// Send delete_membership to the client protocol
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mld6igmp_client_client.send_delete_membership4(
		membership.dst_module_instance_name().c_str(),
		xrl_router().class_name(),
		mld6igmp_vif->name(),
		membership.vif_index(),
		membership.source().get_ipv4(),
		membership.group().get_ipv4(),
		callback(this, &XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb));
	    if (success)
		return;
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mld6igmp_client_client.send_delete_membership6(
		membership.dst_module_instance_name().c_str(),
		xrl_router().class_name(),
		mld6igmp_vif->name(),
		membership.vif_index(),
		membership.source().get_ipv6(),
		membership.group().get_ipv6(),
		callback(this, &XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then try again
	//
	XLOG_ERROR("Failed to send %s for (%s, %s) on vif %s to %s. "
		   "Will try again.",
		   membership.operation_name(),
		   cstring(membership.source()),
		   cstring(membership.group()),
		   mld6igmp_vif->name().c_str(),
		   membership.dst_module_instance_name().c_str());
    start_timer_label:
	_send_add_delete_membership_queue_timer = _eventloop.new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlMld6igmpNode::send_add_delete_membership));
    }
}

void
XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb(
    const XrlError& xrl_error)
{
#ifdef L_ERROR
    const SendAddDeleteMembership& membership = _send_add_delete_membership_queue.front();
#endif

    switch (xrl_error.error_code()) {
    case OKAY:
	//
	// If success, then schedule the next task
	//
	_send_add_delete_membership_queue.pop_front();
	send_add_delete_membership();
	break;

    case COMMAND_FAILED:
	//
	// If a command failed because the other side rejected it,
	// then print an error and send the next one.
	//
	XLOG_ERROR("Cannot %s for a multicast group with a client: %s:  Will continue.",
		   membership.operation_name(),
		   xrl_error.str().c_str());
	_send_add_delete_membership_queue.pop_front();
	send_add_delete_membership();
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
	// TODO:  We must make some progress or otherwise deal with this error.
	// For now, fall through and re-try.
	// Fall through to the retry logic below.
	/* fall through */

    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// If a transient error, then try again
	//
	if (! _send_add_delete_membership_queue_timer.scheduled()) {
	    XLOG_ERROR("Failed to %s for a multicast group with a client: %s. "
		       "Will try again.",
		       membership.operation_name(),
		       xrl_error.str().c_str());
	    _send_add_delete_membership_queue_timer = _eventloop.new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlMld6igmpNode::send_add_delete_membership));
	}
	break;
    }/* switch */
}

//
// Protocol node methods
//

int
XrlMld6igmpNode::proto_send(const string& if_name,
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
XrlMld6igmpNode::send_protocol_message()
{
    bool success = true;

    if (! _is_finder_alive)
	return;		// The Finder is dead

    XLOG_ASSERT(! _xrl_tasks_queue.empty());
    XrlTaskBase* xrl_task_base = _xrl_tasks_queue.front();
    SendProtocolMessage* entry;

    entry = dynamic_cast<SendProtocolMessage*>(xrl_task_base);
    XLOG_ASSERT(entry != NULL);

    //
    // Check whether we have already registered with the FEA
    //
    if (! _is_fea_registered) {
	retry_xrl_task();
	return;
    }

    //
    // Send the protocol message
    //
    do {
	if (Mld6igmpNode::is_ipv4()) {
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
		callback(this, &XrlMld6igmpNode::fea_client_send_protocol_message_cb));
	    if (success)
		return;
	    break;
	}

#ifdef HAVE_IPV6
	if (Mld6igmpNode::is_ipv6()) {
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
		callback(this, &XrlMld6igmpNode::fea_client_send_protocol_message_cb));
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
XrlMld6igmpNode::fea_client_send_protocol_message_cb(const XrlError& xrl_error)
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
	pop_xrl_task();
	send_xrl_task();
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
	pop_xrl_task();
	send_xrl_task();
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
	pop_xrl_task();
	send_xrl_task();
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
	pop_xrl_task();
	send_xrl_task();
	break;
    }
}

//
// Protocol node CLI methods
//
int
XrlMld6igmpNode::add_cli_command_to_cli_manager(const char *command_name,
						const char *command_help,
						bool is_command_cd,
						const char *command_cd_prompt,
						bool is_command_processor
    )
{
    bool success = false;

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
	callback(this, &XrlMld6igmpNode::cli_manager_client_send_add_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to add CLI command '%s' to the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlMld6igmpNode::cli_manager_client_send_add_cli_command_cb(
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
XrlMld6igmpNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    bool success = true;

    if (! _is_finder_alive)
	return (XORP_ERROR);	// The Finder is dead

    success = _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	xrl_router().class_name(),
	string(command_name),
	callback(this, &XrlMld6igmpNode::cli_manager_client_send_delete_cli_command_cb));

    if (! success) {
	XLOG_ERROR("Failed to delete CLI command '%s' with the CLI manager",
		   command_name);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlMld6igmpNode::cli_manager_client_send_delete_cli_command_cb(
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
XrlMld6igmpNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = xrl_router().class_name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::common_0_1_get_status(// Output values, 
				       uint32_t& status,
				       string& reason)
{
    status = Mld6igmpNode::node_status(reason);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::common_0_1_shutdown()
{
    if (shutdown() != XORP_OK) {
	string error_msg = c_format("Failed to shutdown %s",
			     Mld6igmpNode::proto_is_igmp() ?
			     "IGMP" : "MLD");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::common_0_1_startup()
{
    if (startup() != XORP_OK) {
	string error_msg = c_format("Failed to startup %s",
			     Mld6igmpNode::proto_is_igmp() ?
			     "IGMP" : "MLD");
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
XrlMld6igmpNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    if (target_class == _fea_target) {
	_is_fea_alive = true;
	Mld6igmpNode::decr_startup_requests_n();	// XXX: for FEA birth
    }

    if (target_class == _mfea_target) {
	_is_mfea_alive = true;
	Mld6igmpNode::decr_startup_requests_n();	// XXX: for MFEA birth
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	if (_ifmgr.startup() != XORP_OK) {
	    Mld6igmpNode::set_status(SERVICE_FAILED);
	    Mld6igmpNode::update_status();
	}
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
XrlMld6igmpNode::finder_event_observer_0_1_xrl_target_death(
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

    if (do_shutdown)
	stop_mld6igmp();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::cli_processor_0_1_process_command(
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
    Mld6igmpNodeCli::cli_process_command(processor_name,
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
XrlMld6igmpNode::raw_packet4_client_0_1_recv(
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
    if (! Mld6igmpNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the message
    //
    Mld6igmpNode::proto_recv(if_name,
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

XrlCmdError
XrlMld6igmpNode::raw_packet6_client_0_1_recv(
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
    if (! Mld6igmpNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Receive the message
    //
    Mld6igmpNode::proto_recv(if_name,
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

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_vif(
    // Input values,
    const string&	vif_name,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = Mld6igmpNode::enable_vif(vif_name, error_msg);
    else
	ret_value = Mld6igmpNode::disable_vif(vif_name, error_msg);

    if (ret_value != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::start_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::stop_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_all_vifs(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = Mld6igmpNode::enable_all_vifs();
    else
	ret_value = Mld6igmpNode::enable_all_vifs();

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
XrlMld6igmpNode::mld6igmp_0_1_start_all_vifs()
{
    string error_msg;

    if (Mld6igmpNode::start_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_all_vifs()
{
    string error_msg;

    if (Mld6igmpNode::stop_all_vifs() != XORP_OK) {
	error_msg = c_format("Failed to stop all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_mld6igmp(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_mld6igmp();
    else
	ret_value = disable_mld6igmp();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable MLD6IGMP");
	else
	    error_msg = c_format("Failed to disable MLD6IGMP");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_mld6igmp()
{
    string error_msg;

    if (start_mld6igmp() != XORP_OK) {
	error_msg = c_format("Failed to start MLD6IMGP");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_mld6igmp()
{
    string error_msg;

    if (stop_mld6igmp() != XORP_OK) {
	error_msg = c_format("Failed to stop MLD6IMGP");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_enable_cli(
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
	    error_msg = c_format("Failed to enable MLD6IGMP CLI");
	else
	    error_msg = c_format("Failed to disable MLD6IGMP CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_start_cli()
{
    string error_msg;

    if (start_cli() != XORP_OK) {
	error_msg = c_format("Failed to start MLD6IMGP CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_cli()
{
    string error_msg;

    if (stop_cli() != XORP_OK) {
	error_msg = c_format("Failed to stop MLD6IMGP CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		proto_version)
{
    string error_msg;
    
    int v;
    if (Mld6igmpNode::get_vif_proto_version(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    proto_version = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_proto_version(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	proto_version)
{
    string error_msg;
    
    if (Mld6igmpNode::set_vif_proto_version(vif_name, proto_version, error_msg)
	!= XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_proto_version(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_proto_version(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_ip_router_alert_option_check(
    // Input values,
    const string&	vif_name,
    // Output values,
    bool&	enabled)
{
    string error_msg;
    
    bool v;
    if (Mld6igmpNode::get_vif_ip_router_alert_option_check(vif_name, v,
							   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    enabled = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_ip_router_alert_option_check(
    // Input values,
    const string&	vif_name,
    const bool&		enable)
{
    string error_msg;
    
    if (Mld6igmpNode::set_vif_ip_router_alert_option_check(vif_name, enable,
							   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_ip_router_alert_option_check(
    // Input values,
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_ip_router_alert_option_check(vif_name,
							     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_query_interval(
    // Input values,
    const string&	vif_name,
    // Output values,
    uint32_t&		interval_sec,
    uint32_t&		interval_usec)
{
    string error_msg;
    
    TimeVal v;
    if (Mld6igmpNode::get_vif_query_interval(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    interval_sec = v.sec();
    interval_usec = v.usec();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_query_interval(
    // Input values,
    const string&	vif_name,
    const uint32_t&	interval_sec,
    const uint32_t&	interval_usec)
{
    string error_msg;

    TimeVal interval(interval_sec, interval_usec);
    if (Mld6igmpNode::set_vif_query_interval(vif_name, interval, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_query_interval(
    // Input values,
    const string&	vif_name)

{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_query_interval(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_query_last_member_interval(
    // Input values,
    const string&	vif_name,
    // Output values,
    uint32_t&		interval_sec,
    uint32_t&		interval_usec)
{
    string error_msg;
    
    TimeVal v;
    if (Mld6igmpNode::get_vif_query_last_member_interval(vif_name, v,
							 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    interval_sec = v.sec();
    interval_usec = v.usec();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_query_last_member_interval(
    // Input values,
    const string&	vif_name,
    const uint32_t&	interval_sec,
    const uint32_t&	interval_usec)
{
    string error_msg;

    TimeVal interval(interval_sec, interval_usec);
    if (Mld6igmpNode::set_vif_query_last_member_interval(vif_name, interval,
							 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_query_last_member_interval(
    // Input values,
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_query_last_member_interval(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_query_response_interval(
    // Input values,
    const string&	vif_name,
    // Output values,
    uint32_t&		interval_sec,
    uint32_t&		interval_usec)
{
    string error_msg;
    
    TimeVal v;
    if (Mld6igmpNode::get_vif_query_response_interval(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    interval_sec = v.sec();
    interval_usec = v.usec();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_query_response_interval(
    // Input values,
    const string&	vif_name,
    const uint32_t&	interval_sec,
    const uint32_t&	interval_usec)
{
    string error_msg;

    TimeVal interval(interval_sec, interval_usec);
    if (Mld6igmpNode::set_vif_query_response_interval(vif_name, interval,
						      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_query_response_interval(
    // Input values,
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_query_response_interval(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_get_vif_robust_count(
    // Input values,
    const string&	vif_name,
    // Output values,
    uint32_t&	robust_count)
{
    string error_msg;
    
    uint32_t v;
    if (Mld6igmpNode::get_vif_robust_count(vif_name, v, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    robust_count = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_set_vif_robust_count(
    // Input values,
    const string&	vif_name,
    const uint32_t&	robust_count)
{
    string error_msg;

    if (Mld6igmpNode::set_vif_robust_count(vif_name, robust_count, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_reset_vif_robust_count(
    // Input values,
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::reset_vif_robust_count(vif_name, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_log_trace_all(
    // Input values,
    const bool&	enable)
{
    Mld6igmpNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_add_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	error_msg = c_format("Invalid module ID = %d",
			     XORP_INT_CAST(protocol_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	!= XORP_OK) {
	// TODO: must find-out and return the reason for failure
	error_msg = c_format("Cannot add protocol instance '%s' "
			     "on vif %s with vif_index %d",
			     xrl_sender_name.c_str(),
			     vif_name.c_str(),
			     XORP_INT_CAST(vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index, error_msg);
	error_msg = c_format("Cannot add protocol instance '%s' "
			     "on vif %s with vif_index %d: "
			     "no such vif",
			     xrl_sender_name.c_str(),
			     vif_name.c_str(),
			     XORP_INT_CAST(vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    Mld6igmpGroupSet::const_iterator iter;
    for (iter = mld6igmp_vif->group_records().begin();
	 iter != mld6igmp_vif->group_records().end();
	 ++iter) {
	const Mld6igmpGroupRecord *group_record = iter->second;
	send_add_membership(xrl_sender_name.c_str(),
			    src_module_id,
			    mld6igmp_vif->vif_index(),
			    IPvX::ZERO(family()),
			    group_record->group());
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_add_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    string error_msg;

    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	error_msg = c_format("Invalid module ID = %d",
			     XORP_INT_CAST(protocol_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	!= XORP_OK) {
	// TODO: must find-out and return the reason for failure
	error_msg = c_format("Cannot add protocol instance '%s' "
			     "on vif %s with vif_index %d",
			     xrl_sender_name.c_str(),
			     vif_name.c_str(),
			     XORP_INT_CAST(vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index, error_msg);
	error_msg = c_format("Cannot add protocol instance '%s' "
			     "on vif %s with vif_index %d: "
			     "no such vif",
			     xrl_sender_name.c_str(),
			     vif_name.c_str(),
			     XORP_INT_CAST(vif_index));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    Mld6igmpGroupSet::const_iterator iter;
    for (iter = mld6igmp_vif->group_records().begin();
	 iter != mld6igmp_vif->group_records().end();
	 ++iter) {
	const Mld6igmpGroupRecord *group_record = iter->second;
	send_add_membership(xrl_sender_name.c_str(),
			    src_module_id,
			    mld6igmp_vif->vif_index(),
			    IPvX::ZERO(family()),
			    group_record->group());
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_delete_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    UNUSED(vif_name);
    string error_msg;

    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv4()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	error_msg = c_format("Invalid module ID = %d",
			     XORP_INT_CAST(protocol_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_delete_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    UNUSED(vif_name);
    string error_msg;

    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv6()) {
	error_msg = c_format("Received protocol message with "
			     "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	error_msg = c_format("Invalid module ID = %d",
			     XORP_INT_CAST(protocol_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
