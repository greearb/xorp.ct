// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/pim/xrl_pim_node.cc,v 1.56 2004/07/28 05:13:56 pavlin Exp $"

#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/status_codes.h"
#include "pim_mfc.hh"
#include "pim_node.hh"
#include "pim_node_cli.hh"
#include "pim_mrib_table.hh"
#include "pim_vif.hh"
#include "xrl_pim_node.hh"


//
// XrlPimNode front-end interface
//

XrlPimNode::XrlPimNode(int family,
		       xorp_module_id module_id,
		       EventLoop& eventloop,
		       XrlRouter* xrl_router,
		       const string& mfea_target,
		       const string& rib_target,
		       const string& mld6igmp_target)
    : PimNode(family, module_id, eventloop),
      XrlPimTargetBase(xrl_router),
      PimNodeCli(*static_cast<PimNode *>(this)),
      _class_name(xrl_router->class_name()),
      _instance_name(xrl_router->instance_name()),
      _mrib_transaction_manager(eventloop),
      _xrl_mfea_client(xrl_router),
      _xrl_rib_client(xrl_router),
      _xrl_mld6igmp_client(xrl_router),
      _xrl_cli_manager_client(xrl_router),
      _mfea_target(mfea_target),
      _rib_target(rib_target),
      _mld6igmp_target(mld6igmp_target),
      _is_mfea_add_protocol_registered(false),
      _is_mfea_allow_signal_messages_registered(false),
      _is_mfea_allow_mrib_messages_registered(false),
      _is_rib_client_registered(false)
{

}

XrlPimNode::~XrlPimNode()
{
    PimNodeCli::stop();
    PimNode::stop();
}

bool
XrlPimNode::startup()
{
    if (start_pim() < 0)
	return false;

    return true;
}

bool
XrlPimNode::shutdown()
{
    if (stop_pim() < 0)
	return false;

    return true;
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
    if (PimNodeCli::start() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_cli()
{
    if (PimNodeCli::stop() < 0)
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
    if (PimNode::start() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_pim()
{
    if (PimNode::stop() < 0)
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
    if (PimNode::start_bsr() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlPimNode::stop_bsr()
{
    if (PimNode::stop_bsr() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

//
// Protocol node methods
//

void
XrlPimNode::mfea_register_startup()
{
    if (! _is_mfea_add_protocol_registered)
	PimNode::incr_startup_requests_n();

    if (! _is_mfea_allow_signal_messages_registered)
	PimNode::incr_startup_requests_n();

    if (! _is_mfea_allow_mrib_messages_registered) {
	if (PimNode::is_receive_mrib_from_mfea())
	    PimNode::incr_startup_requests_n();
    }

    send_mfea_registration();
}

void
XrlPimNode::mfea_register_shutdown()
{
    if (_is_mfea_add_protocol_registered)
	PimNode::incr_shutdown_requests_n();

    send_mfea_deregistration();
}

void
XrlPimNode::rib_register_startup()
{
    if (PimNode::is_receive_mrib_from_mfea()) {
	// XXX: we don't really need to register with the RIB
	PimNode::update_status();
	return;
    }

    if (! _is_rib_client_registered)
	PimNode::incr_startup_requests_n();

    send_rib_registration();
}

void
XrlPimNode::rib_register_shutdown()
{
    if (PimNode::is_receive_mrib_from_mfea()) {
	// XXX: we don't really need to de-register with the RIB
	PimNode::update_status();
	return;
    }

    if (_is_rib_client_registered)
	PimNode::incr_shutdown_requests_n();

    send_rib_deregistration();
}

//
// Register with the MFEA
//
void
XrlPimNode::send_mfea_registration()
{
    bool success = true;

    //
    // Register the protocol with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_add_protocol4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		callback(this, &XrlPimNode::mfea_client_send_add_protocol_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_add_protocol6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		callback(this, &XrlPimNode::mfea_client_send_add_protocol_cb));
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
	_mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_mfea_registration));
	return;
    }

    //
    // Enable the receiving of kernel signal messages
    //
    if (! _is_mfea_allow_signal_messages_registered) {
	success = _xrl_mfea_client.send_allow_signal_messages(
	    _mfea_target.c_str(),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    true,			// XXX: enable
	    callback(this,
		     &XrlPimNode::mfea_client_send_allow_signal_messages_cb));
	if (success)
	    return;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again
	// TODO: XXX: the timer value is hardcoded here!!
	//
	XLOG_ERROR("Failed to enable the receiving of kernel signal messages "
		   "with the MFEA. "
		   "Will try again.");
	_mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_mfea_registration));
	return;
    }

    //
    // Enable receiving of MRIB information
    //
    if ((! _is_mfea_allow_mrib_messages_registered)
	&& PimNode::is_receive_mrib_from_mfea()) {
	success = _xrl_mfea_client.send_allow_mrib_messages(
	    _mfea_target.c_str(),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    true,			// XXX: enable
	    callback(this,
		     &XrlPimNode::mfea_client_send_allow_mrib_messages_cb));
	if (success)
	    return;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again
	// TODO: XXX: the timer value is hardcoded here!!
	//
	XLOG_ERROR("Failed to enable the receiving of MRIB messages "
		   "with the MFEA. "
		   "Will try again.");
	_mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_mfea_registration));
	return;
    }
}

void
XrlPimNode::mfea_client_send_add_protocol_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_add_protocol_registered = true;
	send_mfea_registration();
	PimNode::decr_startup_requests_n();
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
    _mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlPimNode::send_mfea_registration));
}

void
XrlPimNode::mfea_client_send_allow_signal_messages_cb(
    const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_allow_signal_messages_registered = true;
	send_mfea_registration();
	PimNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot allow signal messages with the MFEA: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_mfea_registration_timer.scheduled())
	return;
    _mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlPimNode::send_mfea_registration));
}

void
XrlPimNode::mfea_client_send_allow_mrib_messages_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_allow_mrib_messages_registered = true;
	send_mfea_registration();
	PimNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot allow MRIB messages with the MFEA: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_mfea_registration_timer.scheduled())
	return;
    _mfea_registration_timer = PimNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlPimNode::send_mfea_registration));
}

//
// De-register with the MFEA
//
void
XrlPimNode::send_mfea_deregistration()
{
    bool success = true;

    if (_is_mfea_add_protocol_registered) {
	if (PimNode::is_ipv4()) {
	    bool success4;
	    success4 = _xrl_mfea_client.send_delete_protocol4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		callback(this, &XrlPimNode::mfea_client_send_delete_protocol_cb));
	    if (success4 != true)
		success = false;
	}

	if (PimNode::is_ipv6()) {
	    bool success6;
	    success6 = _xrl_mfea_client.send_delete_protocol6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		callback(this, &XrlPimNode::mfea_client_send_delete_protocol_cb));
	    if (success6 != true)
		success = false;
	}
    }

    if (! success) {
	XLOG_ERROR("Failed to deregister with the MFEA. "
		   "Will give up.");
	PimNode::set_status(FAILED);
	PimNode::update_status();
    }
}

