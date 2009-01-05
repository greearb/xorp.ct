// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/bgp/packet.cc,v 1.19 2008/10/02 21:56:16 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "packet.hh"

/* **************** BGPPacket *********************** */

const uint8_t BGPPacket::Marker[BGPPacket::MARKER_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

uint8_t *
BGPPacket::basic_encode(size_t len, uint8_t *buf) const
{
    if (buf == 0)
	buf = new uint8_t[len];
    XLOG_ASSERT(len >= BGPPacket::MARKER_SIZE + 3);
    memcpy(buf, Marker, BGPPacket::MARKER_SIZE);
    buf[BGPPacket::MARKER_SIZE] = (len >> 8) & 0xff;
    buf[BGPPacket::MARKER_SIZE + 1] = len & 0xff;
    buf[BGPPacket::MARKER_SIZE + 2] = _Type;
    return buf;
}
