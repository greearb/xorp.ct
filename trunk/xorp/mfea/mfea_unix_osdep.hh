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

// $XORP: xorp/mfea/mfea_unix_osdep.hh,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $

#ifndef __MFEA_MFEA_UNIX_OSDEP_HH__
#define __MFEA_MFEA_UNIX_OSDEP_HH__


//
// The complementary header file for the UN*X kernel access module.
// Covers various kernel access methods (SYSCTL, IOCTL, RAWSOCK, NETLINK)
// used to obtain information about network interfaces, MRIB information, and
// unicast forwarding table.
//


#include "config.h"

#include <sys/time.h>

#include "mrt/include/ip_mroute.h"


//
// Constants definitions
//
//
// Define and assign the various kernel access methods:
// - read list of interfaces
// - read unicast reverse-path forwarding information (to be used as MRIB)
// - read unicast routing table
// XXX: the methods for each type of access are in preference order.
//
#define KERNEL_IF_GETIFADDRS	1
#define KERNEL_IF_SYSCTL	2
#define KERNEL_IF_IOCTL		3
#define KERNEL_MRIB_RAWSOCK	1
#define KERNEL_MRIB_NETLINK	2
#define KERNEL_RTREAD_SYSCTL	1
#define KERNEL_RTREAD_NETLINK	2

//#undef HAVE_GETIFADDRS
//#undef HAVE_NET_RT_IFLIST

// The interface list access method
#undef KERNEL_IF_METHOD
#if defined(HAVE_GETIFADDRS)
#  define KERNEL_IF_METHOD KERNEL_IF_GETIFADDRS
#elif defined(HAVE_NET_RT_IFLIST)
#  define KERNEL_IF_METHOD KERNEL_IF_SYSCTL
#elif defined(HAVE_SIOCGIFCONF)
#  define KERNEL_IF_METHOD KERNEL_IF_IOCTL
#else
#  error "The system does not appear to have a mechanism to read the"
#  error "list of interfaces from the kernel."
#  error "The methods I know about are:"
#  error "  - getifaddrs(3)"
#  error "  - sysctl(NET_RT_IFLIST)"
#  error "  - or ioctl(SIOCGIFCONF)"
#  error "You need a better OS!"
#endif // KERNEL_IF_METHOD


// The unicast reverse-path forwarding information (to be used as MRIB)
#undef  KERNEL_MRIB_METHOD
#if defined(HAVE_AF_ROUTE)
#  define KERNEL_MRIB_METHOD KERNEL_MRIB_RAWSOCK
#elif defined(HAVE_AF_NETLINK)
#  define KERNEL_MRIB_METHOD KERNEL_MRIB_NETLINK
#else
#  error "The system does not appear to have a mechanism to read the"
#  error "Reverse-Path Forwarding information from the kernel that would"
#  error "be used as Multicast Routing Information Base."
#  error "The methods I know about are:"
#  error "  - routing sockets: socket(AF_ROUTE)"
#  error "  - Linux's NETLINK sockets: socket(AF_NETLINK)"
#  error "You need a better OS!"
#endif // KERNEL_MRIB_METHOD

// The unicast routing table read mechanism
#undef KERNEL_RTREAD_METHOD
#if defined(HAVE_NET_RT_DUMP)
#  define KERNEL_RTREAD_METHOD KERNEL_RTREAD_SYSCTL
#elif defined(HAVE_AF_NETLINK)
#  define KERNEL_RTREAD_METHOD KERNEL_RTREAD_NETLINK
#else
#  error "The system does not appear to have a mechanism to read the"
#  error "unicast routing table from the kernel."
#  error "The methods I know about are:"
#  error "  - sysctl(NET_RT_DUMP)"
#  error "  - Linux's NETLINK sockets: socket(AF_NETLINK)"
#  error "You need a better OS!"
#endif // KERNEL_MRIB_METHOD


//
// Structures, typedefs and macros
//
#ifdef HAVE_SA_LEN
	// Round up 'a' to next multiple of 'size'
#define ROUNDUP(a, size) (((a) & ((size)-1)) ?				\
				(1 + ((a) | ((size)-1)))		\
				: (a))
//
// Step to next socket address structure.
// If sa_len is 0, assume it is sizeof(u_long).
//
#define NEXT_SA(sa)							\
do {									\
	(sa) = (struct sockaddr *) ((caddr_t) (sa) + ((sa)->sa_len ?	\
		ROUNDUP((sa)->sa_len, sizeof(u_long))			\
		: sizeof(u_long)));					\
} while (0)
    
#else // ! HAVE_SA_LEN
#ifdef HAVE_IPV6
#define NEXT_SA(sa)							\
do {									\
	switch ((sa)->sin_family) {					\
	case AF_INET:							\
	    (sa) = (struct sockaddr *) ((sa) + sizeof(struct sockaddr_in));  \
	    break;							\
	case AF_INET6:							\
	    (sa) = (struct sockaddr *) ((sa) + sizeof(struct sockaddr_in6)); \
	    break;							\
	default:							\
	    (sa) = (struct sockaddr *) ((sa) + sizeof(struct sockaddr_in));  \
	    break;							\
	}								\
} while (0)
#else // ! HAVE_IPV6
#define NEXT_SA(sa)							\
do {									\
	switch ((sa)->sin_family) {					\
	case AF_INET:							\
	    (sa) = (struct sockaddr *) ((sa) + sizeof(struct sockaddr_in));  \
	    break;							\
	default:							\
	    (sa) = (struct sockaddr *) ((sa) + sizeof(struct sockaddr_in));  \
	    break;							\
	}								\
} while (0)
#endif // ! HAVE_IPV6
#endif // ! HAVE_SA_LEN

