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

#ident "$XORP: xorp/bgp/open_packet.cc,v 1.15 2003/09/27 08:34:05 atanu Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "packet.hh"

extern void dump_bytes(const uint8_t *d, size_t l);

/* **************** OpenPacket *********************** */

OpenPacket::OpenPacket(const AsNum& as, const IPv4& id,
		const uint16_t holdtime) :
    _id(id), _as(as), _HoldTime(holdtime), _Version(BGPVERSION)
{
    debug_msg("OpenPacket(BGPLocalData *d) constructor called\n");

    _Type = MESSAGETYPEOPEN;
    _OptParmLen = 0;
}

const uint8_t *
OpenPacket::encode(size_t& len, uint8_t *d) const
{
    if (d != 0) {	// have a buffer, check length
	// XXX this should become an exception
        assert (len >= MINOPENPACKET + _OptParmLen);
    }
    len = MINOPENPACKET + _OptParmLen;
    d = basic_encode(len, d);

    d[BGP_COMMON_HEADER_LEN] = _Version;
    _as.copy_out(d + BGP_COMMON_HEADER_LEN + 1);

    d[BGP_COMMON_HEADER_LEN + 3] = (_HoldTime >> 8) & 0xff;
    d[BGP_COMMON_HEADER_LEN + 4] = _HoldTime & 0xff;
    _id.copy_out(d + BGP_COMMON_HEADER_LEN + 5);
    d[BGP_COMMON_HEADER_LEN + 9] = _OptParmLen;

    if (!_parameter_list.empty())  {
	size_t i = MINOPENPACKET;
	ParameterList::const_iterator pi = _parameter_list.begin();
	while(pi != _parameter_list.end()) {
	    memcpy(d + i, (*pi)->data(), (*pi)->length());
	    i += (*pi)->length();
	    pi++;
	}
    }

    return d;
}

OpenPacket::OpenPacket(const uint8_t *d, uint16_t l)
	throw(CorruptMessage)
    : _as(AsNum::AS_INVALID)
{
    debug_msg("OpenPacket(const uint8_t *, uint16_t) constructor called\n");
    _Type = MESSAGETYPEOPEN;

    _OptParmLen = 0;

    size_t i, myOptParmLen;

    if (l < MINOPENPACKET) {
	debug_msg("Open message too short\n");
	xorp_throw(CorruptMessage, "Open message too short",
		   MSGHEADERERR, BADMESSLEN);
    }

    d += BGP_COMMON_HEADER_LEN;	// skip common header

    _Version = d[0];

    _as = AsNum(d+1);

    _HoldTime = (d[3]<<8) + d[4];

    _id = IPv4(d+5);
    myOptParmLen = i = d[9];
    d += 10;		// skip over fixed open header

    while (i > 0) {
	size_t len;
	debug_msg("Length of unread parameters : %u\n", (uint32_t)i);

	BGPParameter *p = BGPParameter::create(d, i, len);
	if (p != NULL)
	    add_parameter(p);
	// This assert is safe because if len is bad an exception
	// should already have been thrown.
	XLOG_ASSERT(len > 0);
	i -= len;
	d += len;
    }
    // check to see if the length of the optional parameters defined in the
    // packet is the same as the total length of the decoded parameters.
    if (myOptParmLen != _OptParmLen) {
	xorp_throw(CorruptMessage,  "bad parameters length",
		   OPENMSGERROR, 0);
    }

    return;
}

string
OpenPacket::str() const
{
    string s = "Open Packet\n";
    s += c_format(" - Version : %d\n"
		  " - Autonomous System Number : %s\n"
		  " - Holdtime : %d\n"
		  " - BGP Identifier : %s\n",
		  _Version,
		  _as.str().c_str(),
		  _HoldTime,
		  _id.str().c_str());

    ParameterList::const_iterator pi = parameter_list().begin();
    while (pi != parameter_list().end()) {
	s = s + " - " + (*pi)->str() + "\n";
	++pi;
    }
    return s;
}

bool
OpenPacket::operator==(const OpenPacket& him) const
{
    // version

    // as
    if (_as != him.as())
	return false;
    // hold time
    if (_HoldTime != him.HoldTime())
	return false;
    // bgp id
    if (_id != him.id())
	return false;
    // opt parm len
    if (_OptParmLen != him.OptParmLen())
	return false;
    // parameters ... problem here is that the "sending" packet, stores these
    // in BGPLocalData and the receiving packet stores these in BGPPeerData
    return true;
}

void
OpenPacket::add_parameter(const BGPParameter *p)
{
    _parameter_list.push_back(p);
    _OptParmLen = _OptParmLen + p->length();
}
