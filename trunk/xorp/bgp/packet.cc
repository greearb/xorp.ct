// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/packet.cc,v 1.10 2003/10/03 00:26:59 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "packet.hh"

/* **************** BGPPacket *********************** */

const uint8_t BGPPacket::Marker[MARKER_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

uint8_t *
BGPPacket::basic_encode(size_t len, uint8_t *buf) const
{
    if (buf == 0)
	buf = new uint8_t[len];
    memcpy(buf, Marker, MARKER_SIZE);
    buf[MARKER_SIZE] = (len >> 8) & 0xff;
    buf[MARKER_SIZE+1] = len & 0xff;
    buf[MARKER_SIZE+2] = _Type;
    return buf;
}
