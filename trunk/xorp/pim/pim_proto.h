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
 * $XORP: xorp/pim/pim_proto.h,v 1.1.1.1 2002/12/11 23:56:12 hodson Exp $
 */


#ifndef __PIM_PIM_PROTO_H__
#define __PIM_PIM_PROTO_H__


/*
 * Protocol Independent Multicast protocol-specific definitions
 * (both for PIM-SMv2 and PIM-DMv2).
 */


/* XXX: _PIM_VT is needed if we want the extra features of <netinet/pim.h> */
#define _PIM_VT 1

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#else
#include "mrt/include/netinet/pim.h"
#endif


/*
 * Constants definitions
 */
/* PIM versions definition */
#define PIMSM_V1				1
#define PIMSM_V2				2
#define PIMSM_VERSION_MIN			PIMSM_V2
#define PIMSM_VERSION_MAX			PIMSM_V2
#define PIMSM_VERSION_DEFAULT			PIMSM_V2
#define PIMDM_V1				1
#define PIMDM_V2				2
#define PIMDM_VERSION_MIN			PIMDM_V2
#define PIMDM_VERSION_MAX			PIMDM_V2
#define PIMDM_VERSION_DEFAULT			PIMDM_V2
#define PIM_V1					(proto_is_pimsm() ?	      \
							PIMSM_V1	      \
							: PIMDM_V1)
#define PIM_V2					(proto_is_pimsm() ?	      \
							PIMSM_V2	      \
							: PIMDM_V2)
#define PIM_VERSION_MIN				(proto_is_pimsm() ?	      \
							PIMSM_VERSION_MIN     \
							: PIMDM_VERSION_MIN)
#define PIM_VERSION_MAX				(proto_is_pimsm() ?	      \
							PIMSM_VERSION_MAX     \
							: PIMDM_VERSION_MAX)
#define PIM_VERSION_DEFAULT			(proto_is_pimsm() ?	      \
							PIMSM_VERSION_DEFAULT \
							: PIMDM_VERSION_DEFAULT)

/*
 * Protocol messages specific definitions.
 * XXX: all intervals are in seconds.
 */

/* PIM_HELLO-related definitions */
#define PIM_HELLO_HOLDTIME_OPTION		1
#define PIM_HELLO_HOLDTIME_LENGTH		2
#define PIM_HELLO_HOLDTIME_FOREVER		0xffff
#define PIM_HELLO_LAN_PRUNE_DELAY_OPTION	2
#define PIM_HELLO_LAN_PRUNE_DELAY_LENGTH	4
#define PIM_HELLO_LAN_PRUNE_DELAY_TBIT		(1 << 15)
#define PIM_HELLO_DR_ELECTION_PRIORITY_OPTION	19
#define PIM_HELLO_DR_ELECTION_PRIORITY_LENGTH	4
#define PIM_HELLO_DR_ELECTION_PRIORITY_DEFAULT	1
#define PIM_HELLO_DR_ELECTION_PRIORITY_LOWEST	0
#define PIM_HELLO_DR_ELECTION_PRIORITY_HIGHEST	0xffffffffU
#define PIM_HELLO_GENID_OPTION			20
#define PIM_HELLO_GENID_LENGTH			4

#define PIM_HELLO_HELLO_TRIGGERED_DELAY_DEFAULT	5
#define PIM_HELLO_HELLO_PERIOD_DEFAULT		30
#define PIM_HELLO_HELLO_HOLDTIME_PERIOD_RATIO	3.5
#define PIM_HELLO_HELLO_HOLDTIME_DEFAULT	((int)(PIM_HELLO_HELLO_HOLDTIME_PERIOD_RATIO * PIM_HELLO_HELLO_PERIOD_DEFAULT))

#define LAN_DELAY_MSEC_DEFAULT			500	/* LAN_delay_default */
#define LAN_OVERRIDE_INTERVAL_MSEC_DEFAULT	2500   /* t_override_default */