void
XrlPimNode::mfea_client_send_delete_protocol_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_mfea_add_protocol_registered = false;
	_is_mfea_allow_signal_messages_registered = false;
	_is_mfea_allow_mrib_messages_registered = false;
	PimNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister with the MFEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    PimNode::set_status(FAILED);
    PimNode::update_status();
}

//
// Register with the RIB
//
void
XrlPimNode::send_rib_registration()
{
    bool success = true;

    if ((! _is_rib_client_registered)
	&& (!  PimNode::is_receive_mrib_from_mfea())) {
	if (PimNode::is_ipv4()) {
	    success = _xrl_rib_client.send_redist_transaction_enable4(
		_rib_target.c_str(),
		my_xrl_target_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_enable_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_rib_client.send_redist_transaction_enable6(
		_rib_target.c_str(),
		my_xrl_target_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_enable_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then start a timer to try again
	// TODO: XXX: the timer value is hardcoded here!!
	//
	XLOG_ERROR("Failed to register with the RIB. "
		   "Will try again.");
	_rib_registration_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_rib_registration));
	return;
    }
}

void
XrlPimNode::rib_client_send_redist_transaction_enable_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_client_registered = true;
	send_rib_registration();
	PimNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot register with the RIB: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_rib_registration_timer.scheduled())
	return;
    _rib_registration_timer = PimNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlPimNode::send_rib_registration));
}

//
// De-register with the RIB
//
void
XrlPimNode::send_rib_deregistration()
{
    bool success = true;

    if (_is_rib_client_registered) {
	if (PimNode::is_ipv4()) {
	    bool success4;
	    success4 = _xrl_rib_client.send_redist_transaction_disable4(
		_rib_target.c_str(),
		my_xrl_target_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_disable_cb));
	    if (success4 != true)
		success = false;
	}

	if (PimNode::is_ipv6()) {
	    bool success6;
	    success6 = _xrl_rib_client.send_redist_transaction_disable6(
		_rib_target.c_str(),
		my_xrl_target_name(),
		string("all"),		// TODO: XXX: hard-coded value
		false,		/* unicast */
		true,		/* multicast */
		string("all"),		// TODO: XXX: hard-coded value
		callback(this, &XrlPimNode::rib_client_send_redist_transaction_disable_cb));
	    if (success6 != true)
		success = false;
	}
    }

    if (! success) {
	XLOG_ERROR("Failed to deregister with the RIB. "
		   "Will give up.");
	PimNode::set_status(FAILED);
	PimNode::update_status();
    }
}

void
XrlPimNode::rib_client_send_redist_transaction_disable_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_client_registered = false;
	PimNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    PimNode::set_status(FAILED);
    PimNode::update_status();
}

int
XrlPimNode::start_protocol_kernel_vif(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
XrlPimNode::stop_protocol_kernel_vif(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
XrlPimNode::send_start_stop_protocol_kernel_vif()
{
    bool success = true;
    PimVif *pim_vif = NULL;

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

    pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot %s protocol vif with vif_index %d with the MFEA: "
		   "no such vif",
		   (is_start)? "start" : "stop",
		   vif_index);
	_start_stop_protocol_kernel_vif_queue.pop_front();
	goto start_timer_label;
    }

    if (is_start) {
	// Start a vif with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_start_protocol_vif4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		PimNode::incr_startup_requests_n();
		return;
	    }
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_start_protocol_vif6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		PimNode::incr_startup_requests_n();
		return;
	    }
	}
    } else {
	// Stop a vif with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_stop_protocol_vif4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		PimNode::incr_shutdown_requests_n();
		return;
	    }
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_stop_protocol_vif6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		vif_index,
		callback(this, &XrlPimNode::mfea_client_send_start_stop_protocol_kernel_vif_cb));
	    if (success) {
		PimNode::incr_shutdown_requests_n();
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
		   pim_vif->name().c_str());
    start_timer_label:
	_start_stop_protocol_kernel_vif_queue_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_start_stop_protocol_kernel_vif));
    }
}

void
XrlPimNode::mfea_client_send_start_stop_protocol_kernel_vif_cb(const XrlError& xrl_error)
{
    bool is_start = _start_stop_protocol_kernel_vif_queue.front().second;

    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_start_stop_protocol_kernel_vif_queue.pop_front();
	send_start_stop_protocol_kernel_vif();
	if (is_start)
	    PimNode::decr_startup_requests_n();
	else
	    PimNode::decr_shutdown_requests_n();
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
    _start_stop_protocol_kernel_vif_queue_timer = PimNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlPimNode::send_start_stop_protocol_kernel_vif));
}

int
XrlPimNode::join_multicast_group(uint16_t vif_index,
				 const IPvX& multicast_group)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
XrlPimNode::leave_multicast_group(uint16_t vif_index,
				  const IPvX& multicast_group)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
XrlPimNode::send_join_leave_multicast_group()
{
    bool success = true;
    PimVif *pim_vif = NULL;

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

    pim_vif = PimNode::vif_find_by_vif_index(group.vif_index());
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot %s group %s on vif with vif_index %d: "
		   "no such vif",
		   (is_join)? "join" : "leave",
		   cstring(group.multicast_group()),
		   group.vif_index());
	_join_leave_multicast_group_queue.pop_front();
	goto start_timer_label;
    }

    if (is_join) {
	// Join a multicast group on a vif with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_join_multicast_group4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		PimNode::incr_startup_requests_n();
		return;
	    }
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_join_multicast_group6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		PimNode::incr_startup_requests_n();
		return;
	    }
	}
    } else {
	// Leave a multicast group on a vif with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_leave_multicast_group4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		PimNode::incr_shutdown_requests_n();
		return;
	    }
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_leave_multicast_group6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		string(PimNode::module_name()),
		PimNode::module_id(),
		pim_vif->name(),
		group.vif_index(),
		group.multicast_group().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_join_leave_multicast_group_cb));
	    if (success) {
		PimNode::incr_shutdown_requests_n();
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
		   pim_vif->name().c_str());
    start_timer_label:
	_join_leave_multicast_group_queue_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_join_leave_multicast_group));
    }
}

void
XrlPimNode::mfea_client_send_join_leave_multicast_group_cb(const XrlError& xrl_error)
{
    bool is_join = _join_leave_multicast_group_queue.front().is_join();

    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_join_leave_multicast_group_queue.pop_front();
	send_join_leave_multicast_group();
	if (is_join)
	    PimNode::decr_startup_requests_n();
	else
	    PimNode::decr_shutdown_requests_n();
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
    _join_leave_multicast_group_queue_timer = PimNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlPimNode::send_join_leave_multicast_group));
}

int
XrlPimNode::add_mfc_to_kernel(const PimMfc& pim_mfc)
{
    _add_delete_mfc_dataflow_monitor_queue.push_back(
	AddDeleteMfcDataflowMonitor(AddDeleteMfc(pim_mfc, true)));

    // If the queue was empty before, start sending the changes
    if (_add_delete_mfc_dataflow_monitor_queue.size() == 1) {
	send_add_delete_mfc_dataflow_monitor();
    }

    return (XORP_OK);
}

