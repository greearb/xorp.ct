// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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

// $XORP: xorp/fea/xrl_io_tcpudp_manager.hh,v 1.6 2008/10/02 21:56:51 bms Exp $

#ifndef __FEA_XRL_IO_TCPUDP_MANAGER_HH__
#define __FEA_XRL_IO_TCPUDP_MANAGER_HH__

#include "io_tcpudp_manager.hh"

class XrlRouter;

/**
 * @short A class that is the bridge between the I/O TCP/UDP communications
 * and the XORP XRL interface.
 */
class XrlIoTcpUdpManager : public IoTcpUdpManagerReceiver {
public:
    /**
     * Constructor.
     */
    XrlIoTcpUdpManager(IoTcpUdpManager& io_tcpudp_manager,
		       XrlRouter& xrl_router);

    /**
     * Destructor.
     */
    virtual ~XrlIoTcpUdpManager();

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the data to.
     * @param sockid unique socket ID.
     * @param if_name the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param data the data received.
     */
    void recv_event(const string&		receiver_name,
		    const string&		sockid,
		    const string&		if_name,
		    const string&		vif_name,
		    const IPvX&			src_host,
		    uint16_t			src_port,
		    const vector<uint8_t>&	data);

    /**
     * Inbound connection request received event.
     *
     * It applies only to TCP sockets.
     *
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param new_sockid the new socket ID.
     */
    void inbound_connect_event(const string&	receiver_name,
			       const string&	sockid,
			       const IPvX&	src_host,
			       uint16_t		src_port,
			       const string&	new_sockid);

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    void outgoing_connect_event(int		family,
				const string&	receiver_name,
				const string&	sockid);

    /**
     * Error occured event.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    void error_event(int			family,
		     const string&		receiver_name,
		     const string&		sockid,
		     const string&		error,
		     bool			fatal);

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    void disconnect_event(int			family,
			  const string&		receiver_name,
			  const string&		sockid);

private:
    XrlRouter&		xrl_router() { return _xrl_router; }

    /**
     * XRL callbacks
     */
    void xrl_send_recv_event_cb(const XrlError& xrl_error, int family,
				string receiver_name);
    void xrl_send_inbound_connect_event_cb(const XrlError& xrl_error,
					   const bool* accept, int family,
					   string sockid, 
					   string receiver_name);
    void xrl_send_outgoing_connect_event_cb(const XrlError& xrl_error,
					    int family, string receiver_name);
    void xrl_send_error_event_cb(const XrlError& xrl_error, int family,
				 string receiver_name);
    void xrl_send_disconnect_event_cb(const XrlError& xrl_error, int family,
				      string receiver_name);

    IoTcpUdpManager&	_io_tcpudp_manager;
    XrlRouter&		_xrl_router;
};

#endif // __FEA_XRL_IO_TCPUDP_MANAGER_HH__