/* PIM_JOIN_PRUNE-related definitions */
#define PIM_JOIN_PRUNE_PERIOD_DEFAULT		60
#define PIM_JOIN_PRUNE_HOLDTIME_PERIOD_RATIO	3.5
#define PIM_JOIN_PRUNE_HOLDTIME_DEFAULT		((int)(PIM_JOIN_PRUNE_HOLDTIME_PERIOD_RATIO * PIM_JOIN_PRUNE_PERIOD_DEFAULT))
#define PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MIN 1.1
#define PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MAX 1.4
#define PIM_JOIN_PRUNE_OIF_HOLDTIME_FOREVER	0xffff

/* PIM_ASSERT-related definitions */
#define PIM_ASSERT_ASSERT_TIME_DEFAULT		180
#define PIM_ASSERT_ASSERT_OVERRIDE_INTERVAL_DEFAULT 3
#define PIM_ASSERT_MAX_METRIC_PREFERENCE	0x7fffffffU
#define PIM_ASSERT_MAX_METRIC			0xffffffffU
#define PIM_ASSERT_OIF_RATE_LIMIT		1	/* 1 pkt/second */
#define PIM_ASSERT_RPT_BIT			(1 << 31)

/* PIM_REGISTER-related definitions */
#define PIM_REGISTER_SUPPRESSION_TIME_DEFAULT	60
#define PIM_REGISTER_PROBE_TIME_DEFAULT		5

/* PIM_CAND_RP_ADV-related definitions */
#define PIM_CAND_RP_ADV_PERIOD_DEFAULT		60
#define PIM_CAND_RP_ADV_RP_HOLDTIME_DEFAULT	((int)(2.5 * PIM_CAND_RP_ADV_PERIOD_DEFAULT))
#define PIM_CAND_RP_ADV_RP_PRIORITY_DEFAULT	192

/* PIM_BOOTSTRAP-related definitions */
#define PIM_BOOTSTRAP_LOWEST_PRIORITY		0	/* Larger is better */
#define PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT	60
#define PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT	((int)(2 * PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT + 10))
#define PIM_BOOTSTRAP_RAND_OVERRIDE_DEFAULT	5
#define PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT ((int)(10 * PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT))
#define PIM_BOOTSTRAP_HASH_MASKLEN_IPV4_DEFAULT	30
#define PIM_BOOTSTRAP_HASH_MASKLEN_IPV6_DEFAULT	126
#define PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(ip_family)			\
			((ip_family == AF_INET)?			\
				PIM_BOOTSTRAP_HASH_MASKLEN_IPV4_DEFAULT	\
				: PIM_BOOTSTRAP_HASH_MASKLEN_IPV6_DEFAULT)


/* Other timeout default values */
#define PIM_KEEPALIVE_PERIOD_DEFAULT		210
#define PIM_RP_KEEPALIVE_PERIOD_DEFAULT		(3 * PIM_REGISTER_SUPPRESSION_TIME_DEFAULT + PIM_REGISTER_PROBE_TIME_DEFAULT)

/*
 * Structures, typedefs and macros
 */
/*
 * Address-family related definitions. See this link for a
 * complete listing of all address families:
 *    http://www.isi.edu/in-notes/iana/assignments/address-family-numbers
 */
#define ADDRF_IPv4		1
#define ADDRF_IPv6		2
#define ADDRF_NATIVE_ENCODING	0		/* Type of encoding within
						 * a specific address family
						 */

#ifndef HAVE_IPV6
#define ADDRF2IP_ADDRF(addr_family)					\
		(((addr_family) == ADDRF_IPv4) ? (AF_INET) : (-1))
#define IP_ADDRF2ADDRF(ip_family)					\
		(((ip_family) == AF_INET) ? (ADDRF_IPv4) : (-1))

#else
#define ADDRF2IP_ADDRF(addr_family)					\
		(((addr_family) == ADDRF_IPv4) ? (AF_INET) 		\
			: ((addr_family) == ADDRF_IPv6) ?		\
				(AF_INET6)				\
				: (-1))
#define IP_ADDRF2ADDRF(ip_family)					\
		(((ip_family) == AF_INET) ? (ADDRF_IPv4)		\
			: ((ip_family) == AF_INET6) ?			\
				(ADDRF_IPv6)				\
				: (-1))

#endif /* HAVE_IPV6 */

