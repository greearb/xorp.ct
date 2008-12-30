/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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
 * $XORP: xorp/mld6igmp/igmp_proto.h,v 1.19 2008/07/23 05:11:03 pavlin Exp $
 */

#ifndef __MLD6IGMP_IGMP_PROTO_H__
#define __MLD6IGMP_IGMP_PROTO_H__


/*
 * Internet Group Management Protocol protocol-specific definitions:
 * IGMPv1 and IGMPv2 (RFC 2236), and IGMPv3 (RFC 3376).
 */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IGMP_H
#include <netinet/igmp.h>
#endif


/*
 * Constants definitions
 */
#ifndef IPPROTO_IGMP
#define IPPROTO_IGMP				2
#endif

/* IGMP versions definition */
#define IGMP_V1					1
#define IGMP_V2					2
#define IGMP_V3					3
#define IGMP_VERSION_MIN			IGMP_V1
#define IGMP_VERSION_MAX			IGMP_V3
#define IGMP_VERSION_DEFAULT			IGMP_V2

/*
 * Constants for IGMP Version 2.
 * All intervals are in seconds.
 * XXX: several of these, especially the robustness variable, should be
 * variables and not constants.
 * XXX: some of the definitions are copied from mrouted code.
 */
#define	IGMP_ROBUSTNESS_VARIABLE		2
#define	IGMP_QUERY_INTERVAL			125
#define	IGMP_QUERY_RESPONSE_INTERVAL		10
#define	IGMP_GROUP_MEMBERSHIP_INTERVAL		(IGMP_ROBUSTNESS_VARIABLE     \
						* IGMP_QUERY_INTERVAL	      \
						+ IGMP_QUERY_RESPONSE_INTERVAL)
#define	IGMP_OTHER_QUERIER_PRESENT_INTERVAL	(IGMP_ROBUSTNESS_VARIABLE     \
						* IGMP_QUERY_INTERVAL	      \
					+ IGMP_QUERY_RESPONSE_INTERVAL / 2)
#define	IGMP_STARTUP_QUERY_INTERVAL		(IGMP_QUERY_INTERVAL / 4)
#define	IGMP_STARTUP_QUERY_COUNT		IGMP_ROBUSTNESS_VARIABLE
#define	IGMP_LAST_MEMBER_QUERY_INTERVAL		1
#define	IGMP_LAST_MEMBER_QUERY_COUNT		IGMP_ROBUSTNESS_VARIABLE
#define IGMP_VERSION1_ROUTER_PRESENT_TIMEOUT	400
#define IGMP_OLDER_VERSION_HOST_PRESENT_INTERVAL (IGMP_ROBUSTNESS_VARIABLE    \
						* IGMP_QUERY_INTERVAL	      \
						+ IGMP_QUERY_RESPONSE_INTERVAL)
#ifndef IGMP_TIMER_SCALE
/* the igmp code field is in 10th of seconds */
#define IGMP_TIMER_SCALE			10
#endif

/*
 * DVMRP message types from mrouted/mrinfo
 * (carried in the "code" field of an IGMP header)
 */
/* TODO: remove the things we don't need */
#define DVMRP_PROBE		1	/* for finding neighbors             */
#define DVMRP_REPORT		2	/* for reporting some or all routes  */
#define DVMRP_ASK_NEIGHBORS	3	/* sent by mapper, asking for a list */
					/* of this router's neighbors.	     */
#define DVMRP_NEIGHBORS		4	/* response to such a request	     */
#define DVMRP_ASK_NEIGHBORS2	5	/* as above, want new format reply   */
#define DVMRP_NEIGHBORS2	6
#define DVMRP_PRUNE		7	/* prune message		     */
#define DVMRP_GRAFT		8	/* graft message		     */
#define DVMRP_GRAFT_ACK		9	/* graft acknowledgement	     */
#define DVMRP_INFO_REQUEST	10	/* information request		     */
#define DVMRP_INFO_REPLY	11	/* information reply		     */

/*
 * 'flags' byte values in DVMRP_NEIGHBORS2 reply.
 */
#define DVMRP_NF_TUNNEL		0x01	/* neighbors reached via tunnel	     */
#define DVMRP_NF_SRCRT		0x02	/* tunnel uses IP source routing     */
#define DVMRP_NF_PIM		0x04	/* neighbor is a PIM neighbor	     */
#define DVMRP_NF_DOWN		0x10	/* kernel state of interface	     */
#define DVMRP_NF_DISABLED	0x20	/* administratively disabled	     */
#define DVMRP_NF_QUERIER	0x40	/* I am the subnet's querier	     */
#define DVMRP_NF_LEAF		0x80	/* Neighbor reports that it is a leaf*/

/*
 * Request/reply types for info queries/replies
 */
#define DVMRP_INFO_VERSION	1	/* version string		     */
#define DVMRP_INFO_NEIGHBORS	2	/* neighbors2 data		     */

/*
 * IGMPv1,v2-related missing definitions
 */
#ifndef IGMP_MEMBERSHIP_QUERY
#  ifdef IGMP_HOST_MEMBERSHIP_QUERY
#    define IGMP_MEMBERSHIP_QUERY	IGMP_HOST_MEMBERSHIP_QUERY
#  else
#    define IGMP_MEMBERSHIP_QUERY	0x11
#  endif
#endif

#ifndef IGMP_V1_MEMBERSHIP_REPORT
#  ifdef IGMP_v1_HOST_MEMBERSHIP_REPORT
#    define IGMP_V1_MEMBERSHIP_REPORT	IGMP_v1_HOST_MEMBERSHIP_REPORT
#  else
#    define IGMP_V1_MEMBERSHIP_REPORT	0x12
#  endif
#endif

