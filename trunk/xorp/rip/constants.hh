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

// $XORP: xorp/rip/constants.hh,v 1.11 2004/02/14 00:28:22 hodson Exp $

#ifndef __RIP_CONSTANTS_HH__
#define __RIP_CONSTANTS_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

static const uint32_t DEFAULT_UNSOLICITED_RESPONSE_SECS = 30;
static const uint32_t DEFAULT_VARIATION_UNSOLICITED_RESPONSE_SECS = 5;

static const uint32_t DEFAULT_EXPIRY_SECS = 180;
static const uint32_t DEFAULT_DELETION_SECS = 120;

static const uint32_t DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS = 1;
static const uint32_t DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS = 5;

static const uint32_t DEFAULT_TABLE_REQUEST_SECS = 1;

static const uint32_t RIPv2_ROUTES_PER_PACKET = 25;

/**
 * The default delay between back to back RIP packets when an update
 * is sent that spans more than 1 packet or there are multiple packets
 * to be sent.
 */
static const uint32_t DEFAULT_INTERPACKET_DELAY_MS = 50;

/**
 * The maximum delay between back to back RIP packets when an update
 * is sent that spans more than 1 packet.
 */
static const uint32_t MAXIMUM_INTERPACKET_DELAY_MS = 250;

/**
 * The default delay between accepting route request packets that
 * query specific routes.
 */
static const uint32_t DEFAULT_INTERQUERY_GAP_MS = 250;

/**
 * Protocol specified metric corresponding to an unreachable (or expired)
 * host or network.
 */
static const uint32_t RIP_INFINITY = 16;

/**
 * Time-To-Live value that should be used for multicast packets.
 */
static const uint32_t RIP_TTL = 1;

/**
 * RIP IPv4 protocol port
 */
static const uint16_t RIP_PORT = 520;

/**
 * RIPng protocol port
 */
static const uint16_t RIP_NG_PORT = 521;

static const IPv4Net IPv4_DEFAULT_ROUTE = IPv4Net(IPv4::ZERO(), 0);
static const IPv6Net IPv6_DEFAULT_ROUTE = IPv6Net(IPv6::ZERO(), 0);

/**
 * Basis of specialized classes containing RIP constants.
 */
template <typename A>
struct RIP_AF_CONSTANTS;

template <>
struct RIP_AF_CONSTANTS<IPv4>
{
    static inline const IPv4	 IP_GROUP() { return IPv4::RIP2_ROUTERS(); }
    static const uint16_t	 IP_PORT = RIP_PORT;
    static inline const IPv4Net& DEFAULT_ROUTE() { return IPv4_DEFAULT_ROUTE; }
    static const uint8_t PACKET_VERSION = 2;
};

template <>
struct RIP_AF_CONSTANTS<IPv6>
{
    static inline const IPv6&	 IP_GROUP() { return IPv6::RIP2_ROUTERS(); }
    static const uint16_t	 IP_PORT = RIP_NG_PORT;
    static inline const IPv6Net& DEFAULT_ROUTE() { return IPv6_DEFAULT_ROUTE; }
    static const uint8_t PACKET_VERSION = 1;
};

/**
 * Enumeration of RIP Horizon types.
 */
enum RipHorizon {
    // No filtering
    NONE		 = 0,
    // Don't a route origin its own routes.
    SPLIT		 = 1,
    // Show a route origin its own routes but with cost of infinity.
    SPLIT_POISON_REVERSE = 2
};

inline static const char*
rip_horizon_name(RipHorizon h)
{
    switch (h) {
    case SPLIT_POISON_REVERSE:
	return "split-horizon-poison-reverse";
    case SPLIT:
	return "split-horizon";
    case NONE:
	break;
    }
    return "none";
};

#endif // __RIP_CONSTANTS_HH__
