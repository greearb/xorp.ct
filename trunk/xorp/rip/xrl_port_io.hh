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

// $XORP: xorp/rip/xrl_port_io.hh,v 1.4 2004/04/22 01:11:51 pavlin Exp $

#ifndef __XRL_PORT_IO_HH__
#define __XRL_PORT_IO_HH__

#include <string>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/safe_callback_obj.hh"
#include "libxorp/service.hh"
#include "port_io.hh"

class XrlError;
class XrlRouter;
class SocketXrlQueue;

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
     * RUNNING.  Otherwise, it transitions to failed.
     *
     * @return true on success, false on failure.
     */
    bool startup();

    /**
     * Shutdown.  Sends request to close socket and transitions into
     * SHUTTING_DOWN state.  When socket is closed transition to
     * SHUTDOWN occurs.
     *
     * @return true on success, false on failure.
     */
    bool shutdown();

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
    inline const string& socket_server() const		{ return _ss; }

    inline const string& socket_id() const		{ return _sid; }

private:
    bool request_socket_server();
    void socket_server_cb(const XrlError& xe, const string* pss);

    bool request_open_bind_socket();
    void open_bind_socket_cb(const XrlError& xe, const string* psid);

    bool request_ttl_one();
    void ttl_one_cb(const XrlError& xe);

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
