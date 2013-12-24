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

#include <xorp_config.h>
#ifdef HAVE_ROUTING_SOCKETS


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"



#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <errno.h>

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

#include "libcomm/comm_api.h"
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
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing socket: %s", error_msg.c_str());
    }

    XLOG_ASSERT(_ol.empty());
}


int
RoutingSocket::start(int af, string& error_msg)
{
    if (_fd >= 0)
	return (XORP_OK);

    //
    // Open the socket
    //
    _fd = socket(AF_ROUTE, SOCK_RAW, af);
    if (_fd < 0) {
	error_msg = c_format("Could not open routing socket: %s",
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
    // Add the socket to the event loop
    //
    if (_eventloop.add_ioevent_cb(_fd, IOT_READ,
				callback(this, &RoutingSocket::io_event))
	== false) {
	error_msg = c_format("Failed to add routing socket to EventLoop");
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RoutingSocket::stop(string& error_msg)
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
RoutingSocket::write(const void* data, size_t nbytes)
{
    _seqno++;
    return ::write(_fd, data, nbytes);
}

int
RoutingSocket::force_read(string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(ROUTING_SOCKET_BYTES);
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
	    buffer.resize(buffer.size() + ROUTING_SOCKET_BYTES);
	} while (true);
	
	got = read(_fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Routing socket read error: %s",
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
	    error_msg = c_format("Routing socket read failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(u_short) + 2 * sizeof(u_char)));
	    return (XORP_ERROR);
	}

	//
	// XXX: the routing sockets don't use multipart messages, hence
	// this should be the end of the message.
	//

	//
	// Received message (probably) OK
	//
	const struct if_msghdr* mh = (const struct if_msghdr*)(&(message[0]));
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
	(*i)->routing_socket_data(message);
    }

    return (XORP_OK);
}

void
RoutingSocket::io_event(XorpFd fd, IoEventType type)
{
    string error_msg;

    XLOG_ASSERT(fd == _fd);
    XLOG_ASSERT(type == IOT_READ);
    if (force_read(error_msg) != XORP_OK) {
	XLOG_ERROR("Error force_read() from routing socket: %s",
		   error_msg.c_str());
    }
}


//
// Observe routing sockets activity
//

class RoutingSocketPlumber {
    typedef RoutingSocket::ObserverList ObserverList;
    public:
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
	debug_msg("Unplumbing RoutingSocketObserver %p from "
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
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
RoutingSocketReader::receive_data(RoutingSocket& rs, uint32_t seqno,
				  string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (rs.force_read(error_msg) != XORP_OK)
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
 * @param buffer the buffer with the received data.
 */
void
RoutingSocketReader::routing_socket_data(vector<uint8_t>& buffer)
{
    size_t d = 0, off = 0;
    pid_t my_pid = _rs.pid();

    //
    // Copy data that has been requested to be cached by setting _cache_seqno
    //
    _cache_data.resize(buffer.size());
    while (d < buffer.size()) {
	const struct rt_msghdr* rtm;
	rtm = (const struct rt_msghdr*)(&(buffer[d]));
	if ((rtm->rtm_pid == my_pid)
	    && (rtm->rtm_seq == (signed)_cache_seqno)) {
	    XLOG_ASSERT(buffer.size() - d >= rtm->rtm_msglen);
	    memcpy(&_cache_data[off], rtm, rtm->rtm_msglen);
	    off += rtm->rtm_msglen;
	    _cache_valid = true;
	}
	d += rtm->rtm_msglen;
    }

    // XXX: shrink _cache_data to contain only the data copied to it
    _cache_data.resize(off);
}

#endif // HAVE_ROUTING_SOCKETS
