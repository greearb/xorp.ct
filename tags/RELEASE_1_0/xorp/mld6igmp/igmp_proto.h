/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2004 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/mld6igmp/igmp_proto.h,v 1.3 2003/04/15 18:55:37 pavlin Exp $
 */

#ifndef __MLD6IGMP_IGMP_PROTO_H__
#define __MLD6IGMP_IGMP_PROTO_H__


/*
 * Internet Group Management Protocol protocol-specific definitions.
 * IGMPv1 and IGMPv2 (RFC 2236)
 */


#include <netinet/igmp.h>


/*
 * Constants definitions
 */
/* IGMP versions definition */
#define IGMP_V1					1
#define IGMP_V2					2
#define IGMP_V3					3
#define IGMP_VERSION_MIN			IGMP_V1
#define IGMP_VERSION_MAX			IGMP_V2
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
 * Backward-compatibility definitions
 */
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
#ifndef IGMP_MEMBERSHIP_QUERY
#define IGMP_MEMBERSHIP_QUERY		IGMP_HOST_MEMBERSHIP_QUERY
#endif
#ifndef IGMP_V1_MEMBERSHIP_REPORT
#define IGMP_V1_MEMBERSHIP_REPORT	IGMP_v1_HOST_MEMBERSHIP_REPORT
#endif
#ifndef IGMP_V2_MEMBERSHIP_REPORT
#define IGMP_V2_MEMBERSHIP_REPORT	IGMP_v2_HOST_MEMBERSHIP_REPORT
#endif
#ifndef IGMP_V2_LEAVE_GROUP
#define IGMP_V2_LEAVE_GROUP		IGMP_HOST_LEAVE_MESSAGE
#endif
#ifndef IGMP_MTRACE_RESP
#define IGMP_MTRACE_RESP		IGMP_MTRACE_REPLY
#endif
#ifndef IGMP_MTRACE
#define IGMP_MTRACE			IGMP_MTRACE_QUERY
#endif
#ifndef IGMP_V3_MEMBERSHIP_REPORT
#  ifdef IGMP_v3_HOST_MEMBERSHIP_REPORT
#    define IGMP_V3_MEMBERSHIP_REPORT	IGMP_v3_HOST_MEMBERSHIP_REPORT
#  endif
#endif
#endif /* HOST_OS_NETBSD || HOST_OS_OPENBSD */

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
