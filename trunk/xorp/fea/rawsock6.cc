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

#ident "$XORP: xorp/fea/rawsock6.cc,v 1.3 2004/11/23 19:04:08 atanu Exp $"

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <netinet/ip6.h>

#include <unistd.h>
#include <fcntl.h>

#include "config.h"
#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "rawsock6.hh"

/* ------------------------------------------------------------------------- */
/* RawSocket6 methods */

RawSocket6::RawSocket6(uint32_t protocol) throw (RawSocket6Exception)
{
    _fd = socket(AF_INET6, SOCK_RAW, protocol);
    if (_fd < 0)
	xorp_throw(RawSocket6Exception, "socket", errno);

    int bool_flag = 1;
#ifdef IPV6_RECVPKTINFO
    // The new option (applies to receiving only)
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	(void *)&bool_flag, sizeof(bool_flag)) < 0) {
	xorp_throw(RawSocket6Exception,
	"setsockopt(IPV6_RECVPKTINFO) failed", errno);
    }
#else
    // The old option (see RFC 2292)
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_PKTINFO,
	(void *)&bool_flag, sizeof(bool_flag)) < 0) {
	xorp_throw(RawSocket6Exception,
	"setsockopt(IPV6_PKTINFO) failed", errno);
    }
#endif // ! IPV6_RECVPKTINFO

    // XXX: Add more things we're interested in receiving here.
}

RawSocket6::~RawSocket6()
{
    if (_fd > 0)
	close(_fd);
}

ssize_t
RawSocket6::write(const IPv6& src, const IPv6& dst, const uint8_t* payload,
		  size_t nbytes) const
{
    uint8_t cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    struct cmsghdr* cmsgp = reinterpret_cast<struct cmsghdr*>(cmsgbuf);
    struct in6_pktinfo* pktinfo = reinterpret_cast<struct in6_pktinfo*>(
				  CMSG_DATA(cmsgp));

    // Source.
    memset(&pktinfo->ipi6_addr, 0, sizeof(pktinfo->ipi6_addr));
    src.copy_out(pktinfo->ipi6_addr);

    // Control message header.
    cmsgp->cmsg_level = IPPROTO_IPV6;
    cmsgp->cmsg_type = IPV6_PKTINFO;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));

    // Destination.
    struct sockaddr_in6 sdst;
    memset(&sdst, 0, sizeof(sdst));
    sdst.sin6_family = AF_INET6;
    dst.copy_out(sdst.sin6_addr);
#ifdef HAVE_SIN_LEN
    sdst.sin6_len = sizeof(sockaddr_in6);
#endif /* HAVE_SIN_LEN */

    // Payload.
    struct iovec iov;
    iov.iov_base = reinterpret_cast<caddr_t>(const_cast<uint8_t *>(payload));
    iov.iov_len = nbytes;

    // Message header.
    struct msghdr mh;
    mh.msg_name = (caddr_t)&sdst;
    mh.msg_namelen = sizeof(sdst);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = reinterpret_cast<caddr_t>(const_cast<uint8_t *>(cmsgbuf));
    mh.msg_controllen = sizeof(cmsgbuf);
    mh.msg_flags = 0;
    return ::sendmsg(_fd, &mh, 0);
}

/* ------------------------------------------------------------------------- */
/* IoRawSocket6 methods */

IoRawSocket6::IoRawSocket6(EventLoop&	eventloop,
			   uint32_t	protocol,
			   bool		autohook)
    throw (RawSocket6Exception)
    : RawSocket6(protocol), _eventloop(eventloop), _autohook(autohook)
{
    int fl = fcntl(_fd, F_GETFL);
    if (fcntl(_fd, F_SETFL, fl | O_NONBLOCK) < 0)
	xorp_throw(RawSocket6Exception, "fcntl (O_NON_BLOCK)", errno);

    _cmsgbuf.reserve(CMSGBUF_BYTES);
    if (_cmsgbuf.capacity() != CMSGBUF_BYTES)
	xorp_throw(RawSocket6Exception, "cmsg buffer reserve() failed", 0);

    _optbuf.reserve(OPTBUF_BYTES);
    if (_optbuf.capacity() != OPTBUF_BYTES)
	xorp_throw(RawSocket6Exception, "options buffer reserve() failed", 0);

    _recvbuf.reserve(RECVBUF_BYTES);
    if (_recvbuf.capacity() != RECVBUF_BYTES)
	xorp_throw(RawSocket6Exception, "receive buffer reserve() failed", 0);

    ssize_t sz = RECVBUF_BYTES;
    if (setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz)) < 0)
	xorp_throw(RawSocket6Exception, "setsockopt (SO_RCVBUF)", errno);

    if (_autohook)
	eventloop_hook();
}

