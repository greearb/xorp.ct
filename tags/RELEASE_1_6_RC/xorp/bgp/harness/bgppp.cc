// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/harness/bgppp.cc,v 1.18 2008/10/02 21:56:26 bms Exp $"

/*
** BGP Pretty Print
*/

#include "bgp/bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "bgp/packet.hh"
#include "bgp/peer.hh"
#include "bgppp.hh"

string
bgppp(const uint8_t *buf, const size_t len, const BGPPeerData *peerdata)
{
    string result;
    uint8_t type = extract_8(buf + BGPPacket::TYPE_OFFSET);

    try {
	switch(type) {
	case MESSAGETYPEOPEN: {
	    OpenPacket pac(buf, len);
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPEKEEPALIVE: {
	    KeepAlivePacket pac(buf, len);
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPEUPDATE: {
	    UpdatePacket pac(buf, len, peerdata, 0, false);
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPENOTIFICATION: {
	    NotificationPacket pac(buf, len);
	    result = pac.str().c_str();
	}
	    break;
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    result = c_format("Unknown packet type %d\n", type);
	    XLOG_WARNING("%s", result.c_str());
	}
    } catch(CorruptMessage& c) {
	/*
	** This peer had sent us a bad message.
	*/
	
	result = c_format("BAD Message: %s", c.why().c_str());
	XLOG_WARNING("%s", result.c_str());
    }

    return result;
}
