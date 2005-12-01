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

#ident "$XORP: xorp/fea/rawsock.cc,v 1.10 2005/10/30 21:58:04 pavlin Exp $"

//
// Raw socket support.
//

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/utils.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#ifdef HOST_OS_WINDOWS
#include <mswsock.h>
#include "ip.h"
#endif

#include "libcomm/comm_api.h"
#include "mrt/include/ip_mroute.h"

// XXX: _PIM_VT is needed if we want the extra features of <netinet/pim.h>
#define _PIM_VT 1
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#else
#include "mrt/include/netinet/pim.h"
#endif

#include "iftree.hh"
#include "rawsock.hh"


//
// Local constants definitions
//
#define IO_BUF_SIZE		(64*1024)  // I/O buffer(s) size
#define CMSG_BUF_SIZE		(10*1024)  // 'rcvcmsgbuf' and 'sndcmsgbuf'
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  // Min. socket buffer size
#define SO_RCV_BUF_SIZE_MAX	(256*1024) // Desired socket buffer size

#ifndef MINTTL
#define MINTTL		1
#endif
#ifndef IPDEFTTL
#define IPDEFTTL	64
#endif
#ifndef MAXTTL
#define MAXTTL		255
#endif

#ifndef MLD_MINLEN
#  ifdef HAVE_MLD_HDR
#    define MLD_MINLEN			(sizeof(struct mld_hdr))
#  else
#    define MLD_MINLEN			24
#  endif
#endif

//
// Local structures/classes, typedefs and macros
//

#ifdef HOST_OS_WINDOWS

#define cmsghdr wsacmsghdr
typedef char *caddr_t;

#ifdef __MINGW32__
#ifndef _ALIGNBYTES
#define _ALIGNBYTES	(sizeof(int) - 1)
#endif

#ifndef _ALIGN
#define _ALIGN(p)	(((unsigned)(p) + _ALIGNBYTES) & ~_ALIGNBYTES)
#endif

#define CMSG_DATA(cmsg)		\
	((unsigned char *)(cmsg) + _ALIGN(sizeof(struct cmsghdr))) 
#define CMSG_NXTHDR(mhdr, cmsg)	\
	(((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len) + \
		_ALIGN(sizeof(struct cmsghdr)) > \
		(char *)(mhdr)->Control.buf + (mhdr)->Control.len) ? \
		(struct cmsghdr *)0 : \
		(struct cmsghdr *)((char *)(cmsg) + _ALIGN((cmsg)->cmsg_len)))
#define CMSG_FIRSTHDR(mhdr) \
	((mhdr)->Control.len >= sizeof(struct cmsghdr) ? \
	 (struct cmsghdr *)(mhdr)->Control.buf : \
	 (struct cmsghdr *)NULL)
#define CMSG_SPACE(l)	(_ALIGN(sizeof(struct cmsghdr)) + _ALIGN(l))
#define CMSG_LEN(l)	(_ALIGN(sizeof(struct cmsghdr)) + (l))

typedef INT (WINAPI * LPFN_WSARECVMSG)(SOCKET, LPWSAMSG, LPDWORD,
				       LPWSAOVERLAPPED,
				       LPWSAOVERLAPPED_COMPLETION_ROUTINE);

#define WSAID_WSARECVMSG \
	{ 0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22} }

#else // ! __MINGW32__

#define CMSG_FIRSTHDR(msg)	WSA_CMSG_FIRSTHDR(msg)
#define CMSG_NXTHDR(msg, cmsg)	WSA_CMSG_NEXTHDR(msg, cmsg)
#define CMSG_DATA(cmsg)		WSA_CMSG_DATA(cmsg)
#define CMSG_SPACE(len)		WSA_CMSG_SPACE(len)
#define CMSG_LEN(len)		WSA_CMSG_LEN(len)

#endif // ! __MINGW32__

#ifdef HAVE_IPV6

static const GUID guidWSARecvMsg = WSAID_WSARECVMSG;
static LPFN_WSARECVMSG lpWSARecvMsg = NULL;

#endif // HAVE_IPV6

#endif // HOST_OS_WINDOWS


#ifndef CMSG_LEN
#define CMSG_LEN(l) (ALIGN(sizeof(struct cmsghdr)) + (l)) // XXX
#endif

//
// Local variables
//

// IPv4 Router Alert stuff
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNETCONTROL	0xc0
#endif
#ifndef IPOPT_RA
#define IPOPT_RA			148	/* 0x94 */
#endif

// IPv6 Router Alert stuff
#ifdef HAVE_IPV6
#ifndef IP6OPT_RTALERT
#define IP6OPT_RTALERT		0x05
#endif
#ifndef IP6OPT_RTALERT_LEN
#define IP6OPT_RTALERT_LEN	4
#endif
#ifndef IP6OPT_RTALERT_MLD
#define IP6OPT_RTALERT_MLD	0
#endif
#ifndef IP6OPT_ROUTER_ALERT	// XXX: for compatibility with older systems
#define IP6OPT_ROUTER_ALERT IP6OPT_RTALERT
#endif
#endif // HAVE_IPV6
//
#ifdef HAVE_IPV6
static uint16_t		rtalert_code;
#ifndef HAVE_RFC3542
static uint8_t		raopt[IP6OPT_RTALERT_LEN];
#endif
#endif // HAVE_IPV6


