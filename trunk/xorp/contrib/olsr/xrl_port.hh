// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP$

#ifndef __OLSR_XRL_PORT_HH__
#define __OLSR_XRL_PORT_HH__

#include <string>
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
     * Status of instance must be running.  When packet is sent,
     * the @ref pending() method will return true until the Xrl sending the
     * packet has completed.
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
     * @return true if an XRL operation is pending, otherwise false.
     */
    inline bool pending() const { return _pending; }

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

    bool		_pending;

    string		_sockid;

    bool		_is_undirected_broadcast;
};

#endif // __OLSR_XRL_PORT_HH__
