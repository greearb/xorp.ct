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

#ident "$XORP: xorp/bgp/packet.cc,v 1.4 2003/01/21 16:56:59 rizzo Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "packet.hh"

/* **************** BGPPacket *********************** */

const uint8_t BGPPacket::Marker[MARKER_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const uint8_t *
BGPPacket::flatten(struct iovec *iov, const int cnt, int& len) const
{
    len = 0;
    for (int i = 0; i < cnt; i++) {
	debug_msg("length = %d\n",  (iov + i)->iov_len);
	len += (iov + i)->iov_len;
    }

    uint8_t *ptr = new uint8_t[len];

    uint8_t *base = ptr;
    for (int i = 0; i < cnt; i++) {
	memcpy(base, (iov + i)->iov_base, (iov + i)->iov_len);
	base += (iov + i)->iov_len;
    }

    return ptr;
}

uint8_t *
BGPPacket::basic_encode(int &len, uint8_t *buf) const
{
    len = length();
    if (buf == 0)
	buf = new uint8_t[len];
    memcpy(buf, Marker, MARKER_SIZE);
    uint16_t l = htons(length());
    memcpy(buf + MARKER_SIZE, &l, 2);
    memcpy(buf + MARKER_SIZE + 2, &_Type, 1);
    return buf;
}