IoRawSocket6::~IoRawSocket6()
{
    if (_autohook)
	eventloop_unhook();
}

void
IoRawSocket6::recv(int fd, SelectorMask m)
{
    UNUSED(fd);
    UNUSED(m);

    // Source.
    struct sockaddr_in6 ssrc;
    memset(&ssrc, 0, sizeof(struct sockaddr_in6));

    // Payload.
    struct iovec iov;
    iov.iov_base = (caddr_t)&_recvbuf[0];
    iov.iov_len = RECVBUF_BYTES;

    // Message header.
    struct msghdr mh;
    mh.msg_name = (caddr_t)&ssrc;
    mh.msg_namelen = sizeof(ssrc);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = (caddr_t)&_cmsgbuf[0];
    mh.msg_controllen = CMSGBUF_BYTES;
    mh.msg_flags = 0;

    ssize_t n = recvmsg(_fd, &mh, 0);
    debug_msg("Read fd %d, %d bytes\n", _fd, n);
    if (n <= 0) {
	return;
    }
    _recvbuf.resize(n);

    _hdrinfo.src.copy_in(ssrc.sin6_addr);

    // Process the received control message headers.
    struct cmsghdr *chp;
    for (chp = CMSG_FIRSTHDR(&mh); chp != NULL; chp = CMSG_NXTHDR(&mh, chp)) {
	uint32_t*		intp;
	struct in6_pktinfo*	pip;

	if (chp->cmsg_level == IPPROTO_IPV6) {
	    switch (chp->cmsg_type) {
#ifdef IPV6_PKTINFO
	    case IPV6_PKTINFO:
		pip = (struct in6_pktinfo *)CMSG_DATA(&mh);
		_hdrinfo.dst.copy_in(pip->ipi6_addr);
		_hdrinfo.rcvifindex = pip->ipi6_ifindex;
		break;
#endif
#ifdef IPV6_HOPLIMIT
	    case IPV6_HOPLIMIT:
		intp = (uint32_t *)CMSG_DATA(&mh);
		_hdrinfo.hoplimit = *intp & 0xFF;
		break;
#endif
#ifdef IPV6_TCLASS
	    case IPV6_TCLASS:
		intp = (uint32_t *)CMSG_DATA(&mh);
		_hdrinfo.tclass = *intp & 0xFF;
		break;
#endif
#ifdef IPV6_HOPOPTS
	    case IPV6_HOPOPTS:
		bcopy(chp, &_cmsgbuf[0], chp->cmsg_len -
		    sizeof(struct cmsghdr));
		_cmsgbuf.resize(chp->cmsg_len);
		break;
#endif
	    default:
		continue;
	    }
	}
    }

    process_recv_data(_hdrinfo, _optbuf, _recvbuf);
}

bool
IoRawSocket6::eventloop_hook()
{
    debug_msg("hooking\n");
    return _eventloop.add_selector(_fd, SEL_RD,
				   callback(this, &IoRawSocket6::recv));
}

void
IoRawSocket6::eventloop_unhook()
{
    debug_msg("unhooking\n");
    _eventloop.remove_selector(_fd);
}

/* ------------------------------------------------------------------------- */
/* FilterRawSocket6 methods */

FilterRawSocket6::FilterRawSocket6(EventLoop& eventloop, int protocol)
    throw (RawSocket6Exception)
    : IoRawSocket6(eventloop, protocol, false)
{
}

FilterRawSocket6::~FilterRawSocket6()
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
FilterRawSocket6::add_filter(InputFilter* filter)
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
FilterRawSocket6::remove_filter(InputFilter* filter)
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
FilterRawSocket6::process_recv_data(const struct IPv6HeaderInfo& hdrinfo,
				    const vector<uint8_t>& options,
				    const vector<uint8_t>& payload)
{
    for (list<InputFilter*>::iterator i = _filters.begin();
	 i != _filters.end(); ++i) {
	(*i)->recv(hdrinfo, options, payload);
    }
}
