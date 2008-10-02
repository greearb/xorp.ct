// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libxipc/xuid.cc,v 1.14 2008/07/23 05:10:47 pavlin Exp $"

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/utility.h"
#include "libxorp/timer.hh"
#include "libxorp/timeval.hh"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "xuid.hh"


static const char* sfmt = "%08x-%08x-%08x-%08x";

static uint32_t
local_ip4_addr()
{
    static uint32_t cached_addr;
    struct in_addr ia;

    char name[MAXHOSTNAMELEN];

    if (cached_addr) return cached_addr;

    if (gethostname(name, sizeof(name)/sizeof(name[0]))) {
        return 0;
    }

    if (inet_pton(AF_INET, name, &ia) != 1) {
        struct hostent *h = gethostbyname(name);
        if (h == NULL) {
            return 0;
        }
        memcpy(&ia, h->h_addr_list[0], sizeof(ia));
    }

    cached_addr = ia.s_addr;
    return cached_addr;
}

void
XUID::initialize()
{
    static TimeVal last;	// last time clock reading value
    static uint16_t ticks;	// number of ticks with same clock reading

    // Component 1: Local IPv4 Address - returned in network order
    uint32_t hid = local_ip4_addr();
    _data[0] = hid;

    // Component 2: Time since midnight (0 hour), January 1, 1970
    TimeVal now;
    TimerList::system_gettimeofday(&now);
    _data[1] = htonl(now.sec());
    _data[2] = htonl(now.usec());

    // Component 3: Process ID
    uint16_t pid = (uint16_t)getpid();

    // See if we are issuing xuid's at a critical rate and sleep if
    // ticks are advancing too quickly...a pretty unlikely event
    if (now == last) {
        ticks++;
        if ((ticks & 0x7fff) == 0x7fff) {
	    TimerList::system_sleep(TimeVal(0, 100000));
	}
    } else {
        ticks = 0;
	last = now;
    }

    // Component 4: ticks with same clock readings
    uint16_t tid = ticks;

    _data[3] = htonl((pid << 16) + tid);
}

static const uint32_t XUID_CSTR_BYTES = (32 + 3);

XUID::XUID(const string& s) throw (InvalidString)
{
    static_assert(sizeof(_data) == 16);
    static_assert(sizeof(_data[0]) == 4);

    if (s.size() < XUID_CSTR_BYTES)
	throw InvalidString();

    if (sscanf(s.c_str(), sfmt, &_data[0], &_data[1], &_data[2], &_data[3])
	!= 4) {
            throw InvalidString();
    }
    for (int i = 0; i < 4; i++) {
	_data[i] = htonl(_data[i]);
    }
}

bool
XUID::operator==(const XUID& x) const
{
    return (memcmp(x._data, _data, sizeof(_data)) == 0);
}

bool
XUID::operator<(const XUID& x) const
{
    static_assert(sizeof(_data) == 16);
    static_assert(sizeof(_data[0]) == 4);
    int i;

    for (i = 0; i < 3; i++) {	// Loop ends intentionally at 3 not 4.
	if (_data[i] != x._data[i]) {
	    break;
	}
    }
    return ntohl(_data[i]) < ntohl(x._data[i]);
}

string
XUID::str() const
{
    char dst[XUID_CSTR_BYTES + 1];
    snprintf(dst, sizeof(dst) / sizeof(dst[0]), sfmt, ntohl(_data[0]),
	     ntohl(_data[1]), ntohl(_data[2]), ntohl(_data[3]));
    return string(dst);
}


