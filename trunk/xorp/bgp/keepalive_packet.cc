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

#ident "$XORP: xorp/bgp/keepalive_packet.cc,v 1.20 2002/12/09 18:28:42 hodson Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "packet.hh"

/* **************** KeepAlivePacket *********************** */

KeepAlivePacket::KeepAlivePacket(const uint8_t *d, uint16_t l)
{
    debug_msg("KeepAlivePacket constructor called\n");
    _Length = l;
    _Type = MESSAGETYPEKEEPALIVE;
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _Data = data;
}

KeepAlivePacket::KeepAlivePacket()
{
    debug_msg("KeepAlivePacket constructor called\n");
    _Length = 19;
    _Type = MESSAGETYPEKEEPALIVE;
    _Data = NULL;
}

KeepAlivePacket::~KeepAlivePacket()
{
    if (_Data != NULL)
	delete[] _Data;
}

const uint8_t *
KeepAlivePacket::encode(int &len) const
{
    uint16_t k = 0;
    struct iovec io[3];

    debug_msg("KeepAlivePacket called with Type %d Length %d\n", _Type,
	      _Length);

    io[0].iov_base = const_cast<char*>((const char *)_Marker);
    io[0].iov_len = 16;
    k = htons(_Length);

    io[1].iov_base = const_cast<char*>((const char *)&k);
    io[1].iov_len = 2;

    io[2].iov_base = const_cast<char*>((const char *)&_Type);
    io[2].iov_len = 1;

    return flatten(&io[0], 3, len);
}

void
KeepAlivePacket::decode()
{
	// Do nothing here.
}

string
KeepAlivePacket::str() const
{
    string s;
    s = "Keepalive Packet\n";
    return s;
}

bool
KeepAlivePacket::operator==(const KeepAlivePacket& ) const
{
    return true;
}
