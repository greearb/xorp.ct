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

#ident "$XORP: xorp/fea/mfea_proto_comm.cc,v 1.2 2003/05/16 00:35:03 pavlin Exp $"


//
// Multicast-related raw protocol communications.
//


#include "mfea_module.h"
#include "libxorp/xorp.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "mrt/include/ip_mroute.h"

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/utils.hh"

#include "libcomm/comm_api.h"

#include "mrt/max_vifs.h"
#include "mrt/multicast_defs.h"

// XXX: _PIM_VT is needed if we want the extra features of <netinet/pim.h>
#define _PIM_VT 1
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#else
#include "mrt/include/netinet/pim.h"
#endif

#include "mfea_node.hh"
#include "mfea_osdep.hh"
#include "mfea_proto_comm.hh"
#include "mfea_vif.hh"


//
// Exported variables
//

//
// Static class members
//

//
// Local constants definitions
//
#define IO_BUF_SIZE		(64*1024)  // I/O buffer(s) size
#define CMSG_BUF_SIZE		(10*1024)  // 'rcvcmsgbuf' and 'sndcmsgbuf'
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  // Min. socket buffer size
#define SO_RCV_BUF_SIZE_MAX	(256*1024) // Desired socket buffer size

//
// Local structures/classes, typedefs and macros
//
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
#ifndef IP6OPT_ROUTER_ALERT	// XXX: for compatibility with older systems
#define IP6OPT_ROUTER_ALERT IP6OPT_RTALERT
#endif
#endif // HAVE_IPV6
//
#ifdef HAVE_IPV6
static uint16_t		rtalert_code;
#ifndef HAVE_RFC2292BIS
static uint8_t		raopt[IP6OPT_RTALERT_LEN];
#endif
#endif // HAVE_IPV6


//
// Local functions prototypes
//

/**
 * ProtoComm::ProtoComm:
 * @mfea_node: The MfeaNode I belong to.
 * @proto: The protocol number (e.g., %IPPROTO_IGMP, %IPPROTO_PIM, etc).
 **/
ProtoComm::ProtoComm(MfeaNode& mfea_node, int ipproto,
		     xorp_module_id module_id)
    : ProtoUnit(mfea_node.family(), mfea_node.module_id()),
      _mfea_node(mfea_node),
      _ipproto(ipproto),
      _module_id(module_id)
{
    // Init Router Alert related option stuff
#ifdef HAVE_IPV6
    rtalert_code = htons(IP6OPT_RTALERT_MLD); // XXX: used by MLD only (?)
#ifndef HAVE_RFC2292BIS
    raopt[0] = IP6OPT_ROUTER_ALERT;
    raopt[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&raopt[2], (caddr_t)&rtalert_code, sizeof(rtalert_code));
#endif // ! HAVE_RFC2292BIS
#endif // HAVE_IPV6
    
    _proto_socket = -1;
    
    // Allocate the buffers
    _rcvbuf0 = new uint8_t[IO_BUF_SIZE];
    _sndbuf0 = new uint8_t[IO_BUF_SIZE];
    _rcvbuf1 = new uint8_t[IO_BUF_SIZE];
    _sndbuf1 = new uint8_t[IO_BUF_SIZE];
    _rcvcmsgbuf = new uint8_t[CMSG_BUF_SIZE];
    _sndcmsgbuf = new uint8_t[CMSG_BUF_SIZE];
    
    // recvmsg() and sendmsg() related initialization
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
	XLOG_ASSERT(false);
	break;
    }
    _rcvmh.msg_iov		= _rcviov;
    _sndmh.msg_iov		= _sndiov;
    _rcvmh.msg_iovlen		= 1;
    _sndmh.msg_iovlen		= 1;
    _rcviov[0].iov_base		= (caddr_t)_rcvbuf0;
    _rcviov[1].iov_base		= (caddr_t)_rcvbuf1;
    _rcviov[0].iov_len		= IO_BUF_SIZE;
    _rcviov[1].iov_len		= IO_BUF_SIZE;
    _sndiov[0].iov_base		= (caddr_t)_sndbuf0;
    _sndiov[1].iov_base		= (caddr_t)_sndbuf1;
    _sndiov[0].iov_len		= 0;
    _sndiov[1].iov_len		= 0;
    
    _rcvmh.msg_control		= (caddr_t)_rcvcmsgbuf;
    _sndmh.msg_control		= (caddr_t)_sndcmsgbuf;
    _rcvmh.msg_controllen	= CMSG_BUF_SIZE;
    _sndmh.msg_controllen	= 0;
    
    _ignore_my_packets		= false;
}