#if defined(HAVE_IPV6) && defined(IPV6_STACK_KAME)
//
// XXX: a hack for KAME, because it encodes the local interface
// index (also the link-local scope_id) in the third and fourth
// octet of an IPv6 address (for LINKLOCAL addresses only).
//
#define KERNEL_SADDR_ADJUST(sa6)					\
do {									\
	struct sockaddr_in6 *sin6;					\
	struct in6_addr *in6_addr;					\
									\
	sin6 = (struct sockaddr_in6 *)sa6;				\
	if (sin6->sin6_family != AF_INET6)				\
		break;							\
	in6_addr = &sin6->sin6_addr;					\
	if (IN6_IS_ADDR_LINKLOCAL(in6_addr)) {				\
		in6_addr->s6_addr[2] = 0;				\
		in6_addr->s6_addr[3] = 0;				\
	}								\
} while (0)
#else // ! HAVE_IPV6 && IPV6_STACK_KAME
#define KERNEL_SADDR_ADJUST(sa6)					\
do {									\
} while (0);
#endif // ! HAVE_IPV6 && IPV6_STACK_KAME

//
// XXX: misc. (eventually) missing definitions
// TODO: those should go somewhere else
//
#ifndef MULTICAST_MIN_TTL_THRESHOLD_DEFAULT
#define MULTICAST_MIN_TTL_THRESHOLD_DEFAULT	1
#endif
#ifndef INADDR_ALLRTRS_GROUP
#define INADDR_ALLRTRS_GROUP	(uint32_t)0xe0000002U	// 224.0.0.2
#endif
#ifndef VIFF_REGISTER
#define VIFF_REGISTER		0x4	// Vif for PIM Register encap/decap
#endif
#ifndef IGMPMSG_WHOLEPKT
#define IGMPMSG_WHOLEPKT	3	// Whole packet sent from the kernel to
					// the user-level process (typically
					// a multicast data packet for PIM
					// register encapsulation).

#endif
#ifndef IPPROTO_PIM
#define IPPROTO_PIM		103	// Protocol Independent Multicast
#endif

//
// Test if the kernel multicast signal message types are consistent
// between IPv4 and IPv6.
// E.g., if (IGMPMSG_NOCACHE == MRT6MSG_NOCACHE)
//          (IGMPMSG_WRONGVIF == MRT6MSG_WRONGMIF)
//          (IGMPMSG_WHOLEPKT == MRT6MSG_WHOLEPKT)
//          (IGMPMSG_BW_UPCALL == MRT6MSG_BW_UPCALL)
// Also, check if MFEA_UNIX_KERNEL_MESSAGE_NOCACHE/WRONGVIF/WHOLEPKT/BW_UPCALL
// were defined accurately.
// TODO: Yes, I know this is a very, very bad style, but I wanted to have
// abstract kernel signal types, and I didn't want the upper layer
// protocols to use IGMPMSG/MRT6MSG, and to have special cases
// for IPv4 and IPv6. Maybe later this will change...
//

#ifdef HAVE_IPV6

#if IGMPMSG_NOCACHE != MRT6MSG_NOCACHE
#  error "MFEA message handling needs fix, because IGMPMSG_NOCACHE != MRT6MSG_NOCACHE"
#endif

#if IGMPMSG_WRONGVIF != MRT6MSG_WRONGMIF
#  error "MFEA message handling needs fix, because IGMPMSG_WRONGVIF != MRT6MSG_WRONGMIF"
#endif

#if IGMPMSG_WHOLEPKT != MRT6MSG_WHOLEPKT
#  error "MFEA message handling needs fix, because IGMPMSG_WHOLEPKT != MRT6MSG_WHOLEPKT"
#endif

#if defined(IGMPMSG_BW_UPCALL) && defined(MRT6MSG_BW_UPCALL)
#if IGMPMSG_BW_UPCALL != MRT6MSG_BW_UPCALL
#  error "MFEA message handling needs fix, because IGMPMSG_BW_UPCALL != MRT6MSG_BW_UPCALL"
#endif
#endif

#endif // HAVE_IPV6

#if IGMPMSG_NOCACHE != MFEA_UNIX_KERNEL_MESSAGE_NOCACHE
#  error "MFEA message handling needs fix, because IGMPMSG_NOCACHE != MFEA_UNIX_KERNEL_MESSAGE_NOCACHE"
#endif

#if IGMPMSG_WRONGVIF != MFEA_UNIX_KERNEL_MESSAGE_WRONGVIF
#  error "MFEA message handling needs fix, because IGMPMSG_WRONGVIF != MFEA_UNIX_KERNEL_MESSAGE_WRONGVIF"
#endif

#if IGMPMSG_WHOLEPKT != MFEA_UNIX_KERNEL_MESSAGE_WHOLEPKT
#  error "MFEA message handling needs fix, because IGMPMSG_WHOLEPKT != MFEA_UNIX_KERNEL_MESSAGE_WHOLEPKT"
#endif

#if defined(IGMPMSG_BW_UPCALL)
#if IGMPMSG_BW_UPCALL != MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL
#  error "MFEA message handling needs fix, because IGMPMSG_BW_UPCALL != MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL"
#endif
#endif

#if defined(MRT6MSG_BW_UPCALL)
#if MRT6MSG_BW_UPCALL != MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL
#  error "MFEA message handling needs fix, because MRT6MSG_BW_UPCALL != MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL"
#endif
#endif

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MFEA_MFEA_UNIX_OSDEP_HH__
