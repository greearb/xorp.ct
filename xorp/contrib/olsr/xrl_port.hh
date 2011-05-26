// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/xrl_port.hh,v 1.3 2008/10/02 21:56:37 bms Exp $

#ifndef __OLSR_XRL_PORT_HH__
#define __OLSR_XRL_PORT_HH__


#include "libxorp/ipv4.hh"
#include "libxorp/safe_callback_obj.hh"
#include "libxorp/service.hh"

class XrlError;
class XrlRouter;

/**
 * @short Helper class which encapsulates XRL socket service
 *
 * Deals with setting the specific XRL socket options that OLSR
 * needs to send/receive broadcast/multicast control traffic to
 * the all-nodes address.
 */
class XrlPort : public ServiceBase,
		public CallbackSafeObject {
public:
    /**
     * Begin creation of the broadcast/multicast socket.
     *
     * @param io pointer to parent object
     * @param eventloop process-wide event loop
     * @param xrl_router process-wide XRL router
     * @param ssname name of XRL target containing socket server;
     *               usually this is the FEA.
     * @param ifname interface to listen on
     * @param vifname vif to listen on
     * @param local_addr address to listen on
     * @param local_port port to listen on
     * @param all_nodes_addr address to send to
     */
    XrlPort(IO* io,
	    EventLoop& eventloop,
	    XrlRouter& xrl_router,
	    const string& ssname,
	    const string& ifname,
	    const string& vifname,
	    const IPv4& local_addr,
	    const uint16_t local_port,
	    const IPv4& all_nodes_addr);

    ~XrlPort();

    /**
     * Start the port binding.
     *
     * Sends request to FEA for socket server for address
     * and then attempts to instantiate socket with socket server.  If
     * both operations are successful, instance status transitions to
     * SERVICE_RUNNING.  Otherwise, it transitions to failed.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown the port binding.
     *
     * Sends request to close socket and transitions into
     * SERVICE_SHUTTING_DOWN state.  When socket is closed transition to
     * SERVICE_SHUTDOWN occurs.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Send packet.
     *
     * Status of instance must be running, and should not be pending.
     *
     * @param dst_addr address to send packet.
     * @param dst_port port to send packet to.
     * @param payload vector containing paylaod of packet to be sent.
     *
     * @return false on immediately detectable failure, true otherwise.
     */
    bool send_to(const IPv4& dst_addr,
	         const uint16_t dst_port,
	         const vector<uint8_t>& payload);

    /**
     * @return the name of the socket server in use.
     */
    inline string socket_server() const { return _ss; }

    /**
     * @return the name of the interface to which this socket is bound.
     */
    inline string ifname() const { return _ifname; }

    /**
     * @return the name of the vif to which this socket is bound.
     */
    inline string vifname() const { return _vifname; }

    /**
     * @return the address to which this socket is bound.
     */
    inline IPv4 local_address() const { return _local_addr; }

    /**
     * @return the port to which this socket is bound.
     */
    inline uint16_t local_port() const { return _local_port; }

    /**
     * @return the socket server's socket identifier,
     * if service status is RUNNING; otherwise undefined.
     */
    inline string sockid() const { return _sockid; }

    /**
     * @return the address to which this socket transmits.
     */
    inline IPv4 all_nodes_address() const { return _all_nodes_addr; }

private:
    bool startup_socket();

    //
    // Socket open operations.
    //
    bool request_udp_open_bind_broadcast();
    void udp_open_bind_broadcast_cb(const XrlError& e, const string* psid);

    bool request_tos();
    void tos_cb(const XrlError& xrl_error);

    void socket_setup_complete();

    //
    // BSD hack.
    //
    bool request_reuseport();
    void reuseport_cb(const XrlError& e);

    bool request_onesbcast(const bool enabled);
    void onesbcast_cb(const XrlError& e);

    //
    // Socket close operations.
    //

    bool request_close();
    void close_cb(const XrlError& xrl_error);

    //
    // Socket send operations.
    //

    void send_cb(const XrlError& xrl_error);

private:
    IO*			_io;
    EventLoop&		_eventloop;
    XrlRouter&		_xrl_router;

    string		_ss;
    string		_ifname;
    string		_vifname;
    IPv4		_local_addr;
    uint16_t		_local_port;
    IPv4		_all_nodes_addr;

    /* If true, system is initializing and cannot send normal packets with send_to */
    bool		_pending;

    string		_sockid;

    bool		_is_undirected_broadcast;
};

#endif // __OLSR_XRL_PORT_HH__
