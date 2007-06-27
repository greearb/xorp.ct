// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/io/io_ip_socket.cc,v 1.8 2007/06/21 08:11:11 pavlin Exp $"

//
// I/O IP raw socket support.
//

#include "fea/fea_module.h"

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
#include "libxorp/win_io.h"
#include "fea/ip.h"
#endif

#include "libcomm/comm_api.h"

#include "libproto/packet.hh"

#include "mrt/include/ip_mroute.h"

// XXX: _PIM_VT is needed if we want the extra features of <netinet/pim.h>
#define _PIM_VT 1
#if defined(HAVE_NETINET_PIM_H) && defined(HAVE_STRUCT_PIM_PIM_VT)
#include <netinet/pim.h>
#else
#include "mrt/include/netinet/pim.h"
#endif

#include "fea/data_plane/control_socket/system_utilities.hh"

#include "fea/iftree.hh"
#include "io_ip_socket.hh"


//
// Local constants definitions
//
#define IO_BUF_SIZE		(64*1024)  // I/O buffer(s) size
#define CMSG_BUF_SIZE		(10*1024)  // 'rcvcmsgbuf' and 'sndcmsgbuf'
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  // Min. rcv socket buffer size
#define SO_RCV_BUF_SIZE_MAX	(256*1024) // Desired rcv socket buffer size
#define SO_SND_BUF_SIZE_MIN	(48*1024)  // Min. snd socket buffer size
#define SO_SND_BUF_SIZE_MAX	(256*1024) // Desired snd socket buffer size

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
#  ifdef HAVE_STRUCT_MLD_HDR
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

typedef INT (WINAPI * LPFN_WSASENDMSG)(SOCKET, LPWSAMSG, DWORD, LPDWORD,
				       LPWSAOVERLAPPED,
				       LPWSAOVERLAPPED_COMPLETION_ROUTINE);

#define WSAID_WSARECVMSG \
	{ 0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22} }

#define WSAID_WSASENDMSG \
	{ 0xa441e712,0x754f,0x43ca,{0x84,0xa7,0x0d,0xee,0x44,0xcf,0x60,0x6d} }

#else // ! __MINGW32__

#define CMSG_FIRSTHDR(msg)	WSA_CMSG_FIRSTHDR(msg)
#define CMSG_NXTHDR(msg, cmsg)	WSA_CMSG_NEXTHDR(msg, cmsg)
#define CMSG_DATA(cmsg)		WSA_CMSG_DATA(cmsg)
#define CMSG_SPACE(len)		WSA_CMSG_SPACE(len)
#define CMSG_LEN(len)		WSA_CMSG_LEN(len)

#endif // ! __MINGW32__

#ifdef HAVE_IPV6

static const GUID guidWSARecvMsg = WSAID_WSARECVMSG;
static const GUID guidWSASendMsg = WSAID_WSASENDMSG;
static LPFN_WSARECVMSG lpWSARecvMsg = NULL;
static LPFN_WSASENDMSG lpWSASendMsg = NULL; // Windows Longhorn and up

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
#define IPOPT_RA			148	// 0x94
#endif
static uint32_t		ra_opt4;

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
static uint16_t		rtalert_code6;
#ifndef HAVE_RFC3542
static uint8_t		ra_opt6[IP6OPT_RTALERT_LEN];
#endif
#endif // HAVE_IPV6

// XXX: This is a bit of a dirty hack
#ifdef HOST_OS_WINDOWS
#ifdef strerror
#undef strerror
#endif
#define strerror(errnum) win_strerror(GetLastError())
#endif


