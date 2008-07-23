// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/xrl_io_tcpudp_manager.cc,v 1.5 2008/01/04 03:15:52 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/socket4_user_xif.hh"
#include "xrl/interfaces/socket6_user_xif.hh"

#include "xrl_io_tcpudp_manager.hh"

XrlIoTcpUdpManager::XrlIoTcpUdpManager(IoTcpUdpManager&	io_tcpudp_manager,
				       XrlRouter&	xrl_router)
    : IoTcpUdpManagerReceiver(),
      _io_tcpudp_manager(io_tcpudp_manager),
      _xrl_router(xrl_router)
{
    _io_tcpudp_manager.set_io_tcpudp_manager_receiver(this);
}

XrlIoTcpUdpManager::~XrlIoTcpUdpManager()
{
    _io_tcpudp_manager.set_io_tcpudp_manager_receiver(NULL);
}

void
XrlIoTcpUdpManager::recv_event(const string&		receiver_name,
			       const string&		sockid,
			       const string&		if_name,
			       const string&		vif_name,
			       const IPvX&		src_host,
			       uint16_t			src_port,
			       const vector<uint8_t>&	data)
{
    if (src_host.is_ipv4()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket4UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_recv_event(receiver_name.c_str(),
			   sockid,
			   if_name,
			   vif_name,
			   src_host.get_ipv4(),
			   src_port,
			   data,
			   callback(this,
				    &XrlIoTcpUdpManager::xrl_send_recv_event_cb,
				    src_host.af(), receiver_name));
    }

    if (src_host.is_ipv6()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket6UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_recv_event(receiver_name.c_str(),
			   sockid,
			   if_name,
			   vif_name,
			   src_host.get_ipv6(),
			   src_port,
			   data,
			   callback(this,
				    &XrlIoTcpUdpManager::xrl_send_recv_event_cb,
				    src_host.af(), receiver_name));
    }
}

void
XrlIoTcpUdpManager::inbound_connect_event(const string&		receiver_name,
					  const string&		sockid,
					  const IPvX&		src_host,
					  uint16_t		src_port,
					  const string&		new_sockid)
{
    if (src_host.is_ipv4()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket4UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_inbound_connect_event(receiver_name.c_str(),
				      sockid,
				      src_host.get_ipv4(),
				      src_port,
				      new_sockid,
				      callback(this,
					       &XrlIoTcpUdpManager::xrl_send_inbound_connect_event_cb,
					       src_host.af(), new_sockid,
					       receiver_name));
    }

    if (src_host.is_ipv6()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket6UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_inbound_connect_event(receiver_name.c_str(),
				      sockid,
				      src_host.get_ipv6(),
				      src_port,
				      new_sockid,
				      callback(this,
					       &XrlIoTcpUdpManager::xrl_send_inbound_connect_event_cb,
				       src_host.af(), new_sockid,
				       receiver_name));
    }
}

void
XrlIoTcpUdpManager::outgoing_connect_event(int			family,
					   const string&	receiver_name,
					   const string&	sockid)
{
    if (family == IPv4::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket4UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_outgoing_connect_event(receiver_name.c_str(),
				       sockid,
				       callback(this,
						&XrlIoTcpUdpManager::xrl_send_outgoing_connect_event_cb,
						family, receiver_name));
    }

    if (family == IPv6::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket6UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_outgoing_connect_event(receiver_name.c_str(),
				       sockid,
				       callback(this,
						&XrlIoTcpUdpManager::xrl_send_outgoing_connect_event_cb,
						family, receiver_name));
    }
}

void
XrlIoTcpUdpManager::error_event(int			family,
				const string&		receiver_name,
				const string&		sockid,
				const string&		error,
				bool			fatal)
{
    if (family == IPv4::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket4UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_error_event(receiver_name.c_str(),
			    sockid,
			    error,
			    fatal,
			    callback(this,
				     &XrlIoTcpUdpManager::xrl_send_error_event_cb,
				     family, receiver_name));
    }

    if (family == IPv6::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket6UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_error_event(receiver_name.c_str(),
			    sockid,
			    error,
			    fatal,
			    callback(this,
				     &XrlIoTcpUdpManager::xrl_send_error_event_cb,
				     family, receiver_name));
    }
}

void
XrlIoTcpUdpManager::disconnect_event(int			family,
				     const string&		receiver_name,
				     const string&		sockid)
{
    if (family == IPv4::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket4UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_disconnect_event(receiver_name.c_str(),
				 sockid,
				 callback(this,
					  &XrlIoTcpUdpManager::xrl_send_disconnect_event_cb,
					  family, receiver_name));
    }

    if (family == IPv6::af()) {
	//
	// Instantiate client sending interface
	//
	XrlSocket6UserV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_disconnect_event(receiver_name.c_str(),
				 sockid,
				 callback(this,
					  &XrlIoTcpUdpManager::xrl_send_disconnect_event_cb,
					  family, receiver_name));
    }
}

void
XrlIoTcpUdpManager::xrl_send_recv_event_cb(const XrlError& xrl_error,
					   int family, string receiver_name)
{
    UNUSED(family);

    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_recv_event_cb: error %s\n", xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all communication handlers associated with this receiver.
    //
    _io_tcpudp_manager.instance_death(receiver_name);
}

void
XrlIoTcpUdpManager::xrl_send_inbound_connect_event_cb(const XrlError& xrl_error,
						      const bool* accept,
						      int family,
						      string sockid,
						      string receiver_name)
{
    if (xrl_error == XrlError::OKAY()) {
	string error_msg;
	bool is_accepted = *accept;
	if (_io_tcpudp_manager.accept_connection(family, sockid, is_accepted,
						 error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("Error with %s a connection: %s",
		       (is_accepted)? "accept" : "reject", error_msg.c_str());
	}
	return;
    }

    debug_msg("xrl_send_inbound_connect_event_cb: error %s\n",
	      xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all communication handlers associated with this receiver.
    //
    _io_tcpudp_manager.instance_death(receiver_name);
}

void
XrlIoTcpUdpManager::xrl_send_outgoing_connect_event_cb(const XrlError& xrl_error,
						       int family,
						       string receiver_name)
{
    UNUSED(family);

    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_outgoing_connect_event_cb: error %s\n",
	      xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all communication handlers associated with this receiver.
    //
    _io_tcpudp_manager.instance_death(receiver_name);
}

void
XrlIoTcpUdpManager::xrl_send_error_event_cb(const XrlError& xrl_error,
					    int family, string receiver_name)
{
    UNUSED(family);

    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_error_event_cb: error %s\n", xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all communication handlers associated with this receiver.
    //
    _io_tcpudp_manager.instance_death(receiver_name);
}

void
XrlIoTcpUdpManager::xrl_send_disconnect_event_cb(const XrlError& xrl_error,
						 int family, string receiver_name)
{
    UNUSED(family);

    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_disconnect_event_cb: error %s\n",
	      xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all communication handlers associated with this receiver.
    //
    _io_tcpudp_manager.instance_death(receiver_name);
}