ProtoComm::~ProtoComm()
{
    stop();
    
    // Free the buffers
    delete[] _rcvbuf0;
    delete[] _sndbuf0;
    delete[] _rcvbuf1;
    delete[] _sndbuf1;
    delete[] _rcvcmsgbuf;
    delete[] _sndcmsgbuf;
}

/**
 * ProtoComm::start:
 * @void: 
 * 
 * Start the ProtoComm.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
ProtoComm::start(void)
{
    // XXX: all ProtoComm are automatically enabled by default
    ProtoUnit::enable();
    
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    if (_ipproto >= 0) {
	if (open_proto_socket() < 0) {
	    stop();
	    return (XORP_ERROR);
	}
    }
    
    return (XORP_OK);
}

/**
 * ProtoComm::stop:
 * @void: 
 * 
 * Stop the ProtoComm.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::stop(void)
{
    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    close_proto_socket();
    
    return (XORP_OK);
}

/**
 * ProtoComm::ip_hdr_include:
 * @enable_bool: If true, set the option, otherwise reset it.
 * 
 * Set/reset the "Header Included" option (for IPv4) on the protocol socket.
 * If set, the IP header of a raw packet should be created
 * by the application itself, otherwise the kernel will build it.
 * XXX: Used only for IPv4.
 * In post-RFC-2292, IPV6_PKTINFO has similar functions,
 * but because it requires the interface index and outgoing address,
 * it is of little use for our purpose. Also, in RFC-2292 this option
 * was a flag, so for compatibility reasons we better not set it
 * here; instead, we will use sendmsg() to specify the header's field values. 
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::ip_hdr_include(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
    {
#ifdef IP_HDRINCL
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = enable_bool; 
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_HDRINCL,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_HDRINCL, %u) failed: %s", bool_flag,
		       strerror(errno));
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
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(enable_bool);
}


/**
 * ProtoComm::recv_pktinfo:
 * @enable_bool: If true, set the option, otherwise reset it.
 * 
 * Enable/disable receiving information about some of the fields
 * in the IP header on the protocol socket.
 * If enabled, values such as interface index, destination address and
 * IP TTL (a.k.a. hop-limit in IPv6), and hop-by-hop options will be
 * received as well.
 * XXX: used only for IPv6. In IPv4 we don't have this; the whole IP
 * packet is passed to the application listening on a raw socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::recv_pktinfo(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
	break;
	
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = enable_bool;
	
	//
	// Interface index and address
	//
#ifdef IPV6_RECVPKTINFO
	// The new option (applies to receiving only)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVPKTINFO, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	// The old option (see RFC 2292)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_PKTINFO,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_PKTINFO, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVPKTINFO
	
	//
	// Hop-limit field
	//
#ifdef IPV6_RECVHOPLIMIT
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVHOPLIMIT, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_HOPLIMIT,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_HOPLIMIT, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPLIMIT
	
	//
	// Traffic class value
	//
#ifdef IPV6_RECVTCLASS
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVTCLASS,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVTCLASS, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IPV6_RECVTCLASS
	
	//
	// Hop-by-hop options
	//
#ifdef IPV6_RECVHOPOPTS
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVHOPOPTS,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVHOPOPTS, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_HOPOPTS,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_HOPOPTS, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPOPTS
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(enable_bool);
}


/**
 * ProtoComm::set_mcast_ttl:
 * @ttl: The desired IP TTL (a.k.a. hop-limit in IPv6) value.
 * 
 * Set the default TTL (or hop-limit in IPv6) for the outgoing multicast
 * packets on the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::set_mcast_ttl(int ttl)
{
    switch (family()) {
    case AF_INET:
    {
	u_char ip_ttl = ttl; // XXX: In IPv4 the value argument is 'u_char'
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_TTL,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_TTL, %u) failed: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	int ip_ttl = ttl;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_HOPS, %u) failed: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * ProtoComm::set_multicast_loop:
 * @enable_bool: If true, set the loopback, otherwise reset it.
 * 
 * Set/reset the "Multicast Loop" flag on the protocol socket.
 * If the multicast loopback flag is set, a multicast datagram sent on
 * that socket will be delivered back to this host (assuming the host
 * is a member of the same multicast group).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::set_multicast_loop(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
    {
	u_char loop = enable_bool;
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_LOOP,
		       (void *)&loop, sizeof(loop)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_LOOP, %u) failed: %s",
		       loop, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	uint loop6 = enable_bool;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       (void *)&loop6, sizeof(loop6)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_LOOP, %u) failed: %s",
		       loop6, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * ProtoComm::set_default_multicast_vif:
 * @vif_index: The vif index of the interface to become the default
 * multicast interface.
 *
 * Set default interface for outgoing multicast on the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::set_default_multicast_vif(uint16_t vif_index)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);

    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
    {
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("set_default_multicast_vif() failed: "
		       "vif %s has no address",
		       mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_IF,
		       (void *)&in_addr, sizeof(in_addr)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_IF, %s) failed: %s",
		       cstring(*mfea_vif->addr_ptr()), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	u_int pif_index = mfea_vif->pif_index();
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		       (void *)&pif_index, sizeof(pif_index)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_IF, %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * ProtoComm:join_multicast_group:
 * @vif_index: The vif index of the interface to join the multicast group.
 * @group: The multicast group to join.
 * 
 * Join a multicast group on an interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::join_multicast_group(uint16_t vif_index, const IPvX& group)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
#if 0	// TODO: enable or disable the mfea_vif->is_up() check??
    if (! mfea_vif->is_up()) {
	XLOG_ERROR("Cannot join group %s on vif %s: interface is DOWN",
		   cstring(group), mfea_vif->name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("Cannot join group %s on vif %s: "
		       "interface has no address",
		       cstring(group), mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       (void *)&mreq, sizeof(mreq)) < 0) {
	    XLOG_ERROR("Cannot join group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = mfea_vif->pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		       (void *)&mreq6, sizeof(mreq6)) < 0) {
	    XLOG_ERROR("Cannot join group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * ProtoComm:leave_multicast_group:
 * @vif_index: The vif index of the interface to leave the multicast group.
 * @group: The multicast group to leave.
 * 
 * Leave a multicast group on an interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::leave_multicast_group(uint16_t vif_index, const IPvX& group)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
#if 0	// TODO: enable or disable the mfea_vif->is_up() check??
    if (! mfea_vif->is_up()) {
	XLOG_ERROR("Cannot leave group %s on vif %s: interface is DOWN",
		   cstring(group), mfea_vif->name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: "
		       "interface has no address",
		       cstring(group), mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       (void *)&mreq, sizeof(mreq)) < 0) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = mfea_vif->pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
		       (void *)&mreq6, sizeof(mreq6)) < 0) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * ProtoComm::open_proto_socket:
 * @void: 
 * 
 * Register and 'start' a multicast protocol (IGMP, MLD, PIM, etc)
 * in the kernel.
 * XXX: This function will create the socket to receive the messages,
 * and will assign a function to start listening on that socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::open_proto_socket(void)
{
    if (_ipproto < 0)
	return (XORP_ERROR);
    
    if (_proto_socket < 0) {
	if ( (_proto_socket = socket(family(), SOCK_RAW, _ipproto)) < 0) {
	    XLOG_ERROR("Cannot open ipproto %d raw socket stream: %s",
		       _ipproto, strerror(errno));
	    return (XORP_ERROR);
	}
    }
    // Set various socket options
    // Lots of input buffering
    if (comm_sock_set_rcvbuf(_proto_socket, SO_RCV_BUF_SIZE_MAX,
			     SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Include IP header when sending (XXX: doesn't do anything for IPv6)
    if (ip_hdr_include(true) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Show interest in receiving information from IP header (XXX: IPv6 only)
    if (recv_pktinfo(true) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Restrict multicast TTL
    if (set_mcast_ttl(MINTTL)
	< 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Disable mcast loopback
    if (set_multicast_loop(false) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Protocol-specific setup
    switch (family()) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (_ipproto == IPPROTO_ICMPV6) {
	    struct icmp6_filter filter;
	    
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
	    if (setsockopt(_proto_socket, _ipproto, ICMP6_FILTER,
			   (void *)&filter, sizeof(filter)) < 0) {
		close_proto_socket();
		XLOG_ERROR("setsockopt(ICMP6_FILTER) failed: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    // Assign a function to read from this socket
    if (mfea_node().eventloop().add_selector(_proto_socket, SEL_RD,
				callback(this, &ProtoComm::proto_socket_read))
	== false) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * ProtoComm::close_proto_socket:
 * @void: 
 * 
 * Close the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::close_proto_socket(void)
{
    if (_proto_socket < 0)
	return (XORP_ERROR);
    
    // Remove it just in case, even though it may not be select()-ed
    mfea_node().eventloop().remove_selector(_proto_socket);
    
    close(_proto_socket);
    _proto_socket = -1;
    
    return (XORP_OK);
}

/**
 * ProtoComm::proto_socket_read:
 * @fd: file descriptor of arriving data.
 * @mask: The selector event type mask that describes the status of @fd.
 * 
 * Read data from a protocol socket, and then call the appropriate protocol
 * module to process it.
 **/
