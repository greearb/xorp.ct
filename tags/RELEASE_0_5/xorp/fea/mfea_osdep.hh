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

// $XORP: xorp/fea/mfea_osdep.hh,v 1.2 2003/05/16 19:23:17 pavlin Exp $

#ifndef __FEA_MFEA_OSDEP_HH__
#define __FEA_MFEA_OSDEP_HH__


//
// A header file that adds various definitions that may be missing from the OS
//


#include "config.h"

#include <sys/time.h>

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


//
// Backward-compatibility definitions.
// On newer systems, all MLD-related definitions use
// mld_xxx and MLD_XXX instead of mld6_xxx and MLD6_XXX.
//
#ifdef HAVE_IPV6_MULTICAST_ROUTING
#ifndef MLD_LISTENER_QUERY
#define MLD_LISTENER_QUERY		MLD6_LISTENER_QUERY
#endif
#ifndef MLD_LISTENER_REPORT
#define MLD_LISTENER_REPORT		MLD6_LISTENER_REPORT
#endif
#ifndef MLD_LISTENER_DONE
#define MLD_LISTENER_DONE		MLD6_LISTENER_DONE
#endif
#ifndef MLD_MTRACE_RESP
#define MLD_MTRACE_RESP			MLD6_MTRACE_RESP
#endif
#ifndef MLD_MTRACE
#define MLD_MTRACE			MLD6_MTRACE
#endif
#ifndef MLDV2_LISTENER_REPORT
#  ifdef MLD6V2_LISTENER_REPORT
#    define MLDV2_LISTENER_REPORT	MLD6V2_LISTENER_REPORT
#  endif
#endif
#ifndef MLD_MINLEN
#  ifdef HAVE_MLD_HDR
#    define MLD_MINLEN	(sizeof(struct mld_hdr))
#  else
#    define MLD_MINLEN	(sizeof(struct mld6_hdr))
#  endif
#endif // ! MLD_MINLEN
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

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __FEA_MFEA_OSDEP_HH__
