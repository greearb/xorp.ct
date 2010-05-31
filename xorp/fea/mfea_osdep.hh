// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/mfea_osdep.hh,v 1.14 2008/10/02 21:56:50 bms Exp $

#ifndef __FEA_MFEA_OSDEP_HH__
#define __FEA_MFEA_OSDEP_HH__


//
// A header file that adds various definitions that may be missing from the OS
//


#include "libxorp/xorp.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif

#include "mrt/include/ip_mroute.h"

#include "mfea_kernel_messages.hh"

//
// Constants definitions
//

//
// Structures, typedefs and macros
//

//
// XXX: misc. (eventually) missing definitions
// TODO: those should go somewhere else
//
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
// XXX: in *BSD there is only MRT_ASSERT, but in Linux there are both
// MRT_ASSERT and MRT_PIM (with MRT_PIM defined as a superset of MRT_ASSERT).
//
#ifndef MRT_PIM
#define MRT_PIM			MRT_ASSERT
#endif

#ifndef IP6OPT_RTALERT
#define IP6OPT_RTALERT		0x05
#endif
#ifndef IP6OPT_RTALERT_LEN
#define IP6OPT_RTALERT_LEN	4
#endif
#ifndef IP6OPT_RTALERT_MLD
#define IP6OPT_RTALERT_MLD	0
#endif

//
// MLD-related missing definitions
//
// Note that on newer systems all MLD-related definitions use
// mld_xxx and MLD_XXX instead of mld6_xxx and MLD6_XXX.
//
#ifdef HAVE_IPV6_MULTICAST_ROUTING

#ifndef MLD_LISTENER_QUERY
#  ifdef MLD6_LISTENER_QUERY
#    define MLD_LISTENER_QUERY		MLD6_LISTENER_QUERY
#  else
#    define MLD_LISTENER_QUERY		130
#  endif
#endif

#ifndef MLD_LISTENER_REPORT
#  ifdef MLD6_LISTENER_REPORT
#    define MLD_LISTENER_REPORT		MLD6_LISTENER_REPORT
#  else
#    define MLD_LISTENER_REPORT		131
#  endif
#endif

#ifndef MLD_LISTENER_DONE
#  ifdef MLD6_LISTENER_DONE
#    define MLD_LISTENER_DONE		MLD6_LISTENER_DONE
#  else
#    define MLD_LISTENER_DONE		132
#  endif
#endif

#ifndef MLD_MTRACE_RESP
#  ifdef MLD6_MTRACE_RESP
#    define MLD_MTRACE_RESP		MLD6_MTRACE_RESP
#  else
#    define MLD_MTRACE_RESP		200
#  endif
#endif

#ifndef MLD_MTRACE
#  ifdef MLD6_MTRACE
#    define MLD_MTRACE			MLD6_MTRACE
#  else
#    define MLD_MTRACE			201
#  endif
#endif

#ifndef MLDV2_LISTENER_REPORT
#  ifdef MLD6V2_LISTENER_REPORT
#    define MLDV2_LISTENER_REPORT	MLD6V2_LISTENER_REPORT
#  else
#    define MLDV2_LISTENER_REPORT	143
#  endif
#endif

#ifndef MLD_MINLEN
#  ifdef HAVE_STRUCT_MLD_HDR
#    define MLD_MINLEN			(sizeof(struct mld_hdr))
#  else
#    define MLD_MINLEN			24
#  endif
#endif

#endif // HAVE_IPV6_MULTICAST_ROUTING


//
// Test if the kernel multicast signal message types are consistent
// between IPv4 and IPv6.
// E.g., if (IGMPMSG_NOCACHE == MRT6MSG_NOCACHE)
//          (IGMPMSG_WRONGVIF == MRT6MSG_WRONGMIF)
//          (IGMPMSG_WHOLEPKT == MRT6MSG_WHOLEPKT)
//          (IGMPMSG_BW_UPCALL == MRT6MSG_BW_UPCALL)
// Also, check if MFEA_KERNEL_MESSAGE_NOCACHE/WRONGVIF/WHOLEPKT/BW_UPCALL
// were defined accurately.
// TODO: Yes, I know this is a very, very bad style, but I wanted to have
// abstract kernel signal types, and I didn't want the upper layer
// protocols to use IGMPMSG/MRT6MSG, and to have special cases
// for IPv4 and IPv6. Maybe later this will change...
//

#ifdef HAVE_IPV6_MULTICAST_ROUTING

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

#endif // HAVE_IPV6_MULTICAST_ROUTING

#ifdef HAVE_IPV4_MULTICAST_ROUTING

#if IGMPMSG_NOCACHE != MFEA_KERNEL_MESSAGE_NOCACHE
#  error "MFEA message handling needs fix, because IGMPMSG_NOCACHE != MFEA_KERNEL_MESSAGE_NOCACHE"
#endif

#if IGMPMSG_WRONGVIF != MFEA_KERNEL_MESSAGE_WRONGVIF
#  error "MFEA message handling needs fix, because IGMPMSG_WRONGVIF != MFEA_KERNEL_MESSAGE_WRONGVIF"
#endif

#if IGMPMSG_WHOLEPKT != MFEA_KERNEL_MESSAGE_WHOLEPKT
#  error "MFEA message handling needs fix, because IGMPMSG_WHOLEPKT != MFEA_KERNEL_MESSAGE_WHOLEPKT"
#endif

#if defined(IGMPMSG_BW_UPCALL)
#if IGMPMSG_BW_UPCALL != MFEA_KERNEL_MESSAGE_BW_UPCALL
#  error "MFEA message handling needs fix, because IGMPMSG_BW_UPCALL != MFEA_KERNEL_MESSAGE_BW_UPCALL"
#endif
#endif

#if defined(MRT6MSG_BW_UPCALL)
#if MRT6MSG_BW_UPCALL != MFEA_KERNEL_MESSAGE_BW_UPCALL
#  error "MFEA message handling needs fix, because MRT6MSG_BW_UPCALL != MFEA_KERNEL_MESSAGE_BW_UPCALL"
#endif
#endif

#endif // HAVE_IPV4_MULTICAST_ROUTING

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __FEA_MFEA_OSDEP_HH__
