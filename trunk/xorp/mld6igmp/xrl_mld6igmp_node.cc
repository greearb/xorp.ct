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

#ident "$XORP: xorp/mld6igmp/xrl_mld6igmp_node.cc,v 1.27 2004/05/30 03:41:22 pavlin Exp $"

#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "libxorp/status_codes.h"
#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"
#include "mld6igmp_vif.hh"
#include "xrl_mld6igmp_node.hh"


//
// XrlMld6igmpNode front-end interface
//

XrlMld6igmpNode::XrlMld6igmpNode(int family,
				 xorp_module_id module_id, 
				 EventLoop& eventloop,
				 XrlRouter* xrl_router,
				 const string& mfea_target)
    : Mld6igmpNode(family, module_id, eventloop),
      XrlMld6igmpTargetBase(xrl_router),
      Mld6igmpNodeCli(*static_cast<Mld6igmpNode *>(this)),
      _class_name(xrl_router->class_name()),
      _instance_name(xrl_router->instance_name()),
      _xrl_mfea_client(xrl_router),
      _xrl_mld6igmp_client_client(xrl_router),
      _xrl_cli_manager_client(xrl_router),
      _mfea_target(mfea_target),
      _is_mfea_add_protocol_registered(false)
{

}

XrlMld6igmpNode::~XrlMld6igmpNode()
{
    Mld6igmpNodeCli::stop();
    Mld6igmpNode::stop();
}

bool
XrlMld6igmpNode::startup()
{
    if (start_mld6igmp() < 0)
	return false;

    return true;
}

bool
XrlMld6igmpNode::shutdown()
{
    if (stop_mld6igmp() < 0)
	return false;

    return true;
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
    if (Mld6igmpNodeCli::start() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_cli()
{
    if (Mld6igmpNodeCli::stop() < 0)
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
    if (Mld6igmpNode::start() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_mld6igmp()
{
    if (Mld6igmpNode::stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

//
// Protocol node methods
//

void
XrlMld6igmpNode::mfea_register_startup()
{
    if (! _is_mfea_add_protocol_registered)
	Mld6igmpNode::incr_startup_requests_n();

    send_mfea_registration();
}

void
XrlMld6igmpNode::mfea_register_shutdown()
{
    if (_is_mfea_add_protocol_registered)
	Mld6igmpNode::incr_shutdown_requests_n();

    send_mfea_deregistration();
}

//
// Register with the MFEA
//
void
XrlMld6igmpNode::send_mfea_registration()
{
    bool success = true;

    //
    // Register the protocol with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_add_protocol4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_add_protocol_cb));
	    if (success)
		return;
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_add_protocol6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_add_protocol_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then start a timer to try again
	// TODO: XXX: the timer value is hardcoded here!!
	//
	XLOG_ERROR("Failed to register with the MFEA. "
		   "Will try again.");
	_mfea_registration_timer = Mld6igmpNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlMld6igmpNode::send_mfea_registration));
	return;
    }
}

void
XrlMld6igmpNode::mfea_client_send_add_protocol_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_add_protocol_registered = true;
	send_mfea_registration();
	Mld6igmpNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot register with the MFEA: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_mfea_registration_timer.scheduled())
	return;
    _mfea_registration_timer = Mld6igmpNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlMld6igmpNode::send_mfea_registration));
}

//
// De-register with the MFEA
//
void
XrlMld6igmpNode::send_mfea_deregistration()
{
    bool success = true;

    if (_is_mfea_add_protocol_registered) {
	if (Mld6igmpNode::is_ipv4()) {
	    bool success4;
	    success4 = _xrl_mfea_client.send_delete_protocol4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_delete_protocol_cb));
	    if (success4 != true)
		success = false;
	}

	if (Mld6igmpNode::is_ipv6()) {
	    bool success6;
	    success6 = _xrl_mfea_client.send_delete_protocol6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_delete_protocol_cb));
	    if (success6 != true)
		success = false;
	}
    }

    if (! success) {
	XLOG_ERROR("Failed to deregister with the MFEA. "
		   "Will give up.");
	Mld6igmpNode::set_status(FAILED);
	Mld6igmpNode::update_status();
    }
}

void
XrlMld6igmpNode::mfea_client_send_delete_protocol_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_add_protocol_registered = false;
	Mld6igmpNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister with the MFEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Mld6igmpNode::set_status(FAILED);
    Mld6igmpNode::update_status();
}