void
ProtoComm::proto_socket_read(int fd, SelectorMask mask)
{
    int		nbytes;
    int		ip_hdr_len = 0;
    int		ip_data_len = 0;
    IPvX	src(family());
    IPvX	dst(family());
    int		ip_ttl = -1;		// a.k.a. Hop-Limit in IPv6
    int		ip_tos = -1;
    bool	router_alert_bool = false; // Router Alert option received
    int		pif_index = 0;
    MfeaVif	*mfea_vif = NULL;

    UNUSED(fd);
    UNUSED(mask);
    
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
	XLOG_ASSERT(false);
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
    
    // Check if it is a signal from the kernel to the user-level
    switch (_ipproto) {
    case IPPROTO_IGMP:
    {
	struct igmpmsg *igmpmsg;
	
	igmpmsg = (struct igmpmsg *)_rcvbuf0;
	if (nbytes < (int)sizeof(*igmpmsg)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal packet size %d is smaller than minimum size %u",
			 nbytes, (uint32_t)sizeof(*igmpmsg));
	    return;		// Error
	}
	if (igmpmsg->im_mbz == 0) {
	    //
	    // XXX: Packets sent up from kernel to daemon have
	    //      igmpmsg->im_mbz = ip->ip_p = 0
	    //
	    return; // OK: kernel signals are received and processed elsewhere
	}
	break;
    }
#ifdef HAVE_IPV6
    case IPPROTO_ICMPV6:
    {
	struct mrt6msg *mrt6msg;
	
	mrt6msg = (struct mrt6msg *)_rcvbuf0;
	if ((nbytes < (int)sizeof(*mrt6msg))
	    && (nbytes < (int)sizeof(struct mld_hdr))) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal packet size %d is smaller than minimum size %u",
			 nbytes,
			 min((uint32_t)sizeof(*mrt6msg), (uint32_t)sizeof(struct mld_hdr)));
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
	    return; // OK: kernel signals are received and processed elsewhere
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	break;
    }
    
    //
    // Not a kernel signal. Pass to the registered processing function
    //
    
    // Input check.
    // Get source and destination address, IP TTL (a.k.a. hop-limit),
    // and (eventually) interface address (IPv6 only).
    switch (family()) {
    case AF_INET:
    {
	struct ip *ip;
	struct cmsghdr *cmsgp;
	bool is_datalen_error = false;
	
	// Input check
	ip = (struct ip *)_rcvbuf0;
	if (nbytes < (int)sizeof(*ip)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "packet size %d is smaller than minimum size %u",
			 nbytes, (uint32_t)sizeof(*ip));
	    return;		// Error
	}
	src.copy_in(_from4);
	dst.copy_in(ip->ip_dst);
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
		pim = (struct pim *)((uint8_t *)ip + ip_hdr_len);
		if (PIM_VT_T(pim->pim_vt) != PIM_REGISTER) {
		    is_datalen_error = true;
		    break;
		}
		// XXX: the *BSD kernel might truncate the encapsulated data
		// in PIM Register messages before passing the message up
		// to user level. The reason for the truncation is to reduce
		// the bandwidth overhead, but the price for this is
		// messages with weird IP header. Sigh...
		is_datalen_error = false;
		break;
	    }
	} while (false);
	if (is_datalen_error) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet size from %s to %s with %d bytes instead of "
		       "hdr+datalen=%d+%d=%d",
		       cstring(src), cstring(dst), nbytes,
		       ip_hdr_len, ip_data_len, ip_hdr_len + ip_data_len);
	    return;		// Error
	}
	
	for (cmsgp = (struct cmsghdr *)CMSG_FIRSTHDR(&_rcvmh);
	     cmsgp != NULL;
	     cmsgp = (struct cmsghdr *)CMSG_NXTHDR(&_rcvmh, cmsgp)) {
	    if (cmsgp->cmsg_level != IPPROTO_IP)
		continue;
	    switch (cmsgp->cmsg_type) {
#ifdef IP_RECVIF
	    case IP_RECVIF:
	    {
		struct sockaddr_dl *sdl = NULL;
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct sockaddr_dl)))
		    continue;
		sdl = (struct sockaddr_dl *)CMSG_DATA(cmsgp);
		pif_index = sdl->sdl_index;
		break;
	    }
