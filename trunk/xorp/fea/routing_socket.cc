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

#ident "$XORP: xorp/fea/routing_socket.cc,v 1.15 2004/11/09 20:40:08 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <errno.h>

#include "routing_socket.hh"


uint16_t RoutingSocket::_instance_cnt = 0;
pid_t RoutingSocket::_pid = getpid();

//
// Routing Sockets communication with the kernel
//

RoutingSocket::RoutingSocket(EventLoop& eventloop)
    : _eventloop(eventloop),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

RoutingSocket::~RoutingSocket()
{
    stop();
    XLOG_ASSERT(_ol.empty());
}

#ifndef HAVE_ROUTING_SOCKETS

int
RoutingSocket::start(int )
{
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
RoutingSocket::stop()
{
    return (XORP_ERROR);
}

int
RoutingSocket::force_read(string& errmsg)
{
    XLOG_UNREACHABLE();

    errmsg = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_ROUTING_SOCKETS

int
RoutingSocket::start(int af)
{
    if (_fd >= 0)
	return (XORP_OK);

    //
    // Open the socket
    //
    _fd = socket(AF_ROUTE, SOCK_RAW, af);
    if (_fd < 0) {
	XLOG_ERROR("Could not open routing socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    //
    // Increase the receiving buffer size of the socket to avoid
    // loss of data from the kernel.
    //
    comm_sock_set_rcvbuf(_fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN);

    // TODO: do we want to make the socket non-blocking?
    
    //
    // Add the socket to the event loop
    //
    if (_eventloop.add_selector(_fd, SEL_RD,
				callback(this, &RoutingSocket::select_hook))
	== false) {
	XLOG_ERROR("Failed to add routing socket to EventLoop");
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RoutingSocket::stop()
{
    shutdown();
    
    return (XORP_OK);
}

void
RoutingSocket::shutdown()
{
    if (_fd >= 0) {
	_eventloop.remove_selector(_fd, SEL_ALL);
	close(_fd);
	_fd = -1;
    }
}

ssize_t
RoutingSocket::write(const void* data, size_t nbytes)
{
    _seqno++;
    return ::write(_fd, data, nbytes);
}

int
RoutingSocket::force_read(string& errmsg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(RTSOCK_BYTES);
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
	    buffer.resize(buffer.size() + RTSOCK_BYTES);
	} while (true);
	
	got = read(_fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    errmsg = c_format("Routing socket read error: %s",
			      strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;

	//
	// XXX: all messages received on routing sockets must start
	// with the the following three fields:
	//    {
	//        u_short foo_msglen;
	//        u_char  foo_version;
	//        u_char  foo_type;
	//        ...
	//
	// Hence, the minimum length of a received message is:
	//  sizeof(u_short) + 2 * sizeof(u_char)
	//
	if ((off - last_mh_off)
	    < (ssize_t)(sizeof(u_short) + 2 * sizeof(u_char))) {
	    errmsg = c_format("Routing socket read failed: message truncated: "
			      "received %d bytes instead of (at least) %u "
			      "bytes",
			      got,
			      (uint32_t)(sizeof(u_short) + 2 * sizeof(u_char)));
	    return (XORP_ERROR);
	}

	//
	// XXX: the routing sockets don't use multipart messages, hence
	// this should be the end of the message.
	//

	//
	// Received message (probably) OK
	//
	const struct if_msghdr* mh = reinterpret_cast<const struct if_msghdr*>(&message[0]);
	XLOG_ASSERT(mh->ifm_msglen == message.size());
	XLOG_ASSERT(mh->ifm_msglen == got);
	last_mh_off = off;
	break;
    }
    XLOG_ASSERT(last_mh_off == message.size());

    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->rtsock_data(&message[0], message.size());
    }

    return (XORP_OK);
}

void
RoutingSocket::select_hook(int fd, SelectorMask m)
{
    string errmsg;

    XLOG_ASSERT(fd == _fd);
    XLOG_ASSERT(m == SEL_RD);
    if (force_read(errmsg) != XORP_OK) {
	XLOG_ERROR("Error force_read() from routing socket: %s",
		   errmsg.c_str());
    }
}

#endif // HAVE_ROUTING_SOCKETS

//
// Observe routing sockets activity
//

struct RoutingSocketPlumber {
    typedef RoutingSocket::ObserverList ObserverList;

    static void
    plumb(RoutingSocket& r, RoutingSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	debug_msg("Plumbing RoutingSocketObserver %p to RoutingSocket%p\n",
		  o, &r);
	XLOG_ASSERT(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(RoutingSocket& r, RoutingSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing RoutingSocketObserver%p from "
		  "RoutingSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	XLOG_ASSERT(i != ol.end());
	ol.erase(i);
    }
};

RoutingSocketObserver::RoutingSocketObserver(RoutingSocket& rs)
    : _rs(rs)
{
    RoutingSocketPlumber::plumb(rs, this);
}

RoutingSocketObserver::~RoutingSocketObserver()
{
    RoutingSocketPlumber::unplumb(_rs, this);
}

RoutingSocket&
RoutingSocketObserver::routing_socket()
{
    return _rs;
}

RoutingSocketReader::RoutingSocketReader(RoutingSocket& rs)
    : RoutingSocketObserver(rs),
      _rs(rs),
      _cache_valid(false),
      _cache_seqno(0)
{

}

RoutingSocketReader::~RoutingSocketReader()
{

}

/**
 * Force the reader to receive data from the specified routing socket.
 *
 * @param rs the routing socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param errmsg the error message (if an error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
RoutingSocketReader::receive_data(RoutingSocket& rs, uint32_t seqno,
				  string& errmsg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (rs.force_read(errmsg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Receive data from the routing socket.
 *
 * Note that this method is called asynchronously when the routing socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the routing socket facility itself.
 *
 * @param data the buffer with the received data.
 * @param nbytes the number of bytes in the @param data buffer.
 */
void
RoutingSocketReader::rtsock_data(const uint8_t* data, size_t nbytes)
{
#ifndef HAVE_ROUTING_SOCKETS
    UNUSED(data);
    UNUSED(nbytes);

    XLOG_UNREACHABLE();

#else // HAVE_ROUTING_SOCKETS
    size_t d = 0, off = 0;
    pid_t my_pid = _rs.pid();

    //
    // Copy data that has been requested to be cached by setting _cache_seqno
    //
    _cache_data.resize(nbytes);
    while (d < nbytes) {
	const struct rt_msghdr* rh = reinterpret_cast<const struct rt_msghdr*>(data + d);
	if ((rh->rtm_pid == my_pid)
	    && (rh->rtm_seq == (signed)_cache_seqno)) {
	    XLOG_ASSERT(nbytes - d >= rh->rtm_msglen);
	    memcpy(&_cache_data[off], rh, rh->rtm_msglen);
	    off += rh->rtm_msglen;
	    _cache_valid = true;
	}
	d += rh->rtm_msglen;
    }
#endif // HAVE_ROUTING_SOCKETS
}