#define ESADDR_RPT_BIT		0x1
#define ESADDR_WC_BIT		0x2
#define ESADDR_S_BIT		0x4

#define EGADDR_Z_BIT		0x1

/* PIM message max. payload (IP header and Router Alert IP option excluded) */
#ifndef HAVE_IPV6
#define PIM_MAXPACKET(ip_family) (((ip_family) == AF_INET) ?		\
					(IP_MAXPACKET			\
						- sizeof(struct ip)	\
						- 4*sizeof(uint8_t))	\
					: (0))
#else
#define PIM_MAXPACKET(ip_family) (((ip_family) == AF_INET) ?		\
					(IP_MAXPACKET			\
					- sizeof(struct ip)		\
					- 4*sizeof(uint8_t))		\
					: ((ip_family) == AF_INET6) ?	\
						(IPV6_MAXPACKET		\
						- 4*sizeof(uint8_t))	\
						: (0))
#endif /* HAVE_IPV6 */

/*
 * Macros to get PIM-specific encoded addresses from a datastream.
 *
 * XXX: the macros below assume that the code has the following
 * labels that can be used to jump and process the particular error:
 *	'rcvd_family_error' 'rcvd_masklen_error' 'rcvlen_error'
 * The also assume that function family() is defined (to return
 * the IP address family within the current context.
 */
/* XXX: family2addr_size should be defined somewhere */
#ifndef FAMILY2ADDRSIZE
#define FAMILY2ADDRSIZE(ip_family) family2addr_size(ip_family)
#endif
#ifndef FAMILY2PREFIXLEN
#define FAMILY2PREFIXLEN(ip_family) family2addr_bitlen(ip_family)
#endif
#define GET_ENCODED_UNICAST_ADDR(rcvd_family, unicast_ipaddr, buffer)	\
do {									\
	int addr_family_;						\
									\
	BUFFER_GET_OCTET(addr_family_, (buffer));			\
	(rcvd_family) = ADDRF2IP_ADDRF(addr_family_);			\
	if ((rcvd_family) != family())					\
		goto rcvd_family_error;					\
	BUFFER_GET_SKIP(1, (buffer));		/* Encoding type */	\
	BUFFER_GET_IPADDR((rcvd_family), (unicast_ipaddr), (buffer));	\
} while (0)
#define ENCODED_UNICAST_ADDR_SIZE(ip_family)				\
			(2*sizeof(uint8_t) + FAMILY2ADDRSIZE(ip_family))

#define GET_ENCODED_GROUP_ADDR(rcvd_family, group_ipaddr, masklen, reserved, buffer) \
do {									\
	int addr_family_;						\
									\
	BUFFER_GET_OCTET(addr_family_, (buffer));			\
	(rcvd_family) = ADDRF2IP_ADDRF(addr_family_);			\
	if ((rcvd_family) != family())					\
		goto rcvd_family_error;					\
	BUFFER_GET_SKIP(1, (buffer));		/* Encoding type */	\
	BUFFER_GET_OCTET((reserved), (buffer));				\
	BUFFER_GET_OCTET((masklen), (buffer));				\
	BUFFER_GET_IPADDR((rcvd_family), (group_ipaddr), (buffer));	\
	if ((u_int)(masklen) > FAMILY2PREFIXLEN((rcvd_family)))		\
		goto rcvd_masklen_error;				\
} while (0)
#define ENCODED_GROUP_ADDR_SIZE(ip_family)				\
			(4*sizeof(uint8_t) + FAMILY2ADDRSIZE(ip_family))

#define GET_ENCODED_SOURCE_ADDR(rcvd_family, source_ipaddr, masklen, flags, buffer) \
do {									\
	int addr_family_;						\
									\
	BUFFER_GET_OCTET(addr_family_, (buffer));			\
	(rcvd_family) = ADDRF2IP_ADDRF(addr_family_);			\
	if ((rcvd_family) != family())					\
		goto rcvd_family_error;					\
	BUFFER_GET_SKIP(1, (buffer));		/* Encoding type */	\
	BUFFER_GET_OCTET((flags), (buffer));				\
	BUFFER_GET_OCTET((masklen), (buffer));				\
	BUFFER_GET_IPADDR((rcvd_family), (source_ipaddr), (buffer));	\
	if ((u_int)(masklen) > FAMILY2PREFIXLEN((rcvd_family)))		\
		goto rcvd_masklen_error;				\
} while (0)
#define ENCODED_SOURCE_ADDR_SIZE(ip_family)				\
			(4*sizeof(uint8_t) + FAMILY2ADDRSIZE(ip_family))