int
XrlPimNode::delete_mfc_from_kernel(const PimMfc& pim_mfc)
{
    _add_delete_mfc_dataflow_monitor_queue.push_back(
	AddDeleteMfcDataflowMonitor(AddDeleteMfc(pim_mfc, false)));

    // If the queue was empty before, start sending the changes
    if (_add_delete_mfc_dataflow_monitor_queue.size() == 1) {
	send_add_delete_mfc_dataflow_monitor();
    }

    return (XORP_OK);
}

void
XrlPimNode::send_add_delete_mfc(const AddDeleteMfc& mfc)
{
    bool success = true;
    bool is_add = mfc.is_add();
    size_t max_vifs_oiflist = mfc.olist().size();
    vector<uint8_t> oiflist_vector(max_vifs_oiflist);
    vector<uint8_t> oiflist_disable_wrongvif_vector(max_vifs_oiflist);
    
    mifset_to_vector(mfc.olist(), oiflist_vector);
    mifset_to_vector(mfc.olist_disable_wrongvif(),
		     oiflist_disable_wrongvif_vector);
    
    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	success = false;
	goto start_timer_label;
    }

    if (is_add) {
	// Add a MFC with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_add_mfc4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		mfc.source_addr().get_ipv4(),
		mfc.group_addr().get_ipv4(),
		mfc.iif_vif_index(),
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		mfc.rp_addr().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_add_mfc6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		mfc.source_addr().get_ipv6(),
		mfc.group_addr().get_ipv6(),
		mfc.iif_vif_index(),
		oiflist_vector,
		oiflist_disable_wrongvif_vector,
		max_vifs_oiflist,
		mfc.rp_addr().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}
    } else {
	// Delete a MFC with the MFEA
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_delete_mfc4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		mfc.source_addr().get_ipv4(),
		mfc.group_addr().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_delete_mfc6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		mfc.source_addr().get_ipv6(),
		mfc.group_addr().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
        //
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Failed to %s MFC entry for (%s,%s) with the MFEA. "
		   "Will try again.",
		   (is_add)? "add" : "delete",
		   cstring(mfc.source_addr()),
		   cstring(mfc.group_addr()));
    start_timer_label:
	_add_delete_mfc_dataflow_monitor_queue_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_add_delete_mfc_dataflow_monitor));
    }
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
    _add_delete_mfc_dataflow_monitor_queue.push_back(
	AddDeleteMfcDataflowMonitor(
	    AddDeleteDataflowMonitor(source_addr,
				     group_addr,
				     threshold_interval_sec,
				     threshold_interval_usec,
				     threshold_packets,
				     threshold_bytes,
				     is_threshold_in_packets,
				     is_threshold_in_bytes,
				     is_geq_upcall,
				     is_leq_upcall,
				     true)));

    // If the queue was empty before, start sending the changes
    if (_add_delete_mfc_dataflow_monitor_queue.size() == 1) {
	send_add_delete_mfc_dataflow_monitor();
    }

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
    _add_delete_mfc_dataflow_monitor_queue.push_back(
	AddDeleteMfcDataflowMonitor(
	    AddDeleteDataflowMonitor(source_addr,
				     group_addr,
				     threshold_interval_sec,
				     threshold_interval_usec,
				     threshold_packets,
				     threshold_bytes,
				     is_threshold_in_packets,
				     is_threshold_in_bytes,
				     is_geq_upcall,
				     is_leq_upcall,
				     false)));

    // If the queue was empty before, start sending the changes
    if (_add_delete_mfc_dataflow_monitor_queue.size() == 1) {
	send_add_delete_mfc_dataflow_monitor();
    }

    return (XORP_OK);
}

int
XrlPimNode::delete_all_dataflow_monitor(const IPvX& source_addr,
					const IPvX& group_addr)
{
    _add_delete_mfc_dataflow_monitor_queue.push_back(
	AddDeleteMfcDataflowMonitor(
	    AddDeleteDataflowMonitor(source_addr,
				     group_addr)));

    // If the queue was empty before, start sending the changes
    if (_add_delete_mfc_dataflow_monitor_queue.size() == 1) {
	send_add_delete_mfc_dataflow_monitor();
    }

    return (XORP_OK);
}

