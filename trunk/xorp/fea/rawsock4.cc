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

#ident "$XORP: xorp/fea/rawsock4.cc,v 1.6 2004/06/10 22:40:56 hodson Exp $"

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <unistd.h>
#include <fcntl.h>

#include "config.h"
#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "rawsock4.hh"

/* ------------------------------------------------------------------------- */
/* RawSocket4 methods */

RawSocket4::RawSocket4(uint32_t protocol) throw (RawSocket4Exception)
{
    _fd = socket(AF_INET, SOCK_RAW, protocol);
    if (_fd < 0)
	xorp_throw(RawSocket4Exception, "socket", errno);

    const int on = 1;
    if (setsockopt(_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	xorp_throw(RawSocket4Exception, "setsockopt (IP_HDRINCL)", errno);
    /*
      if (setsockopt(_fd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) < 0)
      xorp_throw(RawSocket4Exception, "setsockopt (SO_DONTROUTE)", errno);
    */
}

RawSocket4::~RawSocket4()
{
    	if (_fd > 0)
	    close(_fd);
}

ssize_t
RawSocket4::write(const uint8_t* buf, size_t nbytes) const
{
    /* length field is in host order */
    static_assert(sizeof(struct ip) == 20);

    if (nbytes < sizeof(struct ip)) {
	XLOG_ERROR("attempting to write a raw ip packet of %u (<20) bytes.",
		   (uint32_t)nbytes);
	return -1;
    }

    const struct ip* hdr = reinterpret_cast<const struct ip*>(buf);
    if (hdr->ip_hl < 5) {
	XLOG_ERROR("bad header length %d", hdr->ip_hl);
	return -1;
    } else if (hdr->ip_v != 4) {
	XLOG_ERROR("bad ip version %d", hdr->ip_v);
	return -1;
    }

    struct iovec iov[2];

#ifdef IPV4_RAW_OUTPUT_IS_RAW
    iov[0].iov_base = reinterpret_cast<caddr_t>(const_cast<uint8_t*>(buf));
    iov[0].iov_len = nbytes;
    iov[1].iov_base = 0;
    iov[1].iov_len = 0;
#else
    /*
     * For not quite raw output the ip packet length needs nobbling from
     * network to host order.
     */
    struct ip newhdr;
    memcpy(&newhdr, buf, sizeof(newhdr));
    newhdr.ip_len = ntohs(newhdr.ip_len);

    iov[0].iov_base = reinterpret_cast<caddr_t>(&newhdr);
    iov[0].iov_len = sizeof(newhdr);
    iov[1].iov_base = reinterpret_cast<caddr_t>(const_cast<uint8_t*>(buf))
	+ sizeof(newhdr);
    iov[1].iov_len  = nbytes - sizeof(newhdr);
#endif /* IPV4_RAW_OUTPUT_IS_RAW */

    int iovcnt = (iov[1].iov_len == 0) ? 1 : 2;

    struct sockaddr_in who;
    memset(&who, 0, sizeof(who));
    who.sin_family = AF_INET;
    who.sin_addr = hdr->ip_dst;
#ifdef HAVE_SIN_LEN
    who.sin_len = sizeof(sockaddr_in);
#endif /* HAVE_SIN_LEN */

    struct msghdr mh;
    mh.msg_name = (caddr_t)&who;
    mh.msg_namelen = sizeof(who);
    mh.msg_iov = iov;
    mh.msg_iovlen = iovcnt;
    mh.msg_control = 0;
    mh.msg_controllen = 0;
    mh.msg_flags = 0;
    return ::sendmsg(_fd, &mh, 0);
}

/* ------------------------------------------------------------------------- */
/* IoRawSocket4 methods */

IoRawSocket4::IoRawSocket4(EventLoop&	e,
			   uint32_t	protocol,
			   bool		autohook)
    throw (RawSocket4Exception)
    : RawSocket4(protocol), _e(e), _autohook(autohook)
{
    int fl = fcntl(_fd, F_GETFL);
    if (fcntl(_fd, F_SETFL, fl | O_NONBLOCK) < 0)
	xorp_throw(RawSocket4Exception, "fcntl (O_NON_BLOCK)", errno);

    _recvbuf.reserve(RECVBUF_BYTES);
    if (_recvbuf.capacity() != RECVBUF_BYTES)
	xorp_throw(RawSocket4Exception, "receive buffer reserve() failed", 0);

    ssize_t sz = RECVBUF_BYTES;
    if (setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz)) < 0)
	xorp_throw(RawSocket4Exception, "setsockopt (SO_RCVBUF)", errno);

    if (_autohook)
	eventloop_hook();
}

IoRawSocket4::~IoRawSocket4()
{
    if (_autohook)
	eventloop_unhook();
}

void
IoRawSocket4::recv(int /* fd */, SelectorMask /* m */)
{
    struct sockaddr from;
    socklen_t from_len = sizeof(from);
    ssize_t n = recvfrom(_fd, &_recvbuf[0], RECVBUF_BYTES, 0,
			 &from, &from_len);
    debug_msg("Read fd %d, %d bytes, from_len %d\n", _fd, n, from_len);
    if (n <= 0) {
	return;
    }
    _recvbuf.resize(n);

#ifndef IPV4_RAW_INPUT_IS_RAW
    struct ip* hdr = reinterpret_cast<struct ip*>(&_recvbuf[0]);
    hdr->ip_len = htons(hdr->ip_len);
#endif

    process_recv_data(_recvbuf);
}

bool
IoRawSocket4::eventloop_hook()
{
    debug_msg("hooking\n");
    return _e.add_selector(_fd, SEL_RD,
			   callback(this, &IoRawSocket4::recv));
}

void
IoRawSocket4::eventloop_unhook()
{
    debug_msg("unhooking\n");
    _e.remove_selector(_fd);
}

/* ------------------------------------------------------------------------- */
/* FilterRawSocket4 methods */

FilterRawSocket4::FilterRawSocket4(EventLoop& e, int protocol)
    throw (RawSocket4Exception)
    : IoRawSocket4(e, protocol, false)
{
}

FilterRawSocket4::~FilterRawSocket4()
{
    if (_filters.empty() == false) {
	eventloop_unhook();

	do {
	    InputFilter* i = _filters.front();
	    _filters.erase(_filters.begin());
	    i->bye();
	} while (_filters.empty() == false);
    }
}

bool
FilterRawSocket4::add_filter(InputFilter* filter)
{
    if (filter == 0) {
	XLOG_FATAL("Adding a null filter");
	return false;
    }

    if (find(_filters.begin(), _filters.end(), filter) != _filters.end()) {
	debug_msg("filter already exists\n");
	return false;
    }

    _filters.push_back(filter);
    if (_filters.front() == filter) {
	eventloop_hook();
    }
    return true;
}

bool
FilterRawSocket4::remove_filter(InputFilter* filter)
{
    list<InputFilter*>::iterator i;
    i = find(_filters.begin(), _filters.end(), filter);
    if (i == _filters.end()) {
	debug_msg("filter does not exist\n");
	return false;
    }

    _filters.erase(i);
    if (_filters.empty()) {
	eventloop_unhook();
    }
    return true;
}

void
FilterRawSocket4::process_recv_data(const vector<uint8_t>& buf)
{
    assert(buf.size() >= sizeof(struct ip));
    for (list<InputFilter*>::iterator i = _filters.begin();
	 i != _filters.end(); ++i) {
	(*i)->recv(buf);
    }
}