#endif // IP_RECVIF
	    default:
		break;
	    }
	}
	
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
		    router_alert_bool = true;
		    break;
		}
		test_ip_options_len -= option_len;
		option_p += option_len;
	    }
	    
	    break;
	} while (false);
	
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct cmsghdr *cmsgp;
	struct in6_pktinfo *pi = NULL;
	
	src.copy_in(_from6);
	if (_rcvmh.msg_flags & MSG_CTRUNC) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s with size of %d bytes is truncated",
		       cstring(src), nbytes);
	    return;		// Error
	}
	if (_rcvmh.msg_controllen < sizeof(struct cmsghdr)) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s has too short msg_controllen "
		       "(%d instead of %u)",
		       cstring(src),
		       _rcvmh.msg_controllen,
		       (uint32_t)sizeof(struct cmsghdr));
	    return;		// Error
	}
	
	//
	// Get pif_index, hop limit, Router Alert option, etc.
	//
	for (cmsgp = (struct cmsghdr *)CMSG_FIRSTHDR(&_rcvmh);
	     cmsgp != NULL;
	     cmsgp = (struct cmsghdr *)CMSG_NXTHDR(&_rcvmh, cmsgp)) {
	    if (cmsgp->cmsg_level != IPPROTO_IPV6)
		continue;
	    switch (cmsgp->cmsg_type) {
	    case IPV6_PKTINFO:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct in6_pktinfo)))
		    continue;
		pi = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
		pif_index = pi->ipi6_ifindex;
		dst.copy_in(pi->ipi6_addr);
		break;
	    case IPV6_HOPLIMIT:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		ip_ttl = *((int *)CMSG_DATA(cmsgp));
		break;
	    case IPV6_HOPOPTS:
		{
		    //
		    // Check for Router Alert option
		    //
#ifdef HAVE_RFC2292BIS
		    {
			struct ip6_hbh *ext;
			int currentlen;
			u_int8_t type;
			size_t extlen, len;
			void *databuf;
			
			ext = (struct ip6_hbh *)CMSG_DATA(cmsgp);
			extlen = (ext->ip6h_len + 1) * 8;
			currentlen = 0;
			while (true) {
			    currentlen = inet6_opt_next(ext, extlen,
							currentlen,
							&type, &len, &databuf);
			    if (currentlen == -1)
				break;
			    if (type == IP6OPT_ROUTER_ALERT) {
				router_alert_bool = true;
				break;
			    }
			}
		    }
#else // ! HAVE_RFC2292BIS (i.e., the old advanced API)
		    {
			uint8_t  *tptr = NULL;
			while (inet6_option_next(cmsgp, &tptr) == 0) {
			    if (*tptr == IP6OPT_ROUTER_ALERT) {
				router_alert_bool = true;
				break;
			    }
			}
		    }
#endif // ! HAVE_RFC2292BIS
		    
		}
		
		break;
