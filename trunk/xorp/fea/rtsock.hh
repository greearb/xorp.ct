// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/fea/rtsock.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_RTSOCK_HH__
#define __FEA_RTSOCK_HH__

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "fti.hh"

class RoutingSocketObserver;
struct RoutingSocketPlumber;

/**
 * RoutingSocket class opens a routing socket and forwards data arriving
 * on the socket to RoutingSocketObservers.  The RoutingSocket hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class RoutingSocket {
public:
    RoutingSocket(EventLoop& e, int af = AF_UNSPEC) throw(FtiError);
    ~RoutingSocket();

    /* RoutingSocket may fail to open routing socket during construction. */
    inline bool is_open() const { return _fd != -1; }

    /**
     * Write data to routing socket.  Update sequence number associated
     * with routing socket.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Get sequence number for next message written into kernel.
     * Sequence number is derived of the instance number of the routing
     * socket and a 16bit counter.
     */
    inline uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get cached process identifier value.
     */
    inline pid_t pid() const { return _pid; }

    /**
     * Force socket to read data - usually performed after writing
     * a request that the kernel will answer, eg after writing a route
     * lookup.
     *
     * Use sparingly, with caution, and at your own risk.
     */
    void force_read();

    typedef list<RoutingSocketObserver*> ObserverList;

private:
    /**
     * Read data available for RoutingSocket and invoke
     * RoutingSocketObserver::rtsock_data on all observers of routing
     * socket.
     */
    void select_hook(int fd, SelectorMask sm);

    void shutdown();

    RoutingSocket& operator=(const RoutingSocket&);	/* Not implemented */
    RoutingSocket(const RoutingSocket&);		/* Not implemented */

private:
    static const size_t RTSOCK_BYTES = 4096; // Guess at largest sock message

private:
    EventLoop&	 _e;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	/* Seqno of next write() */
    uint16_t	 _instance_no;  /* Instance number of this routing socket */

    uint8_t	 _buffer[RTSOCK_BYTES];

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    friend class RoutingSocketPlumber; // class that hooks observers in and out
};

class RoutingSocketObserver {
public:
    RoutingSocketObserver(RoutingSocket& rs);

    virtual ~RoutingSocketObserver();

    /**
     * Method called when the observed routing socket has data to be
     * parsed.
     *
     * @param data pointer to data.
     * @param nbytes number of bytes available.
     */
    virtual void rtsock_data(const uint8_t* data, size_t nbytes) = 0;

    /**
     * Get RoutingSocket associated with Observer.
     */
    RoutingSocket& routing_socket();

private:
    RoutingSocket& _rs;
};

#endif // __FEA_RTSOCK_HH__
