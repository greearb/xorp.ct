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

#ident "$XORP: xorp/fea/xrl_mfea_node.cc,v 1.26 2004/05/06 20:14:29 pavlin Exp $"

#include "mfea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "mfea_node.hh"
#include "mfea_node_cli.hh"
#include "mfea_kernel_messages.hh"
#include "mfea_vif.hh"
#include "xrl_mfea_node.hh"


//
// XrlMfeaNode front-end interface
//
XrlMfeaNode::XrlMfeaNode(int family,
			 xorp_module_id module_id,
			 EventLoop& eventloop,
			 XrlRouter* xrl_router,
			 const string& fea_target,
			 FtiConfig& ftic)
    : MfeaNode(family, module_id, eventloop, ftic),
      XrlMfeaTargetBase(xrl_router),
      MfeaNodeCli(*static_cast<MfeaNode *>(this)),
      _class_name(xrl_router->class_name()),
      _instance_name(xrl_router->instance_name()),
      _fea_target(fea_target),
      _ifmgr(eventloop, fea_target.c_str(), xrl_router->finder_address(),
	     xrl_router->finder_port()),
      _xrl_mfea_client_client(xrl_router),
      _xrl_cli_manager_client(xrl_router)
{
    _ifmgr.set_observer(dynamic_cast<MfeaNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<MfeaNode*>(this));
}

XrlMfeaNode::~XrlMfeaNode()
{
    MfeaNodeCli::stop();
    MfeaNode::stop();

    _ifmgr.detach_hint_observer(dynamic_cast<MfeaNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<MfeaNode*>(this));
}

bool
XrlMfeaNode::startup()
{
    if (start_mfea() < 0)
	return false;

    return true;
}

bool
XrlMfeaNode::shutdown()
{
    if (stop_mfea() < 0)
	return false;

    return true;
}

bool
XrlMfeaNode::ifmgr_startup()
{
    bool ret_value;

    // TODO: XXX: we should startup the ifmgr only if it hasn't started yet
    MfeaNode::incr_startup_requests_n();

    ret_value = _ifmgr.startup();

    //
    // XXX: when the startup is completed, IfMgrHintObserver::tree_complete()
    // will be called.
    //

    return ret_value;
}

bool
XrlMfeaNode::ifmgr_shutdown()
{
    bool ret_value;

    MfeaNode::incr_shutdown_requests_n();

    ret_value = _ifmgr.shutdown();

    //
    // XXX: when the shutdown is completed, MfeaNode::status_change()
    // will be called.
    //

    return ret_value;
}

