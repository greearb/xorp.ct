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

#ident "$XORP: xorp/fea/rtsock.cc,v 1.2 2003/03/10 23:20:16 hodson Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <errno.h>

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "netlink_socket.hh"


uint16_t NetlinkSocket::_instance_cnt = 0;
pid_t NetlinkSocket::_pid = getpid();

//
// Netlink Sockets communication with the kernel
//

#ifndef HAVE_NETLINK_SOCKETS

NetlinkSocket::NetlinkSocket(EventLoop& e)
    : _e(e),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

NetlinkSocket::~NetlinkSocket()
{
    
}

int
NetlinkSocket::start(int )
{
    return (XORP_ERROR);
}

int
NetlinkSocket::stop()
{
    return (XORP_ERROR);
}

#else // HAVE_NETLINK_SOCKETS

NetlinkSocket::NetlinkSocket(EventLoop& e)
    : _e(e),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

NetlinkSocket::~NetlinkSocket()
{
    shutdown();
    assert(_ol.empty());
}

int
NetlinkSocket::start(int af)
{
    int			socket_protocol = -1;
    struct sockaddr_nl	snl;
    socklen_t		snl_len;

    if (_fd >= 0) {
	XLOG_ERROR("Cound not start netlink socket operation: "
		   "already started");
	return (XORP_ERROR);
    }
    
    //
    // Select the protocol
    //
    switch (af) {
    case AF_INET:
	socket_protocol = NETLINK_ROUTE;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	socket_protocol = NETLINK_ROUTE6;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    
    //
    // Open the socket
    //
    _fd = socket(AF_NETLINK, SOCK_RAW, socket_protocol);
    if (_fd < 0) {
	XLOG_ERROR("Could not open netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    // TODO: do we want to make the socket non-blocking?
    
    //
    // Bind the socket
    //
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;
    if (bind(_fd, (struct sockaddr *)&snl, sizeof(snl)) < 0) {
	close(_fd);
	_fd = -1;
	XLOG_ERROR("bind(AF_NETLINK) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // Double-check the result socket is AF_NETLINK
    //
    snl_len = sizeof(snl);
    if (getsockname(_fd, (struct sockaddr *)&snl, &snl_len) < 0) {
	close(_fd);
	_fd = -1;
	XLOG_ERROR("getsockname(AF_NETLINK) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    if (snl_len != sizeof(snl)) {
	close(_fd);
	_fd = -1;
	XLOG_ERROR("Wrong address length of AF_NETLINK socket: "
		   "%d instead of %d",
		   snl_len, sizeof(snl));
	return (XORP_ERROR);
    }
    if (snl.nl_family != AF_NETLINK) {
	close(_fd);
	_fd = -1;
	XLOG_ERROR("Wrong address family of AF_NETLINK socket: "
		   "%d instead of %d",
		   snl.nl_family, AF_NETLINK);
	return (XORP_ERROR);
    }
#if 0				// XXX
    // XXX: 'nl_pid' is supposed to be defined as 'pid_t'
    if ( (pid_t)snl.nl_pid != pid()) {
	close(_fd);
	_fd = -1;
	XLOG_ERROR("Wrong nl_pid of AF_NETLINK socket: %d instead of %d",
		   snl.nl_pid, pid());
	return (XORP_ERROR);
    }
#endif // 0
    
    //
    // Add the socket to the event loop
    //
    if (_e.add_selector(_fd, SEL_RD,
			callback(this, &NetlinkSocket::select_hook))
	== false) {
	XLOG_ERROR("Failed to add netlink socket to EventLoop");
	close(_fd);
	_fd = -1;
    }
    
    return (XORP_OK);
}

int
NetlinkSocket::stop()
{
    shutdown();
    
    return (XORP_OK);
}

void
NetlinkSocket::shutdown()
{
    if (_fd >= 0) {
	_e.remove_selector(_fd, SEL_ALL);
	close(_fd);
	_fd = -1;
    }
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

void
NetlinkSocket::force_read()
{
    ssize_t got = 0;
    
    for ( ; ; ) {
	got = read(_fd, _buffer, NLSOCK_BYTES);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("Netlink socket read error: %s", strerror(errno));
	    shutdown();
	    return;
	}
	if (got < (ssize_t)sizeof(struct nlmsghdr)) {
	    XLOG_ERROR("Netlink socket read failed: message truncated : "
		       "received %d bytes instead of (at least) %u bytes",
		       got, (uint32_t)sizeof(struct nlmsghdr));
	    shutdown();
	    return;
	}
	
	//
	// Received message (probably) OK
	//
	const struct nlmsghdr* mh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	XLOG_ASSERT(mh->nlmsg_len <= NLSOCK_BYTES);
	// XLOG_ASSERT((ssize_t)mh->nlmsg_len == got);
	break;
    }
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(_buffer, got);
    }
}

void
NetlinkSocket::force_recvfrom(int flags, struct sockaddr* from,
			      socklen_t* fromlen)
{
    ssize_t got = 0;
    
    for ( ; ; ) {
	got = recvfrom(_fd, _buffer, NLSOCK_BYTES, flags, from, fromlen);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("Netlink socket recvfrom error: %s", strerror(errno));
	    shutdown();
	    return;
	}
	if (got < (ssize_t)sizeof(struct nlmsghdr)) {
	    XLOG_ERROR("Netlink socket recvfrom failed: message truncated : "
		       "received %d bytes instead of (at least) %u bytes",
		       got, (uint32_t)sizeof(struct nlmsghdr));
	    shutdown();
	    return;
	}
	
	//
	// Received message (probably) OK
	//
	const struct nlmsghdr* mh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	XLOG_ASSERT(mh->nlmsg_len <= NLSOCK_BYTES);
	// XLOG_ASSERT((ssize_t)mh->nlmsg_len == got);
	break;
    }
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(_buffer, got);
    }
}

void
NetlinkSocket::force_recvmsg(int flags)
{
    ssize_t		got = 0;
    struct iovec	iov;
    struct msghdr	msg;
    struct sockaddr_nl	snl;
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    
    // Init the recvmsg() arguments
    iov.iov_base = _buffer;
    iov.iov_len = sizeof(_buffer);
    msg.msg_name = &snl;
    msg.msg_namelen = sizeof(snl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    
    for ( ; ; ) {
	got = recvmsg(_fd, &msg, flags);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("Netlink socket recvmsg error: %s", strerror(errno));
	    shutdown();
	    return;
	}
	if (msg.msg_namelen != sizeof(snl)) {
	    XLOG_ERROR("Netlink socket recvmsg error: "
		       "sender address length %d instead of %u ",
		       msg.msg_namelen, sizeof(snl));
	    shutdown();
	    return;
	}
	if (got < (ssize_t)sizeof(struct nlmsghdr)) {
	    XLOG_ERROR("Netlink socket recvfrom failed: message truncated : "
		       "received %d bytes instead of (at least) %u bytes",
		       got, (uint32_t)sizeof(struct nlmsghdr));
	    shutdown();
	    return;
	}
	
	//
	// Received message (probably) OK
	//
	const struct nlmsghdr* mh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	XLOG_ASSERT(mh->nlmsg_len <= NLSOCK_BYTES);
	// XLOG_ASSERT((ssize_t)mh->nlmsg_len == got);
	break;
    }
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(_buffer, got);
    }
}

void
NetlinkSocket::select_hook(int fd, SelectorMask m)
{
    assert(fd == _fd);
    assert(m == SEL_RD);
    force_read();
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
	assert(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(NetlinkSocket& r, NetlinkSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing NetlinkSocketObserver%p from "
		  "NetlinkSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	assert(i != ol.end());
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