int
XrlMld6igmpNode::start_protocol_kernel_vif(uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot start in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    _start_stop_protocol_kernel_vif_queue.push_back(make_pair(vif_index, true));

    // If the queue was empty before, start sending the changes
    if (_start_stop_protocol_kernel_vif_queue.size() == 1) {
	send_start_stop_protocol_kernel_vif();
    }

    return (XORP_OK);
}

int
XrlMld6igmpNode::stop_protocol_kernel_vif(uint16_t vif_index)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot stop in the kernel vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }
    
    _start_stop_protocol_kernel_vif_queue.push_back(make_pair(vif_index, false));

    // If the queue was empty before, start sending the changes
    if (_start_stop_protocol_kernel_vif_queue.size() == 1) {
	send_start_stop_protocol_kernel_vif();
    }

    return (XORP_OK);
}

void
XrlMld6igmpNode::send_start_stop_protocol_kernel_vif()
{
    bool success = true;
    Mld6igmpVif *mld6igmp_vif = NULL;

    if (_start_stop_protocol_kernel_vif_queue.empty())
	return;			// No more changes

    uint16_t vif_index = _start_stop_protocol_kernel_vif_queue.front().first;
    bool is_start = _start_stop_protocol_kernel_vif_queue.front().second;

    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	success = false;
	goto start_timer_label;
    }

    mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot %s protocol vif with vif_index %d with the MFEA: "
		   "no such vif",
		   (is_start)? "start" : "stop",
		   vif_index);
	_start_stop_protocol_kernel_vif_queue.pop_front();
	return;
    }

    if (is_start) {
	// Start a vif with the MFEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_start_protocol_vif4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		Mld6igmpNode::incr_startup_requests_n();
		return;
	    }
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_start_protocol_vif6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		Mld6igmpNode::incr_startup_requests_n();
		return;
	    }
	}
    } else {
	// Stop a vif with the MFEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_stop_protocol_vif4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		Mld6igmpNode::incr_shutdown_requests_n();
		return;
	    }
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_stop_protocol_vif6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		callback(this, &XrlMld6igmpNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		Mld6igmpNode::incr_shutdown_requests_n();
		return;
	    }
	}
    }

    if (! success) {
        //
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Failed to %s protocol vif %s with the MFEA. "
		   "Will try again.",
		   (is_start)? "start" : "stop",
		   mld6igmp_vif->name().c_str());
    start_timer_label:
	_start_stop_protocol_kernel_vif_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlMld6igmpNode::send_start_stop_protocol_kernel_vif));
    }
}

void
XrlMld6igmpNode::mfea_client_send_start_stop_protocol_kernel_vif_cb(const XrlError& xrl_error)
{
    bool is_start = _start_stop_protocol_kernel_vif_queue.front().second;

    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_start_stop_protocol_kernel_vif_queue.pop_front();
	send_start_stop_protocol_kernel_vif();
	if (is_start)
	    Mld6igmpNode::decr_startup_requests_n();
	else
	    Mld6igmpNode::decr_shutdown_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot %s protocol vif with the MFEA: %s",
		   (is_start)? "start" : "stop",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _start_stop_protocol_kernel_vif_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlMld6igmpNode::send_start_stop_protocol_kernel_vif));
}

int
XrlMld6igmpNode::join_multicast_group(uint16_t vif_index,
				 const IPvX& multicast_group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot join group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    _join_leave_multicast_group_queue.push_back(
	JoinLeaveMulticastGroup(vif_index, multicast_group, true));

    // If the queue was empty before, start sending the changes
    if (_join_leave_multicast_group_queue.size() == 1) {
	send_join_leave_multicast_group();
    }

    return (XORP_OK);
}

int
XrlMld6igmpNode::leave_multicast_group(uint16_t vif_index,
				       const IPvX& multicast_group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot leave group %s on vif with vif_index %d: "
		   "no such vif", cstring(multicast_group), vif_index);
	return (XORP_ERROR);
    }
    
    _join_leave_multicast_group_queue.push_back(
	JoinLeaveMulticastGroup(vif_index, multicast_group, false));

    // If the queue was empty before, start sending the changes
    if (_join_leave_multicast_group_queue.size() == 1) {
	send_join_leave_multicast_group();
    }

    return (XORP_OK);
}

