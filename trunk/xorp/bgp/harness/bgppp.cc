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

#ident "$XORP: xorp/bgp/harness/bgppp.cc,v 1.8 2002/12/09 18:28:51 hodson Exp $"

/*
** BGP Pretty Print
*/

#include "bgp/bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "bgp/packet.hh"
#include "bgppp.hh"

string
bgppp(const uint8_t *buf, const size_t len)
{
    string result;

    const fixed_header *header = 
	reinterpret_cast<const struct fixed_header *>(buf);
    struct fixed_header fh = *header;
    fh._length = ntohs(header->_length);

    try {
	switch(fh._type) {
	case MESSAGETYPEOPEN: {
	    OpenPacket pac(buf, len);
	    pac.decode();
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPEKEEPALIVE: {
	    KeepAlivePacket pac(buf, len);
	    pac.decode();
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPEUPDATE: {
	    UpdatePacket pac(buf, len);
	    pac.decode();
	    result = pac.str().c_str();
	}
	    break;
	case MESSAGETYPENOTIFICATION: {
	    NotificationPacket pac(buf, len);
	    pac.decode();
	    result = pac.str().c_str();
	}
	    break;
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    result = c_format("Unknown packet type %d\n", header->_type);
	    XLOG_WARNING(result.c_str());
	}
    } catch(CorruptMessage c) {
	/*
	** This peer had sent us a bad message.
	*/
	
	result = c_format("BAD Message: %s", c.why().c_str());
	XLOG_WARNING(result.c_str());
    }

    return result;
}