void
XrlPimNode::send_add_delete_dataflow_monitor(
    const AddDeleteDataflowMonitor& monitor)
{
    bool success = true;
    bool is_add = monitor.is_add();
    bool is_delete_all = monitor.is_delete_all();

    //
    // Check whether we have already registered with the MFEA
    //
    if (! _is_mfea_add_protocol_registered) {
	success = false;
	goto start_timer_label;
    }

    if (is_delete_all) {
	// Delete all dataflow monitors for a source and a group addresses
	if (PimNode::is_ipv4()) {
	    success = _xrl_mfea_client.send_delete_all_dataflow_monitor4(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		monitor.source_addr().get_ipv4(),
		monitor.group_addr().get_ipv4(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}

	if (PimNode::is_ipv6()) {
	    success = _xrl_mfea_client.send_delete_all_dataflow_monitor6(
		_mfea_target.c_str(),
		my_xrl_target_name(),
		monitor.source_addr().get_ipv6(),
		monitor.group_addr().get_ipv6(),
		callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
	    if (success)
		return;
	}
    } else {
	if (is_add) {
	    // Add a dataflow monitor with the MFEA
	    if (PimNode::is_ipv4()) {
		success = _xrl_mfea_client.send_add_dataflow_monitor4(
		    _mfea_target.c_str(),
		    my_xrl_target_name(),
		    monitor.source_addr().get_ipv4(),
		    monitor.group_addr().get_ipv4(),
		    monitor.threshold_interval_sec(),
		    monitor.threshold_interval_usec(),
		    monitor.threshold_packets(),
		    monitor.threshold_bytes(),
		    monitor.is_threshold_in_packets(),
		    monitor.is_threshold_in_bytes(),
		    monitor.is_geq_upcall(),
		    monitor.is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
		if (success)
		    return;
	    }

	    if (PimNode::is_ipv6()) {
		success = _xrl_mfea_client.send_add_dataflow_monitor6(
		    _mfea_target.c_str(),
		    my_xrl_target_name(),
		    monitor.source_addr().get_ipv6(),
		    monitor.group_addr().get_ipv6(),
		    monitor.threshold_interval_sec(),
		    monitor.threshold_interval_usec(),
		    monitor.threshold_packets(),
		    monitor.threshold_bytes(),
		    monitor.is_threshold_in_packets(),
		    monitor.is_threshold_in_bytes(),
		    monitor.is_geq_upcall(),
		    monitor.is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
		if (success)
		    return;
	    }
	} else {
	    // Delete a dataflow monitor with the MFEA
	    if (PimNode::is_ipv4()) {
		success = _xrl_mfea_client.send_delete_dataflow_monitor4(
		    _mfea_target.c_str(),
		    my_xrl_target_name(),
		    monitor.source_addr().get_ipv4(),
		    monitor.group_addr().get_ipv4(),
		    monitor.threshold_interval_sec(),
		    monitor.threshold_interval_usec(),
		    monitor.threshold_packets(),
		    monitor.threshold_bytes(),
		    monitor.is_threshold_in_packets(),
		    monitor.is_threshold_in_bytes(),
		    monitor.is_geq_upcall(),
		    monitor.is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
		if (success)
		    return;
	    }

	    if (PimNode::is_ipv6()) {
		success = _xrl_mfea_client.send_delete_dataflow_monitor6(
		    _mfea_target.c_str(),
		    my_xrl_target_name(),
		    monitor.source_addr().get_ipv6(),
		    monitor.group_addr().get_ipv6(),
		    monitor.threshold_interval_sec(),
		    monitor.threshold_interval_usec(),
		    monitor.threshold_packets(),
		    monitor.threshold_bytes(),
		    monitor.is_threshold_in_packets(),
		    monitor.is_threshold_in_bytes(),
		    monitor.is_geq_upcall(),
		    monitor.is_leq_upcall(),
		    callback(this, &XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb));
		if (success)
		    return;
	    }
	}
    }

    if (! success) {
        //
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Failed to %s dataflow entry for (%s,%s) with the MFEA. "
		   "Will try again.",
		   (is_delete_all)? "delete all" : (is_add)? "add" : "delete",
		   cstring(monitor.source_addr()),
		   cstring(monitor.group_addr()));

    start_timer_label:
	_add_delete_mfc_dataflow_monitor_queue_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_add_delete_mfc_dataflow_monitor));
    }
}

void
XrlPimNode::send_add_delete_mfc_dataflow_monitor()
{
    if (_add_delete_mfc_dataflow_monitor_queue.empty())
	return;			// No more changes

    const AddDeleteMfcDataflowMonitor& mfc_dataflow_monitor =
	_add_delete_mfc_dataflow_monitor_queue.front();
    if (mfc_dataflow_monitor.mfc() != NULL)
	send_add_delete_mfc(*mfc_dataflow_monitor.mfc());
    if (mfc_dataflow_monitor.dataflow_monitor() != NULL)
	send_add_delete_dataflow_monitor(*mfc_dataflow_monitor.dataflow_monitor());
}

void
XrlPimNode::mfea_client_send_add_delete_mfc_dataflow_monitor_cb(const XrlError& xrl_error)
{
    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_add_delete_mfc_dataflow_monitor_queue.pop_front();
	send_add_delete_mfc_dataflow_monitor();
	return;
    }

    //
    // If a command failed because the other side rejected it,
    // then send the next one.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	const AddDeleteMfcDataflowMonitor& mfc_dataflow_monitor =
	    _add_delete_mfc_dataflow_monitor_queue.front();
	if (mfc_dataflow_monitor.mfc() != NULL) {
	    const AddDeleteMfc& mfc = *mfc_dataflow_monitor.mfc();
	    bool is_add = mfc.is_add();

	    XLOG_ERROR("Cannot %s a multicast group with the MFEA: %s",
		       (is_add)? "add" : "delete",
		       xrl_error.str().c_str());
	}

	if (mfc_dataflow_monitor.dataflow_monitor() != NULL) {
	    const AddDeleteDataflowMonitor& monitor =
		*mfc_dataflow_monitor.dataflow_monitor();
	    bool is_add = monitor.is_add();
	    bool is_delete_all = monitor.is_delete_all();

	    XLOG_ERROR("Cannot %s with the MFEA: %s",
		       (is_add)? "add a dataflow monitor"
		       : (is_delete_all)? "delete multiple dataflow monitors"
		       : "delete a dataflow monitor",
		       xrl_error.str().c_str());
	}

	_add_delete_mfc_dataflow_monitor_queue.pop_front();
	send_add_delete_mfc_dataflow_monitor();
	return;
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _add_delete_mfc_dataflow_monitor_queue_timer =
	PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_add_delete_mfc_dataflow_monitor));
}

int
XrlPimNode::add_protocol_mld6igmp(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot add protocol with MLD6IGMP "
		   "for vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }

    _add_delete_protocol_mld6igmp_queue.push_back(make_pair(vif_index, true));

    PimNode::incr_startup_requests_n();

    // If the queue was empty before, start sending the changes
    if (_add_delete_protocol_mld6igmp_queue.size() == 1) {
	send_add_delete_protocol_mld6igmp();
    }

    return (XORP_OK);
}

int
XrlPimNode::delete_protocol_mld6igmp(uint16_t vif_index)
{
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot delete protocol with MLD6IGMP "
		   "for vif with vif_index %d: "
		   "no such vif", vif_index);
	return (XORP_ERROR);
    }

    _add_delete_protocol_mld6igmp_queue.push_back(make_pair(vif_index, false));

    PimNode::incr_shutdown_requests_n();

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

    if (_add_delete_protocol_mld6igmp_queue.empty())
	return;			// No more changes

    uint16_t vif_index = _add_delete_protocol_mld6igmp_queue.front().first;
    bool is_add = _add_delete_protocol_mld6igmp_queue.front().second;

    pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot %s vif with vif_index %d with the MLD6IGMP: "
		   "no such vif",
		   (is_add)? "add" : "delete",
		   vif_index);
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
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
		my_xrl_target_name(),
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
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	XLOG_ERROR("Cannot %s vif %s with the MLD6IGMP. "
		   "Will try again.",
		   (is_add)? "add" : "delete",
		   pim_vif->name().c_str());
    start_timer_label:
	_add_delete_protocol_mld6igmp_queue_timer = PimNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlPimNode::send_add_delete_protocol_mld6igmp));
    }
}

void
XrlPimNode::mld6igmp_client_send_add_delete_protocol_mld6igmp_cb(const XrlError& xrl_error)
{
    bool is_add = _add_delete_protocol_mld6igmp_queue.front().second;

    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_add_delete_protocol_mld6igmp_queue.pop_front();
	send_add_delete_protocol_mld6igmp();
	if (is_add)
	    PimNode::decr_startup_requests_n();
	else
	    PimNode::decr_shutdown_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot %s with the MFEA: %s",
		   (is_add)? "register" : "deregister",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _add_delete_protocol_mld6igmp_queue_timer = PimNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlPimNode::send_add_delete_protocol_mld6igmp));
}