int
XrlMfeaNode::enable_cli()
{
    MfeaNodeCli::enable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::disable_cli()
{
    MfeaNodeCli::disable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::start_cli()
{
    if (MfeaNodeCli::start() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMfeaNode::stop_cli()
{
    if (MfeaNodeCli::stop() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
XrlMfeaNode::enable_mfea()
{
    MfeaNode::enable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::disable_mfea()
{
    MfeaNode::disable();
    
    return (XORP_OK);
}

int
XrlMfeaNode::start_mfea()
{
    if (MfeaNode::start() < 0)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
XrlMfeaNode::stop_mfea()
{
    int ret_code = XORP_OK;

    if (MfeaNode::stop() < 0)
	return (XORP_ERROR);
    
    return (ret_code);
}

//
// Protocol node methods
//

/**
 * XrlMfeaNode::send_add_mrib:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the protocol-destination of the
 * message.
 * @mrib: The #Mrib entry to add.
 * 
 * Add a MRIB entry to an user-level protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_add_mrib(const string& dst_module_instance_name,
			   xorp_module_id dst_module_id,
			   const Mrib& mrib)
{
    _send_add_delete_mrib_queue.push_back(SendAddDeleteMrib(
					      dst_module_instance_name,
					      dst_module_id,
					      mrib,
					      true));
    // If the queue was empty before, start sending the changes
    if (_send_add_delete_mrib_queue.size() == 1) {
	send_add_delete_mrib();
    }

    return (XORP_OK);
}

/**
 * XrlMfeaNode::send_delete_mrib:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the protocol-destination of the
 * message.
 * @mrib: The #Mrib entry to delete.
 * 
 * Delete a MRIB entry from an user-level protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_delete_mrib(const string& dst_module_instance_name,
			      xorp_module_id dst_module_id,
			      const Mrib& mrib)
{
    _send_add_delete_mrib_queue.push_back(SendAddDeleteMrib(
					      dst_module_instance_name,
					      dst_module_id,
					      mrib,
					      false));
    // If the queue was empty before, start sending the changes
    if (_send_add_delete_mrib_queue.size() == 1) {
	send_add_delete_mrib();
    }

    return (XORP_OK);
}

/**
 * XrlMfeaNode::send_set_mrib_done:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the protocol-destination of the
 * message.
 * 
 * Complete add/delete MRIB transaction.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::send_set_mrib_done(const string& dst_module_instance_name,
				xorp_module_id dst_module_id)
{
    _send_add_delete_mrib_queue.push_back(SendAddDeleteMrib(
					      dst_module_instance_name,
					      dst_module_id,
					      family()));
    // If the queue was empty before, start sending the changes
    if (_send_add_delete_mrib_queue.size() == 1) {
	send_add_delete_mrib();
    }

    return (XORP_OK);
}

void
XrlMfeaNode::send_add_delete_mrib()
{
    bool success = false;
    MfeaVif *mfea_vif = NULL;
    string vif_name;

    if (_send_add_delete_mrib_queue.empty())
	return;			// No more changes

    const SendAddDeleteMrib& add_delete_mrib = _send_add_delete_mrib_queue.front();
    const Mrib& mrib = add_delete_mrib.mrib();
    bool is_add = add_delete_mrib.is_add();

    mfea_vif = MfeaNode::vif_find_by_vif_index(mrib.next_hop_vif_index());
    if (mfea_vif != NULL)
	vif_name = mfea_vif->name();

    if (add_delete_mrib.is_done()) {
	// Signal that we are done with add/delete of MRIB entries
	success = _xrl_mfea_client_client.send_set_mrib_done(
	    add_delete_mrib.dst_module_instance_name().c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb));

    } else {
	if (is_add) {
	    // Add MRIB entry
	    if (MfeaNode::is_ipv4()) {
		success = _xrl_mfea_client_client.send_add_mrib4(
		    add_delete_mrib.dst_module_instance_name().c_str(),
		    my_xrl_target_name(),
		    mrib.dest_prefix().get_ipv4net(),
		    mrib.next_hop_router_addr().get_ipv4(),
		    vif_name,
		    mrib.next_hop_vif_index(),
		    mrib.metric_preference(),
		    mrib.metric(),
		    callback(this, &XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb));
	    }
	    if (MfeaNode::is_ipv6()) {
		success = _xrl_mfea_client_client.send_add_mrib6(
		    add_delete_mrib.dst_module_instance_name().c_str(),
		    my_xrl_target_name(),
		    mrib.dest_prefix().get_ipv6net(),
		    mrib.next_hop_router_addr().get_ipv6(),
		    vif_name,
		    mrib.next_hop_vif_index(),
		    mrib.metric_preference(),
		    mrib.metric(),
		    callback(this, &XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb));
	    }
	} else {
	    // Delete MRIB entry
	    if (MfeaNode::is_ipv4()) {
		success = _xrl_mfea_client_client.send_delete_mrib4(
		    add_delete_mrib.dst_module_instance_name().c_str(),
		    my_xrl_target_name(),
		    mrib.dest_prefix().get_ipv4net(),
		    callback(this, &XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb));
	    }
	    if (MfeaNode::is_ipv6()) {
		success = _xrl_mfea_client_client.send_delete_mrib6(
		    add_delete_mrib.dst_module_instance_name().c_str(),
		    my_xrl_target_name(),
		    mrib.dest_prefix().get_ipv6net(),
		    callback(this, &XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb));
	    }
	}
    }
    
    if (! success) {
        //
        // If an error, then start a timer to try again
        // TODO: XXX: the timer value is hardcoded here!!
        //
	_send_add_delete_mrib_queue_timer = MfeaNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlMfeaNode::send_add_delete_mrib));
    }
}

void
XrlMfeaNode::mfea_client_client_send_add_delete_mrib_cb(const XrlError& xrl_error)
{
    // If success, then send the next change
    if (xrl_error == XrlError::OKAY()) {
	_send_add_delete_mrib_queue.pop_front();
	send_add_delete_mrib();
	return;
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _send_add_delete_mrib_queue_timer = MfeaNode::eventloop().new_oneoff_after(
        TimeVal(1, 0),
	callback(this, &XrlMfeaNode::send_add_delete_mrib));
}

/**
 * XrlMfeaNode::proto_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the protocol-destination of the
 * message.
 * @vif_index: The vif index of the interface used to receive this message.
 * @src: The source address of the message.
 * @dst: The destination address of the message.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: If true, the Router Alert IP option for the IP
 * packet of the incoming message was set.
 * @sndbuf: The data buffer with the message to send.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a protocol message received from the kernel to the user-level process.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::proto_send(const string& dst_module_instance_name,
			xorp_module_id	, // dst_module_id,
			uint16_t vif_index,
			const IPvX& src,
			const IPvX& dst,
			int ip_ttl,
			int ip_tos,
			bool router_alert_bool,
			const uint8_t *sndbuf,
			size_t sndlen
    )
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
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
	    _xrl_mfea_client_client.send_recv_protocol_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		mfea_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_protocol_message_cb));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    _xrl_mfea_client_client.send_recv_protocol_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		mfea_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		ip_ttl,
		ip_tos,
		router_alert_bool,
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_protocol_message_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMfeaNode::mfea_client_client_send_recv_protocol_message_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send a data message to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}

/**
 * XrlMfeaNode::signal_message_send:
 * @dst_module_instance name: The name of the protocol instance-destination
 * of the message.
 * @dst_module_id: The #xorp_module_id of the protocol-destination of the
 * message.
 * @message_type: The message type of the kernel signal.
 * At this moment, one of the following:
 * %MFEA_KERNEL_MESSAGE_NOCACHE (if a cache-miss in the kernel)
 * %MFEA_KERNEL_MESSAGE_WRONGVIF (multicast packet received on wrong vif)
 * %MFEA_KERNEL_MESSAGE_WHOLEPKT (typically, a packet that should be
 * encapsulated as a PIM-Register).
 * %MFEA_KERNEL_MESSAGE_BW_UPCALL (the bandwidth of a predefined
 * source-group flow is above or below a given threshold).
 * (XXX: The above types correspond to %IGMPMSG_* or %MRT6MSG_*).
 * @vif_index: The vif index of the related interface (message-specific
 * relation).
 * @src: The source address in the message.
 * @dst: The destination address in the message.
 * @sndbuf: The data buffer with the additional information in the message.
 * @sndlen: The data length in @sndbuf.
 * 
 * Send a kernel signal to an user-level protocol that expects it.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
XrlMfeaNode::signal_message_send(const string& dst_module_instance_name,
				 xorp_module_id	, // dst_module_id,
				 int message_type,
				 uint16_t vif_index,
				 const IPvX& src,
				 const IPvX& dst,
				 const uint8_t *sndbuf,
				 size_t sndlen)
{
    MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_ERROR("Cannot send a kernel signal message on vif with vif_index %d: "
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
	    _xrl_mfea_client_client.send_recv_kernel_signal_message4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv4(),
		dst.get_ipv4(),
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb));
	    break;
	}
	
	if (dst.is_ipv6()) {
	    _xrl_mfea_client_client.send_recv_kernel_signal_message6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		string(MfeaNode::module_name()),
		MfeaNode::module_id(),
		message_type,
		mfea_vif->name(),
		vif_index,
		src.get_ipv6(),
		dst.get_ipv6(),
		snd_vector,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while(false);
    
    return (XORP_OK);
}

//
// Misc. callback functions for setup a vif.
// TODO: those are almost identical, and should be replaced by a single
// function.
//
void
XrlMfeaNode::mfea_client_client_send_recv_kernel_signal_message_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send a kernel signal message to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}

/**
 * Send a message to a client to add a configured vif.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @param vif_name the name of the vif to add.
 * @param vif_index the vif index of the vif to add.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_add_config_vif(const string& dst_module_instance_name,
				 xorp_module_id , // dst_module_id,
				 const string& vif_name,
				 uint16_t vif_index)
{
    _xrl_mfea_client_client.send_new_vif(
	dst_module_instance_name.c_str(),
	vif_name,
	vif_index,
	callback(this, &XrlMfeaNode::mfea_client_client_send_new_vif_cb));
    
    return (XORP_OK);
}

/**
 * Send a message to a client to delete a configured vif.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @param vif_name the name of the vif to delete.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_delete_config_vif(const string& dst_module_instance_name,
				    xorp_module_id , // dst_module_id,
				    const string& vif_name)
{
    _xrl_mfea_client_client.send_delete_vif(
	dst_module_instance_name.c_str(),
	vif_name,
	callback(this, &XrlMfeaNode::mfea_client_client_send_delete_vif_cb));
    
    return (XORP_OK);
}

/**
 * Send a message to a client to add an address to a configured vif.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @param vif_name the name of the vif.
 * @param addr the address to add.
 * @param subnet the subnet address to add.
 * @param broadcast the broadcast address to add.
 * @param peer the peer address to add.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_add_config_vif_addr(const string& dst_module_instance_name,
				      xorp_module_id , // dst_module_id,
				      const string& vif_name,
				      const IPvX& addr,
				      const IPvXNet& subnet,
				      const IPvX& broadcast,
				      const IPvX& peer)
{
    do {
	if (MfeaNode::is_ipv4()) {
	    _xrl_mfea_client_client.send_add_vif_addr4(
		dst_module_instance_name.c_str(),
		vif_name,
		addr.get_ipv4(),
		subnet.get_ipv4net(),
		broadcast.get_ipv4(),
		peer.get_ipv4(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_add_vif_addr_cb));
	    break;
	}
	
	if (MfeaNode::is_ipv6()) {
	    _xrl_mfea_client_client.send_add_vif_addr6(
		dst_module_instance_name.c_str(),
		vif_name,
		addr.get_ipv6(),
		subnet.get_ipv6net(),
		broadcast.get_ipv6(),
		peer.get_ipv6(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_add_vif_addr_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while (false);
    
    return (XORP_OK);
}

/**
 * Send a message to a client to delete an address from a configured vif.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @param vif_name the name of the vif.
 * @param addr the address to delete.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_delete_config_vif_addr(const string& dst_module_instance_name,
					 xorp_module_id , // dst_module_id,
					 const string& vif_name,
					 const IPvX& addr)
{
    do {
	if (MfeaNode::is_ipv4()) {
	    _xrl_mfea_client_client.send_delete_vif_addr4(
		dst_module_instance_name.c_str(),
		vif_name,
		addr.get_ipv4(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_delete_vif_addr_cb));
	    break;
	}
	
	if (MfeaNode::is_ipv6()) {
	    _xrl_mfea_client_client.send_delete_vif_addr6(
		dst_module_instance_name.c_str(),
		vif_name,
		addr.get_ipv6(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_delete_vif_addr_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while (false);
    
    return (XORP_OK);
}

/**
 * Send a message to a client to set the vif flags of a configured vif.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @param vif_name the name of the vif.
 * @param is_pim_register true if the vif is a PIM Register interface.
 * @param is_p2p true if the vif is point-to-point interface.
 * @param is_loopback true if the vif is a loopback interface.
 * @param is_multicast true if the vif is multicast capable.
 * @param is_broadcast true if the vif is broadcast capable.
 * @param is_up true if the underlying vif is UP.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_set_config_vif_flags(const string& dst_module_instance_name,
				       xorp_module_id , // dst_module_id,
				       const string& vif_name,
				       bool is_pim_register,
				       bool is_p2p,
				       bool is_loopback,
				       bool is_multicast,
				       bool is_broadcast,
				       bool is_up)
{
    _xrl_mfea_client_client.send_set_vif_flags(
	dst_module_instance_name.c_str(),
	vif_name,
	is_pim_register,
	is_p2p,
	is_loopback,
	is_multicast,
	is_broadcast,
	is_up,
	callback(this, &XrlMfeaNode::mfea_client_client_send_set_vif_flags_cb));
    
    return (XORP_OK);
}

/**
 * Send a message to a client to complete the set of vif configuration
 * changes.
 * 
 * @param dst_module_instance_name the name of the protocol
 * instance-destination of the message.
 * @param dst_module_id the module ID of the protocol-destination
 * of the message.
 * @return  XORP_OK on success, otherwise XORP_ERROR.
 */
int
XrlMfeaNode::send_set_config_all_vifs_done(const string& dst_module_instance_name,
					   xorp_module_id // dst_module_id
    )
{
    _xrl_mfea_client_client.send_set_all_vifs_done(
	dst_module_instance_name.c_str(),
	callback(this, &XrlMfeaNode::mfea_client_client_send_set_all_vifs_done_cb));
    
    return (XORP_OK);
}

void
XrlMfeaNode::mfea_client_client_send_new_vif_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send new vif info to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}
void
XrlMfeaNode::mfea_client_client_send_delete_vif_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send delete_vif to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}
void
XrlMfeaNode::mfea_client_client_send_add_vif_addr_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send new vif address to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}
void
XrlMfeaNode::mfea_client_client_send_delete_vif_addr_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send delete vif address to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}
void
XrlMfeaNode::mfea_client_client_send_set_vif_flags_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send new vif flags to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}

void
XrlMfeaNode::mfea_client_client_send_set_all_vifs_done_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send set_all_vifs_done to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}

int
XrlMfeaNode::dataflow_signal_send(const string& dst_module_instance_name,
				  xorp_module_id	, // dst_module_id,
				  const IPvX& source_addr,
				  const IPvX& group_addr,
				  uint32_t threshold_interval_sec,
				  uint32_t threshold_interval_usec,
				  uint32_t measured_interval_sec,
				  uint32_t measured_interval_usec,
				  uint32_t threshold_packets,
				  uint32_t threshold_bytes,
				  uint32_t measured_packets,
				  uint32_t measured_bytes,
				  bool is_threshold_in_packets,
				  bool is_threshold_in_bytes,
				  bool is_geq_upcall,
				  bool is_leq_upcall)
{
    do {
	if (source_addr.is_ipv4()) {
	    _xrl_mfea_client_client.send_recv_dataflow_signal4(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		source_addr.get_ipv4(),
		group_addr.get_ipv4(),
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
		is_leq_upcall,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb));
	    break;
	}
	
	if (source_addr.is_ipv6()) {
	    _xrl_mfea_client_client.send_recv_dataflow_signal6(
		dst_module_instance_name.c_str(),
		my_xrl_target_name(),
		source_addr.get_ipv6(),
		group_addr.get_ipv6(),
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
		is_leq_upcall,
		callback(this, &XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb));
	    break;
	}
	
	XLOG_UNREACHABLE();
	break;
    } while (false);
    
    return (XORP_OK);
}

void
XrlMfeaNode::mfea_client_client_send_recv_dataflow_signal_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to send recv_dataflow_signal to a protocol: %s",
		   xrl_error.str().c_str());
	return;
    }
}

