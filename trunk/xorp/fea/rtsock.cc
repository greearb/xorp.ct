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

#error "OBSOLETE FILE"

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

#include "rtsock.hh"


uint16_t RoutingSocket::_instance_cnt = 0;
pid_t RoutingSocket::_pid = getpid();

//
// Routing Sockets communication with the kernel
//

#ifndef HAVE_ROUTING_SOCKETS

RoutingSocket::RoutingSocket(EventLoop& e)
    : _e(e),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

RoutingSocket::~RoutingSocket()
{
    
}

int
RoutingSocket::start(int )
{
    return (XORP_ERROR);
}

int
RoutingSocket::stop()
{
    return (XORP_ERROR);
}

#else // HAVE_ROUTING_SOCKETS

RoutingSocket::RoutingSocket(EventLoop& e)
    : _e(e),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

RoutingSocket::~RoutingSocket()
{
    shutdown();
    assert(_ol.empty());
}

int
RoutingSocket::start(int af)
{
    if (_fd >= 0) {
	XLOG_ERROR("Cound not start routing socket operation: "
		   "already started");
	return (XORP_ERROR);
    }

    //
    // Open the socket
    //
    _fd = socket(AF_ROUTE, SOCK_RAW, af);
    if (_fd < 0) {
	XLOG_ERROR("Could not open routing socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // Add the socket to the event loop
    //
    if (_e.add_selector(_fd, SEL_RD,
			callback(this, &RoutingSocket::select_hook))
	== false) {
	XLOG_ERROR("Failed to add routing socket to EventLoop");
	close(_fd);
	_fd = -1;
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
	_e.remove_selector(_fd, SEL_ALL);
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

void
RoutingSocket::force_read()
{
    ssize_t got = 0;
    
    for ( ; ; ) {
	got = read(_fd, _buffer, RTSOCK_BYTES);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("Routing socket read error: %s", strerror(errno));
	    shutdown();
	    return;
	}
	if (got < (ssize_t)sizeof(struct if_msghdr)) {
	    XLOG_ERROR("Routing socket read failed: message truncated : "
		       "received %d bytes instead of (at least) %u bytes",
		       got, (uint32_t)sizeof(struct if_msghdr));
	    shutdown();
	    return;
	}
	
	//
	// Received message (probably) OK
	//
	const if_msghdr* mh = reinterpret_cast<if_msghdr*>(_buffer);
	XLOG_ASSERT(mh->ifm_msglen <= RTSOCK_BYTES);
	XLOG_ASSERT(mh->ifm_msglen == got);
	break;
    }
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->rtsock_data(_buffer, got);
    }
}

void
RoutingSocket::select_hook(int fd, SelectorMask m)
{
    assert(fd == _fd);
    assert(m == SEL_RD);
    force_read();
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
	assert(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(RoutingSocket& r, RoutingSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing RoutingSocketObserver%p from "
		  "RoutingSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	assert(i != ol.end());
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

