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

// $XORP: xorp/fea/netlink_socket.hh,v 1.6 2003/10/14 01:34:08 pavlin Exp $

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
    int start(int af);

    /**
     * Stop the netlink socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop();

    /**
     * Test if the netlink socket is open.
     * 
     * This method is needed because NetlinkSocket may fail to open
     * netlink socket during construction.
     * 
     * @return true if the netlink socket is open, otherwise false.
     */
    inline bool is_open() const { return _fd >= 0; }

    /**
     * Write data to netlink socket.
     * 
     * This method also updates the sequence number associated with
     * this netlink socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Sendto data on netlink socket.
     * 
     * This method also updates the sequence number associated with
     * this netlink socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t sendto(const void* data, size_t nbytes, int flags,
		   const struct sockaddr* to, socklen_t tolen);
    
    /**
     * Get the sequence number for next message written into the kernel.
     * 
     * The sequence number is derived from the instance number of this netlink
     * socket and a 16-bit counter.
     * 
     * @return the sequence number for the next message written into the
     * kernel.
     */
    inline uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get cached process identifier value.
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

    /**
     * Force socket to recvfrom data.
     * 
     * This usually is performed after writing a sendto() request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     */
    void force_recvfrom(int flags, struct sockaddr* from, socklen_t* fromlen);

    /**
     * Force socket to recvmsg data.
     * 
     * This usually is performed after writing a senmsg() request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     */
    void force_recvmsg(int flags);

    /**
     * Set the netlink multicast groups to listen for on the netlink socket.
     *
     * Note that this method must be called before method start() is called.
     * If this method is not called, then the netlink socket will listen
     * to the default set of netlink multicast groups (the empty set).
     *
     * @param v the set of netlink multicast groups to listen for on the
     * netlink socket.
     */
    void	set_nl_groups(uint32_t v) { _nl_groups = v; }

private:
    typedef list<NetlinkSocketObserver*> ObserverList;

    /**
     * Read data available for NetlinkSocket and invoke
     * NetlinkSocketObserver::nlsock_data() on all observers of netlink
     * socket.
     */
    void select_hook(int fd, SelectorMask sm);

    void shutdown();

    NetlinkSocket& operator=(const NetlinkSocket&);	// Not implemented
    NetlinkSocket(const NetlinkSocket&);		// Not implemented

    static const size_t NLSOCK_BYTES = 8*1024; // Initial guess at msg size

    EventLoop&	 _e;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this netlink socket
    
    static uint16_t _instance_cnt;
    static pid_t    _pid;

    uint32_t	    _nl_groups;	// The netlink multicast groups to listen for

    friend class NetlinkSocketPlumber; // class that hooks observers in and out
};

/**
 * NetlinkSocket4 class is a wrapper for NetlinkSocket class for IPv4.
 */
class NetlinkSocket4 : public NetlinkSocket {
public:
    NetlinkSocket4(EventLoop& e) : NetlinkSocket(e) { }

    /**
     * Start the netlink socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start() { return NetlinkSocket::start(AF_INET); }
};

/**
 * NetlinkSocket6 class is a wrapper for NetlinkSocket class for IPv6.
 */
class NetlinkSocket6 : public NetlinkSocket {
public:
    NetlinkSocket6(EventLoop& e) : NetlinkSocket(e) { }

    /**
     * Start the netlink socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start() {
#ifdef HAVE_IPV6
	return NetlinkSocket::start(AF_INET6);
#else
	return (XORP_ERROR);
#endif
    }
};

class NetlinkSocketObserver {
public:
    NetlinkSocketObserver(NetlinkSocket4& ns4, NetlinkSocket6& ns6);
    virtual ~NetlinkSocketObserver();

    /**
     * Receive data from the netlink socket.
     *
     * Note that this method is called asynchronously when the netlink socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the netlink socket facility itself.
     *
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void nlsock_data(const uint8_t* data, size_t nbytes) = 0;

    /**
     * Get NetlinkSocket for IPv4 associated with Observer.
     */
    NetlinkSocket& netlink_socket4();

    /**
     * Get NetlinkSocket for IPv6 associated with Observer.
     */
    NetlinkSocket& netlink_socket6();

private:
    NetlinkSocket4& _ns4;
    NetlinkSocket6& _ns6;
};

class NetlinkSocketReader : public NetlinkSocketObserver {
public:
    NetlinkSocketReader(NetlinkSocket4& ns4, NetlinkSocket6& ns6);
    virtual ~NetlinkSocketReader();

    /**
     * Force the reader to receive data from the IPv4 netlink socket.
     *
     * @param seqno the sequence number of the data to receive.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data4(uint32_t seqno);

    /**
     * Force the reader to receive data from the IPv6 netlink socket.
     *
     * @param seqno the sequence number of the data to receive.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data6(uint32_t seqno);

    /**
     * Force the reader to receive data from the specified netlink socket.
     *
     * @param ns the netlink socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data(NetlinkSocket& ns, uint32_t seqno);

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
     * Receive data from the netlink socket.
     *
     * Note that this method is called asynchronously when the netlink socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the netlink socket facility itself.
     *
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void nlsock_data(const uint8_t* data, size_t nbytes);

private:
    NetlinkSocket4& _ns4;
    NetlinkSocket6& _ns6;

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of netlink socket data to
					// cache so reading via netlink
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached netlink socket data.
};

#endif // __FEA_NETLINK_SOCKET_HH__