IoIpSocket::IoIpSocket(EventLoop& eventloop, const IfTree& iftree,
		       int init_family, uint8_t ip_protocol)
    : _eventloop(eventloop),
      _iftree(iftree),
      _family(init_family),
      _ip_protocol(ip_protocol),
      _is_ip_hdr_included(false),
      _ip_id(xorp_random())
{
    // Init Router Alert related option stuff
    ra_opt4 = htonl((IPOPT_RA << 24) | (0x04 << 16));
#ifdef HAVE_IPV6
    rtalert_code6 = htons(IP6OPT_RTALERT_MLD); // XXX: used by MLD only (?)
#ifndef HAVE_RFC3542
    ra_opt6[0] = IP6OPT_ROUTER_ALERT;
    ra_opt6[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&ra_opt6[2], (caddr_t)&rtalert_code6, sizeof(rtalert_code6));
#endif // ! HAVE_RFC3542
#endif // HAVE_IPV6
    
    // Allocate the buffers
    _rcvbuf = new uint8_t[IO_BUF_SIZE];
    _sndbuf = new uint8_t[IO_BUF_SIZE];
    _rcvcmsgbuf = new uint8_t[CMSG_BUF_SIZE];
    _sndcmsgbuf = new uint8_t[CMSG_BUF_SIZE];

    // Scatter/gatter array initialization
    _rcviov[0].iov_base		= (caddr_t)_rcvbuf;
    _rcviov[0].iov_len		= IO_BUF_SIZE;
    _sndiov[0].iov_base		= (caddr_t)_sndbuf;
    _sndiov[0].iov_len		= 0;

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

IoIpSocket::~IoIpSocket()
{
    string dummy_error_msg;

    stop(dummy_error_msg);

    // Free the buffers
    delete[] _rcvbuf;
    delete[] _sndbuf;
    delete[] _rcvcmsgbuf;
    delete[] _sndcmsgbuf;
}

int
IoIpSocket::start(string& error_msg)
{
    return (open_proto_sockets(error_msg));
}

int
IoIpSocket::stop(string& error_msg)
{
    return (close_proto_sockets(error_msg));
}

int
IoIpSocket::enable_ip_hdr_include(bool is_enabled, string& error_msg)
{
    UNUSED(is_enabled);

    switch (family()) {
    case AF_INET:
    {
#ifdef IP_HDRINCL
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled; 
	
	if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_HDRINCL,
		       XORP_SOCKOPT_CAST(&bool_flag),
		       sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IP_HDRINCL, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
	_is_ip_hdr_included = is_enabled;
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
}

int
IoIpSocket::enable_recv_pktinfo(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled;
	
	//
	// Interface index
	//
#ifdef IP_RECVIF
	// XXX: BSD
	if (setsockopt(_proto_socket_in, IPPROTO_IP, IP_RECVIF,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_RECVIF, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IP_RECVIF

#ifdef IP_PKTINFO
	// XXX: Linux
	if (setsockopt(_proto_socket_in, IPPROTO_IP, IP_PKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_PKTINFO, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IP_PKTINFO

	UNUSED(bool_flag);
	break;
    }

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
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVPKTINFO, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	// The old option (see RFC-2292)
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_PKTINFO,
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
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVHOPLIMIT, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_HOPLIMIT,
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
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVTCLASS,
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
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVHOPOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVHOPOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_HOPOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_HOPOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPOPTS

	//
	// Routing header options
	//
#ifdef IPV6_RECVRTHDR
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVRTHDR,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVRTHDR, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RTHDR,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RTHDR, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVRTHDR

	//
	// Destination options
	//
#ifdef IPV6_RECVDSTOPTS
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_RECVDSTOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVDSTOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_DSTOPTS,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_DSTOPTS, %u) failed: %s",
				 bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVDSTOPTS
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
IoIpSocket::set_multicast_ttl(int ttl, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	u_char ip_ttl = ttl; // XXX: In IPv4 the value argument is 'u_char'
	
	if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_MULTICAST_TTL,
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
	
	if (setsockopt(_proto_socket_out, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
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
IoIpSocket::enable_multicast_loopback(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	u_char loop = is_enabled;
	
	if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_MULTICAST_LOOP,
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
	
	if (setsockopt(_proto_socket_out, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
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
IoIpSocket::set_default_multicast_interface(const string& if_name,
					    const string& vif_name,
					    string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = _iftree.find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Setting the default multicast interface failed:"
			     "interface %s vif %s not found",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }

    switch (family()) {
    case AF_INET:
    {
	struct in_addr in_addr;

	// Find the first address
	IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	if (ai == vifp->ipv4addrs().end()) {
	    error_msg = c_format("Setting the default multicast interface "
				 "failed: "
				 "interface %s vif %s has no address",
				 if_name.c_str(), vif_name.c_str());
	    return (XORP_ERROR);
	}
	const IfTreeAddr4& fa = ai->second;

	fa.addr().copy_out(in_addr);
	if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_MULTICAST_IF,
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
	u_int pif_index = vifp->pif_index();
	
	if (setsockopt(_proto_socket_out, IPPROTO_IPV6, IPV6_MULTICAST_IF,
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
IoIpSocket::join_multicast_group(const string& if_name,
				 const string& vif_name,
				 const IPvX& group,
				 string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = _iftree.find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Joining multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
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
	IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	if (ai == vifp->ipv4addrs().end()) {
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

	if (setsockopt(_proto_socket_in, IPPROTO_IP, IP_ADD_MEMBERSHIP,
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
	mreq6.ipv6mr_interface = vifp->pif_index();
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_JOIN_GROUP,
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
IoIpSocket::leave_multicast_group(const string& if_name,
				  const string& vif_name,
				  const IPvX& group,
				  string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = _iftree.find_vif(if_name, vif_name);
    if (vifp == NULL) {
	error_msg = c_format("Leaving multicast group %s failed: "
			     "interface %s vif %s not found",
			     cstring(group),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    
#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
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
	IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	if (ai == vifp->ipv4addrs().end()) {
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
	if (setsockopt(_proto_socket_in, IPPROTO_IP, IP_DROP_MEMBERSHIP,
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
	mreq6.ipv6mr_interface = vifp->pif_index();
	if (setsockopt(_proto_socket_in, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
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
IoIpSocket::open_proto_sockets(string& error_msg)
{
    string dummy_error_msg;

    if (_proto_socket_in.is_valid() && _proto_socket_out.is_valid())
	return (XORP_OK);
    
    // If necessary, open the protocol sockets
    if (! _proto_socket_in.is_valid()) {
	_proto_socket_in = socket(family(), SOCK_RAW, _ip_protocol);
	if (!_proto_socket_in.is_valid()) {
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
    }

    if (! _proto_socket_out.is_valid()) {
	_proto_socket_out = socket(family(), SOCK_RAW, _ip_protocol);
	if (!_proto_socket_out.is_valid()) {
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
    }

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

	    result = WSAIoctl(_proto_socket_in,
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

	// Obtain the pointer to the extension function WSASendMsg() if needed
	// XXX: Only available on Windows Longhorn.
	if (lpWSASendMsg == NULL) {
	    int result;
	    DWORD nbytes;

	    result = WSAIoctl(_proto_socket_in,
			      SIO_GET_EXTENSION_FUNCTION_POINTER,
			      const_cast<GUID *>(&guidWSASendMsg),
			      sizeof(guidWSASendMsg),
			      &lpWSASendMsg, sizeof(lpWSASendMsg), &nbytes,
			      NULL, NULL);
	    if (result == SOCKET_ERROR) {
		XLOG_ERROR("Cannot obtain WSASendMsg function pointer; "
			   "unable to send raw IPv6 traffic.");
		lpWSASendMsg = NULL;
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

    //
    // Set various socket options
    //

    // Lots of input buffering
    if (comm_sock_set_rcvbuf(_proto_socket_in, SO_RCV_BUF_SIZE_MAX,
			     SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	error_msg = c_format("Cannot set the receiver buffer size: %s",
			     comm_get_last_error_str());
	close_proto_sockets(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Lots of output buffering
    if (comm_sock_set_rcvbuf(_proto_socket_out, SO_SND_BUF_SIZE_MAX,
			     SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	error_msg = c_format("Cannot set the sender buffer size: %s",
			     comm_get_last_error_str());
	close_proto_sockets(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Include IP header when sending (XXX: doesn't do anything for IPv6)
    if (enable_ip_hdr_include(true, error_msg) != XORP_OK) {
	close_proto_sockets(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Show interest in receiving information from IP header (XXX: IPv6 only)
    if (enable_recv_pktinfo(true, error_msg) != XORP_OK) {
	close_proto_sockets(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Restrict multicast TTL
    if (set_multicast_ttl(MINTTL, error_msg) != XORP_OK) {
	close_proto_sockets(dummy_error_msg);
	return (XORP_ERROR);
    }
    // Disable mcast loopback
    if (enable_multicast_loopback(false, error_msg) != XORP_OK) {
	close_proto_sockets(dummy_error_msg);
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
#if 0		// TODO: XXX: used only for multicast routing purpose by MLD
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
	    if (setsockopt(_proto_socket_in, _ip_protocol, ICMP6_FILTER,
			   XORP_SOCKOPT_CAST(&filter), sizeof(filter)) < 0) {
		close_proto_sockets(dummy_error_msg);
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

#ifdef HOST_OS_WINDOWS
    //
    // Winsock requires that raw sockets be bound, either to the IPv4
    // address of a physical interface, or the INADDR_ANY address,
    // in order to receive traffic or to join multicast groups.
    //
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    
    if (SOCKET_ERROR == bind(_proto_socket_in, (sockaddr *)&sin,
			     sizeof(sockaddr_in))) {
	XLOG_WARNING("bind() failed: %s\n", win_strerror(GetLastError()));
    }
#endif // HOST_OS_WINDOWS

    // Assign a method to read from this socket
    if (eventloop().add_ioevent_cb(_proto_socket_in, IOT_READ,
				   callback(this,
					    &IoIpSocket::proto_socket_read))
	== false) {
	close_proto_sockets(dummy_error_msg);
	error_msg = c_format("Cannot add a protocol socket to the set of "
			     "sockets to read from in the event loop");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoIpSocket::close_proto_sockets(string& error_msg)
{
    error_msg = "";

    //
    // Close the outgoing protocol socket
    //
    if (_proto_socket_out.is_valid()) {
	comm_close(_proto_socket_out);
	_proto_socket_out.clear();
    }

    //
    // Close the incoming protocol socket
    //
    if (_proto_socket_in.is_valid()) {
	// Remove it just in case, even though it may not be select()-ed
	eventloop().remove_ioevent_cb(_proto_socket_in);

#ifdef HOST_OS_WINDOWS
	switch (family()) {
	case AF_INET:
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    // Reset the pointer to the WSARecvMsg()/WSASendMsg() functions.
	    lpWSARecvMsg = NULL;
	    lpWSASendMsg = NULL;
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    error_msg = c_format("Invalid address family %d", family());
	    return (XORP_ERROR);
	}
#endif // HOST_OS_WINDOWS

	comm_close(_proto_socket_in);
	_proto_socket_in.clear();
    }
    
    return (XORP_OK);
}

void
IoIpSocket::proto_socket_read(XorpFd fd, IoEventType type)
{
    ssize_t	nbytes;
    size_t	ip_hdr_len = 0;
    size_t	ip_data_len = 0;
    IPvX	src_address(family());
    IPvX	dst_address(family());
    int		int_val;
    int32_t	ip_ttl = -1;		// a.k.a. Hop-Limit in IPv6
    int32_t	ip_tos = -1;
    bool	ip_router_alert = false;	// Router Alert option received
    bool	ip_internet_control = false;	// IP Internet Control pkt rcvd
    uint32_t	pif_index = 0;
    vector<uint8_t> ext_headers_type;
    vector<vector<uint8_t> > ext_headers_payload;
    void*	cmsg_data;	// XXX: CMSG_DATA() is aligned, hence void ptr

    UNUSED(fd);
    UNUSED(type);
    UNUSED(int_val);
    UNUSED(cmsg_data);

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
    nbytes = recvmsg(_proto_socket_in, &_rcvmh, 0);
    if (nbytes < 0) {
	if (errno == EINTR)
	    return;		// OK: restart receiving
	XLOG_ERROR("recvmsg() on socket %s failed: %s",
		   _proto_socket_in.str().c_str(), strerror(errno));
	return;			// Error
    }

#else // HOST_OS_WINDOWS

    switch (family()) {
    case AF_INET:
    {
	struct sockaddr_storage from;
	socklen_t from_len = sizeof(from);

	nbytes = recvfrom(_proto_socket_in, XORP_BUF_CAST(_rcvbuf),
			  IO_BUF_SIZE, 0,
			  reinterpret_cast<struct sockaddr *>(&from),
			  &from_len);
	debug_msg("Read fd %s, %d bytes\n",
		  _proto_socket_in.str().c_str(), XORP_INT_CAST(nbytes));
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
	error = lpWSARecvMsg(_proto_socket_in, &mh, &nrecvd, NULL, NULL);
	nbytes = (ssize_t)nrecvd;
	debug_msg("Read fd %s, %d bytes\n",
		  _proto_socket_in.str().c_str(), XORP_INT_CAST(nbytes));
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
    
    //
    // Check whether this is a multicast forwarding related upcall from the
    // system to the user-level.
    //
    switch (family()) {
    case AF_INET:
    {
	if (_ip_protocol != IPPROTO_IGMP)
	    break;

#ifdef HAVE_IPV4_MULTICAST_ROUTING
	if (nbytes < (ssize_t)sizeof(struct igmpmsg)) {
	    // Probably not a system upcall
	    break;
	}
	struct igmpmsg* igmpmsg;
	// XXX: "void" casting to fix alignment warning that can be ignored
	igmpmsg = reinterpret_cast<struct igmpmsg *>((void *)_rcvbuf);
	if (igmpmsg->im_mbz == 0) {
	    //
	    // XXX: Packets sent up from system to daemon have
	    //      igmpmsg->im_mbz = ip->ip_p = 0
	    //
	    vector<uint8_t> payload(nbytes);
	    memcpy(&payload[0], _rcvbuf, nbytes);
	    process_system_multicast_upcall(payload);
	    return;		// OK
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	if (_ip_protocol != IPPROTO_ICMPV6)
	    break;

#ifdef HAVE_IPV6_MULTICAST_ROUTING
	if (nbytes < (ssize_t)sizeof(struct mrt6msg)) {
	    // Probably not a system upcall 
	    break;
	}
	struct mrt6msg* mrt6msg;
	// XXX: "void" casting to fix alignment warning that can be ignored
	mrt6msg = reinterpret_cast<struct mrt6msg *>((void *)_rcvbuf);
	if ((mrt6msg->im6_mbz == 0) || (_rcvmh.msg_controllen == 0)) {
	    //
	    // XXX: Packets sent up from system to daemon have
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
	    vector<uint8_t> payload(nbytes);
	    memcpy(&payload[0], _rcvbuf, nbytes);
	    process_system_multicast_upcall(payload);
	    return;		// OK
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return;			// Error
    }
    
    //
    // Not a system upcall. Pass to the registered processing function.
    //

    //
    // Input check.
    // Get source and destination address, IP TTL (a.k.a. hop-limit),
    // and (eventually) interface address (IPv6 only).
    //
    switch (family()) {
    case AF_INET:
    {
	IpHeader4 ip4(_rcvbuf);
	bool is_datalen_error = false;
	
	// Input check
	if (nbytes < (ssize_t)ip4.size()) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "packet size %d is smaller than minimum size %u",
			 XORP_INT_CAST(nbytes),
			 XORP_UINT_CAST(ip4.size()));
	    return;		// Error
	}
#ifndef HOST_OS_WINDOWS
	// TODO: get rid of this and always use ip4.ip_src() ??
	src_address.copy_in(_from4);
#else
	src_address = ip4.ip_src();
#endif
	dst_address = ip4.ip_dst();
	ip_ttl = ip4.ip_ttl();
	ip_tos = ip4.ip_tos();

	if (ip_tos == IPTOS_PREC_INTERNETCONTROL)
	    ip_internet_control = true;
	
	ip_hdr_len  = ip4.ip_header_len();
#ifdef IPV4_RAW_INPUT_IS_RAW
	ip_data_len = ip4.ip_len() - ip_hdr_len;
#else
	//
	// XXX: The original value is in host order, and excludes the
	// IPv4 header length.
	//
	ip_data_len = ip4.ip_len_host();
#endif // ! IPV4_RAW_INPUT_IS_RAW
	// Check length
	is_datalen_error = false;
	do {
	    if (ip_hdr_len + ip_data_len == static_cast<size_t>(nbytes)) {
		is_datalen_error = false;
		break;		// OK
	    }
	    if (nbytes < static_cast<ssize_t>(ip_hdr_len)) {
		is_datalen_error = true;
		break;
	    }
	    if (ip4.ip_p() == IPPROTO_PIM) {
		if (nbytes < static_cast<ssize_t>(ip_hdr_len + PIM_REG_MINLEN)) {
		    is_datalen_error = true;
		    break;
		}
		struct pim pim;
		memcpy(&pim, ip4.data() + ip_hdr_len, sizeof(pim));
		if (PIM_VT_T(pim.pim_vt) != PIM_REGISTER) {
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
		       "hdr+datalen=%u+%u=%u",
		       cstring(src_address), cstring(dst_address),
		       XORP_INT_CAST(nbytes),
		       XORP_UINT_CAST(ip_hdr_len),
		       XORP_UINT_CAST(ip_data_len),
		       XORP_UINT_CAST(ip_hdr_len + ip_data_len));
	    return;		// Error
	}

	//
	// Get the pif_index.
	//
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
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct sockaddr_dl)))
		    continue;
		cmsg_data = CMSG_DATA(cmsgp);
		sdl = reinterpret_cast<struct sockaddr_dl *>(cmsg_data);
		pif_index = sdl->sdl_index;
	    }
	    break;
#endif // IP_RECVIF

#ifdef IP_PKTINFO
	    case IP_PKTINFO:
	    {
		struct in_pktinfo *inp = NULL;
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct in_pktinfo)))
		    continue;
		cmsg_data = CMSG_DATA(cmsgp);
		inp = reinterpret_cast<struct in_pktinfo *>(cmsg_data);
		pif_index = inp->ipi_ifindex;
	    }
	    break;
#endif // IP_PKTINFO

	    default:
		break;
	    }
	}
#endif // ! HOST_OS_WINDOWS
	
	//
	// Check for Router Alert option
	//
	do {
	    const uint8_t *option_p = ip4.data() + ip4.size();
	    uint8_t option_value, option_len;
	    uint32_t test_ip_options_len = ip_hdr_len - ip4.size();
	    while (test_ip_options_len) {
		if (test_ip_options_len < 4)
		    break;
		option_value = *option_p;
		option_len = *(option_p + 1);
		if (test_ip_options_len < option_len)
		    break;
		if ((option_value == IPOPT_RA) && (option_len == 4)) {
		    ip_router_alert = true;
		    break;
		}
		if (option_len == 0)
		    break;	// XXX: a guard against bogus option_len value

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
	    {
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct in6_pktinfo)))
		    continue;
		cmsg_data = CMSG_DATA(cmsgp);
		pi = reinterpret_cast<struct in6_pktinfo *>(cmsg_data);
		pif_index = pi->ipi6_ifindex;
		dst_address.copy_in(pi->ipi6_addr);
	    }
	    break;

	    case IPV6_HOPLIMIT:
	    {
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(int)))
		    continue;
		int_val = extract_host_int(CMSG_DATA(cmsgp));
		ip_ttl = int_val;
	    }
	    break;

	    case IPV6_HOPOPTS:
	    {
		//
		// Check for Router Alert option
		//
#ifdef HAVE_RFC3542
		struct ip6_hbh *ext;
		int currentlen;
		uint8_t type;
		size_t extlen;
		socklen_t len;
		void *databuf;

		cmsg_data = CMSG_DATA(cmsgp);
		ext = reinterpret_cast<struct ip6_hbh *>(cmsg_data);
		extlen = (ext->ip6h_len + 1) * 8;
		currentlen = 0;
		while (true) {
		    currentlen = inet6_opt_next(ext, extlen, currentlen,
						&type, &len, &databuf);
		    if (currentlen == -1)
			break;
		    if (type == IP6OPT_ROUTER_ALERT) {
			ip_router_alert = true;
			break;
		    }
		}

#else // ! HAVE_RFC3542 (i.e., the old advanced API)

#ifdef HAVE_IPV6_MULTICAST_ROUTING
		//
		// TODO: XXX: temporary use HAVE_IPV6_MULTICAST_ROUTING
		// to conditionally compile, because Linux doesn't
		// have inet6_option_*
		//
		uint8_t *tptr = NULL;
		while (inet6_option_next(cmsgp, &tptr) == 0) {
		    if (*tptr == IP6OPT_ROUTER_ALERT) {
			ip_router_alert = true;
			break;
		    }
		}
#endif // HAVE_IPV6_MULTICAST_ROUTING
#endif // ! HAVE_RFC3542
	    }		
	    break;

#ifdef IPV6_TCLASS
	    case IPV6_TCLASS:
	    {
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(int)))
		    continue;
		int_val = extract_host_int(CMSG_DATA(cmsgp));
		ip_tos = int_val;

		//
		// TODO: XXX: Here we need to compute ip_internet_control
		// for IPv6, but we don't know how.
		//
	    }
	    break;
#endif // IPV6_TCLASS

#ifdef IPV6_RTHDR
	    case IPV6_RTHDR:
	    {
		vector<uint8_t> opt_payload(cmsgp->cmsg_len);
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct ip6_rthdr)))
		    continue;
		cmsg_data = CMSG_DATA(cmsgp);
		memcpy(&opt_payload[0], cmsg_data, cmsgp->cmsg_len);
		// XXX: Use the protocol number instead of message type
		ext_headers_type.push_back(IPPROTO_ROUTING);
		ext_headers_payload.push_back(opt_payload);
	    }
	    break;
#endif // IPV6_RTHDR

#ifdef IPV6_DSTOPTS
	    case IPV6_DSTOPTS:
	    {
		vector<uint8_t> opt_payload(cmsgp->cmsg_len);
		if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct ip6_dest)))
		    continue;
		cmsg_data = CMSG_DATA(cmsgp);
		memcpy(&opt_payload[0], cmsg_data, cmsgp->cmsg_len);
		// XXX: Use the protocol number instead of message type
		ext_headers_type.push_back(IPPROTO_DSTOPTS);
		ext_headers_payload.push_back(opt_payload);
	    }
	    break;
#endif // IPV6_DSTOPTS

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
    if (! (src_address.is_unicast() || src_address.is_zero())) {
	// XXX: Accept zero source addresses because of protocols like IGMPv3
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
		   "invalid Hop-Limit (TTL) from %s to %s: %d",
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
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;
    do {
	//
	// Find interface and vif based on src address, if a directly
	// connected sender, or based on dst address if unicast and one
	// of my local addresses.
	// However, check first whether we can use 'pif_index' instead.
	//
	if (pif_index != 0) {
	    ifp = _iftree.find_interface(pif_index);
	    if (ifp != NULL)
		vifp = ifp->find_vif(pif_index);
	    break;
	}
	
	if (dst_address.is_multicast()) {
	    _iftree.find_interface_vif_same_subnet_or_p2p(src_address, ifp, vifp);
	} else {
	    _iftree.find_interface_vif_by_addr(dst_address, ifp, vifp);
	}
	break;
    } while (false);
    
    if ((ifp == NULL) || (vifp == NULL)) {
	// No vif found. Ignore this packet.
	XLOG_WARNING("proto_socket_read() failed: "
		     "RX packet from %s to %s: no vif found",
		     cstring(src_address), cstring(dst_address));
	return;			// Error
    }
    if (! (ifp->enabled() || vifp->enabled())) {
	// This vif is down. Silently ignore this packet.
	return;			// Error
    }
    
    // Process the result
    vector<uint8_t> payload(nbytes - ip_hdr_len);
    memcpy(&payload[0], _rcvbuf + ip_hdr_len, nbytes - ip_hdr_len);
    process_recv_data(ifp->ifname(),
		      vifp->vifname(),
		      src_address, dst_address,
		      ip_ttl, ip_tos,
		      ip_router_alert,
		      ip_internet_control,
		      ext_headers_type,
		      ext_headers_payload,
		      payload);

    return;			// OK
}

int
IoIpSocket::proto_socket_write(const string& if_name,
			       const string& vif_name,
			       const IPvX& src_address,
			       const IPvX& dst_address,
			       int32_t ip_ttl,
			       int32_t ip_tos,
			       bool ip_router_alert,
			       bool ip_internet_control,
			       const vector<uint8_t>& ext_headers_type,
			       const vector<vector<uint8_t> >& ext_headers_payload,
			       const vector<uint8_t>& payload,
			       string& error_msg)
{
    size_t	ip_hdr_len = 0;
    int		int_val;
    int		ret_value = XORP_OK;
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;
    void*	cmsg_data;	// XXX: CMSG_DATA() is aligned, hence void ptr

    UNUSED(int_val);
    UNUSED(cmsg_data);

    XLOG_ASSERT(ext_headers_type.size() == ext_headers_payload.size());

    ifp = _iftree.find_interface(if_name);
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", if_name.c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(vif_name);
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }
    if (! ifp->enabled()) {
	error_msg = c_format("Interface %s is down",
			     ifp->ifname().c_str());
	return (XORP_ERROR);
    }
    if (! vifp->enabled()) {
	error_msg = c_format("Interface %s vif %s is down",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }

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

    //
    // Assign the TTL and TOS if they were not specified
    //
    switch (family()) {
    case AF_INET:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (ip_internet_control)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (ip_internet_control)
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
	    if (ip_internet_control)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (ip_internet_control) {
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
    // In case of IPv4, if the IP_HDRINCL socket option is enabled, the IP
    // header and the data are specified as a single entry in the sndiov[]
    // scatter/gatter array, otherwise we use socket options to set the IP
    // header information and a single sndiov[] entry with the payload.
    // In case of IPv6, the IP header information is specified as
    // ancillary data.
    //
    switch (family()) {
    case AF_INET:
    {
	//
	// XXX: In case of Linux IP_HDRINCL IP packets are not fragmented and
	// are limited to the interface MTU. The raw(7) Linux manual is wrong
	// by saying it is a limitation only in Linux 2.2. It is a limitation
	// in 2.4 and 2.6 and there is no indication this is going to be fixed
	// in the future.
	// Hence, in case of Linux we do the following:
	//   - If the IP packet fits in the MTU, then send the packet using
	//     IP_HDRINCL option.
	//   - Otherwise, if the IP source address belongs to the outgoing
	//     interface, then use various socket options to specify some of
	//     the IP header options.
	//   - Otherwise, use IP_HDRINCL and fragment the IP packet in user
	//     space before transmitting it. Note that in this case we need
	//     to manage the ip_id field in the IP header.
	//
	// The reasoning behind the above logic is: (1) If the IP source
	// address doesn't belong to one of the router's IP addresses, then
	// we must use IP_HDRINCL; (2) We want to avoid as much as possible
	// user-level IP packet fragmentation, because managing the ip_id
	// field in user space does not guarantee the same ip_id is reused
	// by the kernel as well (for the same tuple of <src, dest, protocol>).
	//
	// Note that in case of all other OS-es we always use the IP_HDRINCL
	// option to transmit the packets.
	//
	bool do_fragmentation = false;
	bool do_ip_hdr_include = true;

	//
	// Decide whether we should use IP_HDRINCL and whether we should
	// do IP packet fragmentation.
	//
	do {
#ifndef HOST_OS_LINUX
	    break;	// XXX: The extra processing below is for Linux only
#endif
	    // Calculate the final packet size and whether it fits in the MTU
	    ip_hdr_len = IpHeader4::SIZE;
	    if (ip_router_alert)
		ip_hdr_len += sizeof(ra_opt4);

	    if (ip_hdr_len + payload.size() <= ifp->mtu())
		break;

	    if (vifp->find_addr(src_address.get_ipv4()) != NULL) {
		do_ip_hdr_include = false;
		break;
	    }

	    do_fragmentation = true;
	    break;
	} while (false);

	if (do_ip_hdr_include != _is_ip_hdr_included) {
	    if (enable_ip_hdr_include(do_ip_hdr_include, error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }
	}

	if (! _is_ip_hdr_included) {
	    //
	    // Use socket options to set the IP header information
	    //

	    //
	    // Include the Router Alert option
	    //
#ifdef IP_OPTIONS
	    if (ip_router_alert) {
		if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_OPTIONS,
			       XORP_SOCKOPT_CAST(&ra_opt4),
			       sizeof(ra_opt4))
		    < 0) {
		    error_msg = c_format("setsockopt(IP_OPTIONS, IPOPT_RA) "
					 "failed: %s",
					 strerror(errno));
		    XLOG_ERROR("%s", error_msg.c_str());
		    return (XORP_ERROR);
		}
	    }
#endif // IP_OPTIONS

	    //
	    // Set the TTL
	    //
#ifdef IP_TTL
	    int_val = ip_ttl;
	    if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_TTL,
			   XORP_SOCKOPT_CAST(&int_val), sizeof(int_val))
		< 0) {
		error_msg = c_format("setsockopt(IP_TTL, %d) failed: %s",
				     int_val, strerror(errno));
		XLOG_ERROR("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }
#endif // IP_TTL

	    //
	    // Set the TOS
	    //
#ifdef IP_TOS
	    int_val = ip_tos;
	    if (setsockopt(_proto_socket_out, IPPROTO_IP, IP_TOS,
			   XORP_SOCKOPT_CAST(&int_val), sizeof(int_val))
		< 0) {
		error_msg = c_format("setsockopt(IP_TOS, 0x%x) failed: %s",
				     int_val, strerror(errno));
		XLOG_ERROR("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }
#endif // IP_TOS

	    //
	    // XXX: Bind to the source address so the outgoing IP packet
	    // will use that address.
	    //
	    struct sockaddr_in sin;
	    src_address.copy_out(sin);
	    if (bind(_proto_socket_out,
		     reinterpret_cast<struct sockaddr*>(&sin),
		     sizeof(sin))
		< 0) {
		error_msg = c_format("raw socket bind(%s) failed: %s",
				     cstring(src_address), strerror(errno));
		XLOG_ERROR("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }

	    //
	    // Now hook the data
	    //
	    memcpy(_sndbuf, &payload[0], payload.size()); // XXX: _sndiov[0].iov_base
	    _sndiov[0].iov_len = payload.size();

	    // Transmit the packet
	    ret_value = proto_socket_transmit(ifp, vifp,
					      src_address, dst_address,
					      error_msg);
	    break;
	}

	//
	// Set the IPv4 header
	//
	IpHeader4Writer ip4(_sndbuf);
	if (ip_router_alert) {
	    // Include the Router Alert option
	    uint8_t* ip_option_p = ip4.data() + ip4.size();
	    //
	    // XXX: ra_opt4 is in network order, hence we write it as-is
	    // by using embed_host_32().
	    //
	    embed_host_32(ip_option_p, ra_opt4);
	    ip_hdr_len	= ip4.size() + sizeof(ra_opt4);
	} else {
	    ip_hdr_len	= ip4.size();
	}
	ip4.set_ip_version(IPVERSION);
	ip4.set_ip_header_len(ip_hdr_len);
	ip4.set_ip_tos(ip_tos);
	ip4.set_ip_id(0);				// Let kernel fill in
	ip4.set_ip_off(0);
	ip4.set_ip_ttl(ip_ttl);
	ip4.set_ip_p(_ip_protocol);
	ip4.set_ip_sum(0);				// Let kernel fill in
	ip4.set_ip_src(src_address.get_ipv4());
	ip4.set_ip_dst(dst_address.get_ipv4());
	ip4.set_ip_len(ip_hdr_len + payload.size());

	//
	// Now hook the data
	//
    	memcpy(_sndbuf + ip_hdr_len, &payload[0], payload.size()); // XXX: _sndiov[0].iov_base
	_sndiov[0].iov_len	= ip_hdr_len + payload.size();

	if (! do_fragmentation) {
	    // Transmit the packet
	    ret_value = proto_socket_transmit(ifp, vifp,
					      src_address, dst_address,
					      error_msg);
	    break;
	}

	//
	// Perform fragmentation and transmit each fragment
	//
	list<vector<uint8_t> > fragments;
	list<vector<uint8_t> >::iterator iter;

	// Calculate and set the IPv4 packet ID
	_ip_id++;
	if (_ip_id == 0)
	    _ip_id++;
	ip4.set_ip_id(_ip_id);
	if (ip4.fragment(ifp->mtu(), fragments, false, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	XLOG_ASSERT(! fragments.empty());
	for (iter = fragments.begin(); iter != fragments.end(); ++iter) {
	    vector<uint8_t>& ip_fragment = *iter;
	    _sndiov[0].iov_len = ip_fragment.size();
	    memcpy(_sndbuf, &ip_fragment[0], ip_fragment.size());
	    ret_value = proto_socket_transmit(ifp, vifp,
					      src_address, dst_address,
					      error_msg);
	    if (ret_value != XORP_OK)
		break;
	}
    }
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
	
	if (ip_router_alert) {
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
	    hbhlen = inet6_option_space(sizeof(ra_opt6));
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

	// Space for the Extension headers
	for (size_t i = 0; i < ext_headers_type.size(); i++) {
	    // Ignore the types that are not supported
	    switch (ext_headers_type[i]) {
	    case IPPROTO_ROUTING:
		break;
	    case IPPROTO_DSTOPTS:
		break;
	    default:
		continue;	// XXX: all other types are not supported
		break;
	    }
	    ctllen += CMSG_SPACE(ext_headers_payload[i].size());
	}

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
	cmsg_data = CMSG_DATA(cmsgp);
	sndpktinfo = reinterpret_cast<struct in6_pktinfo *>(cmsg_data);
	memset(sndpktinfo, 0, sizeof(*sndpktinfo));
	if (dst_address.is_unicast() && !dst_address.is_linklocal_unicast()) {
	    //
	    // XXX: don't set the outgoing interface index if we are
	    // sending an unicast packet to a non-link local unicast address.
	    // Otherwise, the sending may fail with EHOSTUNREACH error.
	    //
	    sndpktinfo->ipi6_ifindex = 0;		// Let kernel fill in
	} else {
	    sndpktinfo->ipi6_ifindex = ifp->pif_index();
	}
	src_address.copy_out(sndpktinfo->ipi6_addr);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Include the Router Alert option if needed
	//
	if (ip_router_alert) {
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
	    inet6_opt_set_val(optp, 0, &rtalert_code6, sizeof(rtalert_code6));
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
	    if (inet6_option_append(cmsgp, ra_opt6, 4, 0)) {
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
	embed_host_int(CMSG_DATA(cmsgp), int_val);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Set the TOS
	//
#ifdef IPV6_TCLASS
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_TCLASS;
	int_val = ip_tos;
	embed_host_int(CMSG_DATA(cmsgp), int_val);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
#endif // IPV6_TCLASS

	//
	// Set the Extension headers
	//
	for (size_t i = 0; i < ext_headers_type.size(); i++) {
	    uint8_t header_type = 0;

	    // Translate protocol number (header type) to message type
	    switch (ext_headers_type[i]) {
	    case IPPROTO_ROUTING:
		header_type = IPV6_RTHDR;
		break;
	    case IPPROTO_DSTOPTS:
		header_type = IPV6_DSTOPTS;
		break;
	    default:
		continue;	// XXX: all other types are not supported
		break;
	    }

	    // Set the header
	    const vector<uint8_t>& opt_payload(ext_headers_payload[i]);
	    cmsgp->cmsg_len = CMSG_LEN(opt_payload.size());
	    cmsgp->cmsg_level = IPPROTO_IPV6;
	    cmsgp->cmsg_type = header_type;
	    cmsg_data = CMSG_DATA(cmsgp);
	    memcpy(cmsg_data, &opt_payload[0], opt_payload.size());
	    cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	}
	
	//
	// Now hook the data
	//
	memcpy(_sndbuf, &payload[0], payload.size()); // XXX: _sndiov[0].iov_base
	_sndiov[0].iov_len  = payload.size();

	// Transmit the packet
	ret_value = proto_socket_transmit(ifp, vifp,
					  src_address, dst_address,
					  error_msg);
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }

    return (ret_value);
}

int
IoIpSocket::proto_socket_transmit(const IfTreeInterface* ifp,
				  const IfTreeVif*	vifp,
				  const IPvX&		src_address,
				  const IPvX&		dst_address,
				  string&		error_msg)
{
    bool setloop = false;
    int ret_value = XORP_OK;

    //
    // Adjust some IPv4 header fields
    //
#ifndef IPV4_RAW_OUTPUT_IS_RAW
    if (_is_ip_hdr_included && src_address.is_ipv4()) {
	//
	// XXX: The stored value should be in host order, and should
	// include the IPv4 header length.
	//
	IpHeader4Writer ip4(_sndbuf);
	ip4.set_ip_len_host(ip4.ip_len());
    }
#endif // ! IPV4_RAW_INPUT_IS_RAW

    //
    // Multicast-related setting
    //
    if (dst_address.is_multicast()) {
	if (set_default_multicast_interface(ifp->ifname(),
					    vifp->vifname(),
					    error_msg)
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

    //
    // Transmit the packet
    //

#ifndef HOST_OS_WINDOWS    

    // Set some sendmsg()-related fields
    switch (family()) {
    case AF_INET:
	dst_address.copy_out(_to4);
	_sndmh.msg_namelen	= sizeof(_to4);
	_sndmh.msg_control	= NULL;
	_sndmh.msg_controllen	= 0;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	dst_address.copy_out(_to6);
	system_adjust_sockaddr_in6_send(_to6, ifp->pif_index());
	_sndmh.msg_namelen  = sizeof(_to6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }

    if (sendmsg(_proto_socket_out, &_sndmh, 0) < 0) {
	ret_value = XORP_ERROR;
	if (errno == ENETDOWN) {
	    // TODO: check the interface status. E.g. vif_state_check(family);
	} else {
	    error_msg = c_format("sendmsg(proto %d size %u from %s to %s "
				 "on interface %s vif %s) failed: %s",
				 _ip_protocol,
				 XORP_UINT_CAST(_sndiov[0].iov_len),
				 cstring(src_address),
				 cstring(dst_address),
				 ifp->ifname().c_str(),
				 vifp->vifname().c_str(),
				 strerror(errno));
	}
    }

#else // HOST_OS_WINDOWS
    // XXX: We may use WSASendMsg() on Longhorn to support IPv6.

    DWORD sent, error;
    struct sockaddr_storage to;
    DWORD buffer_count = 1;
    int to_len = 0;

    memset(&to, 0, sizeof(to));
    dst_address.copy_out(reinterpret_cast<struct sockaddr&>(to));

    // Set some family-specific arguments
    switch (family()) {
    case AF_INET:
	to_len = sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	to_len = sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }

    error = WSASendTo(_proto_socket_out,
		      reinterpret_cast<WSABUF *>(_sndiov),
		      buffer_count, &sent, 0,
		      reinterpret_cast<struct sockaddr *>(&to),
		      to_len, NULL, NULL);
    if (error != 0) {
	ret_value = XORP_ERROR;
	error_msg = c_format("WSASendTo(proto %d size %u from %s to %s "
			     "on interface %s vif %s) failed: %s",
			     _ip_protocol,
			     XORP_UINT_CAST(_sndiov[0].iov_len),
			     cstring(src_address),
			     cstring(dst_address),
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str(),
			     win_strerror(GetLastError()));
	XLOG_ERROR("%s", error_msg.c_str());
    }
#endif // HOST_OS_WINDOWS

    //
    // Restore some multicast related settings
    //
    if (setloop) {
	string dummy_error_msg;
	enable_multicast_loopback(false, dummy_error_msg);
    }

    return (ret_value);
}
