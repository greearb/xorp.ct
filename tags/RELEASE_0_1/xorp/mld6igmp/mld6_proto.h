/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001,2002 International Computer Science Institute
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
 * $XORP: xorp/mld6igmp/mld6_proto.h,v 1.4 2002/12/09 18:29:20 hodson Exp $
 */


#ifndef __MLD6IGMP_MLD6_PROTO_H__
#define __MLD6IGMP_MLD6_PROTO_H__


/*
 * Multicast Listener Discovery protocol-specific definitions.
 * MLDv1 (RFC 2710)
 */


#ifdef HAVE_IPV6

#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif


/*
 * Constants definitions
 */
/* MLD6 versions definition */
#define MLD6_V1					1
#define MLD6_VERSION_MIN			MLD6_V1
#define MLD6_VERSION_MAX			MLD6_V1
#define MLD6_VERSION_DEFAULT			MLD6_V1


/*
 * Constans for Multicast Listener Discovery protocol for IPv6.
 * All intervals are in seconds.
 * XXX: Several of these, especially the robustness variable, should be
 * variables and not constants.
 */
#define	MLD6_ROBUSTNESS_VARIABLE		2
#define	MLD6_QUERY_INTERVAL			125
#define	MLD6_QUERY_RESPONSE_INTERVAL		10
#define	MLD6_MULTICAST_LISTENER_INTERVAL	(MLD6_ROBUSTNESS_VARIABLE     \
						* MLD6_QUERY_INTERVAL	      \
						+ MLD6_QUERY_RESPONSE_INTERVAL)
#define	MLD6_OTHER_QUERIER_PRESENT_INTERVAL	(MLD6_ROBUSTNESS_VARIABLE     \
						* MLD6_QUERY_INTERVAL	      \
					+ MLD6_QUERY_RESPONSE_INTERVAL / 2)
#define	MLD6_STARTUP_QUERY_INTERVAL		(MLD6_QUERY_INTERVAL / 4)
#define	MLD6_STARTUP_QUERY_COUNT		MLD6_ROBUSTNESS_VARIABLE
#define	MLD6_LAST_LISTENER_QUERY_INTERVAL	1
#define	MLD6_LAST_LISTENER_QUERY_COUNT		MLD6_ROBUSTNESS_VARIABLE
#ifndef MLD6_TIMER_SCALE
/* the mld6 max. response delay is in 1000th of seconds */
#define MLD6_TIMER_SCALE			1000
#endif

/*
 * Structures, typedefs and macros
 */
/*
 * The ASCII names of the MLD6 protocol control messages
 */
#define MLD6TYPE2ASCII(t)						\
(((t) == MLD6_LISTENER_QUERY) ?						\
    "MLD6_LISTENER_QUERY"						\
    : ((t) == MLD6_LISTENER_REPORT) ?					\
	"MLD6_LISTENER_REPORT"						\
	: ((t) == MLD6_LISTENER_DONE) ?					\
	    "MLD6_LISTENER_DONE"					\
	    : ((t) == MLD6_MTRACE_RESP) ?				\
		"MLD6_MTRACE_RESP"					\
		: ((t) == MLD6_MTRACE) ?				\
		    "MLD6_MTRACE"					\
		    : "MLD6_type_unknown")

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS

#endif /* HAVE_IPV6 */

#endif /* __MPD6IGMP_MLD6_PROTO_H__ */
