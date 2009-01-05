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

#ident "$XORP: xorp/fea/data_plane/control_socket/netlink_socket.cc,v 1.6 2008/10/02 21:56:53 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <algorithm>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include <errno.h>

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "libcomm/comm_api.h"

#include "netlink_socket.hh"
#include "netlink_socket_utilities.hh"


uint16_t NetlinkSocket::_instance_cnt = 0;

//
// Netlink Sockets (see netlink(7)) communication with the kernel
//

NetlinkSocket::NetlinkSocket(EventLoop& eventloop)
    : _eventloop(eventloop),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++),
      _nl_groups(0),		// XXX: no netlink multicast groups
      _is_multipart_message_read(false)
{
    
}

NetlinkSocket::~NetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink socket: %s", error_msg.c_str());
    }

    XLOG_ASSERT(_ol.empty());
}

#ifndef HAVE_NETLINK_SOCKETS

int
NetlinkSocket::start(string& error_msg)
{
    error_msg = c_format("The system does not support netlink sockets");
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
NetlinkSocket::stop(string& error_msg)
{
    UNUSED(error_msg);

    //
    // XXX: Even if the system doesn not support netlink sockets, we
    // still allow to call the no-op stop() method and return success.
    // This is needed to cover the case when a NetlinkSocket is allocated
    // but not used.
    //
    return (XORP_OK);
}

int
NetlinkSocket::force_read(string& error_msg)
{
    XLOG_UNREACHABLE();

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
NetlinkSocket::force_recvfrom(int flags, struct sockaddr* from,
			      socklen_t* fromlen, string& error_msg)
{
    UNUSED(flags);
    UNUSED(from);
    UNUSED(fromlen);

    XLOG_UNREACHABLE();

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
NetlinkSocket::force_recvmsg(int flags, bool only_kernel_messages,
			     string& error_msg)
{
    UNUSED(flags);
    UNUSED(only_kernel_messages);

    XLOG_UNREACHABLE();

    error_msg = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_NETLINK_SOCKETS

int
NetlinkSocket::start(string& error_msg)
{
    struct sockaddr_nl	snl;
    socklen_t		snl_len;

    if (_fd >= 0)
	return (XORP_OK);

    //
    // Open the socket
    //
    // XXX: Older versions of the  netlink(7) manual page are incorrect
    // that for IPv6 we need NETLINK_ROUTE6.
    // The truth is that for both IPv4 and IPv6 it has to be NETLINK_ROUTE.
    _fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (_fd < 0) {
	error_msg = c_format("Could not open netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    //
    // Increase the receiving buffer size of the socket to avoid
    // loss of data from the kernel.
    //
    comm_sock_set_rcvbuf(_fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN);
    
    // TODO: do we want to make the socket non-blocking?
    
    //
    // Bind the socket
    //
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// Let the kernel assign the pid to the socket
    snl.nl_groups = _nl_groups;
    if (bind(_fd, reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl)) < 0) {
	error_msg = c_format("bind(AF_NETLINK) failed: %s", strerror(errno));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    //
    // Double-check the result socket is AF_NETLINK
    //
    snl_len = sizeof(snl);
    if (getsockname(_fd, reinterpret_cast<struct sockaddr*>(&snl), &snl_len) < 0) {
	error_msg = c_format("getsockname(AF_NETLINK) failed: %s",
			     strerror(errno));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    if (snl_len != sizeof(snl)) {
	error_msg = c_format("Wrong address length of AF_NETLINK socket: "
			     "%u instead of %u",
			     XORP_UINT_CAST(snl_len),
			     XORP_UINT_CAST(sizeof(snl)));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    if (snl.nl_family != AF_NETLINK) {
	error_msg = c_format("Wrong address family of AF_NETLINK socket: "
			     "%d instead of %d",
			     snl.nl_family, AF_NETLINK);
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }

    //
    // Store the pid of the socket for checking the unicast destination of
    // the netlink(7) messages.
    //
    _nl_pid = snl.nl_pid;

    //
    // Add the socket to the event loop
    //
    if (_eventloop.add_ioevent_cb(_fd, IOT_READ,
				callback(this, &NetlinkSocket::io_event))
	== false) {
	error_msg = c_format("Failed to add netlink socket to EventLoop");
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
NetlinkSocket::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (_fd >= 0) {
	_eventloop.remove_ioevent_cb(_fd);
	close(_fd);
	_fd = -1;
    }
    
    return (XORP_OK);
}

ssize_t
NetlinkSocket::write(const void* data, size_t nbytes)
{
    _seqno++;
    return ::write(_fd, data, nbytes);
}

ssize_t
NetlinkSocket::sendto(const void* data, size_t nbytes, int flags,
		      const struct sockaddr* to, socklen_t tolen)
{
    _seqno++;
    return ::sendto(_fd, data, nbytes, flags, to, tolen);
}

int
NetlinkSocket::force_read(string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NETLINK_SOCKET_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NETLINK_SOCKET_BYTES);
	} while (true);
	
	got = read(_fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket read error: %s",
			      strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket read failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	AlignData<struct nlmsghdr> align_data(message);
	const struct nlmsghdr* mh;
	for (mh = align_data.payload_by_offset(last_mh_off);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->netlink_socket_data(message);
    }

    return (XORP_OK);
}

int
NetlinkSocket::force_recvfrom(int flags, struct sockaddr* from,
			      socklen_t* fromlen, string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NETLINK_SOCKET_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NETLINK_SOCKET_BYTES);
	} while (true);
	
	got = recvfrom(_fd, &buffer[0], buffer.size(), flags, from, fromlen);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket recvfrom error: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket recvfrom failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	AlignData<struct nlmsghdr> align_data(message);
	const struct nlmsghdr* mh;
	for (mh = align_data.payload_by_offset(last_mh_off);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->netlink_socket_data(message);
    }

    return (XORP_OK);
}

int
NetlinkSocket::force_recvmsg(int flags, bool only_kernel_messages,
			     string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NETLINK_SOCKET_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    struct iovec	iov;
    struct msghdr	msg;
    struct sockaddr_nl	snl;
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    
    // Init the recvmsg() arguments
    iov.iov_base = &buffer[0];
    iov.iov_len = buffer.size();
    msg.msg_name = &snl;
    msg.msg_namelen = sizeof(snl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NETLINK_SOCKET_BYTES);
	} while (true);
	
	// Re-init the iov argument
	iov.iov_base = &buffer[0];
	iov.iov_len = buffer.size();
	
	got = recvmsg(_fd, &msg, flags);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket recvmsg error: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}

	//
	// If necessary, ignore messages that were not originated by the kernel
	//
	if (only_kernel_messages && (snl.nl_pid != 0))
	    continue;

	if (msg.msg_namelen != sizeof(snl)) {
	    error_msg = c_format("Netlink socket recvmsg error: "
				 "sender address length %d instead of %u",
				 XORP_INT_CAST(msg.msg_namelen),
				 XORP_UINT_CAST(sizeof(snl)));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket recvmsg failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	AlignData<struct nlmsghdr> align_data(message);
	const struct nlmsghdr* mh;
	for (mh = align_data.payload_by_offset(last_mh_off);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->netlink_socket_data(message);
    }

    return (XORP_OK);
}

void
NetlinkSocket::io_event(XorpFd fd, IoEventType type)
{
    string error_msg;

    XLOG_ASSERT(fd == _fd);
    XLOG_ASSERT(type == IOT_READ);
    if (force_recvmsg(0, true, error_msg) != XORP_OK) {
	XLOG_ERROR("Error force_recvmsg() from netlink socket: %s",
		   error_msg.c_str());
    }
}

#endif // HAVE_NETLINK_SOCKETS

//
// Observe netlink sockets activity
//

struct NetlinkSocketPlumber {
    typedef NetlinkSocket::ObserverList ObserverList;

    static void
    plumb(NetlinkSocket& r, NetlinkSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	debug_msg("Plumbing NetlinkSocketObserver %p to NetlinkSocket%p\n",
		  o, &r);
	XLOG_ASSERT(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(NetlinkSocket& r, NetlinkSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing NetlinkSocketObserver %p from "
		  "NetlinkSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	XLOG_ASSERT(i != ol.end());
	ol.erase(i);
    }
};

NetlinkSocketObserver::NetlinkSocketObserver(NetlinkSocket& ns)
    : _ns(ns)
{
    NetlinkSocketPlumber::plumb(ns, this);
}

NetlinkSocketObserver::~NetlinkSocketObserver()
{
    NetlinkSocketPlumber::unplumb(_ns, this);
}

NetlinkSocket&
NetlinkSocketObserver::netlink_socket()
{
    return _ns;
}

NetlinkSocketReader::NetlinkSocketReader(NetlinkSocket& ns)
    : NetlinkSocketObserver(ns),
      _ns(ns),
      _cache_valid(false),
      _cache_seqno(0)
{

}

NetlinkSocketReader::~NetlinkSocketReader()
{

}

/**
 * Force the reader to receive data from the specified netlink socket.
 *
 * @param ns the netlink socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NetlinkSocketReader::receive_data(NetlinkSocket& ns, uint32_t seqno,
				  string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (ns.force_recvmsg(0, true, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Receive data from the netlink socket.
 *
 * Note that this method is called asynchronously when the netlink socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the netlink socket facility itself.
 *
 * @param buffer the buffer with the received data.
 */
void
NetlinkSocketReader::netlink_socket_data(const vector<uint8_t>& buffer)
{
#ifndef HAVE_NETLINK_SOCKETS
    UNUSED(buffer);

    XLOG_UNREACHABLE();

#else // HAVE_NETLINK_SOCKETS

    size_t d = 0, off = 0;
    AlignData<struct nlmsghdr> align_data(buffer);

    //
    // Copy data that has been requested to be cached by setting _cache_seqno
    //
    _cache_data.resize(buffer.size());
    while (d < buffer.size()) {
	const struct nlmsghdr* nlh;
	nlh = align_data.payload_by_offset(d);
	if ((nlh->nlmsg_seq == _cache_seqno)
	    && (nlh->nlmsg_pid == _ns.nl_pid())) {
	    XLOG_ASSERT(buffer.size() - d >= nlh->nlmsg_len);
	    memcpy(&_cache_data[off], nlh, nlh->nlmsg_len);
	    off += nlh->nlmsg_len;
	    _cache_valid = true;
	}
	d += nlh->nlmsg_len;
    }

    // XXX: shrink _cache_data to contain only the data copied to it
    _cache_data.resize(off);
#endif // HAVE_NETLINK_SOCKETS
}