/**
 * XrlPimNode::proto_send:
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
XrlPimNode::proto_send(const string& dst_module_instance_name,
		       xorp_module_id	, // dst_module_id,
		       uint16_t vif_index,
		       const IPvX& src,
		       const IPvX& dst,
		       int ip_ttl,
		       int ip_tos,
		       bool router_alert_bool,
		       const uint8_t *sndbuf,
		       size_t sndlen)
{
    bool success = true;
    PimVif *pim_vif = PimNode::vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL) {
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
    
    if (PimNode::is_ipv4()) {
	success = _xrl_mfea_client.send_send_protocol_message4(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    src.get_ipv4(),
	    dst.get_ipv4(),
	    ip_ttl,
	    ip_tos,
	    router_alert_bool,
	    snd_vector,
	    callback(this, &XrlPimNode::mfea_client_send_protocol_message_cb));
	if (success)
	    return (XORP_OK);
    }

    if (PimNode::is_ipv6()) {
	success = _xrl_mfea_client.send_send_protocol_message6(
	    dst_module_instance_name.c_str(),
	    my_xrl_target_name(),
	    string(PimNode::module_name()),
	    PimNode::module_id(),
	    pim_vif->name(),
	    vif_index,
	    src.get_ipv6(),
	    dst.get_ipv6(),
	    ip_ttl,
	    ip_tos,
	    router_alert_bool,
	    snd_vector,
	    callback(this, &XrlPimNode::mfea_client_send_protocol_message_cb));
	if (success)
	    return (XORP_OK);
    }

    if (! success) {
	XLOG_ERROR("Failed to send a protocol message on vif %s",
		   pim_vif->name().c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
XrlPimNode::mfea_client_send_protocol_message_cb(const XrlError& xrl_error)
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
XrlPimNode::add_cli_command_to_cli_manager(const char *command_name,
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
	callback(this, &XrlPimNode::cli_manager_client_send_add_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlPimNode::cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error)
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
XrlPimNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlPimNode::cli_manager_client_send_delete_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlPimNode::cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error)
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
XrlPimNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    
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
    bool is_error = false;
    string error_msg;

    if (stop_pim() != XORP_OK) {
	if (! is_error)
	    error_msg = c_format("Failed to stop PIM");
	is_error = true;
    }

    if (is_error)
	return XrlCmdError::COMMAND_FAILED(error_msg);

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
XrlPimNode::mfea_client_0_1_new_vif(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	vif_index)
{
    string error_msg;
    
    if (PimNode::add_config_vif(vif_name, vif_index, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::delete_config_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_add_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		addr, 
    const IPv4Net&	subnet, 
    const IPv4&		broadcast, 
    const IPv4&		peer)
{
    string error_msg;
    
    if (PimNode::add_config_vif_addr(vif_name,
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
XrlPimNode::mfea_client_0_1_add_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		addr, 
    const IPv6Net&	subnet, 
    const IPv6&		broadcast, 
    const IPv6&		peer)
{
    string error_msg;
    
    if (PimNode::add_config_vif_addr(vif_name,
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
XrlPimNode::mfea_client_0_1_delete_vif_addr4(
    // Input values, 
    const string&	vif_name, 
    const IPv4&		addr)
{
    string error_msg;
    
    if (PimNode::delete_config_vif_addr(vif_name,
					IPvX(addr),
					error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_vif_addr6(
    // Input values, 
    const string&	vif_name, 
    const IPv6&		addr)
{
    string error_msg;
    
    if (PimNode::delete_config_vif_addr(vif_name,
					IPvX(addr),
					error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_vif_flags(
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
    
    if (PimNode::set_config_vif_flags(vif_name,
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
XrlPimNode::mfea_client_0_1_set_all_vifs_done()
{
    string error_msg;
    
    if (PimNode::set_config_all_vifs_done(error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_is_vif_setup_completed(
    // Output values, 
    bool&	is_completed)
{
    is_completed = PimNode::is_vif_setup_completed();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_protocol_message4(
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
    if (! PimNode::is_ipv4()) {
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
    PimNode::proto_recv(xrl_sender_name,
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
XrlPimNode::mfea_client_0_1_recv_protocol_message6(
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
    if (! PimNode::is_ipv6()) {
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
    PimNode::proto_recv(xrl_sender_name,
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
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message4(
    // Input values, 
    const string&		xrl_sender_name, 
    const string&		, // protocol_name, 
    const uint32_t&		protocol_id, 
    const uint32_t&		message_type, 
    const string&		, // vif_name, 
    const uint32_t&		vif_index, 
    const IPv4&			source_address, 
    const IPv4&			dest_address, 
    const vector<uint8_t>&	protocol_message)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
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
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 src_module_id,
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

XrlCmdError
XrlPimNode::mfea_client_0_1_recv_kernel_signal_message6(
    // Input values, 
    const string&		xrl_sender_name, 
    const string&		, // protocol_name, 
    const uint32_t&		protocol_id, 
    const uint32_t&		message_type, 
    const string&		, //  vif_name, 
    const uint32_t&		vif_index, 
    const IPv6&			source_address, 
    const IPv6&			dest_address, 
    const vector<uint8_t>&	protocol_message)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
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
    // Receive the kernel signal message
    //
    PimNode::signal_message_recv(xrl_sender_name,
				 src_module_id,
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

XrlCmdError
XrlPimNode::mfea_client_0_1_add_mrib4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4Net&	dest_prefix, 
    const IPv4&		next_hop_router_addr, 
    const string&	next_hop_vif_name, 
    const uint32_t&	next_hop_vif_index, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric_preference(metric_preference);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(0, mrib, next_hop_vif_name);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_add_mrib6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6Net&	dest_prefix, 
    const IPv6&		next_hop_router_addr, 
    const string&	next_hop_vif_name, 
    const uint32_t&	next_hop_vif_index, 
    const uint32_t&	metric_preference, 
    const uint32_t&	metric)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    mrib.set_next_hop_router_addr(IPvX(next_hop_router_addr));
    mrib.set_next_hop_vif_index(next_hop_vif_index);
    mrib.set_metric_preference(metric_preference);
    mrib.set_metric(metric);
    
    //
    // Add the Mrib to the list of pending transactions as an 'insert()' entry
    //
    PimNode::pim_mrib_table().add_pending_insert(0, mrib, next_hop_vif_name);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv4Net&	dest_prefix)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(0, mrib);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_delete_mrib6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const IPv6Net&	dest_prefix)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(dest_prefix));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(0, mrib);
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mfea_client_0_1_set_mrib_done(
    // Input values, 
    const string&	// xrl_sender_name, 
    )
{
    //
    // Commit all pending Mrib transactions
    //
    PimNode::pim_mrib_table().commit_pending_transactions(0);
    
    return XrlCmdError::OKAY();
}

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

XrlCmdError
XrlPimNode::redist_transaction4_0_1_start_transaction(
    // Output values, 
    uint32_t&	tid)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.start(tid) != true) {
	string error_msg = c_format("Resource limit on number of pending "
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.commit(tid) != true) {
	string error_msg = c_format("Cannot commit MRIB transaction "
				    "for tid %u",
				    tid);
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.abort(tid) != true) {
	string error_msg = c_format("Cannot abort MRIB transaction for tid %u",
				    tid);
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
    const string&	/* ifname */, 
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    uint16_t vif_index = Vif::VIF_INDEX_INVALID;
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
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

    UNUSED(cookie);
    UNUSED(protocol_origin);
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_delete_route(
    // Input values, 
    const uint32_t&	tid, 
    const IPv4Net&	network,
    const string&	cookie)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(network));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);

    //
    // Success
    //
    return XrlCmdError::OKAY();

    UNUSED(cookie);
}

