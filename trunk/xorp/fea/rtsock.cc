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

#ident "$XORP: xorp/fea/rtsock.cc,v 1.11 2002/12/09 18:28:58 hodson Exp $"

#include <algorithm>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <errno.h>
#include <sys/filio.h>

#include "config.h"
#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "rtsock.hh"
#include "fti.hh"

uint16_t RoutingSocket::_instance_cnt = 0;
pid_t RoutingSocket::_pid = getpid();

/* ------------------------------------------------------------------------- */
/* Routing Socket related */

RoutingSocket::RoutingSocket(EventLoop& e, int af) throw(FtiError) :
    _e(e), _seqno(0),
    _instance_no(_instance_cnt++)
{
    _fd = socket(PF_ROUTE, SOCK_RAW, af);
    if (_fd < 0) {
	xorp_throw(FtiError,
		   c_format("Could not open routing socket: %s",
			    strerror(errno)));
    }

    if (e.add_selector(_fd, SEL_RD,
		       callback(this, &RoutingSocket::select_hook)) == false) {
	XLOG_ERROR("Failed to add routing socket to EventLoop");
	close(_fd);
	_fd = -1;
    }
}

RoutingSocket::~RoutingSocket()
{
    shutdown();
    assert(_ol.empty());
}

void
RoutingSocket::shutdown()
{
    if (_fd != -1) {
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
    ssize_t got = read(_fd, _buffer, RTSOCK_BYTES);
    if (got < 0) {
	    XLOG_ERROR("Routing socket read error.");
	    shutdown();
	    return;
    }

    const if_msghdr* mh = reinterpret_cast<if_msghdr*>(_buffer);
    assert(mh->ifm_msglen <= RTSOCK_BYTES);
    assert(mh->ifm_msglen == got);

    // Notify observers
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

/* ------------------------------------------------------------------------- */
/* Plumbing */

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

/* ------------------------------------------------------------------------- */
/* Routing Socket Observer */

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
