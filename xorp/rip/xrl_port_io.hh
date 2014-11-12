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

#ifndef __XRL_PORT_IO_HH__
#define __XRL_PORT_IO_HH__


#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/safe_callback_obj.hh"
#include "libxorp/service.hh"
#include "port_io.hh"

class XrlError;
class XrlRouter;

template <typename A>
class XrlPortIO
    : public PortIOBase<A>, public ServiceBase, public CallbackSafeObject
{
public:
    typedef A			Addr;
    typedef PortIOUserBase<A>	PortIOUser;
public:
    XrlPortIO(XrlRouter&	xr,
	      PortIOUser&	port,
	      const string& 	ifname,
	      const string&	vifname,
	      const Addr&	addr);

    /**
     * Startup.  Sends request to FEA for socket server for address
     * and then attempts to instantiate socket with socket server.  If
     * both operations are successful, instance status transitions to
     * SERVICE_RUNNING.  Otherwise, it transitions to failed.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown.  Sends request to close socket and transitions into
     * SERVICE_SHUTTING_DOWN state.  When socket is closed transition to
     * SERVICE_SHUTDOWN occurs.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Send packet.  Status of instance must be running.  When packet is sent,
     * the @ref pending() method will return true until the Xrl sending the
     * packet has completed.
     *
     * @param dst_addr address to send packet.
     * @param dst_port port to send packet to.
     * @param rip_packet vector containing rip packet to be sent.
     *
     * @return false on immediately detectable failure, true otherwise.
     */
    bool send(const Addr&		dst_addr,
	      uint16_t			dst_port,
	      const vector<uint8_t>& 	rip_packet);

    /**
     * Return true if awaiting @ref send() completion, false otherwise.
     */
    bool pending() const;

    /**
     * Get name of socket server used to instantiate socket.
     */
    const string& socket_server() const		{ return _ss; }

    const string& socket_id() const		{ return _sid; }

private:
    bool startup_socket();

    bool request_open_bind_socket();
    void open_bind_socket_cb(const XrlError& xe, const string* psid);

    bool request_ttl();
    void ttl_cb(const XrlError& xe);

    bool request_no_loop();
    void no_loop_cb(const XrlError& xe);

    bool request_socket_join();
    void socket_join_cb(const XrlError& xe);

    bool request_socket_leave();
    void socket_leave_cb(const XrlError& xe);

    void send_cb(const XrlError& xe);

protected:
    XrlRouter&	_xr;
    string	_ss;			// Socket Server Target Name
    string	_sid;			// Unicast Socket id
    bool	_pending;
};

#endif // __XRL_PORT_IO_HH__