#ifdef IPV6_TCLASS
	    case IPV6_TCLASS:
		if (cmsgp->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		ip_tos = *((int *)CMSG_DATA(cmsgp));
		break;
#endif // IPV6_TCLASS
	    default:
		break;
	    }
	}
	
	ip_hdr_len = 0;
	ip_data_len = nbytes;
	
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return;			// Error
    }
    
    // Various checks
    if (! src.is_unicast()) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid unicast sender address: %s", cstring(src));
	return;			// Error
    }
    if (! (dst.is_multicast() || dst.is_unicast())) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid destination address: %s", cstring(dst));
	return;			// Error
    }
    if (ip_ttl < 0) {
	// TODO: what about ip_ttl = 0? Is it OK?
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid Hop-Limit (TTL) from %s to %s: %d",
		   cstring(src), cstring(dst), ip_ttl);
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
		       "invalid interface pif_index from %s to %s: %d",
		       cstring(src), cstring(dst), pif_index);
	    return;		// Error
#endif // HAVE_IPV6
	default:
	    XLOG_ASSERT(false);
	    return;		// Error
	}
    }
    
    // XXX: silently ignore the messages originated by myself
    // TODO: this search is probably too much overhead?
    if (ignore_my_packets()) {
	if (mfea_node().vif_find_by_addr(src) != NULL)
	    return;		// Error
    }
    
    // Find the vif this message was received on.
    // XXX: note that in case of IPv4 (except Linux?) we may be able
    // only to "guess" the vif by using the sender or the destination address.
    mfea_vif = NULL;
    do {
	//
	// Find vif based on src address, if a directly connected sender,
	// or based on dst address if unicast and one of my local addresses.
	// However, check first whether we can use 'pif_index' instead.
	if (pif_index != 0) {
	    mfea_vif = mfea_node().vif_find_by_pif_index(pif_index);
	    break;
	}
	
	if (dst.is_multicast()) {
	    // TODO: use VIF_LOCAL_DISABLE flag
	    mfea_vif = mfea_node().vif_find_direct(src);
	} else {
	    mfea_vif = mfea_node().vif_find_by_addr(dst);
	}
	break;
    } while (false);
    
    if (mfea_vif == NULL) {
	// No vif found. Ignore this packet.
	XLOG_WARNING("proto_socket_read() failed: "
		     "RX packet from %s to %s: no vif found",
		     cstring(src), cstring(dst));
	return;			// Error
    }
    if (! mfea_vif->is_up()) {
	// This vif is down. Silently ignore this packet.
	return;			// Error
    }
    
    // Process the result
    if (mfea_node().proto_comm_recv(module_id(),
				    mfea_vif->vif_index(),
				    src, dst, ip_ttl, ip_tos,
				    router_alert_bool,
				    _rcvbuf0 + ip_hdr_len, nbytes - ip_hdr_len)
	!= XORP_OK) {
	return;			// Error
    }
    
    return;			// OK
}


