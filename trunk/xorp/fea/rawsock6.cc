// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/fea/rawsock6.cc,v 1.12 2005/03/25 02:53:14 pavlin Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "rawsock6.hh"


#ifdef HOST_OS_WINDOWS

#include <mswsock.h>

#define cmsghdr wsacmsghdr

typedef char *caddr_t;

#ifdef __MINGW32__

#ifndef _ALIGNBYTES
#define _ALIGNBYTES     (sizeof(int) - 1)
#endif

#ifndef _ALIGN
#define _ALIGN(p)       (((unsigned)(p) + _ALIGNBYTES) & ~_ALIGNBYTES)
#endif

#define CMSG_DATA(cmsg)		\
	((unsigned char *)(cmsg) + _ALIGN(sizeof(struct cmsghdr)))
#define	CMSG_NXTHDR(mhdr, cmsg)	\
	(((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len) + \
		_ALIGN(sizeof(struct cmsghdr)) > \
		(char *)(mhdr)->Control.buf + (mhdr)->Control.len) ? \
		(struct cmsghdr *)0 : \
		(struct cmsghdr *)((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len)))
#define	CMSG_FIRSTHDR(mhdr) \
	((mhdr)->Control.len >= sizeof(struct cmsghdr) ? \
	 (struct cmsghdr *)(mhdr)->Control.buf : \
	 (struct cmsghdr *)NULL)
#define	CMSG_SPACE(l)	(_ALIGN(sizeof(struct cmsghdr)) + _ALIGN(l))
#define	CMSG_LEN(l)	(_ALIGN(sizeof(struct cmsghdr)) + (l))

typedef INT (WINAPI * LPFN_WSARECVMSG)(SOCKET, LPWSAMSG, LPDWORD,
LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE); 

#define WSAID_WSARECVMSG \
    {0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22}}

#else	/* !__MINGW32__ */

#define CMSG_FIRSTHDR(msg)	WSA_CMSG_FIRSTHDR(msg)
#define CMSG_NXTHDR(msg, cmsg)	WSA_CMSG_NEXTHDR(msg, cmsg)
#define CMSG_DATA(cmsg) 	WSA_CMSG_DATA(cmsg)
#define CMSG_SPACE(len)		WSA_CMSG_SPACE(len)
#define CMSG_LEN(len)		WSA_CMSG_LEN(len)

#endif /* __MINGW32__ */

static const GUID guidWSARecvMsg = WSAID_WSARECVMSG;
static LPFN_WSARECVMSG lpWSARecvMsg = NULL;

#endif /* HOST_OS_WINDOWS */

/* ------------------------------------------------------------------------- */
/* RawSocket6 methods */

RawSocket6::RawSocket6(uint32_t protocol) throw (RawSocket6Exception)
{
    _fd = socket(AF_INET6, SOCK_RAW, protocol);
    if (!_fd.is_valid())
	xorp_throw(RawSocket6Exception, "socket", errno);

#ifdef HOST_OS_WINDOWS
    // Obtain the pointer to the extension function WSARecvMsg() if required. 
    if (lpWSARecvMsg == NULL) {
	int result;
	DWORD nbytes;

	result = WSAIoctl(_fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
			  const_cast<GUID *>(&guidWSARecvMsg),
			  sizeof(guidWSARecvMsg),
			  &lpWSARecvMsg, sizeof(lpWSARecvMsg), &nbytes,
			  NULL, NULL);
	if (result == SOCKET_ERROR) {
	    XLOG_ERROR("Cannot obtain WSARecvMsg function pointer; "
		       "unable to receive raw IPv6 traffic.");
	    lpWSARecvMsg = NULL;
	}
   }
#endif

    int bool_flag = 1;
#ifdef IPV6_RECVPKTINFO
    // The new option (applies to receiving only)
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	xorp_throw(RawSocket6Exception,
	"setsockopt(IPV6_RECVPKTINFO) failed", errno);
    }