void
XrlMld6igmpNode::send_join_leave_multicast_group()
{
    bool success = true;
    Mld6igmpVif *mld6igmp_vif = NULL;

    if (_join_leave_multicast_group_queue.empty())
	return;			// No more changes

    JoinLeaveMulticastGroup& group = _join_leave_multicast_group_queue.front();
    bool is_join = group.is_join();

    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	success = false;
	goto start_timer_label;
    }

    mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(group.vif_index());
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot %s group %s on vif with vif_index %d: "
		   "no such vif",
		   (is_join)? "join" : "leave",
		   cstring(group.multicast_group()),
		   group.vif_index());
	_join_leave_multicast_group_queue.pop_front();
	return;
    }

    if (is_join) {
	// Join a multicast group on a vif with the MFEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_join_multicast_group4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv4(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		Mld6igmpNode::incr_startup_requests_n();
		return;
	    }
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_join_multicast_group6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv6(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		Mld6igmpNode::incr_startup_requests_n();
		return;
	    }
	}
    } else {
	// Leave a multicast group on a vif with the MFEA
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_leave_multicast_group4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv4(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		Mld6igmpNode::incr_shutdown_requests_n();
		return;
	    }
	}

	if (Mld6igmpNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_leave_multicast_group6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv6(),
		callback(this, &XrlMld6igmpNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		Mld6igmpNode::incr_shutdown_requests_n();
		return;
	    }
	}
    }

    if (! success) {
        //
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Failed to %s group %s on vif %s with the MFEA. "
		   "Will try again.",
		   (is_join)? "join" : "leave",
		   cstring(group.multicast_group()),
		   mld6igmp_vif->name().c_str());
    start_timer_label:
	_join_leave_multicast_group_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlMld6igmpNode::send_join_leave_multicast_group));
    }
}

void
XrlMld6igmpNode::mfea_client_send_join_leave_multicast_group_cb(const XrlError& xrl_error)
{
    bool is_join = _join_leave_multicast_group_queue.front().is_join();

    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_join_leave_multicast_group_queue.pop_front();
	send_join_leave_multicast_group();
	if (is_join)
	    Mld6igmpNode::decr_startup_requests_n();
	else
	    Mld6igmpNode::decr_shutdown_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot %s a multicast group with the MFEA: %s",
		   (is_join)? "join" : "leave",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _join_leave_multicast_group_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlMld6igmpNode::send_join_leave_multicast_group));
}

int
XrlMld6igmpNode::send_add_membership(const string& dst_module_instance_name,
				     xorp_module_id dst_module_id,
				     uint16_t vif_index,
				     const IPvX& source,
				     const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send add_membership to %s for (%s,%s) on vif "
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
					uint16_t vif_index,
					const IPvX& source,
					const IPvX& group)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send delete_membership to %s for (%s,%s) on vif "
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

    if (_send_add_delete_membership_queue.empty())
	return;			// No more changes

    const SendAddDeleteMembership& membership = _send_add_delete_membership_queue.front();
    bool is_add = membership.is_add();

    mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(membership.vif_index());
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send %s for (%s,%s) on vif "
		   "with vif_index %d to %s: no such vif",
		   (is_add)? "add_membership" : "delete_membership",
		   cstring(membership.source()),
		   cstring(membership.group()),
		   membership.vif_index(),
		   membership.dst_module_instance_name().c_str());
	_send_add_delete_membership_queue.pop_front();
	goto start_timer_label;
    }

    if (is_add) {
	// Send add_membership to the client protocol
	if (Mld6igmpNode::is_ipv4()) {
	    success = _xrl_mld6igmp_client_client.send_add_membership4(
		membership.dst_module_instance_name().c_str(),
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Failed to send %s for (%s,%s) on vif %s to %s. "
		   "Will try again.",
		   (is_add)? "add_membership" : "delete_membership",
		   cstring(membership.source()),
		   cstring(membership.group()),
		   mld6igmp_vif->name().c_str(),
		   membership.dst_module_instance_name().c_str());
    start_timer_label:
	_send_add_delete_membership_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlMld6igmpNode::send_add_delete_membership));
    }
}

void
XrlMld6igmpNode::mld6igmp_client_send_add_delete_membership_cb(const XrlError& xrl_error)
{
    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_send_add_delete_membership_queue.pop_front();
	send_add_delete_membership();
	return;
    }

    //
    // If a command failed because the other side rejected it,
    // then send the next one.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	const SendAddDeleteMembership& membership = _send_add_delete_membership_queue.front();
	bool is_add = membership.is_add();

	XLOG_ERROR("Cannot %s a multicast group with a client: %s",
		   (is_add)? "add" : "delete",
		   xrl_error.str().c_str());

	_send_add_delete_membership_queue.pop_front();
	send_add_delete_membership();
	return;
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _send_add_delete_membership_queue_timer = Mld6igmpNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlMld6igmpNode::send_add_delete_membership));
}

