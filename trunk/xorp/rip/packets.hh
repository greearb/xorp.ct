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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_PACKETS_HH__
#define __RIP_PACKETS_HH__

/**
 * @short Header appearing at the start of each RIP packet.
 */
struct RipPacketHeader {
    uint8_t command;		// 1 - request, 2 - response
    uint8_t version;		// 1 - IPv4 RIPv1/IPv6 RIPng, 2 - IPv4 RIP v2
    uint8_t unused[2];
};

/**
 * @short Route Entry as it appears in a RIP Packet.
 *
 * This structure is useful only it's specialized forms
 * @ref PacketRouteEntry<IPv4> and @ref PacketRouteEntry<IPv6>.
 */
template <typename A>
struct PacketRouteEntry {
};

/**
 * @short Route Entry appearing in RIP packets on IPv4.
 *
 * This payload is carried in RIP packets on IPv4.  The entry contains
 * all the fields for RIPv2.  RIPv1 and RIPv2 use the same size structure,
 * except RIPv1 treats the route tag, subnet mask and nexthop fields as
 * Must-Be-Zero (MBZ) items.  The interpretation of the fields is described
 * in RFC2453.
 *
 * All items in the packet are stored in network order.
 */
struct PacketRouteEntry<IPv4> {
    uint16_t addr_family;	// 2 - IPv4
    uint16_t route_tag;
    uint32_t ip_addr;
    uint32_t next_hop;
    uint32_t metric;
};

/**
 * @short Route Entry appearing in RIP packets on IPv6.
 *
 * This payload is carried in RIP packets on IPv6.  The interpretation
 * of the fields is defined in RFC2080.
 *
 * All fields in this structure are stored in network order.
 */
struct PacketRouteEntry<IPv6> {
    uint32_t prefix[4];
    uint16_t route_tag;
    uint8_t  prefix_length;
    uint8_t  metric;
};

#endif // __RIP_PACKETS_HH__
