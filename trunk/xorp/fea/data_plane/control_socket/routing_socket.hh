// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/control_socket/routing_socket.hh,v 1.5 2008/01/04 03:15:55 pavlin Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__

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
class RoutingSocket {
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

    RoutingSocket& operator=(const RoutingSocket&);	// Not implemented
    RoutingSocket(const RoutingSocket&);		// Not implemented

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

#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_ROUTING_SOCKET_HH__