#else
    // The old option (see RFC-2292)
    if (setsockopt(_fd, IPPROTO_IPV6, IPV6_PKTINFO,
	XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	xorp_throw(RawSocket6Exception,
	"setsockopt(IPV6_PKTINFO) failed", errno);
    }
#endif // ! IPV6_RECVPKTINFO
}

RawSocket6::~RawSocket6()
{
    if (_fd.is_valid())
	comm_close(_fd);
}

ssize_t
RawSocket6::write(const IPv6& src, const IPv6& dst, const uint8_t* payload,
		  size_t nbytes) const
{
#ifndef HOST_OS_WINDOWS
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
#endif

    // Destination.
    struct sockaddr_in6 sdst;
    memset(&sdst, 0, sizeof(sdst));
    sdst.sin6_family = AF_INET6;
    dst.copy_out(sdst.sin6_addr);
#ifdef HAVE_SIN_LEN
    sdst.sin6_len = sizeof(sockaddr_in6);
#endif

    // Payload.
    struct iovec iov;
    iov.iov_base = reinterpret_cast<caddr_t>(const_cast<uint8_t *>(payload));
    iov.iov_len = nbytes;

    // Message header.
#ifdef HOST_OS_WINDOWS
    // XXX: Currently there is no way of specifying the source address.
    DWORD sent, error;
    error = WSASendTo(_fd, reinterpret_cast<WSABUF *>(&iov), 1, &sent, 0,
		      reinterpret_cast<struct sockaddr *>(&sdst), sizeof(sdst),
		      NULL, NULL);
    return sent;
    UNUSED(src);
#else
    struct msghdr mh;
    mh.msg_name = (caddr_t)&sdst;
    mh.msg_namelen = sizeof(sdst);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = reinterpret_cast<caddr_t>(const_cast<uint8_t *>(cmsgbuf));
    mh.msg_controllen = sizeof(cmsgbuf);
    mh.msg_flags = 0;
    return ::sendmsg(_fd, &mh, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/* IoRawSocket6 methods */

IoRawSocket6::IoRawSocket6(EventLoop&	eventloop,
			   uint32_t	protocol,
			   bool		autohook)
    throw (RawSocket6Exception)
    : RawSocket6(protocol), _eventloop(eventloop), _autohook(autohook)
{
    if (comm_sock_set_blocking(_fd, 0) != XORP_OK) {
	xorp_throw(RawSocket6Exception, "comm_sock_set_blocking()",
		   comm_get_last_error());
    }

    _cmsgbuf.reserve(CMSGBUF_BYTES);
    if (_cmsgbuf.capacity() != CMSGBUF_BYTES)
	xorp_throw(RawSocket6Exception, "cmsg buffer reserve() failed", 0);

    _hoptbuf.reserve(OPTBUF_BYTES);
    if (_hoptbuf.capacity() != OPTBUF_BYTES)
	xorp_throw(RawSocket6Exception, "hopopts buffer reserve() failed", 0);

    _recvbuf.reserve(RECVBUF_BYTES);
    if (_recvbuf.capacity() != RECVBUF_BYTES)
	xorp_throw(RawSocket6Exception, "receive buffer reserve() failed", 0);

    if (comm_sock_set_rcvbuf(_fd, RECVBUF_BYTES, RECVBUF_BYTES) != XORP_OK)
	xorp_throw(RawSocket6Exception, "comm_sock_set_rcvbuf() failed", 0);

    if (_autohook)
	eventloop_hook();
}

IoRawSocket6::~IoRawSocket6()
{
    if (_autohook)
	eventloop_unhook();
}

void
IoRawSocket6::recv(XorpFd fd, IoEventType type)
{
    UNUSED(fd);
    UNUSED(type);

    // Source.
    struct sockaddr_in6 ssrc;
    memset(&ssrc, 0, sizeof(struct sockaddr_in6));

    // Payload.
    struct iovec iov;
    iov.iov_base = (caddr_t)&_recvbuf[0];
    iov.iov_len = RECVBUF_BYTES;

    ssize_t n;
#ifdef HOST_OS_WINDOWS
    WSAMSG mh;
    DWORD error;
    DWORD nrecvd;

    mh.name = (LPSOCKADDR)&ssrc;
    mh.namelen = sizeof(ssrc);
    mh.lpBuffers = (LPWSABUF)&iov;
    mh.dwBufferCount = 1;
    mh.Control.len = CMSGBUF_BYTES;
    mh.Control.buf = (caddr_t)&_cmsgbuf[0];
    mh.dwFlags = 0;

    if (lpWSARecvMsg == NULL) {
	XLOG_ERROR("lpWSARecvMsg is NULL");
	return;
    }
    error = lpWSARecvMsg(_fd, &mh, &nrecvd, NULL, NULL);
    n = (ssize_t)nrecvd;
#else
    // Message header.
    struct msghdr mh;
    mh.msg_name = (caddr_t)&ssrc;
    mh.msg_namelen = sizeof(ssrc);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = (caddr_t)&_cmsgbuf[0];
    mh.msg_controllen = CMSGBUF_BYTES;
    mh.msg_flags = 0;

    n = ::recvmsg(_fd, &mh, 0);
#endif

    debug_msg("Read fd %s, %d bytes\n", _fd.str().c_str(), XORP_INT_CAST(n));
    if (n <= 0) {
	return;
    }
    _recvbuf.resize(n);
    _hdrinfo.src.copy_in(ssrc.sin6_addr);

    // Process the received control message headers.
    struct cmsghdr *chp;
    for (chp = CMSG_FIRSTHDR(&mh); chp != NULL; chp = CMSG_NXTHDR(&mh, chp)) {
#ifdef IPV6_PKTINFO
	struct in6_pktinfo*	pip;
#endif
#if defined(IPV6_HOPLIMIT) || defined(IPV6_TCLASS)
	uint32_t*		intp;
#endif

	if (chp->cmsg_level == IPPROTO_IPV6) {
	    switch (chp->cmsg_type) {
#ifdef IPV6_PKTINFO
	    case IPV6_PKTINFO:
		pip = (struct in6_pktinfo *)CMSG_DATA(chp);
		_hdrinfo.dst.copy_in(pip->ipi6_addr);
		_hdrinfo.rcvifindex = pip->ipi6_ifindex;
		break;
#endif
#ifdef IPV6_HOPLIMIT
	    case IPV6_HOPLIMIT:
		intp = (uint32_t *)CMSG_DATA(chp);
		_hdrinfo.hoplimit = *intp & 0xFF;
		break;
#endif
#ifdef IPV6_TCLASS
	    case IPV6_TCLASS:
		intp = (uint32_t *)CMSG_DATA(chp);
		_hdrinfo.tclass = *intp & 0xFF;
		break;
#endif
#ifdef IPV6_HOPOPTS
	    case IPV6_HOPOPTS:
		bcopy(CMSG_DATA(chp), &_hoptbuf[0], chp->cmsg_len -
		    sizeof(struct cmsghdr));
		_cmsgbuf.resize(chp->cmsg_len);
		break;
#endif
	    default:
		continue;
	    }
	}
    }

    process_recv_data(_hdrinfo, _hoptbuf, _recvbuf);
}

bool
IoRawSocket6::eventloop_hook()
{
    debug_msg("hooking\n");
    return _eventloop.add_ioevent_cb(_fd, IOT_READ,
				   callback(this, &IoRawSocket6::recv));
}

void
IoRawSocket6::eventloop_unhook()
{
    debug_msg("unhooking\n");
    _eventloop.remove_ioevent_cb(_fd);
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
				    const vector<uint8_t>& hopopts,
				    const vector<uint8_t>& payload)
{
    for (list<InputFilter*>::iterator i = _filters.begin();
	 i != _filters.end(); ++i) {
	(*i)->recv(hdrinfo, hopopts, payload);
    }
}