/**
 * ProtoComm::proto_socket_write:
 * @vif_index: The vif index of the vif that will be used to send-out
 * the packet.
 * @src: The source address of the packet.
 * @dst: The destination address of the packet.
 * @ip_ttl: The TTL (a.k.a. Hop-limit in IPv6) of the packet. If it has a
 * negative value, the TTL will be set here or by the lower layers.
 * @ip_tos: The TOS (a.k.a. Traffic Class in IPv6) of the packet. If it has a
 * negative value, the TOS will be set here or by the lower layers.
 * @router_alert_bool: If true, then the IP packet with the data should
 * have the Router Alert option included.
 * @databuf: The data buffer.
 * @datalen: The length of the data in @databuf.
 * 
 * Send a packet on a protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
ProtoComm::proto_socket_write(uint16_t vif_index,
			      const IPvX& src, const IPvX& dst,
			      int ip_ttl, int ip_tos, bool router_alert_bool,
			      const uint8_t *databuf, size_t datalen)
{
    struct ip	*ip;
    uint32_t	ip_option;
    uint32_t	*ip_option_p;
    int		ip_hdr_len = 0;
    int		setloop = false;
    int		ret;
    MfeaVif	*mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    if (! mfea_vif->is_up()) {
	// The vif is DOWN. Silently ignore the packet
	return (XORP_ERROR);
    }
    
    //
    // Assign the TTL and TOS if they were not specified
    //
    switch (family()) {
    case AF_INET:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (router_alert_bool)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (router_alert_bool)
		ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
	    else
		ip_tos = 0;
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (router_alert_bool)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (router_alert_bool) {
		// TODO: XXX: IPTOS for IPv6 is bogus??
		// ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
		ip_tos = 0;
	    } else {
		ip_tos = 0;
	    }
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
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
	ip = (struct ip *)_sndbuf0;
	if (router_alert_bool) {
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
	ip->ip_p	= _ipproto;
	ip->ip_ttl	= ip_ttl;
	ip->ip_tos	= ip_tos;
	
	src.copy_out(ip->ip_src);
	dst.copy_out(ip->ip_dst);
	ip->ip_len = (ip->ip_hl << 2) + datalen;
#ifdef IPV4_RAW_OUTPUT_IS_RAW
	ip->ip_len = htons(ip->ip_len);
#endif
	ip->ip_sum = 0;					// Let kernel fill in
	
	// Now hook the data
	dst.copy_out(_to4);
	_sndmh.msg_namelen	= sizeof(_to4);
	_sndmh.msg_iovlen	= 2;
	_sndiov[0].iov_len	= ip_hdr_len;
	if (datalen > IO_BUF_SIZE) {
	    XLOG_ERROR("proto_socket_write() failed: "
		       "cannot send packet on vif %s from %s to %s: "
		       "too much data: %u octets (max = %u)",
		       mfea_vif->name().c_str(),
		       src.str().c_str(),
		       dst.str().c_str(),
		       (uint32_t)datalen,
		       (uint32_t)IO_BUF_SIZE);
	    return (XORP_ERROR);
	}
    	memcpy(_sndbuf1, databuf, datalen); // XXX: goes to _sndiov[1].iov_base
	// _sndiov[1].iov_base	= (caddr_t)databuf;
	_sndiov[1].iov_len	= datalen;
	_sndmh.msg_control	= NULL;
	_sndmh.msg_controllen	= 0;
	
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
	
	if (router_alert_bool) {
	    // Space for Router Alert option
#ifdef HAVE_RFC2292BIS
	    if ((hbhlen = inet6_opt_init(NULL, 0)) == -1)
		XLOG_ERROR("inet6_opt_init(NULL) failed");
	    if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen,
					   IP6OPT_ROUTER_ALERT, 2,
					   2, NULL)) == -1) {
		XLOG_ERROR("inet6_opt_append(NULL) failed");
		return (XORP_ERROR);
	    }
	    if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1) {
		XLOG_ERROR("inet6_opt_finish(NULL) failed");
		return (XORP_ERROR);
	    }
	    ctllen += CMSG_SPACE(hbhlen);
#else
	    hbhlen = inet6_option_space(sizeof(raopt));
	    ctllen += hbhlen;
#endif // ! HAVE_RFC2292BIS
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
	sndpktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
	memset(sndpktinfo, 0, sizeof(*sndpktinfo));
	if (mfea_vif->pif_index() > 0)
	    sndpktinfo->ipi6_ifindex = mfea_vif->pif_index();
	else
	    sndpktinfo->ipi6_ifindex = 0;		// Let kernel fill in
	src.copy_out(sndpktinfo->ipi6_addr);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Include the Router Alert option if needed
	//
	if (router_alert_bool) {
#ifdef HAVE_RFC2292BIS
	    int currentlen;
	    void *hbhbuf, *optp = NULL;
	    
	    cmsgp->cmsg_len   = CMSG_LEN(hbhlen);
	    cmsgp->cmsg_level = IPPROTO_IPV6;
	    cmsgp->cmsg_type  = IPV6_HOPOPTS;
	    hbhbuf = CMSG_DATA(cmsgp);
	    currentlen = inet6_opt_init(hbhbuf, hbhlen);
	    if (currentlen == -1) {
		XLOG_ERROR("inet6_opt_init(len = %d) failed", hbhlen);
		return (XORP_ERROR);
	    }
	    currentlen = inet6_opt_append(hbhbuf, hbhlen, currentlen,
					  IP6OPT_ROUTER_ALERT, 2, 2, &optp);
	    if (currentlen == -1) {
		XLOG_ERROR("inet6_opt_append(len = %d) failed", currentlen);
		return (XORP_ERROR);
	    }
	    inet6_opt_set_val(optp, 0, &rtalert_code, sizeof(rtalert_code));
	    if (inet6_opt_finish(hbhbuf, hbhlen, currentlen) == -1) {
		XLOG_ERROR("inet6_opt_finish(len = %d) failed", currentlen);
		return (XORP_ERROR);
	    }
	    
#else  // ! HAVE_RFC2292BIS (i.e., the old advanced API)
	    
	    if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS)) {
		XLOG_ERROR("inet6_option_init(IPV6_HOPOPTS) failed");
		return (XORP_ERROR);
	    }
	    if (inet6_option_append(cmsgp, raopt, 4, 0)) {
		XLOG_ERROR("inet6_option_append(Router Alert) failed");
		return (XORP_ERROR);
	    }
	    
#endif // ! HAVE_RFC2292BIS
	    
	    cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	}
	
	//
	// Set the TTL
	//
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_HOPLIMIT;
	*(int *)(CMSG_DATA(cmsgp)) = ip_ttl;
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Set the TOS
	//
#ifdef IPV6_TCLASS
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_TCLASS;
	*(int *)(CMSG_DATA(cmsgp)) = ip_tos;
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
#endif // IPV6_TCLASS
	
	//
	// Now hook the data
	//
	dst.copy_out(_to6);
	_sndmh.msg_namelen  = sizeof(_to6);
	_sndmh.msg_iovlen   = 1;
	if (datalen > IO_BUF_SIZE) {
	    XLOG_ERROR("proto_socket_write() failed: "
		       "error sending packet on vif %s from %s to %s: "
		       "too much data: %u octets (max = %u)",
		       mfea_vif->name().c_str(),
		       src.str().c_str(),
		       dst.str().c_str(),
		       (uint32_t)datalen,
		       (uint32_t)IO_BUF_SIZE);
	    return (XORP_ERROR);
	}
	memcpy(_sndbuf0, databuf, datalen); // XXX: goes to _sndiov[0].iov_base
	// _sndiov[0].iov_base	= (caddr_t)databuf;
	_sndiov[0].iov_len  = datalen;
	
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    setloop = false;
    if (dst.is_multicast()) {
	set_default_multicast_vif(mfea_vif->vif_index());
	set_multicast_loop(true);
	setloop = true;
    }
    
    ret = XORP_OK;
    if (sendmsg(_proto_socket, &_sndmh, 0) < 0) {
	ret = XORP_ERROR;
	if (errno == ENETDOWN) {
	    // TODO: check the interface status. E.g. vif_state_check(family);
	} else {
	    XLOG_ERROR("sendmsg(proto %d from %s to %s on vif %s) failed: %s",
		       _ipproto, cstring(src), cstring(dst),
		       mfea_vif->name().c_str(), strerror(errno));
	}
    }
    if (setloop) {
	set_multicast_loop(false);
    }
    
    return (ret);
}


