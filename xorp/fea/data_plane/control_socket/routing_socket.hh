// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/control_socket/routing_socket.hh,v 1.7 2008/10/02 21:56:54 bms Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__

#include <xorp_config.h>
#ifdef HAVE_ROUTING_SOCKETS

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"


class RoutingSocketObserver;
struct RoutingSocketPlumber;

/**
 * RoutingSocket class opens a routing socket and forwards data arriving
 * on the socket to RoutingSocketObservers.  The RoutingSocket hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class RoutingSocket :
    public NONCOPYABLE
{
public:
    RoutingSocket(EventLoop& eventloop);
    ~RoutingSocket();

    /**
     * Start the routing socket operation for a given address family.
     * 
     * @param af the address family.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(int af, string& error_msg);

    /**
     * Start the routing socket operation for an unspecified address family.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg) { return start(AF_UNSPEC, error_msg); }

    /**
     * Stop the routing socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Test if the routing socket is open.
     * 
     * This method is needed because RoutingSocket may fail to open
     * routing socket during startup.
     * 
     * @return true if the routing socket is open, otherwise false.
     */
    bool is_open() const { return _fd >= 0; }

    /**
     * Write data to routing socket.
     * 
     * This method also updates the sequence number associated with
     * this routing socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Get the sequence number for next message written into the kernel.
     * 
     * The sequence number is derived from the instance number of this routing
     * socket and a 16-bit counter.
     * 
     * @return the sequence number for the next message written into the
     * kernel.
     */
    uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get the cached process identifier value.
     * 
     * @return the cached process identifier value.
     */
    pid_t pid() const { return _pid; }

    /**
     * Force socket to read data.
     * 
     * This usually is performed after writing a request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_read(string& error_msg);

private:
    typedef list<RoutingSocketObserver*> ObserverList;

    /**
     * Read data available for RoutingSocket and invoke
     * RoutingSocketObserver::routing_socket_data() on all observers of routing
     * socket.
     */
    void io_event(XorpFd fd, IoEventType type);

private:
    static const size_t ROUTING_SOCKET_BYTES = 8*1024;	// Initial guess at msg size

private:
    EventLoop&	 _eventloop;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this routing socket

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    friend class RoutingSocketPlumber; // class that hooks observers in and out
};

class RoutingSocketObserver {
public:
    RoutingSocketObserver(RoutingSocket& rs);

    virtual ~RoutingSocketObserver();

    /**
     * Receive data from the routing socket.
     *
     * Note that this method is called asynchronously when the routing socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the routing socket facility itself.
     *
     * @param buffer the buffer with the received data.
     */
    virtual void routing_socket_data(const vector<uint8_t>& buffer) = 0;

    /**
     * Get RoutingSocket associated with Observer.
     */
    RoutingSocket& routing_socket();

private:
    RoutingSocket& _rs;
};

class RoutingSocketReader : public RoutingSocketObserver {
public:
    RoutingSocketReader(RoutingSocket& rs);
    virtual ~RoutingSocketReader();

    /**
     * Force the reader to receive data from the specified routing socket.
     *
     * @param rs the routing socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data(RoutingSocket& rs, uint32_t seqno, string& error_msg);

    /**
     * Get the buffer with the data that was received.
     *
     * @return a reference to the buffer with the data that was received.
     */
    const vector<uint8_t>& buffer() const { return (_cache_data); }

    /**
     * Receive data from the routing socket.
     *
     * Note that this method is called asynchronously when the routing socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the routing socket facility itself.
     *
     * @param buffer the buffer with the received data.
     */
    virtual void routing_socket_data(const vector<uint8_t>& buffer);

private:
    RoutingSocket&  _rs;

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of routing socket data to
					// cache so reading via routing
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached routing socket data.
};

#endif
#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__