/*
 * Macros to put PIM-specific encoded addresses to a datastream.
 *
 * XXX: the macros below assume that the code has the following
 * labels that can be used to jump and process the particular error:
 *	'invalid_addr_family_error' 'buflen_error'
 */
#define PUT_ENCODED_UNICAST_ADDR(ip_family, unicast_ipaddr, buffer)	\
do {									\
	int addr_family_;						\
									\
	addr_family_ = IP_ADDRF2ADDRF((ip_family));			\
	if (addr_family_ < 0)						\
		goto invalid_addr_family_error;				\
	BUFFER_PUT_OCTET(addr_family_, (buffer));			\
	BUFFER_PUT_OCTET(ADDRF_NATIVE_ENCODING, (buffer));		\
	BUFFER_PUT_IPADDR((unicast_ipaddr), (buffer));			\
} while (0)

#define PUT_ENCODED_GROUP_ADDR(ip_family, group_ipaddr, masklen, reserved, buffer)\
do {									\
	int addr_family_;						\
									\
	addr_family_ = IP_ADDRF2ADDRF((ip_family));			\
	if (addr_family_ < 0)						\
		goto invalid_addr_family_error;				\
	BUFFER_PUT_OCTET(addr_family_, (buffer));			\
	BUFFER_PUT_OCTET(ADDRF_NATIVE_ENCODING, (buffer));		\
	BUFFER_PUT_OCTET(reserved, (buffer));	/* E.g., EGADDR_Z_BIT */\
	BUFFER_PUT_OCTET((masklen), (buffer));				\
	BUFFER_PUT_IPADDR((group_ipaddr), (buffer));			\
} while (0)

#define PUT_ENCODED_SOURCE_ADDR(ip_family, source_ipaddr, masklen, flags, buffer) \
do {									\
	int addr_family_;						\
									\
	addr_family_ = IP_ADDRF2ADDRF((ip_family));			\
	if (addr_family_ < 0)						\
		goto invalid_addr_family_error;				\
	BUFFER_PUT_OCTET(addr_family_, (buffer));			\
	BUFFER_PUT_OCTET(ADDRF_NATIVE_ENCODING, (buffer));		\
	BUFFER_PUT_OCTET((flags), (buffer));				\
	BUFFER_PUT_OCTET((masklen), (buffer));				\
	BUFFER_PUT_IPADDR((source_ipaddr), (buffer));			\
} while (0)

#define BUFFER_PUT_SKIP_PIM_HEADER(buffer)				\
do {									\
	BUFFER_PUT_SKIP(sizeof(struct pim), (buffer));			\
} while (0)

/*
 * The ASCII names of the PIM protocol control messages
 */
#define PIMTYPE2ASCII(t)						\
(((t) == PIM_HELLO) ?							\
    "PIM_HELLO"								\
    : ((t) == PIM_REGISTER) ?						\
	"PIM_REGISTER"							\
	: ((t) == PIM_REGISTER_STOP) ?					\
	    "PIM_REGISTER_STOP"						\
	    : ((t) == PIM_JOIN_PRUNE) ?					\
		"PIM_JOIN_PRUNE"					\
		: ((t) == PIM_BOOTSTRAP) ?				\
		    "PIM_BOOTSTRAP"					\
		    : ((t) == PIM_ASSERT) ?				\
			"PIM_ASSERT"					\
			: ((t) == PIM_GRAFT) ?				\
			    "PIM_GRAFT"					\
			    : ((t) == PIM_GRAFT_ACK) ?			\
				"PIM_GRAFT_ACK"				\
				: ((t) == PIM_CAND_RP_ADV) ?		\
				    "PIM_CAND_RP_ADV"			\
				    : "PIM_type_unknown")

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

__END_DECLS

#endif /* __PIM_PIM_PROTO_H__ */