RawSocket::RawSocket(EventLoop& eventloop, int init_family,
		     uint8_t ip_protocol, const IfTree& iftree)
    : _eventloop(eventloop),
      _family(init_family),
      _ip_protocol(ip_protocol),
      _iftree(iftree)
{
    // Init Router Alert related option stuff
#ifdef HAVE_IPV6
    rtalert_code = htons(IP6OPT_RTALERT_MLD); // XXX: used by MLD only (?)
#ifndef HAVE_RFC3542
    raopt[0] = IP6OPT_ROUTER_ALERT;
    raopt[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&raopt[2], (caddr_t)&rtalert_code, sizeof(rtalert_code));
#endif // ! HAVE_RFC3542
#endif // HAVE_IPV6
    
    // Allocate the buffers
    _rcvbuf0 = new uint8_t[IO_BUF_SIZE];
    _sndbuf0 = new uint8_t[IO_BUF_SIZE];
    _rcvbuf1 = new uint8_t[IO_BUF_SIZE];
    _sndbuf1 = new uint8_t[IO_BUF_SIZE];
    _rcvcmsgbuf = new uint8_t[CMSG_BUF_SIZE];
    _sndcmsgbuf = new uint8_t[CMSG_BUF_SIZE];

    // Scatter/gatter array initialization
    _rcviov[0].iov_base		= (caddr_t)_rcvbuf0;
    _rcviov[1].iov_base		= (caddr_t)_rcvbuf1;
    _rcviov[0].iov_len		= IO_BUF_SIZE;
    _rcviov[1].iov_len		= IO_BUF_SIZE;
    _sndiov[0].iov_base		= (caddr_t)_sndbuf0;
    _sndiov[1].iov_base		= (caddr_t)_sndbuf1;
    _sndiov[0].iov_len		= 0;
    _sndiov[1].iov_len		= 0;

    // recvmsg() and sendmsg() related initialization
#ifndef HOST_OS_WINDOWS    
    switch (family()) {
    case AF_INET:
	_rcvmh.msg_name		= (caddr_t)&_from4;
	_sndmh.msg_name		= (caddr_t)&_to4;
	_rcvmh.msg_namelen	= sizeof(_from4);
	_sndmh.msg_namelen	= sizeof(_to4);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	_rcvmh.msg_name		= (caddr_t)&_from6;
	_sndmh.msg_name		= (caddr_t)&_to6;
	_rcvmh.msg_namelen	= sizeof(_from6);
	_sndmh.msg_namelen	= sizeof(_to6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    _rcvmh.msg_iov		= _rcviov;
    _sndmh.msg_iov		= _sndiov;
    _rcvmh.msg_iovlen		= 1;
    _sndmh.msg_iovlen		= 1;
    _rcvmh.msg_control		= (caddr_t)_rcvcmsgbuf;
    _sndmh.msg_control		= (caddr_t)_sndcmsgbuf;
    _rcvmh.msg_controllen	= CMSG_BUF_SIZE;
    _sndmh.msg_controllen	= 0;
#endif // ! HOST_OS_WINDOWS
}

RawSocket::~RawSocket()
{
    string dummy_error_msg;

    stop(dummy_error_msg);
    
    // Free the buffers
    delete[] _rcvbuf0;
    delete[] _sndbuf0;
    delete[] _rcvbuf1;
    delete[] _sndbuf1;
    delete[] _rcvcmsgbuf;
    delete[] _sndcmsgbuf;
}

int
RawSocket::start(string& error_msg)
{
    if (_proto_socket.is_valid())
	return (XORP_OK);
    
    return (open_proto_socket(error_msg));
}

int
RawSocket::stop(string& error_msg)
{
    if (! _proto_socket.is_valid())
	return (XORP_OK);

    return (close_proto_socket(error_msg));
}

int
RawSocket::enable_ip_hdr_include(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
#ifdef IP_HDRINCL
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled; 
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_HDRINCL,
			XORP_SOCKOPT_CAST(&bool_flag),
			sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IP_HDRINCL, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IP_HDRINCL
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
	break;		// XXX
#endif // HAVE_IPV6
	
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(is_enabled);
}

int
RawSocket::enable_recv_pktinfo(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
	break;
	
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled;
	
	//
	// Interface index and address
	//
#ifdef IPV6_RECVPKTINFO
	// The new option (applies to receiving only)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVPKTINFO, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	// The old option (see RFC-2292)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_PKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_PKTINFO, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVPKTINFO
	
	//
	// Hop-limit field
	//
#ifdef IPV6_RECVHOPLIMIT
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVHOPLIMIT, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_HOPLIMIT,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_HOPLIMIT, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPLIMIT
	
	//
	// Traffic class value
	//
#ifdef IPV6_RECVTCLASS
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVTCLASS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVTCLASS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IPV6_RECVTCLASS
	
	//
	// Hop-by-hop options
	//
#ifdef IPV6_RECVHOPOPTS
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVHOPOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVHOPOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_HOPOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_HOPOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPOPTS
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(is_enabled);
}

int
RawSocket::set_multicast_ttl(int ttl, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	u_char ip_ttl = ttl; // XXX: In IPv4 the value argument is 'u_char'
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_TTL,
		       XORP_SOCKOPT_CAST(&ip_ttl), sizeof(ip_ttl)) < 0) {
	    error_msg = c_format("setsockopt(IP_MULTICAST_TTL, %u) failed: %s",
				 ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST
	error_msg = c_format("set_multicast_ttl() failed: "
			     "IPv6 multicast not supported");
	return (XORP_ERROR);
#else
	int ip_ttl = ttl;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       XORP_SOCKOPT_CAST(&ip_ttl), sizeof(ip_ttl)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_MULTICAST_HOPS, %u) failed: %s",
				 ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RawSocket::enable_multicast_loopback(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	u_char loop = is_enabled;
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_LOOP,
		       XORP_SOCKOPT_CAST(&loop), sizeof(loop)) < 0) {
	    error_msg = c_format("setsockopt(IP_MULTICAST_LOOP, %u) failed: %s",
				 loop, strerror(errno));
	    return (XORP_ERROR);
	}
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST
	error_msg = c_format("enable_multicast_loop() failed: "
			     "IPv6 multicast not supported");
	return (XORP_ERROR);
#else
	uint loop6 = is_enabled;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       XORP_SOCKOPT_CAST(&loop6), sizeof(loop6)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_MULTICAST_LOOP, %u) failed: %s",
				 loop6, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RawSocket::set_default_multicast_interface(const string& if_name,
					   const string& vif_name,
					   string& error_msg)
{
    // Find the interface
    IfTree::IfMap::const_iterator ii = _iftree.get_if(if_name);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Setting the default multicast interface failed:"
			     "interface %s not found",
			     if_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeInterface& fi = ii->second;

    // Find the vif
    IfTreeInterface::VifMap::const_iterator vi = fi.get_vif(vif_name);
    if (vi == fi.vifs().end()) {
	error_msg = c_format("Setting the default multicast interface failed:"
			     "interface %s vif %s not found",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeVif& fv = vi->second;

    switch (family()) {
    case AF_INET:
    {
	struct in_addr in_addr;

	// Find the first address
	IfTreeVif::V4Map::const_iterator ai = fv.v4addrs().begin();
	if (ai == fv.v4addrs().end()) {
	    error_msg = c_format("Setting the default multicast interface "
				 "failed: "
				 "interface %s vif %s has no address",
				 if_name.c_str(), vif_name.c_str());
	    return (XORP_ERROR);
	}
	const IfTreeAddr4& fa = ai->second;

	fa.addr().copy_out(in_addr);
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_IF,
		       XORP_SOCKOPT_CAST(&in_addr), sizeof(in_addr)) < 0) {
	    error_msg = c_format("setsockopt(IP_MULTICAST_IF, %s) failed: %s",
				 cstring(fa.addr()), strerror(errno));
	    return (XORP_ERROR);
	}
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST
	error_msg = c_format("set_default_multicast_interface() failed: "
			     "IPv6 multicast not supported");
	return (XORP_ERROR);
#else
	u_int pif_index = fv.pif_index();
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		       XORP_SOCKOPT_CAST(&pif_index), sizeof(pif_index)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_MULTICAST_IF, %s/%s) failed: %s",
				 if_name.c_str(), vif_name.c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RawSocket::join_multicast_group(const string& if_name,
				const string& vif_name,
				const IPvX& group,
				string& error_msg)
{
    // Find the interface
    IfTree::IfMap::const_iterator ii = _iftree.get_if(if_name);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Joining multicast group %s failed: "
			     "interface %s not found",
			     cstring(group),
			     if_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeInterface& fi = ii->second;

    // Find the vif
    IfTreeInterface::VifMap::const_iterator vi = fi.get_vif(vif_name);
    if (vi == fi.vifs().end()) {
	error_msg = c_format("Joining multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeVif& fv = vi->second;
    
#if 0	// TODO: enable or disable the enabled() check?
    if (! (fi.enabled() || fv.enabled())) {
	error_msg = c_format("Cannot join group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;

	// Find the first address
	IfTreeVif::V4Map::const_iterator ai = fv.v4addrs().begin();
	if (ai == fv.v4addrs().end()) {
	    error_msg = c_format("Cannot join group %s on interface %s vif %s: "
				 "interface/vif has no address",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str());
	    return (XORP_ERROR);
	}
	const IfTreeAddr4& fa = ai->second;
	
	fa.addr().copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       XORP_SOCKOPT_CAST(&mreq), sizeof(mreq)) < 0) {
	    error_msg = c_format("Cannot join group %s on interface %s vif %s: %s",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST
	error_msg = c_format("join_multicast_group() failed: "
			     "IPv6 multicast not supported");
	return (XORP_ERROR);
#else
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = fv.pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		       XORP_SOCKOPT_CAST(&mreq6), sizeof(mreq6)) < 0) {
	    error_msg = c_format("Cannot join group %s on interface %s vif %s: %s",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RawSocket::leave_multicast_group(const string& if_name,
				 const string& vif_name,
				 const IPvX& group,
				 string& error_msg)
{
    // Find the interface
    IfTree::IfMap::const_iterator ii = _iftree.get_if(if_name);
    if (ii == _iftree.ifs().end()) {
	error_msg = c_format("Leaving multicast group %s failed: "
			     "interface %s not found",
			     cstring(group),
			     if_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeInterface& fi = ii->second;

    // Find the vif
    IfTreeInterface::VifMap::const_iterator vi = fi.get_vif(vif_name);
    if (vi == fi.vifs().end()) {
	error_msg = c_format("Leaving multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    const IfTreeVif& fv = vi->second;
    
#if 0	// TODO: enable or disable the enabled() check?
    if (! (fi.enabled() || fv.enabled())) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;

	// Find the first address
	IfTreeVif::V4Map::const_iterator ai = fv.v4addrs().begin();
	if (ai == fv.v4addrs().end()) {
	    error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
				 "interface/vif has no address",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str());
	    return (XORP_ERROR);
	}
	const IfTreeAddr4& fa = ai->second;

	fa.addr().copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       XORP_SOCKOPT_CAST(&mreq), sizeof(mreq)) < 0) {
	    error_msg = c_format("Cannot leave group %s on interface %s vif %s: %s",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST
	error_msg = c_format("leave_multicast_group() failed: "
			     "IPv6 multicast not supported");
	return (XORP_ERROR);
#else
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = fv.pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
		       XORP_SOCKOPT_CAST(&mreq6), sizeof(mreq6)) < 0) {
	    error_msg = c_format("Cannot leave group %s on interface %s vif %s: %s",
				 cstring(group),
				 if_name.c_str(),
				 vif_name.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
RawSocket::open_proto_socket(string& error_msg)
{
    string dummy_error_msg;

    // If necessary, open the protocol socket
    do {
	if (_proto_socket.is_valid())
	    break;
	_proto_socket = socket(family(), SOCK_RAW, _ip_protocol);
	if (!_proto_socket.is_valid()) {
	    char *errstr;

#ifdef HAVE_STRERROR
	    errstr = strerror(errno);
#else
	    errstr = "unknown error";
#endif
	    error_msg = c_format("Cannot open IP protocol %u raw socket: %s",
				 _ip_protocol, errstr);
	    return (XORP_ERROR);
	}
    } while (false);

#ifdef HOST_OS_WINDOWS
    switch (family()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// Obtain the pointer to the extension function WSARecvMsg() if needed
	if (lpWSARecvMsg == NULL) {
	    int result;
	    DWORD nbytes;

	    result = WSAIoctl(_proto_socket,
			      SIO_GET_EXTENSION_FUNCTION_POINTER,
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
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
#endif // HOST_OS_WINDOWS

    // Set various socket options
    // Lots of input buffering
    if (comm_sock_set_rcvbuf(_proto_socket, SO_RCV_BUF_SIZE_MAX,
			     SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	error_msg = c_format("Cannot set the receiver buffer size: %s",
			     comm_get_last_error_str());
	close_proto_socket(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Include IP header when sending (XXX: doesn't do anything for IPv6)
    if (enable_ip_hdr_include(true, error_msg) != XORP_OK) {
	close_proto_socket(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Show interest in receiving information from IP header (XXX: IPv6 only)
    if (enable_recv_pktinfo(true, error_msg) != XORP_OK) {
	close_proto_socket(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Restrict multicast TTL
    if (set_multicast_ttl(MINTTL, error_msg) != XORP_OK) {
	close_proto_socket(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Disable mcast loopback
    if (enable_multicast_loopback(false, error_msg) != XORP_OK) {
	close_proto_socket(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Protocol-specific setup
    switch (family()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	if (ip_protocol() == IPPROTO_ICMPV6) {
	    struct icmp6_filter filter;
	    
	    // Pass all ICMPv6 messages
	    ICMP6_FILTER_SETPASSALL(&filter);    

#ifdef HAVE_IPV6_MULTICAST_ROUTING
#if 0		// XXX: used only for multicast routing purpose by MLD
	    if (module_id() == XORP_MODULE_MLD6IGMP) {
		// Filter all non-MLD ICMPv6 messages
		ICMP6_FILTER_SETBLOCKALL(&filter);
		ICMP6_FILTER_SETPASS(MLD_LISTENER_QUERY, &filter);
		ICMP6_FILTER_SETPASS(MLD_LISTENER_REPORT, &filter);
		ICMP6_FILTER_SETPASS(MLD_LISTENER_DONE, &filter);
		ICMP6_FILTER_SETPASS(MLD_MTRACE_RESP, &filter);
		ICMP6_FILTER_SETPASS(MLD_MTRACE, &filter);
#ifdef MLDV2_LISTENER_REPORT
		ICMP6_FILTER_SETPASS(MLDV2_LISTENER_REPORT, &filter);
#endif
	    }
#endif // 0
#endif // HAVE_IPV6_MULTICAST_ROUTING
	    if (setsockopt(_proto_socket, _ip_protocol, ICMP6_FILTER,
			   XORP_SOCKOPT_CAST(&filter), sizeof(filter)) < 0) {
		close_proto_socket(dummy_error_msg);
		error_msg = c_format("setsockopt(ICMP6_FILTER) failed: %s",
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }

    // Assign a method to read from this socket
    if (eventloop().add_ioevent_cb(_proto_socket,
				   IOT_READ,
				   callback(this,
					    &RawSocket::proto_socket_read))
	== false) {
	error_msg = c_format("Cannot add a protocol socket to the set of "
			     "sockets to read from in the event loop");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
RawSocket::close_proto_socket(string& error_msg)
{
    if (! _proto_socket.is_valid()) {
	error_msg = c_format("Invalid protocol socket");
	return (XORP_ERROR);
    }

    // Remove it just in case, even though it may not be select()-ed
    eventloop().remove_ioevent_cb(_proto_socket);

#ifdef HOST_OS_WINDOWS
    switch (family()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	// Reset the pointer to the extension function WSARecvMsg()
	lpWSARecvMsg = NULL;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
#endif // HOST_OS_WINDOWS

    comm_close(_proto_socket);
    _proto_socket.clear();
    
    return (XORP_OK);
}

void
RawSocket::proto_socket_read(XorpFd fd, IoEventType type)
{
    ssize_t	nbytes;
    int		ip_hdr_len = 0;
    int		ip_data_len = 0;
    IPvX	src_address(family());
    IPvX	dst_address(family());
    int		int_val;
    int32_t	ip_ttl = -1;		// a.k.a. Hop-Limit in IPv6
    int32_t	ip_tos = -1;
    bool	is_router_alert = false; // Router Alert option received
    uint32_t	pif_index = 0;

    UNUSED(fd);
    UNUSED(type);
    UNUSED(int_val);

#ifndef HOST_OS_WINDOWS
    // Zero and reset various fields
    _rcvmh.msg_controllen = CMSG_BUF_SIZE;

    // TODO: when resetting _from4 and _from6 do we need to set the address
    // family and the sockaddr len?
    switch (family()) {
    case AF_INET:
	memset(&_from4, 0, sizeof(_from4));
	_rcvmh.msg_namelen = sizeof(_from4);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	memset(&_from6, 0, sizeof(_from6));
	_rcvmh.msg_namelen = sizeof(_from6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	return;			// Error
    }
   
    // Read from the socket
    nbytes = recvmsg(_proto_socket, &_rcvmh, 0);
    if (nbytes < 0) {
	if (errno == EINTR)
	    return;		// OK: restart receiving
	XLOG_ERROR("recvmsg() failed: %s", strerror(errno));
	return;			// Error
    }

#else // HOST_OS_WINDOWS

    switch (family()) {
    case AF_INET:
    {
	struct sockaddr_storage from;
	socklen_t from_len = sizeof(from);

	nbytes = recvfrom(_proto_socket, XORP_BUF_CAST(_rcvbuf0), IO_BUF_SIZE,
			  0, reinterpret_cast<struct sockaddr *>(&from),
			  &from_len);
	debug_msg("Read fd %s, %d bytes\n",
		  _proto_socket.str().c_str(), XORP_INT_CAST(nbytes));
	if (nbytes < 0) {
	    // XLOG_ERROR("recvfrom() failed: %s", strerror(errno));
	    return;			// Error
	}
    }
    break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	WSAMSG mh;
	DWORD error, nrecvd;
	struct sockaddr_in6 from;

	mh.name = (LPSOCKADDR)&from;
	mh.namelen = sizeof(from);
	mh.lpBuffers = (LPWSABUF)_rcviov;
	mh.dwBufferCount = 1;
	mh.Control.len = CMSG_BUF_SIZE;
	mh.Control.buf = (caddr_t)_rcvcmsgbuf;
	mh.dwFlags = 0;

	if (lpWSARecvMsg == NULL) {
	    XLOG_ERROR("lpWSARecvMsg is NULL");
	    return;			// Error
	}
	error = lpWSARecvMsg(_proto_socket, &mh, &nrecvd, NULL, NULL);
	nbytes = (ssize_t)nrecvd;
	debug_msg("Read fd %s, %d bytes\n",
		  _proto_socket.str().c_str(), XORP_INT_CAST(nbytes));
	if (nbytes < 0) {
	    // XLOG_ERROR("lpWSARecvMsg() failed: %s", strerror(errno));
	    return;			// Error
	}
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	return;			// Error
    }
#endif // HOST_OS_WINDOWS
    
    // Check if it is a signal from the kernel to the user-level
    switch (_ip_protocol) {
    case IPPROTO_IGMP:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_WARNING("proto_socket_read(): "
		     "IGMP is unsupported on this platform");
	return;		// Error
#else
	struct igmpmsg *igmpmsg;
	
	igmpmsg = reinterpret_cast<struct igmpmsg *>(_rcvbuf0);
	if (nbytes < (ssize_t)sizeof(*igmpmsg)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal packet size %d is smaller than minimum size %u",
			 XORP_INT_CAST(nbytes),
			 XORP_UINT_CAST(sizeof(*igmpmsg)));
	    return;		// Error
	}
	if (igmpmsg->im_mbz == 0) {
	    //
	    // XXX: Packets sent up from kernel to daemon have
	    //      igmpmsg->im_mbz = ip->ip_p = 0
	    //
	    // TODO: not implemented
	    // kernel_call_process(_rcvbuf0, nbytes);
	    return;		// OK
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case IPPROTO_ICMPV6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	if (nbytes < (ssize_t)sizeof(struct icmp6_hdr)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "packet size %d is smaller than minimum size %u",
			 XORP_INT_CAST(nbytes),
			 XORP_UINT_CAST(sizeof(struct icmp6_hdr)));
	    return;		// Error
	}
#else
	struct mrt6msg *mrt6msg;
	
	mrt6msg = reinterpret_cast<struct mrt6msg *>(_rcvbuf0);
	if ((nbytes < (ssize_t)sizeof(*mrt6msg))
	    && (nbytes < (ssize_t)MLD_MINLEN)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal or packet size %d is smaller than minimum size %u",
			 XORP_INT_CAST(nbytes),
			 XORP_UINT_CAST(min(sizeof(*mrt6msg),
					    (size_t)MLD_MINLEN)));
	    return;		// Error
	}
	if ((mrt6msg->im6_mbz == 0) || (_rcvmh.msg_controllen == 0)) {
	    //
	    // XXX: Packets sent up from kernel to daemon have
	    //      mrt6msg->im6_mbz = icmp6_hdr->icmp6_type = 0
	    // Because we set ICMP6 filters on the socket,
	    // we should never see a real ICMPv6 packet
	    // with icmp6_type = 0 .
	    //
	    //
	    // TODO: XXX: (msg_controllen == 0) is presumably
	    // true for older IPv6 systems (e.g. KAME circa
	    // April 2000, FreeBSD-4.0) which don't have the
	    //     'icmp6_type = 0' mechanism.
	    //
	    // TODO: not implemented
	    // kernel_call_process(_rcvbuf0, nbytes);
	    return;		// OK
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	break;
    }
    
    //
    // Not a kernel signal. Pass to the registered processing function
    //

    //
    // Input check.
    // Get source and destination address, IP TTL (a.k.a. hop-limit),
    // and (eventually) interface address (IPv6 only).
    //
    switch (family()) {
    case AF_INET:
    {
	struct ip *ip;
	bool is_datalen_error = false;
	
	// Input check
	ip = reinterpret_cast<struct ip *>(_rcvbuf0);
	if (nbytes < (ssize_t)sizeof(*ip)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "packet size %d is smaller than minimum size %u",
			 XORP_INT_CAST(nbytes),
			 XORP_UINT_CAST(sizeof(*ip)));
	    return;		// Error
	}
#ifndef HOST_OS_WINDOWS
	// TODO: get rid of this and always use ip->ip_src ??
	src_address.copy_in(_from4);
#else
	src_address.copy_in(ip->ip_src);
#endif
	dst_address.copy_in(ip->ip_dst);
	ip_ttl = ip->ip_ttl;
	ip_tos = ip->ip_tos;
	
	ip_hdr_len  = ip->ip_hl << 2;
#ifdef IPV4_RAW_INPUT_IS_RAW
	ip_data_len = ntohs(ip->ip_len) - ip_hdr_len;
#else
	ip_data_len = ip->ip_len;
#endif // ! IPV4_RAW_INPUT_IS_RAW
	// Check length
	is_datalen_error = false;
	do {
	    if (ip_hdr_len + ip_data_len == nbytes) {
		is_datalen_error = false;
		break;		// OK
	    }
	    if (nbytes < ip_hdr_len) {
		is_datalen_error = true;
		break;
	    }
	    if (ip->ip_p == IPPROTO_PIM) {
		struct pim *pim;
		if (nbytes < ip_hdr_len + PIM_REG_MINLEN) {
		    is_datalen_error = true;
		    break;
		}
		pim = reinterpret_cast<struct pim *>((uint8_t *)ip + ip_hdr_len);
		if (PIM_VT_T(pim->pim_vt) != PIM_REGISTER) {
		    is_datalen_error = true;
		    break;
		}
		//
		// XXX: the *BSD kernel might truncate the encapsulated data
		// in PIM Register messages before passing the message up
		// to user level. The reason for the truncation is to reduce
		// the bandwidth overhead, but the price for this is
		// messages with weird IP header. Sigh...
		//
		is_datalen_error = false;
		break;
	    }
	    break;
	} while (false);
	if (is_datalen_error) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet size from %s to %s with %d bytes instead of "
		       "hdr+datalen=%d+%d=%d",
		       cstring(src_address), cstring(dst_address),
		       XORP_INT_CAST(nbytes),
		       ip_hdr_len, ip_data_len, ip_hdr_len + ip_data_len);
	    return;		// Error
	}

#ifndef HOST_OS_WINDOWS
	for (struct cmsghdr *cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_FIRSTHDR(&_rcvmh));
	     cmsgp != NULL;
	     cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_NXTHDR(&_rcvmh, cmsgp))) {
	    if (cmsgp->cmsg_level != IPPROTO_IP)
		continue;
	    switch (cmsgp->cmsg_type) {
#ifdef IP_RECVIF
	    case IP_RECVIF:
	    {
		struct sockaddr_dl *sdl = NULL;
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct sockaddr_dl)))
		    continue;
		sdl = reinterpret_cast<struct sockaddr_dl *>(CMSG_DATA(cmsgp));
		pif_index = sdl->sdl_index;
	    }
	    break;
#endif // IP_RECVIF
	    default:
		break;
	    }
	}
#endif // ! HOST_OS_WINDOWS
	
	//
	// Check for Router Alert option
	//
	do {
	    uint8_t *option_p = (uint8_t *)(ip + 1);
	    uint8_t option_value, option_len;
	    uint32_t test_ip_options_len = ip_hdr_len - sizeof(*ip);
	    while (test_ip_options_len) {
		if (test_ip_options_len < 4)
		    break;
		option_value = *option_p;
		option_len = *(option_p + 1);
		if (test_ip_options_len < option_len)
		    break;
		if ((option_value == IPOPT_RA) && (option_len == 4)) {
		    is_router_alert = true;
		    break;
		}
		test_ip_options_len -= option_len;
		option_p += option_len;
	    }
	    
	    break;
	} while (false);
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_pktinfo *pi = NULL;
	
	src_address.copy_in(_from6);
	if (_rcvmh.msg_flags & MSG_CTRUNC) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s with size of %d bytes is truncated",
		       cstring(src_address),
		       XORP_INT_CAST(nbytes));
	    return;		// Error
	}
	size_t controllen =  static_cast<size_t>(_rcvmh.msg_controllen);
	if (controllen < sizeof(struct cmsghdr)) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s has too short msg_controllen "
		       "(%u instead of %u)",
		       cstring(src_address),
		       XORP_UINT_CAST(controllen),
		       XORP_UINT_CAST(sizeof(struct cmsghdr)));
	    return;		// Error
	}
	
	//
	// Get pif_index, hop limit, Router Alert option, etc.
	//
	for (struct cmsghdr *cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_FIRSTHDR(&_rcvmh));
	     cmsgp != NULL;
	     cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_NXTHDR(&_rcvmh, cmsgp))) {
	    if (cmsgp->cmsg_level != IPPROTO_IPV6)
		continue;
	    switch (cmsgp->cmsg_type) {
	    case IPV6_PKTINFO:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct in6_pktinfo)))
		    continue;
		pi = reinterpret_cast<struct in6_pktinfo *>(CMSG_DATA(cmsgp));
		pif_index = pi->ipi6_ifindex;
		dst_address.copy_in(pi->ipi6_addr);
		break;
	    case IPV6_HOPLIMIT:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		int_val = *((int *)CMSG_DATA(cmsgp));
		ip_ttl = int_val;
		break;
	    case IPV6_HOPOPTS:
		{
		    //
		    // Check for Router Alert option
		    //
#ifdef HAVE_RFC3542
		    {
			struct ip6_hbh *ext;
			int currentlen;
			u_int8_t type;
			size_t extlen;
			socklen_t len;
			void *databuf;
			
			ext = reinterpret_cast<struct ip6_hbh *>(CMSG_DATA(cmsgp));
			extlen = (ext->ip6h_len + 1) * 8;
			currentlen = 0;
			while (true) {
			    currentlen = inet6_opt_next(ext, extlen,
							currentlen,
							&type, &len, &databuf);
			    if (currentlen == -1)
				break;
			    if (type == IP6OPT_ROUTER_ALERT) {
				is_router_alert = true;
				break;
			    }
			}
		    }
#else // ! HAVE_RFC3542 (i.e., the old advanced API)
		    {
#ifdef HAVE_IPV6_MULTICAST_ROUTING
			//
			// TODO: XXX: temporary use HAVE_IPV6_MULTICAST_ROUTING
			// to conditionally compile, because Linux doesn't
			// have inet6_option_*
			//
			uint8_t  *tptr = NULL;
			while (inet6_option_next(cmsgp, &tptr) == 0) {
			    if (*tptr == IP6OPT_ROUTER_ALERT) {
				is_router_alert = true;
				break;
			    }
			}
#endif // HAVE_IPV6_MULTICAST_ROUTING
		    }
#endif // ! HAVE_RFC3542
		    
		}
		
		break;
#ifdef IPV6_TCLASS
	    case IPV6_TCLASS:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		int_val = *((int *)CMSG_DATA(cmsgp));
		ip_tos = int_val;
		break;
#endif // IPV6_TCLASS
	    default:
		break;
	    }
	}
	
	ip_hdr_len = 0;
	ip_data_len = nbytes;
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return;			// Error
    }
    
    // Various checks
    if (! src_address.is_unicast()) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid unicast sender address: %s", cstring(src_address));
	return;			// Error
    }
    if (! (dst_address.is_multicast() || dst_address.is_unicast())) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid destination address: %s", cstring(dst_address));
	return;			// Error
    }
    if (ip_ttl < 0) {
	// TODO: what about ip_ttl = 0? Is it OK?
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid TTL (hop-limit) from %s to %s: %d",
		   cstring(src_address), cstring(dst_address), ip_ttl);
	return;			// Error
    }
    if (pif_index == 0) {
	switch (family()) {
	case AF_INET:
	    // TODO: take care of Linux??
	    break;	// XXX: in IPv4 (except Linux?) there is no pif_index
#ifdef HAVE_IPV6
	case AF_INET6:
	    XLOG_ERROR("proto_socket_read() failed: "
		       "invalid interface pif_index from %s to %s: %u",
		       cstring(src_address), cstring(dst_address),
		       XORP_UINT_CAST(pif_index));
	    return;		// Error
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    return;		// Error
	}
    }

    //
    // Find the interface and the vif this message was received on.
    // XXX: note that in case of IPv4 (except Linux?) we may be able
    // only to "guess" by using the sender or the destination address.
    //
    const IfTreeInterface* iftree_if = NULL;
    const IfTreeVif* iftree_vif = NULL;
    do {
	//
	// Find interface and vif based on src address, if a directly
	// connected sender, or based on dst address if unicast and one
	// of my local addresses.
	// However, check first whether we can use 'pif_index' instead.
	//
	if (pif_index != 0) {
	    find_interface_vif_by_pif_index(pif_index, iftree_if, iftree_vif);
	    break;
	}
	
	if (dst_address.is_multicast()) {
	    find_interface_vif_same_subnet_or_p2p(src_address, iftree_if,
						  iftree_vif);
	} else {
	    find_interface_vif_by_addr(dst_address, iftree_if, iftree_vif);
	}
	break;
    } while (false);
    
    if ((iftree_if == NULL) || (iftree_vif == NULL)) {
	// No vif found. Ignore this packet.
	XLOG_WARNING("proto_socket_read() failed: "
		     "RX packet from %s to %s: no vif found",
		     cstring(src_address), cstring(dst_address));
	return;			// Error
    }
    if (! (iftree_if->enabled() || iftree_vif->enabled())) {
	// This vif is down. Silently ignore this packet.
	return;			// Error
    }
    
    // Process the result
    vector<uint8_t> payload(nbytes - ip_hdr_len);
    memcpy(&payload[0], _rcvbuf0 + ip_hdr_len, nbytes - ip_hdr_len);
    process_recv_data(iftree_if->ifname(),
		      iftree_vif->vifname(),
		      src_address, dst_address, ip_ttl, ip_tos,
		      is_router_alert,
		      payload);

    return;			// OK
}

int
RawSocket::proto_socket_write(const string& if_name,
			      const string& vif_name,
			      const IPvX& src_address,
			      const IPvX& dst_address,
			      int32_t ip_ttl,
			      int32_t ip_tos,
			      bool is_router_alert,
			      const vector<uint8_t>& payload,
			      string& error_msg)
{
    struct ip	*ip;
    uint32_t	ip_option;
    uint32_t	*ip_option_p;
    int		ip_hdr_len = 0;
    int		int_val;
    int		setloop = false;
    int		ret;
    const IfTreeInterface* iftree_if = NULL;
    const IfTreeVif* iftree_vif = NULL;

    UNUSED(int_val);

    find_interface_vif_by_name(if_name, vif_name, iftree_if, iftree_vif);
    
    if (iftree_if == NULL) {
	error_msg = c_format("No interface %s", if_name.c_str());
	return (XORP_ERROR);
    }	
    if (iftree_vif == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }
    if (! iftree_if->enabled()) {
	error_msg = c_format("Interface %s is down",
			     iftree_if->ifname().c_str());
	return (XORP_ERROR);
    }
    if (! iftree_vif->enabled()) {
	error_msg = c_format("Interface %s vif %s is down",
			     iftree_if->ifname().c_str(),
			     iftree_vif->vifname().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Assign the TTL and TOS if they were not specified
    //
    switch (family()) {
    case AF_INET:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (is_router_alert)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (is_router_alert)
		ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
	    else
		ip_tos = 0;
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (is_router_alert)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (is_router_alert) {
		// TODO: XXX: IPTOS for IPv6 is bogus??
		// ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
		ip_tos = 0;
	    } else {
		ip_tos = 0;
	    }
	}
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    //
    // Setup the IP header (including the Router Alert option, if specified)
    // In case of IPv4, the IP header and the data are specified as
    // two entries to the sndiov[] scatter/gatter array.
    // In case of IPv6, the IP header information is specified as
    // ancillary data.
    //
    switch (family()) {
    case AF_INET:
	ip = reinterpret_cast<struct ip *>(_sndbuf0);
	if (is_router_alert) {
	    // Include the Router Alert option
	    ip_option	= htonl((IPOPT_RA << 24) | (0x04 << 16));
	    ip_option_p	= (uint32_t *)(ip + 1);
	    *ip_option_p = ip_option;
	    ip_hdr_len	= (sizeof(*ip) + sizeof(*ip_option_p));
	} else {
	    ip_hdr_len	= sizeof(*ip);
	}
	ip->ip_hl	= ip_hdr_len >> 2;
	ip->ip_v	= IPVERSION;
	ip->ip_id	= 0;				// Let kernel fill in
	ip->ip_off	= 0x0;
	ip->ip_p	= _ip_protocol;
	ip->ip_ttl	= ip_ttl;
	ip->ip_tos	= ip_tos;

	//
	// XXX: we need to use a temporary in_addr storage as a work-around
	// if "struct ip" is __packed
	//
	struct in_addr in_addr_tmp;
	src_address.copy_out(in_addr_tmp);
	ip->ip_src = in_addr_tmp;
	dst_address.copy_out(in_addr_tmp);
	ip->ip_dst = in_addr_tmp;
	ip->ip_len = (ip->ip_hl << 2) + payload.size();
#ifdef IPV4_RAW_OUTPUT_IS_RAW
	ip->ip_len = htons(ip->ip_len);
#endif
	ip->ip_sum = 0;					// Let kernel fill in
	
	// Now hook the data
#ifndef HOST_OS_WINDOWS
	dst_address.copy_out(_to4);
	_sndmh.msg_namelen	= sizeof(_to4);
	_sndmh.msg_iovlen	= 2;
	_sndmh.msg_control	= NULL;
	_sndmh.msg_controllen	= 0;
#endif // ! HOST_OS_WINDOWS
	_sndiov[0].iov_len	= ip_hdr_len;
	if (payload.size() > IO_BUF_SIZE) {
	    error_msg = c_format("proto_socket_write() failed: "
				 "cannot send packet on interface %s vif %s "
				 "from %s to %s: "
				 "too much data: %u octets (max = %u)",
				 if_name.c_str(),
				 vif_name.c_str(),
				 src_address.str().c_str(),
				 dst_address.str().c_str(),
				 XORP_UINT_CAST(payload.size()),
				 XORP_UINT_CAST(IO_BUF_SIZE));
	    return (XORP_ERROR);
	}
    	memcpy(_sndbuf1, &payload[0], payload.size()); // XXX: goes to _sndiov[1].iov_base
	// _sndiov[1].iov_base	= (caddr_t)&payload[0];
	_sndiov[1].iov_len	= payload.size();
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
    {
	int ctllen = 0;
	int hbhlen = 0;
	struct cmsghdr *cmsgp;
	struct in6_pktinfo *sndpktinfo;
	
	//
	// XXX: unlikely IPv4, in IPv6 the 'header' is specified as
	// ancillary data.
	//
	
	//
	// First, estimate total length of ancillary data
	//
	// Space for IPV6_PKTINFO
	ctllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
	
	if (is_router_alert) {
	    // Space for Router Alert option
#ifdef HAVE_RFC3542
	    if ((hbhlen = inet6_opt_init(NULL, 0)) == -1) {
		error_msg = c_format("inet6_opt_init(NULL) failed");
		return (XORP_ERROR);
	    }
	    if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen,
					   IP6OPT_ROUTER_ALERT, 2,
					   2, NULL)) == -1) {
		error_msg = c_format("inet6_opt_append(NULL) failed");
		return (XORP_ERROR);
	    }
	    if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1) {
		error_msg = c_format("inet6_opt_finish(NULL) failed");
		return (XORP_ERROR);
	    }
	    ctllen += CMSG_SPACE(hbhlen);
#else
#ifdef HAVE_IPV6_MULTICAST_ROUTING
	    //
	    // TODO: XXX: temporary use HAVE_IPV6_MULTICAST_ROUTING
	    // to conditionally compile, because Linux doesn't
	    // have inet6_option_*
	    //
	    hbhlen = inet6_option_space(sizeof(raopt));
	    ctllen += hbhlen;
#else
	    UNUSED(hbhlen);
#endif
#endif // ! HAVE_RFC3542
	}
	// Space for IPV6_TCLASS
#ifdef IPV6_TCLASS 
	ctllen += CMSG_SPACE(sizeof(int));
#endif
	// Space for IPV6_HOPLIMIT
	ctllen += CMSG_SPACE(sizeof(int));
	XLOG_ASSERT(ctllen <= CMSG_BUF_SIZE);   // XXX
	
	//
	// Now setup the ancillary data
	//
	_sndmh.msg_controllen = ctllen;
	cmsgp = CMSG_FIRSTHDR(&_sndmh);
	
	// Add the IPV6_PKTINFO ancillary data
	cmsgp->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type  = IPV6_PKTINFO;
	sndpktinfo = reinterpret_cast<struct in6_pktinfo *>(CMSG_DATA(cmsgp));
	memset(sndpktinfo, 0, sizeof(*sndpktinfo));
	if ((iftree_if->pif_index() > 0)
	    && !(dst_address.is_unicast()
		 && !dst_address.is_linklocal_unicast())) {
	    //
	    // XXX: don't set the outgoing interface index if we are
	    // sending an unicast packet to a non-link local unicast address.
	    // Otherwise, the sending may fail with EHOSTUNREACH error.
	    //
	    sndpktinfo->ipi6_ifindex = iftree_if->pif_index();
	} else {
	    sndpktinfo->ipi6_ifindex = 0;		// Let kernel fill in
	}
	src_address.copy_out(sndpktinfo->ipi6_addr);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Include the Router Alert option if needed
	//
	if (is_router_alert) {
#ifdef HAVE_RFC3542
	    int currentlen;
	    void *hbhbuf, *optp = NULL;
	    
	    cmsgp->cmsg_len   = CMSG_LEN(hbhlen);
	    cmsgp->cmsg_level = IPPROTO_IPV6;
	    cmsgp->cmsg_type  = IPV6_HOPOPTS;
	    hbhbuf = CMSG_DATA(cmsgp);
	    currentlen = inet6_opt_init(hbhbuf, hbhlen);
	    if (currentlen == -1) {
		error_msg = c_format("inet6_opt_init(len = %d) failed",
				     hbhlen);
		return (XORP_ERROR);
	    }
	    currentlen = inet6_opt_append(hbhbuf, hbhlen, currentlen,
					  IP6OPT_ROUTER_ALERT, 2, 2, &optp);
	    if (currentlen == -1) {
		error_msg = c_format("inet6_opt_append(len = %d) failed",
				     currentlen);
		return (XORP_ERROR);
	    }
	    inet6_opt_set_val(optp, 0, &rtalert_code, sizeof(rtalert_code));
	    if (inet6_opt_finish(hbhbuf, hbhlen, currentlen) == -1) {
		error_msg = c_format("inet6_opt_finish(len = %d) failed",
				     currentlen);
		return (XORP_ERROR);
	    }
	    
#else  // ! HAVE_RFC3542 (i.e., the old advanced API)

#ifdef HAVE_IPV6_MULTICAST_ROUTING
	    //
	    // TODO: XXX: temporary use HAVE_IPV6_MULTICAST_ROUTING
	    // to conditionally compile, because Linux doesn't
	    // have inet6_option_*
	    //
	    if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS)) {
		error_msg = c_format("inet6_option_init(IPV6_HOPOPTS) failed");
		return (XORP_ERROR);
	    }
	    if (inet6_option_append(cmsgp, raopt, 4, 0)) {
		error_msg = c_format("inet6_option_append(Router Alert) failed");
		return (XORP_ERROR);
	    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
	    
#endif // ! HAVE_RFC3542
	    
	    cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	}
	
	//
	// Set the TTL
	//
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_HOPLIMIT;
	int_val = ip_ttl;
	*(int *)(CMSG_DATA(cmsgp)) = int_val;
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Set the TOS
	//
#ifdef IPV6_TCLASS
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_TCLASS;
	int_val = ip_tos;
	*(int *)(CMSG_DATA(cmsgp)) = int_val;
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
#endif // IPV6_TCLASS
	
	//
	// Now hook the data
	//
#ifndef HOST_OS_WINDOWS
	dst_address.copy_out(_to6);
	_sndmh.msg_namelen  = sizeof(_to6);
	_sndmh.msg_iovlen   = 1;
#endif // ! HOST_OS_WINDOWS
	if (payload.size() > IO_BUF_SIZE) {
	    error_msg = c_format("proto_socket_write() failed: "
				 "error sending packet on interface %s vif %s "
				 "from %s to %s: "
				 "too much data: %u octets (max = %u)",
				 if_name.c_str(),
				 vif_name.c_str(),
				 src_address.str().c_str(),
				 dst_address.str().c_str(),
				 XORP_UINT_CAST(payload.size()),
				 XORP_UINT_CAST(IO_BUF_SIZE));
	    return (XORP_ERROR);
	}
	memcpy(_sndbuf0, &payload[0], payload.size()); // XXX: goes to _sndiov[0].iov_base
	// _sndiov[0].iov_base	= (caddr_t)&payload[0];
	_sndiov[0].iov_len  = payload.size();
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    setloop = false;
    if (dst_address.is_multicast()) {
	if (set_default_multicast_interface(if_name, vif_name, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	//
	// XXX: we need to enable the multicast loopback so other processes
	// on the same host can receive the multicast packets.
	//
	if (enable_multicast_loopback(true, error_msg) != XORP_OK) {
	    return (XORP_ERROR);
	}
	setloop = true;
    }

#ifndef HOST_OS_WINDOWS    
    ret = XORP_OK;
    if (sendmsg(_proto_socket, &_sndmh, 0) < 0) {
	ret = XORP_ERROR;
	if (errno == ENETDOWN) {
	    // TODO: check the interface status. E.g. vif_state_check(family);
	} else {
	    error_msg = c_format("sendmsg(proto %d size %u from %s to %s on "
				 "interface %s vif %s) "
				 "failed: %s",
				 _ip_protocol,
				 XORP_UINT_CAST(payload.size()),
				 cstring(src_address),
				 cstring(dst_address),
				 if_name.c_str(),
				 vif_name.c_str(),
				 strerror(errno));
	}
    }

#else // HOST_OS_WINDOWS

    ret = XORP_OK;
    switch (family()) {
    case AF_INET:
    {
	struct sockaddr_in to;
	DWORD sent, error;

	dst_address.copy_out(to);
	error = WSASendTo(_proto_socket, reinterpret_cast<WSABUF *>(_sndiov),
			  2, &sent, 0,
			  reinterpret_cast<struct sockaddr *>(&to),
			  sizeof(to), NULL, NULL);
	if (error != 0) {
	    ret = XORP_ERROR;
	}
    }
    break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct sockaddr_in6 to;
	DWORD sent, error;

	dst_address.copy_out(to);
	error = WSASendTo(_proto_socket, reinterpret_cast<WSABUF *>(_sndiov),
			  1, &sent, 0,
			  reinterpret_cast<struct sockaddr *>(&to),
			  sizeof(to), NULL, NULL);
	if (error != 0) {
	    ret = XORP_ERROR;
	}
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }

#endif // HOST_OS_WINDOWS

    if (setloop) {
	string dummy_error_msg;
	enable_multicast_loopback(false, dummy_error_msg);
    }
    
    return (ret);
}

bool
RawSocket::find_interface_vif_by_name(const string& if_name,
				      const string& vif_name,
				      const IfTreeInterface*& iftree_if,
				      const IfTreeVif*& iftree_vif) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;

    iftree_if = NULL;
    iftree_vif = NULL;

    // Find the interface
    ii = _iftree.get_if(if_name);
    if (ii == _iftree.ifs().end())
	return (false);
    const IfTreeInterface& fi = ii->second;

    // Find the vif
    vi = fi.get_vif(vif_name);
    if (vi == fi.vifs().end())
	return (false);
    const IfTreeVif& fv = vi->second;

    // Found a match
    iftree_if = &fi;
    iftree_vif = &fv;
    return (true);
}

bool
RawSocket::find_interface_vif_by_pif_index(uint32_t pif_index,
					   const IfTreeInterface*& iftree_if,
					   const IfTreeVif*& iftree_vif) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;

    iftree_if = NULL;
    iftree_vif = NULL;

    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	if (fi.pif_index() != pif_index)
	    continue;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;

	    if (fv.pif_index() == pif_index) {
		// Found a match
		iftree_if = &fi;
		iftree_vif = &fv;
		return (true);
	    }
	}
    }

    return (false);
}

bool
RawSocket::find_interface_vif_same_subnet_or_p2p(
    const IPvX& addr,
    const IfTreeInterface*& iftree_if,
    const IfTreeVif*& iftree_vif) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai4;
    IfTreeVif::V6Map::const_iterator ai6;

    iftree_if = NULL;
    iftree_vif = NULL;

    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;

	    if (addr.is_ipv4()) {
		IPv4 addr4 = addr.get_ipv4();
		for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		    const IfTreeAddr4& a4 = ai4->second;

		    // Test if same subnet
		    IPv4Net subnet(a4.addr(), a4.prefix_len());
		    if (subnet.contains(addr4)) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }

		    // Test if same p2p
		    if (! a4.point_to_point())
			continue;
		    if ((a4.addr() == addr4) || (a4.endpoint() == addr4)) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }
		}
		continue;
	    }

	    if (addr.is_ipv6()) {
		IPv6 addr6 = addr.get_ipv6();
		for (ai6 = fv.v6addrs().begin(); ai6 != fv.v6addrs().end(); ++ai6) {
		    const IfTreeAddr6& a6 = ai6->second;

		    // Test if same subnet
		    IPv6Net subnet(a6.addr(), a6.prefix_len());
		    if (subnet.contains(addr6)) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }

		    // Test if same p2p
		    if (! a6.point_to_point())
			continue;
		    if ((a6.addr() == addr6) || (a6.endpoint() == addr6)) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }
		}
		continue;
	    }
	}
    }

    return (false);
}

bool
RawSocket::find_interface_vif_by_addr(
    const IPvX& addr,
    const IfTreeInterface*& iftree_if,
    const IfTreeVif*& iftree_vif) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai4;
    IfTreeVif::V6Map::const_iterator ai6;

    iftree_if = NULL;
    iftree_vif = NULL;

    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;

	    if (addr.is_ipv4()) {
		IPv4 addr4 = addr.get_ipv4();
		for (ai4 = fv.v4addrs().begin(); ai4 != fv.v4addrs().end(); ++ai4) {
		    const IfTreeAddr4& a4 = ai4->second;

		    if (a4.addr() == addr4) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }
		}
		continue;
	    }

	    if (addr.is_ipv6()) {
		IPv6 addr6 = addr.get_ipv6();
		for (ai6 = fv.v6addrs().begin(); ai6 != fv.v6addrs().end(); ++ai6) {
		    const IfTreeAddr6& a6 = ai6->second;

		    if (a6.addr() == addr6) {
			// Found a match
			iftree_if = &fi;
			iftree_vif = &fv;
			return (true);
		    }
		}
		continue;
	    }
	}
    }

    return (false);
}