#ifndef IGMP_V2_MEMBERSHIP_REPORT
#  ifdef IGMP_v2_HOST_MEMBERSHIP_REPORT
#    define IGMP_V2_MEMBERSHIP_REPORT	IGMP_v2_HOST_MEMBERSHIP_REPORT
#  else
#    define IGMP_V2_MEMBERSHIP_REPORT	0x16
#  endif
#endif

#ifndef IGMP_V2_LEAVE_GROUP
#  ifdef IGMP_HOST_LEAVE_MESSAGE
#    define IGMP_V2_LEAVE_GROUP		IGMP_HOST_LEAVE_MESSAGE
#  else
#    define IGMP_V2_LEAVE_GROUP		0x17
#  endif
#endif

#ifndef IGMP_DVMRP
#  define IGMP_DVMRP			0x13
#endif

#ifndef IGMP_PIM
#  define IGMP_PIM			0x14
#endif

#ifndef IGMP_MTRACE_RESP
#  ifdef IGMP_MTRACE_REPLY
#    define IGMP_MTRACE_RESP		IGMP_MTRACE_REPLY
#  else
#    define IGMP_MTRACE_RESP		0x1e
#  endif
#endif

#ifndef IGMP_MTRACE
#  ifdef IGMP_MTRACE_QUERY
#    define IGMP_MTRACE			IGMP_MTRACE_QUERY
#  else
#    define IGMP_MTRACE			0x1f
#  endif
#endif

#ifndef IGMP_MINLEN
#  define IGMP_MINLEN			8
#endif

/*
 * IGMPv3-related missing definitions
 */
#ifndef IGMP_V3_MEMBERSHIP_REPORT
#  ifdef IGMP_v3_HOST_MEMBERSHIP_REPORT
#    define IGMP_V3_MEMBERSHIP_REPORT	IGMP_v3_HOST_MEMBERSHIP_REPORT
#  else
#    define IGMP_V3_MEMBERSHIP_REPORT	0x22
#  endif
#endif

#ifndef IGMP_MODE_IS_INCLUDE
#  define IGMP_MODE_IS_INCLUDE		1
#endif

#ifndef IGMP_MODE_IS_EXCLUDE
#  define IGMP_MODE_IS_EXCLUDE		2
#endif

#ifndef IGMP_CHANGE_TO_INCLUDE_MODE
#  define IGMP_CHANGE_TO_INCLUDE_MODE	3
#endif

#ifndef IGMP_CHANGE_TO_EXCLUDE_MODE
#  define IGMP_CHANGE_TO_EXCLUDE_MODE	4
#endif

#ifndef IGMP_ALLOW_NEW_SOURCES
#  define IGMP_ALLOW_NEW_SOURCES	5
#endif

#ifndef IGMP_BLOCK_OLD_SOURCES
#  define IGMP_BLOCK_OLD_SOURCES	6
#endif

#ifndef IGMP_V3_QUERY_MINLEN
#  define IGMP_V3_QUERY_MINLEN		12
#endif

#ifndef IGMP_EXP
#  define IGMP_EXP(x)			(((x) >> 4) & 0x07)
#endif

#ifndef IGMP_MANT
#  define IGMP_MANT(x)			((x) & 0x0f)
#endif

#ifndef IGMP_QRESV
#  define GMP_QRESV(x)			(((x) >> 4) & 0x0f)
#endif

#ifndef IGMP_SFLAG
#  define IGMP_SFLAG(x)			(((x) >> 3) & 0x01)
#endif

#ifndef IGMP_QRV
#  define IGMP_QRV(x)			((x) & 0x07)
#endif

/*
 * Structures, typedefs and macros
 */

/*
 * The ASCII names of the IGMP protocol control messages
 */
#ifdef IGMP_V3_MEMBERSHIP_REPORT
#define IGMPV3TYPE2ASCII(t)						\
(((t) == IGMP_V3_MEMBERSHIP_REPORT) ?					\
    "IGMP_V3_MEMBERSHIP_REPORT"						\
    : "IGMP_type_unknown")
#else
#define IGMPV3TYPE2ASCII(t)	"IGMP_type_unknown"
#endif

#define IGMPTYPE2ASCII(t)						\
(((t) == IGMP_MEMBERSHIP_QUERY) ?					\
    "IGMP_MEMBERSHIP_QUERY"						\
    : ((t) == IGMP_V1_MEMBERSHIP_REPORT) ?				\
	"IGMP_V1_MEMBERSHIP_REPORT"					\
	: ((t) == IGMP_V2_MEMBERSHIP_REPORT) ?				\
	    "IGMP_V2_MEMBERSHIP_REPORT"					\
	    : ((t) == IGMP_V2_LEAVE_GROUP) ?				\
		"IGMP_V2_LEAVE_GROUP"					\
		: ((t) == IGMP_DVMRP) ?					\
		    "IGMP_DVMRP"					\
		    : ((t) == IGMP_PIM) ?				\
			"IGMP_PIM"					\
			: ((t) == IGMP_MTRACE_RESP) ?			\
			    "IGMP_MTRACE_RESP"				\
			    : ((t) == IGMP_MTRACE) ?			\
				"IGMP_MTRACE"				\
				: IGMPV3TYPE2ASCII(t))

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS

#endif /* __MLD6IGMP_IGMP_PROTO_H__ */