XrlCmdError
XrlPimNode::redist_transaction4_0_1_delete_all_routes(
    // Input values, 
    const uint32_t&	tid, 
    const string&	cookie)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
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

    UNUSED(cookie);
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_start_transaction(
    // Output values, 
    uint32_t&	tid)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.start(tid) != true) {
	string error_msg = c_format("Resource limit on number of pending "
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.commit(tid) != true) {
	string error_msg = c_format("Cannot commit MRIB transaction "
				    "for tid %u",
				    tid);
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_mrib_transaction_manager.abort(tid) != true) {
	string error_msg = c_format("Cannot abort MRIB transaction for tid %u",
				    tid);
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
    const string&	/* ifname */, 
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    uint16_t vif_index = Vif::VIF_INDEX_INVALID;
    PimVif *pim_vif = PimNode::vif_find_by_name(vifname);

    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
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

    UNUSED(cookie);
    UNUSED(protocol_origin);
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_delete_route(
    // Input values, 
    const uint32_t&	tid, 
    const IPv6Net&	network,
    const string&	cookie)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Create the Mrib entry
    //
    Mrib mrib = Mrib(IPvXNet(network));
    
    //
    // Add the Mrib to the list of pending transactions as an 'remove()' entry
    //
    PimNode::pim_mrib_table().add_pending_remove(tid, mrib);

    //
    // Success
    //
    return XrlCmdError::OKAY();

    UNUSED(cookie);
}

XrlCmdError
XrlPimNode::redist_transaction6_0_1_delete_all_routes(
    // Input values, 
    const uint32_t&	tid, 
    const string&	cookie)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
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

    UNUSED(cookie);
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string error_msg = c_format("Failed to add membership for (%s, %s)",
				    cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_add_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string error_msg = c_format("Failed to add membership for (%s, %s)",
				    cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership4(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		source, 
    const IPv4&		group)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string error_msg = c_format("Failed to delete membership for (%s, %s)",
				    cstring(source), cstring(group));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
    
}

XrlCmdError
XrlPimNode::mld6igmp_client_0_1_delete_membership6(
    // Input values, 
    const string&	, // xrl_sender_name, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		source, 
    const IPv6&		group)
{
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::delete_membership(vif_index, IPvX(source), IPvX(group))
	!= XORP_OK) {
	string error_msg = c_format("Failed to delete membership for (%s, %s)",
				    cstring(source), cstring(group));
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
    if (PimNode::start_all_vifs() < 0) {
	string error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_all_vifs()
{
    if (PimNode::stop_all_vifs() < 0) {
	string error_msg = c_format("Failed to stop all vifs");
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
    if (start_pim() != XORP_OK) {
	string error_msg = c_format("Failed to start PIM");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_pim()
{
    if (stop_pim() != XORP_OK) {
	string error_msg = c_format("Failed to stop PIM");
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
    if (start_cli() != XORP_OK) {
	string error_msg = c_format("Failed to start PIM CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_cli()
{
    if (stop_cli() != XORP_OK) {
	string error_msg = c_format("Failed to stop PIM CLI");
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
    if (start_bsr() != XORP_OK) {
	string error_msg = c_format("Failed to start PIM BSR");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_stop_bsr()
{
    if (stop_bsr() != XORP_OK) {
	string error_msg = c_format("Failed to stop PIM BSR");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
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
    
    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name, error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::add_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						   vif_name, error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_scope_zone_by_vif_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const IPv4&		vif_addr)
{
    string error_msg;
    
    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr), error_msg)
	< 0) {
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
    
    if (PimNode::add_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						   IPvX(vif_addr), error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name, error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_scope_zone_by_vif_name(IPvXNet(scope_zone_id),
						      vif_name, error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr), error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_scope_zone_by_vif_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const IPv6&		vif_addr)
{
    string error_msg;
    
    if (PimNode::delete_config_scope_zone_by_vif_addr(IPvXNet(scope_zone_id),
						      IPvX(vif_addr), error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;
    
    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %d",
			     bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr_by_vif_name(IPvXNet(scope_zone_id),
						 is_scope_zone,
						 vif_name,
						 reinterpret_cast<const uint8_t&>(bsr_priority),
						 reinterpret_cast<const uint8_t&>(hash_mask_len),
						 error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_vif_name6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;
    
    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %d",
			     bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr_by_vif_name(IPvXNet(scope_zone_id),
						 is_scope_zone,
						 vif_name,
						 reinterpret_cast<const uint8_t&>(bsr_priority),
						 reinterpret_cast<const uint8_t&>(hash_mask_len),
						 error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr4(
    // Input values, 
    const IPv4Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;
    
    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %d",
			     bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr_by_addr(IPvXNet(scope_zone_id),
					     is_scope_zone,
					     IPvX(cand_bsr_addr),
					     reinterpret_cast<const uint8_t&>(bsr_priority),
					     reinterpret_cast<const uint8_t&>(hash_mask_len),
					     error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_bsr_by_addr6(
    // Input values, 
    const IPv6Net&	scope_zone_id, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_bsr_addr, 
    const uint32_t&	bsr_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;
    
    if (bsr_priority > 0xff) {
	error_msg = c_format("Invalid BSR priority = %d",
			     bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_bsr_by_addr(IPvXNet(scope_zone_id),
					     is_scope_zone,
					     IPvX(cand_bsr_addr),
					     reinterpret_cast<const uint8_t&>(bsr_priority),
					     reinterpret_cast<const uint8_t&>(hash_mask_len),
					     error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone, error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_cand_bsr(IPvXNet(scope_zone_id),
					is_scope_zone, error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %d",
			     rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						is_scope_zone,
						vif_name,
						reinterpret_cast<const uint8_t&>(rp_priority),
						reinterpret_cast<const uint16_t&>(rp_holdtime),
						error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_vif_name6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %d",
			     rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						is_scope_zone,
						vif_name,
						reinterpret_cast<const uint8_t&>(rp_priority),
						reinterpret_cast<const uint16_t&>(rp_holdtime),
						error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %d",
			     rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp_by_addr(IPvXNet(group_prefix),
					    is_scope_zone,
					    IPvX(cand_rp_addr),
					    reinterpret_cast<const uint8_t&>(rp_priority),
					    reinterpret_cast<const uint16_t&>(rp_holdtime),
					    error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_cand_rp_by_addr6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    string error_msg;
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	error_msg = c_format("Invalid RP holdtime = %d",
			     rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_cand_rp_by_addr(IPvXNet(group_prefix),
					    is_scope_zone,
					    IPvX(cand_rp_addr),
					    reinterpret_cast<const uint8_t&>(rp_priority),
					    reinterpret_cast<const uint16_t&>(rp_holdtime),
					    error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::delete_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						   is_scope_zone,
						   vif_name,
						   error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_vif_name6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::delete_config_cand_rp_by_vif_name(IPvXNet(group_prefix),
						   is_scope_zone,
						   vif_name,
						   error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv4&		cand_rp_addr)
{
    string error_msg;
    
    if (PimNode::delete_config_cand_rp_by_addr(IPvXNet(group_prefix),
					       is_scope_zone,
					       IPvX(cand_rp_addr),
					       error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_cand_rp_by_addr6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const IPv6&		cand_rp_addr)
{
    string error_msg;
    
    if (PimNode::delete_config_cand_rp_by_addr(IPvXNet(group_prefix),
					       is_scope_zone,
					       IPvX(cand_rp_addr),
					       error_msg)
	< 0) {
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
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_static_rp(IPvXNet(group_prefix),
				      IPvX(rp_addr),
				      reinterpret_cast<const uint8_t&>(rp_priority),
				      reinterpret_cast<const uint8_t&>(hash_mask_len),
				      error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_config_static_rp6(
    // Input values, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	hash_mask_len)
{
    string error_msg;
    
    if (rp_priority > 0xff) {
	error_msg = c_format("Invalid RP priority = %d",
			     rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	error_msg = c_format("Invalid hash mask length = %d",
			     hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_config_static_rp(IPvXNet(group_prefix),
				      IPvX(rp_addr),
				      reinterpret_cast<const uint8_t&>(rp_priority),
				      reinterpret_cast<const uint8_t&>(hash_mask_len),
				      error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_config_static_rp4(
    // Input values, 
    const IPv4Net&	group_prefix, 
    const IPv4&		rp_addr)
{
    string error_msg;
    
    if (PimNode::delete_config_static_rp(IPvXNet(group_prefix),
					 IPvX(rp_addr),
					 error_msg)
	< 0) {
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
    
    if (PimNode::delete_config_static_rp(IPvXNet(group_prefix),
					 IPvX(rp_addr), error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_config_static_rp_done()
{
    string error_msg;
    
    if (PimNode::config_static_rp_done(error_msg) < 0)
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
	error_msg = c_format("Invalid Hello triggered delay value %d: "
			     "max allowed is %d",
			     hello_triggered_delay,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_triggered_delay(vif_name,
					       hello_triggered_delay,
					       error_msg)
	< 0) {
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
    
    if (PimNode::reset_vif_hello_triggered_delay(vif_name, error_msg) < 0) {
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
	error_msg = c_format("Invalid Hello period value %d: "
			     "max allowed is %d",
			     hello_period,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_period(vif_name, hello_period, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_hello_period(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_hello_period(vif_name, error_msg) < 0)
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
	error_msg = c_format("Invalid Hello holdtime value %d: "
			     "max allowed is %d",
			     hello_holdtime,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_hello_holdtime(vif_name, hello_holdtime, error_msg)
	< 0) {
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
    
    if (PimNode::reset_vif_hello_holdtime(vif_name, error_msg) < 0)
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
    
    if (dr_priority > 0xffffffff) {
	error_msg = c_format("Invalid DR priority value %d: "
			     "max allowed is %d",
			     dr_priority,
			     0xffffffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_dr_priority(vif_name, dr_priority, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_dr_priority(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_dr_priority(vif_name, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_get_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    // Output values, 
    uint32_t&		lan_delay)
{
    string error_msg;
    
    uint16_t v;
    if (PimNode::get_vif_lan_delay(vif_name, v, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    lan_delay = v;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_set_vif_lan_delay(
    // Input values, 
    const string&	vif_name, 
    const uint32_t&	lan_delay)
{
    string error_msg;
    
    if (lan_delay > 0xffff) {
	error_msg = c_format("Invalid LAN delay value %d: "
			     "max allowed is %d",
			     lan_delay,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_lan_delay(vif_name, lan_delay, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_vif_lan_delay(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (PimNode::reset_vif_lan_delay(vif_name, error_msg) < 0)
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
	error_msg = c_format("Invalid Override interval value %d: "
			     "max allowed is %d",
			     override_interval,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_override_interval(vif_name, override_interval,
					   error_msg)
	< 0) {
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
    
    if (PimNode::reset_vif_override_interval(vif_name, error_msg) < 0)
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
	< 0) {
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
	< 0) {
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
	< 0) {
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
    
    if (PimNode::reset_vif_accept_nohello_neighbors(vif_name, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
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
	error_msg = c_format("Invalid Join/Prune period value %d: "
			     "max allowed is %d",
			     join_prune_period,
			     0xffff);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::set_vif_join_prune_period(vif_name, join_prune_period,
					   error_msg)
	< 0) {
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
    
    if (PimNode::reset_vif_join_prune_period(vif_name, error_msg) < 0)
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
	    < 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_reset_switch_to_spt_threshold()
{
    string error_msg;
    
    if (PimNode::reset_switch_to_spt_threshold(error_msg) < 0) {
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

    if (PimNode::add_alternative_subnet(vif_name, IPvXNet(subnet), error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_alternative_subnet6(
    // Input values,
    const string&	vif_name,
    const IPv6Net&	subnet)
{
    string error_msg;

    if (PimNode::add_alternative_subnet(vif_name, IPvXNet(subnet), error_msg)
	< 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_delete_alternative_subnet4(
    // Input values,
    const string&	vif_name,
    const IPv4Net&	subnet)
{
    string error_msg;

    if (PimNode::delete_alternative_subnet(vif_name, IPvXNet(subnet),
					   error_msg)
	< 0) {
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

    if (PimNode::delete_alternative_subnet(vif_name, IPvXNet(subnet),
					   error_msg)
	< 0) {
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

    if (PimNode::remove_all_alternative_subnets(vif_name, error_msg) < 0) {
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
    const bool&		new_group_bool)
{
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;
    
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
	string error_msg = c_format("Invalid entry type = %s",
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
	string error_msg = c_format("Invalid action = %s",
				    action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    if (group_mask_len > 0xff) {
	string error_msg = c_format("Invalid group mask length = %d",
				    group_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (group_mask_len > 0xff) {
	string error_msg = c_format("Invalid group mask length = %d",
				    group_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (holdtime > 0xffff) {
	string error_msg = c_format("Invalid holdtime = %d",
				    holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   reinterpret_cast<const uint8_t&>(group_mask_len),
				   entry_type, action_type,
				   reinterpret_cast<const uint16_t&>(holdtime),
				   new_group_bool)
	< 0) {
	string error_msg = c_format("Failed to add Join/Prune test entry "
				    "for (%s, %s)",
				    cstring(source_addr),
				    cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_jp_entry6(
    // Input values, 
    const IPv6&		source_addr, 
    const IPv6&		group_addr, 
    const uint32_t&	group_mask_len, 
    const string&	mrt_entry_type, 
    const string&	action_jp, 
    const uint32_t&	holdtime, 
    const bool&		new_group_bool)
{
    mrt_entry_type_t entry_type = MRT_ENTRY_UNKNOWN;
    action_jp_t action_type;
    
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
	string error_msg = c_format("Invalid entry type = %s",
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
	string error_msg = c_format("Invalid action = %s",
				    action_jp.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    } while (false);
    
    if (group_mask_len > 0xff) {
	string error_msg = c_format("Invalid group mask length = %d",
				    group_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (holdtime > 0xffff) {
	string error_msg = c_format("Invalid holdtime = %d",
				    holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_jp_entry(IPvX(source_addr), IPvX(group_addr),
				   reinterpret_cast<const uint8_t&>(group_mask_len),
				   entry_type, action_type,
				   reinterpret_cast<const uint16_t&>(holdtime),
				   new_group_bool)
	< 0) {
	string error_msg = c_format("Failed to add Join/Prune test entry "
				    "for (%s, %s)",
				    cstring(source_addr),
				    cstring(group_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry4(
    // Input values, 
    const IPv4&		nbr_addr)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	string error_msg = c_format("Failed to send Join/Prune test message "
				    "to %s",
				    cstring(nbr_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_jp_entry6(
    // Input values, 
    const IPv6&		nbr_addr)
{
    if (PimNode::send_test_jp_entry(IPvX(nbr_addr)) < 0) {
	string error_msg = c_format("Failed to send Join/Prune test message "
				    "to %s",
				    cstring(nbr_addr));
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
    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric)
	< 0) {
	string error_msg = c_format("Failed to send Assert test message "
				    "for (%s, %s) on vif %s",
				    cstring(source_addr),
				    cstring(group_addr),
				    vif_name.c_str());
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
    if (PimNode::send_test_assert(vif_name,
				  IPvX(source_addr),
				  IPvX(group_addr),
				  rpt_bit,
				  metric_preference,
				  metric)
	< 0) {
	string error_msg = c_format("Failed to send Assert test message "
				    "for (%s, %s) on vif %s",
				    cstring(source_addr),
				    cstring(group_addr),
				    vif_name.c_str());
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
    if (bsr_priority > 0xff) {
	string error_msg = c_format("Invalid BSR priority = %d",
				    bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	string error_msg = c_format("Invalid hash mask length = %d",
				    hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (fragment_tag > 0xffff) {
	string error_msg = c_format("Invalid fragment tag = %d",
				    fragment_tag);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   reinterpret_cast<const uint8_t&>(bsr_priority),
				   reinterpret_cast<const uint8_t&>(hash_mask_len),
				   reinterpret_cast<const uint16_t&>(fragment_tag))
	< 0) {
	string error_msg = c_format("Failed to add BSR test zone %s "
				    "with BSR address %s",
				    cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
							   zone_id_is_scope_zone)),
				    cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

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
    if (bsr_priority > 0xff) {
	string error_msg = c_format("Invalid BSR priority = %d",
				    bsr_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (hash_mask_len > 0xff) {
	string error_msg = c_format("Invalid hash mask length = %d",
				    hash_mask_len);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (fragment_tag > 0xffff) {
	string error_msg = c_format("Invalid fragment tag = %d",
				    fragment_tag);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (PimNode::add_test_bsr_zone(PimScopeZoneId(zone_id_scope_zone_prefix,
						  zone_id_is_scope_zone),
				   IPvX(bsr_addr),
				   reinterpret_cast<const uint8_t&>(bsr_priority),
				   reinterpret_cast<const uint8_t&>(hash_mask_len),
				   reinterpret_cast<const uint16_t&>(fragment_tag))
	< 0) {
	string error_msg = c_format("Failed to add BSR test zone %s "
				    "with BSR address %s",
				    cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
							   zone_id_is_scope_zone)),
				    cstring(bsr_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_add_test_bsr_group_prefix4(
    // Input values, 
    const IPv4Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv4Net&	group_prefix, 
    const bool&		is_scope_zone, 
    const uint32_t&	expected_rp_count)
{
    if (expected_rp_count > 0xff) {
	string error_msg = c_format("Invalid expected RP count = %d",
				    expected_rp_count);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	PimScopeZoneId(zone_id_scope_zone_prefix,
		       zone_id_is_scope_zone),
	IPvXNet(group_prefix),
	is_scope_zone,
	reinterpret_cast<const uint8_t&>(expected_rp_count))
	< 0) {
	string error_msg = c_format("Failed to add group prefix %s "
				    "for BSR test zone %s",
				    cstring(group_prefix),
				    cstring(PimScopeZoneId(zone_id_scope_zone_prefix,
							   zone_id_is_scope_zone)));
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
    if (expected_rp_count > 0xff) {
	string error_msg = c_format("Invalid expected RP count = %d",
				    expected_rp_count);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_group_prefix(
	PimScopeZoneId(zone_id_scope_zone_prefix,
		       zone_id_is_scope_zone),
	IPvXNet(group_prefix),
	is_scope_zone,
	reinterpret_cast<const uint8_t&>(expected_rp_count))
	< 0) {
	string error_msg = c_format("Failed to add group prefix %s "
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
    if (rp_priority > 0xff) {
	string error_msg = c_format("Invalid RP priority = %d",
				    rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	string error_msg = c_format("Invalid RP holdtime = %d",
				    rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 reinterpret_cast<const uint8_t&>(rp_priority),
				 reinterpret_cast<const uint16_t&>(rp_holdtime))
	< 0) {
	string error_msg = c_format("Failed to add test Cand-RP %s "
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
XrlPimNode::pim_0_1_add_test_bsr_rp6(
    // Input values, 
    const IPv6Net&	zone_id_scope_zone_prefix, 
    const bool&		zone_id_is_scope_zone, 
    const IPv6Net&	group_prefix, 
    const IPv6&		rp_addr, 
    const uint32_t&	rp_priority, 
    const uint32_t&	rp_holdtime)
{
    if (rp_priority > 0xff) {
	string error_msg = c_format("Invalid RP priority = %d",
				    rp_priority);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (rp_holdtime > 0xffff) {
	string error_msg = c_format("Invalid RP holdtime = %d",
				    rp_holdtime);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (PimNode::add_test_bsr_rp(PimScopeZoneId(zone_id_scope_zone_prefix,
						zone_id_is_scope_zone),
				 IPvXNet(group_prefix),
				 IPvX(rp_addr),
				 reinterpret_cast<const uint8_t&>(rp_priority),
				 reinterpret_cast<const uint16_t&>(rp_holdtime))
	< 0) {
	string error_msg = c_format("Failed to add test Cand-RP %s "
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
XrlPimNode::pim_0_1_send_test_bootstrap(
    // Input values, 
    const string&	vif_name)
{
    if (PimNode::send_test_bootstrap(vif_name) < 0) {
	string error_msg = c_format("Failed to send Bootstrap test message "
				    "on vif %s",
				    vif_name.c_str());
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
    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr)) < 0) {
	string error_msg = c_format("Failed to send Bootstrap test message "
				    "on vif %s to address %s",
				    vif_name.c_str(),
				    cstring(dest_addr));
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
    if (PimNode::send_test_bootstrap_by_dest(vif_name, IPvX(dest_addr)) < 0) {
	string error_msg = c_format("Failed to send Bootstrap test message "
				    "on vif %s to address %s",
				    vif_name.c_str(),
				    cstring(dest_addr));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlPimNode::pim_0_1_send_test_cand_rp_adv()
{
    if (PimNode::send_test_cand_rp_adv() < 0) {
	string error_msg = c_format("Failed to send Cand-RP-Adv test message");
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    TimeVal now;
    TimerList::system_gettimeofday(&now);
    
    nbrs_number = 0;
    
    for (uint16_t i = 0; i < PimNode::maxvifs(); i++) {
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    TimeVal now;
    TimerList::system_gettimeofday(&now);
    
    nbrs_number = 0;
    
    for (uint16_t i = 0; i < PimNode::maxvifs(); i++) {
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    string error_msg;
    
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    string error_msg;
    
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
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
    //
    // Verify the address family
    //
    if (! PimNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
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
XrlPimNode::pim_0_1_pimstat_##stat_name##_per_vif(			\
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