/**
 * XrlMld6igmpNode::proto_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the destination of the message.
 * @vif_index: The vif index of the interface to send this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * the TTL will be set by the lower layers.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * the TOS will be set by the lower layers.
 * @router_alert_bool: If true, the Router Alert IP option for the IP
 * packet of the incoming message should be set.
 * @sndbuf: The data buffer with the message to send.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a protocol message through the FEA/MFEA.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMld6igmpNode::proto_send(const string& dst_module_instance_name,
			    xorp_module_id		, // dst_module_id,
			    uint16_t vif_index,
			    const IPvX& src,
			    const IPvX& dst,
			    int ip_ttl,
			    int ip_tos,
			    bool router_alert_bool,
			    const uint8_t *sndbuf,
			    size_t sndlen)
{
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    
    if (mld6igmp_vif == NULL) {
	XLOG_ERROR("Cannot send a protocol message on vif with vif_index %d: "
		   "no such vif",
		   vif_index);
	return (XORP_ERROR);
    }
    
    // Copy 'sndbuf' to a vector
    vector<uint8_t> snd_vector;
    snd_vector.resize(sndlen);
    for (size_t i = 0; i < sndlen; i++)
	snd_vector[i] = sndbuf[i];
    
    do {
	if (dst.is_ipv4()) {
	    _xrl_mfea_client.send_send_protocol_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMld6igmpNode::mfea_client_send_protocol_message_cb));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    _xrl_mfea_client.send_send_protocol_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(Mld6igmpNode::module_name()),
		Mld6igmpNode::module_id(),
		mld6igmp_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMld6igmpNode::mfea_client_send_protocol_message_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::mfea_client_send_protocol_message_cb(const XrlError& xrl_error)
{
    if (xrl_error == XrlError::OKAY())
	return;

    //
    // XXX: all protocol messages are soft-state
    // (i.e., they are retransmitted periodically by the protocol),
    // hence we don't retransmit them here if there was an error.
    //
    XLOG_ERROR("Failed to send a protocol message: %s",
	       xrl_error.str().c_str());
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
    _xrl_cli_manager_client.send_add_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	string(command_help),
	is_command_cd,
	string(command_cd_prompt),
	is_command_processor,
	callback(this, &XrlMld6igmpNode::cli_manager_client_send_add_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error)
{
    if (xrl_error == XrlError::OKAY())
	return;

    //
    // TODO: if the command failed, then we should retransmit it
    //
    XLOG_ERROR("Failed to add a command to CLI manager: %s",
	       xrl_error.str().c_str());
}

int
XrlMld6igmpNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlMld6igmpNode::cli_manager_client_send_delete_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlMld6igmpNode::cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error)
{
    if (xrl_error == XrlError::OKAY())
	return;

    //
    // TODO: if the command failed, then we should retransmit it
    //
    XLOG_ERROR("Failed to delete a command from CLI manager: %s",
	       xrl_error.str().c_str());
}


//
// XRL target methods
//

XrlCmdError
XrlMld6igmpNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    
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
    bool is_error = false;
    string error_msg;

    if (stop_mld6igmp() != XORP_OK) {
	if (! is_error)
	    error_msg = c_format("Failed to stop %s",
				 Mld6igmpNode::proto_is_igmp() ?
				 "IGMP" : "MLD");
	is_error = true;
    }

    if (is_error)
	return XrlCmdError::COMMAND_FAILED(error_msg);

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
XrlMld6igmpNode::mfea_client_0_1_new_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    string error_msg;
    
    if (Mld6igmpNode::add_vif(vif_name, vif_index, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (Mld6igmpNode::delete_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_add_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		addr, 
    const IPv4Net&	subnet, 
    const IPv4&		broadcast, 
    const IPv4&		peer)
{
    string error_msg;
    
    if (Mld6igmpNode::add_vif_addr(vif_name,
				   IPvX(addr),
				   IPvXNet(subnet),
				   IPvX(broadcast),
				   IPvX(peer),
				   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_add_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		addr, 
    const IPv6Net&	subnet, 
    const IPv6&		broadcast, 
    const IPv6&		peer)
{
    string error_msg;
    
    if (Mld6igmpNode::add_vif_addr(vif_name,
				   IPvX(addr),
				   IPvXNet(subnet),
				   IPvX(broadcast),
				   IPvX(peer),
				   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		addr)
{
    string error_msg;
    
    if (Mld6igmpNode::delete_vif_addr(vif_name,
				      IPvX(addr),
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_delete_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		addr)
{
    string error_msg;
    
    if (Mld6igmpNode::delete_vif_addr(vif_name,
				      IPvX(addr),
				      error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_set_vif_flags(
    // Input values, 
    const string&	vif_name, 
    const bool&		is_pim_register, 
    const bool&		is_p2p, 
    const bool&		is_loopback, 
    const bool&		is_multicast, 
    const bool&		is_broadcast, 
    const bool&		is_up)
{
    string error_msg;
    
    if (Mld6igmpNode::set_vif_flags(vif_name,
				    is_pim_register,
				    is_p2p,
				    is_loopback,
				    is_multicast,
				    is_broadcast,
				    is_up,
				    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_set_all_vifs_done()
{
    Mld6igmpNode::set_vif_setup_completed(true);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_is_vif_setup_completed(
    // Output values, 
    bool&	is_completed)
{
    is_completed = Mld6igmpNode::is_vif_setup_completed();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_recv_protocol_message4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source_address, 
    const IPv4&		dest_address, 
    const int32_t&	ip_ttl, 
    const int32_t&	ip_tos, 
    const bool&		is_router_alert, 
    const vector<uint8_t>& protocol_message)
{
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Receive the message
    //
    Mld6igmpNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mfea_client_0_1_recv_protocol_message6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source_address, 
    const IPv6&		dest_address, 
    const int32_t&	ip_ttl, 
    const int32_t&	ip_tos, 
    const bool&		is_router_alert, 
    const vector<uint8_t>& protocol_message)
{
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Receive the message
    //
    Mld6igmpNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size());
    // XXX: no error returned, because if there is any, it is at the
    // protocol level, and the MFEA shoudn't care about it.
    
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
    if (Mld6igmpNode::start_all_vifs() < 0) {
	string error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_all_vifs()
{
    if (Mld6igmpNode::stop_all_vifs() < 0) {
	string error_msg = c_format("Failed to stop all vifs");
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
    if (start_mld6igmp() != XORP_OK) {
	string error_msg = c_format("Failed to start MLD6IMGP");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_mld6igmp()
{
    if (stop_mld6igmp() != XORP_OK) {
	string error_msg = c_format("Failed to stop MLD6IMGP");
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
    if (start_cli() != XORP_OK) {
	string error_msg = c_format("Failed to start MLD6IMGP CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMld6igmpNode::mld6igmp_0_1_stop_cli()
{
    if (stop_cli() != XORP_OK) {
	string error_msg = c_format("Failed to stop MLD6IMGP CLI");
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
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "on vif %s with vif_index %d",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index);
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "on vif %s with vif_index %d: "
				    "no such vif",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    list<MemberQuery *>::const_iterator iter;
    for (iter = mld6igmp_vif->members().begin();
	 iter != mld6igmp_vif->members().end();
	 ++iter) {
	const MemberQuery *member_query = *iter;
	send_add_membership(xrl_sender_name.c_str(),
			    src_module_id,
			    mld6igmp_vif->vif_index(),
			    member_query->source(),
			    member_query->group());
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
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::add_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "on vif %s with vif_index %d",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all existing membership on the particular vif.
    //
    Mld6igmpVif *mld6igmp_vif = Mld6igmpNode::vif_find_by_vif_index(vif_index);
    if (mld6igmp_vif == NULL) {
	Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index);
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "on vif %s with vif_index %d: "
				    "no such vif",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    list<MemberQuery *>::const_iterator iter;
    for (iter = mld6igmp_vif->members().begin();
	 iter != mld6igmp_vif->members().end();
	 ++iter) {
	const MemberQuery *member_query = *iter;
	send_add_membership(xrl_sender_name.c_str(),
			    src_module_id,
			    mld6igmp_vif->vif_index(),
			    member_query->source(),
			    member_query->group());
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
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete protocol instance '%s' "
				    "on vif %s with vif_index %d",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
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
    //
    // Verify the address family
    //
    if (! Mld6igmpNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (Mld6igmpNode::delete_protocol(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete protocol instance '%s' "
				    "on vif %s with vif_index %d",
				    xrl_sender_name.c_str(),
				    vif_name.c_str(),
				    vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}
