/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * $XORP: xorp/mld6igmp/mld6_proto.h,v 1.23 2008/10/02 21:57:43 bms Exp $
 */

#ifndef __MLD6IGMP_MLD6_PROTO_H__
#define __MLD6IGMP_MLD6_PROTO_H__


/*
 * Multicast Listener Discovery protocol-specific definitions.
 * MLDv1 (RFC 2710), and MLDv2 (RFC 3810).
 */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif


/*
 * Constants definitions
 */
#ifndef IPPROTO_ICMPV6
#define IPPROTO_ICMPV6				58
#endif

/* MLD versions definition */
#define MLD_V1					1
#define MLD_V2					2
#define MLD_VERSION_MIN				MLD_V1
#define MLD_VERSION_MAX				MLD_V2
#define MLD_VERSION_DEFAULT			MLD_V1

/*
 * Constans for Multicast Listener Discovery protocol for IPv6.
 * All intervals are in seconds.
 * XXX: Several of these, especially the robustness variable, should be
 * variables and not constants.
 */
#define	MLD_ROBUSTNESS_VARIABLE			2
#define	MLD_QUERY_INTERVAL			125
#define	MLD_QUERY_RESPONSE_INTERVAL		10
#define	MLD_MULTICAST_LISTENER_INTERVAL		(MLD_ROBUSTNESS_VARIABLE      \
						* MLD_QUERY_INTERVAL	      \
						+ MLD_QUERY_RESPONSE_INTERVAL)
#define	MLD_OTHER_QUERIER_PRESENT_INTERVAL	(MLD_ROBUSTNESS_VARIABLE      \
						* MLD_QUERY_INTERVAL	      \
					+ MLD_QUERY_RESPONSE_INTERVAL / 2)
#define	MLD_STARTUP_QUERY_INTERVAL		(MLD_QUERY_INTERVAL / 4)
#define	MLD_STARTUP_QUERY_COUNT			MLD_ROBUSTNESS_VARIABLE
#define	MLD_LAST_LISTENER_QUERY_INTERVAL	1
#define	MLD_LAST_LISTENER_QUERY_COUNT		MLD_ROBUSTNESS_VARIABLE
#define MLD_OLDER_VERSION_HOST_PRESENT_INTERVAL (MLD_ROBUSTNESS_VARIABLE      \
						* MLD_QUERY_INTERVAL	      \
						+ MLD_QUERY_RESPONSE_INTERVAL)
#ifndef MLD_TIMER_SCALE
/* the MLD max. response delay is in 1000th of seconds */
#define MLD_TIMER_SCALE				1000
#endif

/*
 * MLDv1-related missing definitions
 *
 * Note that on newer systems all MLD-related definitions use
 * mld_xxx and MLD_XXX instead of mld6_xxx and MLD6_XXX.
 */
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

#ifndef MLD_MINLEN
#  ifdef HAVE_STRUCT_MLD_HDR
#    define MLD_MINLEN			(sizeof(struct mld_hdr))
#  else
#    define MLD_MINLEN			24
#  endif
#endif

/*
 * MLDv2-related missing definitions
 */
#ifndef MLDV2_LISTENER_REPORT
#  ifdef MLD6V2_LISTENER_REPORT
#    define MLDV2_LISTENER_REPORT	MLD6V2_LISTENER_REPORT
#  else
#    define MLDV2_LISTENER_REPORT	143
#  endif
#endif

#ifndef MLD_MODE_IS_INCLUDE
#  define MLD_MODE_IS_INCLUDE		1
#endif

#ifndef MLD_MODE_IS_EXCLUDE
#  define MLD_MODE_IS_EXCLUDE		2
#endif

#ifndef MLD_CHANGE_TO_INCLUDE_MODE
#  define MLD_CHANGE_TO_INCLUDE_MODE	3
#endif

#ifndef MLD_CHANGE_TO_EXCLUDE_MODE
#  define MLD_CHANGE_TO_EXCLUDE_MODE	4
#endif

#ifndef MLD_ALLOW_NEW_SOURCES
#  define MLD_ALLOW_NEW_SOURCES		5
#endif

#ifndef MLD_BLOCK_OLD_SOURCES
#  define MLD_BLOCK_OLD_SOURCES		6
#endif

#ifndef MLD_V2_QUERY_MINLEN
#  define MLD_V2_QUERY_MINLEN		28
#endif

#ifndef MLD_HOST_MRC_EXP
#  define MLD_HOST_MRC_EXP(x)		(((x) >> 12) & 0x0007)
#endif

#ifndef MLD_HOST_MRC_MANT
#  define MLD_HOST_MRC_MANT(x)		((x) & 0x0fff)
#endif

#ifndef MLD_QQIC_EXP
#  define MLD_QQIC_EXP(x)		(((x) >> 4) & 0x07)
#endif

#ifndef MLD_QQIC_MANT
#  define MLD_QQIC_MANT(x)		((x) & 0x0f)
#endif

#ifndef MLD_QRESV
#  define MLD_QRESV(x)			(((x) >> 4) & 0x0f)
#endif

#ifndef MLD_SFLAG
#  define MLD_SFLAG(x)			(((x) >> 3) & 0x01)
#endif

#ifndef MLD_QRV
#  define MLD_QRV(x)			((x) & 0x07)
#endif

/*
 * Structures, typedefs and macros
 */
/*
 * The ASCII names of the MLD protocol control messages
 */
#ifdef MLDV2_LISTENER_REPORT
#define MLDV2TYPE2ASCII(t)						\
(((t) == MLDV2_LISTENER_REPORT) ?					\
    "MLDV2_LISTENER_REPORT"						\
    : "MLD_type_unknown")
#else
#define MLDV2TYPE2ASCII(t)	"MLD_type_unknown"
#endif

#define MLDTYPE2ASCII(t)						\
(((t) == MLD_LISTENER_QUERY) ?						\
    "MLD_LISTENER_QUERY"						\
    : ((t) == MLD_LISTENER_REPORT) ?					\
	"MLD_LISTENER_REPORT"						\
	: ((t) == MLD_LISTENER_DONE) ?					\
	    "MLD_LISTENER_DONE"						\
	    : ((t) == MLD_MTRACE_RESP) ?				\
		"MLD_MTRACE_RESP"					\
		: ((t) == MLD_MTRACE) ?					\
		    "MLD_MTRACE"					\
		    : MLDV2TYPE2ASCII(t))

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS

#endif /* __MPD6IGMP_MLD6_PROTO_H__ */
