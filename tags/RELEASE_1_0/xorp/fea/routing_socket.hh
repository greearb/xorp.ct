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

// $XORP: xorp/fea/routing_socket.hh,v 1.4 2003/10/14 01:32:18 pavlin Exp $

#ifndef __FEA_ROUTING_SOCKET_HH__
#define __FEA_ROUTING_SOCKET_HH__

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
    RoutingSocket(EventLoop& e);
    ~RoutingSocket();

    /**
     * Start the routing socket operation.
     * 
     * @param af the address family.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(int af = AF_UNSPEC);

    /**
     * Stop the routing socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop();

    /**
     * Test if the routing socket is open.
     * 
     * This method is needed because RoutingSocket may fail to open
     * routing socket during construction.
     * 
     * @return true if the routing socket is open, otherwise false.
     */
    inline bool is_open() const { return _fd >= 0; }

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
    inline uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get the cached process identifier value.
     * 
     * @return the cached process identifier value.
     */
    inline pid_t pid() const { return _pid; }

    /**
     * Force socket to read data.
     * 
     * This usually is performed after writing a request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     */
    void force_read();

private:
    typedef list<RoutingSocketObserver*> ObserverList;

    /**
     * Read data available for RoutingSocket and invoke
     * RoutingSocketObserver::rtsock_data() on all observers of routing
     * socket.
     */
    void select_hook(int fd, SelectorMask sm);

    void shutdown();

    RoutingSocket& operator=(const RoutingSocket&);	// Not implemented
    RoutingSocket(const RoutingSocket&);		// Not implemented

private:
    static const size_t RTSOCK_BYTES = 8*1024; // Initial guess at msg size

private:
    EventLoop&	 _e;
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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void rtsock_data(const uint8_t* data, size_t nbytes) = 0;

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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data(RoutingSocket& rs, uint32_t seqno);

    /**
     * Get the buffer with the data that was received.
     *
     * @return a pointer to the beginning of the buffer with the data that
     * was received.
     */
    const uint8_t* buffer() const { return (&_cache_data[0]); }

    /**
     * Get the size of the buffer with the data that was received.
     * 
     * @return the size of the buffer with the data that was received.
     */
    const size_t   buffer_size() const { return (_cache_data.size()); }

    /**
     * Receive data from the routing socket.
     *
     * Note that this method is called asynchronously when the routing socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the routing socket facility itself.
     *
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void rtsock_data(const uint8_t* data, size_t nbytes);

private:
    RoutingSocket&  _rs;

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of routing socket data to
					// cache so reading via routing
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached routing socket data.
};

#endif // __FEA_ROUTING_SOCKET_HH__
