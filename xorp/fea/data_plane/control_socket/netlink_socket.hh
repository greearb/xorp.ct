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


#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_HH__

#include <xorp_config.h>
#ifdef HAVE_NETLINK_SOCKETS



#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

class NetlinkSocketObserver;
class NetlinkSocketPlumber;


/**
 * NetlinkSocket class opens a netlink socket and forwards data arriving
 * on the socket to NetlinkSocketObservers.  The NetlinkSocket hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class NetlinkSocket :
    public NONCOPYABLE
{
public:
    NetlinkSocket(EventLoop& eventloop, uint32_t table_id);
    virtual ~NetlinkSocket();

    /**
     * Start the netlink socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg);

    /**
     * Stop the netlink socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Test if the netlink socket is open.
     * 
     * This method is needed because NetlinkSocket may fail to open
     * netlink socket during startup.
     * 
     * @return true if the netlink socket is open, otherwise false.
     */
    bool is_open() const { return _fd >= 0; }

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
    uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get cached netlink socket identifier value.
     * 
     * @return the cached netlink socket identifier value.
     */
    uint32_t nl_pid() const { return _nl_pid; }

    /**
     * Force socket to recvmsg data.
     * 
     * This usually is performed after writing a sendmsg() request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param flags the flags argument to the underlying recvmsg(2)
     * system call.
     * @param only_kernel_messages if true, accept only messages originated
     * by the kernel.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_recvmsg_flgs(int flags, bool only_kernel_messages, string& error_msg);

    /** Same as above, but always passes MSG_DONTWAIT as flags so that we don't block
     * if packet is filtered, for example.
     */
    int force_recvmsg(bool only_kernel_messages, string& err_msg);

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

    /**
     * Set a flag to expect to read a multipart message that is terminated
     * with NLMSG_DONE.
     *
     * This flag is required to fix a bug with the Linux kernel:
     * if we try to read the whole forwarding table, the kernel
     * doesn't set the NLM_F_MULTI flag in each part of the multi-part
     * message. The problem is similar when we read all addresses on
     * an interface.
     *
     * @param v if true, set the flag to expect to read a multi-part message
     * that is terminated with NLMSG_DONE.
     */
    void	set_multipart_message_read(bool v) { _is_multipart_message_read = v; }

    /** Routing table ID that we are interested in might have changed.
     */
    virtual int notify_table_id_change(uint32_t new_tbl);

private:
    typedef list<NetlinkSocketObserver*> ObserverList;

    /**
     * Read data available for NetlinkSocket and invoke
     * NetlinkSocketObserver::netlink_socket_data() on all observers of netlink
     * socket.
     */
    void io_event(XorpFd fd, IoEventType sm);

    int bind_table_id();

    static const size_t NETLINK_SOCKET_BYTES = 8*1024;	// Initial guess at msg size

    EventLoop&	 _eventloop;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this netlink socket
    
    static uint16_t _instance_cnt;
    uint32_t	_nl_pid;

    uint32_t	_nl_groups;	// The netlink multicast groups to listen for
    uint32_t _table_id; // routing table.. or 0 if any/all (default behaviour) 
    bool	_is_multipart_message_read; // If true, expect to read a multipart message

    uint32_t   _nlm_count; // keep track of how many msgs received.

    friend class NetlinkSocketPlumber; // class that hooks observers in and out
};

class NetlinkSocketObserver {
public:
    NetlinkSocketObserver(NetlinkSocket& ns);
    virtual ~NetlinkSocketObserver();

    /**
     * Receive data from the netlink socket.
     *
     * Note that this method is called asynchronously when the netlink socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the netlink socket facility itself.
     *
     * @param buffer the buffer with the received data.
     */
    virtual void netlink_socket_data(const vector<uint8_t>& buffer) = 0;

    /**
     * Get NetlinkSocket associated with Observer.
     */
    NetlinkSocket& netlink_socket();

private:
    NetlinkSocket& _ns;
};

class NetlinkSocketReader : public NetlinkSocketObserver {
public:
    NetlinkSocketReader(NetlinkSocket& ns);
    virtual ~NetlinkSocketReader();

    /**
     * Force the reader to receive data from the specified netlink socket.
     *
     * @param ns the netlink socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data(NetlinkSocket& ns, uint32_t seqno, string& error_msg);

    /**
     * Get the buffer with the data that was received.
     *
     * @return a reference to the buffer with the data that was received.
     */
    const vector<uint8_t>& buffer() const { return (_cache_data); }

    /**
     * Receive data from the netlink socket.
     *
     * Note that this method is called asynchronously when the netlink socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the netlink socket facility itself.
     *
     * @param buffer the buffer with the received data.
     */
    virtual void netlink_socket_data(const vector<uint8_t>& buffer);

private:
    NetlinkSocket&  _ns;

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of netlink socket data to
					// cache so reading via netlink
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached netlink socket data.
};



#endif // HAVE_NETLINK_SOCKETS
#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_HH__
