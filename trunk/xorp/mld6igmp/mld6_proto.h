/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2003 International Computer Science Institute
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
 * $XORP: xorp/mld6igmp/mld6_proto.h,v 1.5 2003/04/16 16:39:17 pavlin Exp $
 */


#ifndef __MLD6IGMP_MLD6_PROTO_H__
#define __MLD6IGMP_MLD6_PROTO_H__


/*
 * Multicast Listener Discovery protocol-specific definitions.
 * MLDv1 (RFC 2710)
 */


#ifdef HAVE_IPV6_MULTICAST_ROUTING

#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif


/*
 * Constants definitions
 */
/* MLD versions definition */
#define MLD_V1					1
#define MLD_V2					2
#define MLD_VERSION_MIN				MLD_V1
#define MLD_VERSION_MAX				MLD_V1
#define MLD_VERSION_DEFAULT			MLD_V1

/*
 * Backward-compatibility definitions.
 * On newer systems, all MLD-related definitions use
 * mld_xxx and MLD_XXX instead of mld6_xxx and MLD6_XXX.
 */
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
#endif /* ! MLD_MINLEN */

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
#ifndef MLD_TIMER_SCALE
/* the MLD max. response delay is in 1000th of seconds */
#define MLD_TIMER_SCALE				1000
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

#endif /* HAVE_IPV6_MULTICAST_ROUTING */

#endif /* __MPD6IGMP_MLD6_PROTO_H__ */
