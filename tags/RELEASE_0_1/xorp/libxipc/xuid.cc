// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/xuid.cc,v 1.17 2002/12/09 18:29:10 hodson Exp $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#include "xrl_module.h"
#include "config.h"
#include "libxorp/utility.h"
#include "libxorp/timeval.hh"
#include "xuid.hh"

static const char* sfmt = "%08x-%08x-%08x-%08x";

static uint32_t 
local_ip4_addr() {
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
XUID::initialize() {
    static struct timeval last;	// last time clock reading value
    static uint16_t ticks;	// number of ticks with same clock reading

    // Component 1: Local IPv4 Address - returned in network order
    uint32_t hid = local_ip4_addr();
    _data[0] = hid;

    // Component 2: Time since midnight (0 hour), January 1, 1970
    struct timeval now;
    gettimeofday(&now, NULL);
    _data[1] = htonl(now.tv_sec);
    _data[2] = htonl(now.tv_usec);

    // Component 3: Process ID
    uint16_t pid = (uint16_t)getpid();

    // See if we are issuing xuid's at a critical rate and sleep if
    // ticks are advancing too quickly...a pretty unlikely event
    if (now == last) {
        ticks++;
        if ((ticks & 0x7fff) == 0x7fff) {
	    usleep(100000);
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

XUID::XUID(const string& s) throw (InvalidString) {
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
XUID::operator==(const XUID& x) const {
    return (memcmp(x._data, _data, sizeof(_data)) == 0);
}

bool
XUID::operator<(const XUID& x) const {
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


