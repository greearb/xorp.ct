// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/fea/rtsock.hh,v 1.3 2003/03/10 23:20:16 hodson Exp $

#ifndef __FEA_NETLINK_SOCKET_HH__
#define __FEA_NETLINK_SOCKET_HH__

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

class NetlinkSocketObserver;
struct NetlinkSocketPlumber;

/**
 * NetlinkSocket class opens a netlink socket and forwards data arriving
 * on the socket to NetlinkSocketObservers.  The NetlinkSocket hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class NetlinkSocket {
public:
    NetlinkSocket(EventLoop& e);
    ~NetlinkSocket();

    /**
     * Start the netlink socket operation.
     * 
     * @param af the address family.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(int af = AF_UNSPEC);

    /**
     * Stop the netlink socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop();

    // NetlinkSocket may fail to open netlink socket during construction.
    inline bool is_open() const { return _fd >= 0; }

    /**
     * Write data to netlink socket.  Update sequence number associated
     * with netlink socket.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Sendto data on netlink socket.  Update sequence number associated
     * with netlink socket.
     */
    ssize_t sendto(const void* data, size_t nbytes, int flags,
		   const struct sockaddr* to, socklen_t tolen);
    
    /**
     * Get sequence number for next message written into kernel.
     * Sequence number is derived of the instance number of the netlink
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

    /**
     * Force socket to recvfrom data - usually performed after writing
     * a sendto() request that the kernel will answer, eg after writing a route
     * lookup.
     *
     * Use sparingly, with caution, and at your own risk.
     */
    void force_recvfrom(int flags, struct sockaddr* from, socklen_t* fromlen);

    /**
     * Force socket to recvmsg data - usually performed after writing
     * a sendto() request that the kernel will answer, eg after writing a route
     * lookup.
     *
     * Use sparingly, with caution, and at your own risk.
     */
    void force_recvmsg(int flags);
    
    typedef list<NetlinkSocketObserver*> ObserverList;

private:
    /**
     * Read data available for NetlinkSocket and invoke
     * NetlinkSocketObserver::nlsock_data on all observers of netlink
     * socket.
     */
    void select_hook(int fd, SelectorMask sm);

    void shutdown();

    NetlinkSocket& operator=(const NetlinkSocket&);	// Not implemented
    NetlinkSocket(const NetlinkSocket&);		// Not implemented

private:
    static const size_t NLSOCK_BYTES = 8*1024; // Guess at largest sock message

private:
    EventLoop&	 _e;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this netlink socket

    uint8_t	 _buffer[NLSOCK_BYTES];

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    friend class NetlinkSocketPlumber; // class that hooks observers in and out
};

class NetlinkSocketObserver {
public:
    NetlinkSocketObserver(NetlinkSocket& ns);

    virtual ~NetlinkSocketObserver();

    /**
     * Method called when the observed netlink socket has data to be
     * parsed.
     *
     * @param data pointer to data.
     * @param nbytes number of bytes available.
     */
    virtual void nlsock_data(const uint8_t* data, size_t nbytes) = 0;

    /**
     * Get NetlinkSocket associated with Observer.
     */
    NetlinkSocket& netlink_socket();

private:
    NetlinkSocket& _ns;
};

#endif // __FEA_NETLINK_SOCKET_HH__