//
// Protocol node CLI methods
//
int
XrlMfeaNode::add_cli_command_to_cli_manager(const char *command_name,
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
	callback(this, &XrlMfeaNode::cli_manager_client_send_add_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlMfeaNode::cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to add a command to CLI manager: %s",
		   xrl_error.str().c_str());
	return;
    }
}

int
XrlMfeaNode::delete_cli_command_from_cli_manager(const char *command_name)
{
    _xrl_cli_manager_client.send_delete_cli_command(
	xorp_module_name(family(), XORP_MODULE_CLI),
	my_xrl_target_name(),
	string(command_name),
	callback(this, &XrlMfeaNode::cli_manager_client_send_delete_cli_command_cb));
    
    return (XORP_OK);
}

void
XrlMfeaNode::cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_ERROR("Failed to delete a command from CLI manager: %s",
		   xrl_error.str().c_str());
	return;
    }
}


//
// XRL target methods
//

XrlCmdError
XrlMfeaNode::common_0_1_get_target_name(
    // Output values, 
    string&		name)
{
    name = my_xrl_target_name();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::common_0_1_get_version(
    // Output values, 
    string&		version)
{
    version = XORP_MODULE_VERSION;
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&		reason)
{
    status = MfeaNode::node_status(reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::common_0_1_shutdown()
{
    if (stop_mfea() != XORP_OK) {
	string error_msg = c_format("Failed to stop MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::cli_processor_0_1_process_command(
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
    MfeaNodeCli::cli_process_command(processor_name,
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
XrlMfeaNode::mfea_0_1_have_multicast_routing4(
    // Output values, 
    bool&	result)
{
    result = MfeaNode::have_multicast_routing4();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_have_multicast_routing6(
    // Output values, 
    bool&	result)
{
    result = MfeaNode::have_multicast_routing6();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    // Add the protocol
    //
    if (MfeaNode::add_protocol(xrl_sender_name, src_module_id) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "with module_id = %d",
				    xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all vifs
    //
    for (uint16_t i = 0; i < MfeaNode::maxvifs(); i++) {
	MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	_xrl_mfea_client_client.send_new_vif(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::mfea_client_client_send_new_vif_cb));
	list<VifAddr>::const_iterator iter;
	for (iter = mfea_vif->addr_list().begin();
	     iter != mfea_vif->addr_list().end();
	     ++iter) {
	    const VifAddr& vif_addr = *iter;
	    _xrl_mfea_client_client.send_add_vif_addr4(
		xrl_sender_name.c_str(),
		mfea_vif->name(),
		vif_addr.addr().get_ipv4(),
		vif_addr.subnet_addr().get_ipv4net(),
		vif_addr.broadcast_addr().get_ipv4(),
		vif_addr.peer_addr().get_ipv4(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_add_vif_addr_cb));
	}
	_xrl_mfea_client_client.send_set_vif_flags(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->is_pim_register(),
	    mfea_vif->is_p2p(),
	    mfea_vif->is_loopback(),
	    mfea_vif->is_multicast_capable(),
	    mfea_vif->is_broadcast_capable(),
	    mfea_vif->is_underlying_vif_up(),
	    callback(this, &XrlMfeaNode::mfea_client_client_send_set_vif_flags_cb));
    }
    
    // We are done with the vifs
    _xrl_mfea_client_client.send_set_all_vifs_done(
	xrl_sender_name.c_str(),
	callback(this, &XrlMfeaNode::mfea_client_client_send_set_all_vifs_done_cb));
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    // Add the protocol
    //
    if (MfeaNode::add_protocol(xrl_sender_name, src_module_id) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add protocol instance '%s' "
				    "with module_id = %d",
				    xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Send info about all vifs
    //
    for (uint16_t i = 0; i < MfeaNode::maxvifs(); i++) {
	MfeaVif *mfea_vif = MfeaNode::vif_find_by_vif_index(i);
	if (mfea_vif == NULL)
	    continue;
	_xrl_mfea_client_client.send_new_vif(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->vif_index(),
	    callback(this, &XrlMfeaNode::mfea_client_client_send_new_vif_cb));
	list<VifAddr>::const_iterator iter;
	for (iter = mfea_vif->addr_list().begin();
	     iter != mfea_vif->addr_list().end();
	     ++iter) {
	    const VifAddr& vif_addr = *iter;
	    _xrl_mfea_client_client.send_add_vif_addr6(
		xrl_sender_name.c_str(),
		mfea_vif->name(),
		vif_addr.addr().get_ipv6(),
		vif_addr.subnet_addr().get_ipv6net(),
		vif_addr.broadcast_addr().get_ipv6(),
		vif_addr.peer_addr().get_ipv6(),
		callback(this, &XrlMfeaNode::mfea_client_client_send_add_vif_addr_cb));
	}
	_xrl_mfea_client_client.send_set_vif_flags(
	    xrl_sender_name.c_str(),
	    mfea_vif->name(),
	    mfea_vif->is_pim_register(),
	    mfea_vif->is_p2p(),
	    mfea_vif->is_loopback(),
	    mfea_vif->is_multicast_capable(),
	    mfea_vif->is_broadcast_capable(),
	    mfea_vif->is_underlying_vif_up(),
	    callback(this, &XrlMfeaNode::mfea_client_client_send_set_vif_flags_cb));
    }
    
    // We are done with the vifs
    _xrl_mfea_client_client.send_set_all_vifs_done(
	xrl_sender_name.c_str(),
	callback(this, &XrlMfeaNode::mfea_client_client_send_set_all_vifs_done_cb));
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_protocol4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    // Delete the protocol
    //
    if (MfeaNode::delete_protocol(xrl_sender_name, src_module_id) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete protocol instance '%s' "
				    "with module_id =  %d",
				    xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_protocol6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    // Delete the protocol
    //
    if (MfeaNode::delete_protocol(xrl_sender_name, src_module_id) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete protocol instance '%s' "
				    "with module_id =  %d",
				    xrl_sender_name.c_str(), src_module_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_protocol_vif4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    // Start the protocol on the vif
    //
    if (MfeaNode::start_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot start protocol instance '%s' "
				    "on vif_index %d",
				    xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_protocol_vif6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    // Start the protocol on the vif
    //
    if (MfeaNode::start_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot start protocol instance '%s' "
				    "on vif_index %d",
				    xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_protocol_vif4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    // Stop the protocol on the vif
    //
    if (MfeaNode::stop_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot stop protocol instance '%s' "
				    "on vif_index %d",
				    xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_protocol_vif6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	, // vif_name, 
    const uint32_t&	vif_index)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    // Stop the protocol on the vif
    //
    if (MfeaNode::stop_protocol_vif(xrl_sender_name, src_module_id, vif_index)
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot stop protocol instance '%s' "
				    "on vif_index %d",
				    xrl_sender_name.c_str(), vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_allow_signal_messages(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const bool&		is_allow)
{
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (is_allow) {
	if (MfeaNode::add_allow_kernel_signal_messages(xrl_sender_name,
						       src_module_id)
	    != XORP_OK) {
	    string error_msg = c_format("Failed to enable kernel signal "
					"messages for target %s with "
					"protocol_id = %d",
					xrl_sender_name.c_str(), protocol_id);
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    } else {
	if (MfeaNode::delete_allow_kernel_signal_messages(xrl_sender_name,
							  src_module_id)
	    != XORP_OK) {
	    string error_msg = c_format("Failed to disable kernel signal "
					"messages for target %s with "
					"protocol_id = %d",
					xrl_sender_name.c_str(), protocol_id);
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_allow_mrib_messages(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const bool&		is_allow)
{
    //
    // Verify the module ID
    //
    xorp_module_id src_module_id = static_cast<xorp_module_id>(protocol_id);
    if (! is_valid_module_id(src_module_id)) {
	string error_msg = c_format("Invalid module ID = %d", protocol_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    if (is_allow) {
	if (MfeaNode::add_allow_mrib_messages(xrl_sender_name, src_module_id)
	    != XORP_OK) {
	    string error_msg = c_format("Failed to enable MRIB messages "
					"for target %s with protocol_id = %d",
					xrl_sender_name.c_str(), protocol_id);
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    } else {
	if (MfeaNode::delete_allow_mrib_messages(xrl_sender_name, src_module_id)
	    != XORP_OK) {
	    string error_msg = c_format("Failed to disable MRIB messages "
					"for target %s with protocol_id = %d",
					xrl_sender_name.c_str(), protocol_id);
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    }
    
    if (is_allow) {
	//
	// Send MRIB info
	//
	MribTable::iterator iter;
	bool is_sent = false;
	
	for (iter = MfeaNode::mrib_table().begin();
	     iter != MfeaNode::mrib_table().end();
	     ++iter) {
	    Mrib *mrib = *iter;
	    if (mrib == NULL)
		continue;
	    string vif_name;
	    Vif *vif = MfeaNode::vif_find_by_vif_index(mrib->next_hop_vif_index());
	    if (vif != NULL)
		vif_name = vif->name();
	    is_sent = true;

	    send_add_mrib(xrl_sender_name, src_module_id, *mrib);
	}
	//
	// Done
	//
	if (is_sent) {
	    send_set_mrib_done(xrl_sender_name, src_module_id);
	}
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_join_multicast_group4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    
    if (MfeaNode::join_multicast_group(xrl_sender_name, src_module_id, vif_index,
				       IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot join group %s on vif %s",
				    group_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_join_multicast_group6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    
    if (MfeaNode::join_multicast_group(xrl_sender_name, src_module_id, vif_index,
				       IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot join group %s on vif %s",
				    group_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_leave_multicast_group4(
    // Input values, 
    const string&	xrl_sender_name,
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv4&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
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
    
    if (MfeaNode::leave_multicast_group(xrl_sender_name, src_module_id,
					vif_index, IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot leave group %s on vif %s",
				    group_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_leave_multicast_group6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
    const uint32_t&	vif_index, 
    const IPv6&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
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
    
    if (MfeaNode::leave_multicast_group(xrl_sender_name, src_module_id,
					vif_index, IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot leave group %s on vif %s",
				    group_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_mfc4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	iif_vif_index, 
    const vector<uint8_t>& oiflist, 
    const vector<uint8_t>& oiflist_disable_wrongvif, 
    const uint32_t&	max_vifs_oiflist, 
    const IPv4&		rp_address)
{
    Mifset mifset;
    Mifset mifset_disable_wrongvif;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	string error_msg = c_format("Received 'add_mfc' with invalid "
				    "'max_vifs_oiflist' = %u (expected <= %u)",
				    max_vifs_oiflist, (uint32_t)mifset.size());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address))
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add MFC for "
				    "source %s and group %s "
				    "with iif_vif_index = %d",
				    source_address.str().c_str(),
				    group_address.str().c_str(),
				    iif_vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_mfc6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	iif_vif_index, 
    const vector<uint8_t>& oiflist, 
    const vector<uint8_t>& oiflist_disable_wrongvif, 
    const uint32_t&	max_vifs_oiflist, 
    const IPv6&		rp_address)
{
    Mifset mifset;
    Mifset mifset_disable_wrongvif;

    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Check the number of covered interfaces
    if (max_vifs_oiflist > mifset.size()) {
	// Invalid number of covered interfaces
	string error_msg = c_format("Received 'add_mfc' with invalid "
				    "'max_vifs_oiflist' = %u (expected <= %u)",
				    max_vifs_oiflist, (uint32_t)mifset.size());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    // Get the set of outgoing interfaces
    vector_to_mifset(oiflist, mifset);
    
    // Get the set of interfaces to disable the WRONGVIF signal.
    vector_to_mifset(oiflist_disable_wrongvif, mifset_disable_wrongvif);
    
    if (MfeaNode::add_mfc(xrl_sender_name,
			  IPvX(source_address), IPvX(group_address),
			  iif_vif_index, mifset, mifset_disable_wrongvif,
			  max_vifs_oiflist,
			  IPvX(rp_address))
	< 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add MFC for "
				    "source %s and group %s "
				    "with iif_vif_index = %d",
				    source_address.str().c_str(),
				    group_address.str().c_str(),
				    iif_vif_index);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete MFC for "
				    "source %s and group %s",
				    source_address.str().c_str(),
				    group_address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_mfc6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_mfc(xrl_sender_name,
			     IPvX(source_address), IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete MFC for "
				    "source %s and group %s",
				    source_address.str().c_str(),
				    group_address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_send_protocol_message4(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
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
    if (! MfeaNode::is_ipv4()) {
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
    if (MfeaNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size()) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot send %s protocol message "
				    "from %s to %s on vif %s",
				    xrl_sender_name.c_str(),
				    source_address.str().c_str(),
				    dest_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_send_protocol_message6(
    // Input values, 
    const string&	xrl_sender_name, 
    const string&	, // protocol_name, 
    const uint32_t&	protocol_id, 
    const string&	vif_name, 
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
    if (! MfeaNode::is_ipv6()) {
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
    if (MfeaNode::proto_recv(xrl_sender_name,
			     src_module_id,
			     vif_index,
			     IPvX(source_address),
			     IPvX(dest_address),
			     ip_ttl,
			     ip_tos,
			     is_router_alert,
			     &protocol_message[0],
			     protocol_message.size()) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot send %s protocol message "
				    "from %s to %s on vif %s",
				    xrl_sender_name.c_str(),
				    source_address.str().c_str(),
				    dest_address.str().c_str(),
				    vif_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       TimeVal(threshold_interval_sec,
					       threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add dataflow monitoring for "
				    "source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_add_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::add_dataflow_monitor(xrl_sender_name,
				       IPvX(source_address),
				       IPvX(group_address),
				       TimeVal(threshold_interval_sec,
					       threshold_interval_usec),
				       threshold_packets,
				       threshold_bytes,
				       is_threshold_in_packets,
				       is_threshold_in_bytes,
				       is_geq_upcall,
				       is_leq_upcall) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot add dataflow monitoring for "
				    "source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  TimeVal(threshold_interval_sec,
						  threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete dataflow monitoring for "
				    "source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address, 
    const uint32_t&	threshold_interval_sec, 
    const uint32_t&	threshold_interval_usec, 
    const uint32_t&	threshold_packets, 
    const uint32_t&	threshold_bytes, 
    const bool&		is_threshold_in_packets, 
    const bool&		is_threshold_in_bytes, 
    const bool&		is_geq_upcall, 
    const bool&		is_leq_upcall)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_dataflow_monitor(xrl_sender_name,
					  IPvX(source_address),
					  IPvX(group_address),
					  TimeVal(threshold_interval_sec,
						  threshold_interval_usec),
					  threshold_packets,
					  threshold_bytes,
					  is_threshold_in_packets,
					  is_threshold_in_bytes,
					  is_geq_upcall,
					  is_leq_upcall) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete dataflow monitoring for "
				    "source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor4(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv4&		source_address, 
    const IPv4&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv4()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv4");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete all dataflow monitoring "
				    "for source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_delete_all_dataflow_monitor6(
    // Input values, 
    const string&	xrl_sender_name, 
    const IPv6&		source_address, 
    const IPv6&		group_address)
{
    //
    // Verify the address family
    //
    if (! MfeaNode::is_ipv6()) {
	string error_msg = c_format("Received protocol message with "
				    "invalid address family: IPv6");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (MfeaNode::delete_all_dataflow_monitor(xrl_sender_name,
					      IPvX(source_address),
					      IPvX(group_address)) < 0) {
	// TODO: must find-out and return the reason for failure
	string error_msg = c_format("Cannot delete all dataflow monitoring "
				    "for source %s and group %s",
				    cstring(source_address),
				    cstring(group_address));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    //
    // Success
    //
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable)
{
    string error_msg;
    int ret_code;

    if (enable)
	ret_code = MfeaNode::enable_vif(vif_name, error_msg);
    else
	ret_code = MfeaNode::disable_vif(vif_name, error_msg);

    if (ret_code != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (MfeaNode::start_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_vif(
    // Input values, 
    const string&	vif_name)
{
    string error_msg;
    
    if (MfeaNode::stop_vif(vif_name, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_all_vifs(
    // Input values,
    const bool&	enable)
{
    string error_msg;
    int ret_code;

    if (enable)
	ret_code = MfeaNode::enable_all_vifs();
    else
	ret_code = MfeaNode::disable_all_vifs();

    if (ret_code != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable all vifs");
	else
	    error_msg = c_format("Failed to disable all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_all_vifs()
{
    if (MfeaNode::start_all_vifs() < 0) {
	string error_msg = c_format("Failed to start all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_all_vifs()
{
    if (MfeaNode::stop_all_vifs() < 0) {
	string error_msg = c_format("Failed to stop all vifs");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_mfea(
	// Input values,
	const bool&	enable)
{
    string error_msg;
    int ret_value;

    if (enable)
	ret_value = enable_mfea();
    else
	ret_value = disable_mfea();

    if (ret_value != XORP_OK) {
	if (enable)
	    error_msg = c_format("Failed to enable MFEA");
	else
	    error_msg = c_format("Failed to disable MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_mfea()
{
    if (start_mfea() != XORP_OK) {
	string error_msg = c_format("Failed to start MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_mfea()
{
    if (stop_mfea() != XORP_OK) {
	string error_msg = c_format("Failed to stop MFEA");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_enable_cli(
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
	    error_msg = c_format("Failed to enable MFEA CLI");
	else
	    error_msg = c_format("Failed to disable MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_start_cli()
{
    if (start_cli() != XORP_OK) {
	string error_msg = c_format("Failed to start MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_stop_cli()
{
    if (stop_cli() != XORP_OK) {
	string error_msg = c_format("Failed to start MFEA CLI");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_log_trace_all(
    // Input values,
    const bool&	enable)
{
    MfeaNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_get_mrib_table_default_metric_preference(
    // Output values, 
    uint32_t&	metric_preference)
{
    metric_preference = MfeaNode::mrib_table_default_metric_preference().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_set_mrib_table_default_metric_preference(
    // Input values, 
    const uint32_t&	metric_preference)
{
    if (MfeaNode::set_mrib_table_default_metric_preference(metric_preference)
	< 0) {
	string error_msg = c_format("Failed to set default "
				    "metric preference to %d",
				    metric_preference);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_reset_mrib_table_default_metric_preference()
{
    if (MfeaNode::reset_mrib_table_default_metric_preference() < 0) {
	string error_msg = c_format("Failed to reset default "
				    "metric preference");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_get_mrib_table_default_metric(
    // Output values, 
    uint32_t&	metric)
{
    metric = MfeaNode::mrib_table_default_metric().get();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_set_mrib_table_default_metric(
    // Input values, 
    const uint32_t&	metric)
{
    if (MfeaNode::set_mrib_table_default_metric(metric) < 0) {
	string error_msg = c_format("Failed to set default metric to %d",
				    metric);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlMfeaNode::mfea_0_1_reset_mrib_table_default_metric()
{
    if (MfeaNode::reset_mrib_table_default_metric() < 0) {
	string error_msg = c_format("Failed to reset default metric");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    
    return XrlCmdError::OKAY();
}
