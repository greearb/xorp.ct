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

// $XORP: xorp/rip/constants.hh,v 1.2 2003/04/11 22:00:18 hodson Exp $

#ifndef __RIP_CONSTANTS_HH__
#define __RIP_CONSTANTS_HH__

static const uint32_t DEFAULT_UNSOLICITED_RESPONSE_SECS = 30;
static const uint32_t DEFAULT_VARIATION_UNSOLICITED_RESPONSE_SECS = 5;

static const uint32_t DEFAULT_EXPIRY_SECS = 180;
static const uint32_t DEFAULT_DELETION_SECS = 120;

static const uint32_t DEFAULT_TRIGGERED_UPDATE_MIN_WAIT_SECS = 1;
static const uint32_t DEFAULT_TRIGGERED_UPDATE_MAX_WAIT_SECS = 5;

static const uint32_t RIPv2_ROUTES_PER_PACKET = 25;

/**
 * The default delay between back to back RIP packets when an update
 * is sent that spans more than 1 packet or there are multiple packets
 * to be sent.
 */
static const uint32_t DEFAULT_INTERPACKET_DELAY_MS = 50;

/**
 * Protocol specified metric corresponding to an unreachable (or expired)
 * host or network.
 */
static const uint32_t RIP_INFINITY = 16;

/**
 * RIP IPv4 protocol port
 */
static const uint16_t RIP_PORT = 520;

/**
 * RIPng protocol port
 */
static const uint16_t RIP_NG_PORT = 521;

/**
 * Enumeration of RIP Horizon types.
 */
enum RipHorizon {
    // No filtering
    NONE,			
    // Don't a route origin its own routes.
    SPLIT,
    // Show a route origin its own routes but with cost of infinity.
    SPLIT_POISON_REVERSE
};

#endif // __RIP_CONSTANTS_HH__
