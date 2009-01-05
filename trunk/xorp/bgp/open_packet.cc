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

#ident "$XORP: xorp/bgp/open_packet.cc,v 1.33 2008/10/02 21:56:16 bms Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
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

bool
OpenPacket::encode(uint8_t *d, size_t& len, const BGPPeerData *peerdata) const
{
    UNUSED(peerdata);
    XLOG_ASSERT(d != 0);

    if (len < BGPPacket::MINOPENPACKET + _OptParmLen)
	return false;

    len = BGPPacket::MINOPENPACKET + _OptParmLen;
    d = basic_encode(len, d);

    d[BGPPacket::COMMON_HEADER_LEN] = _Version;
    _as.copy_out(d + BGPPacket::COMMON_HEADER_LEN + 1);

    d[BGPPacket::COMMON_HEADER_LEN + 3] = (_HoldTime >> 8) & 0xff;
    d[BGPPacket::COMMON_HEADER_LEN + 4] = _HoldTime & 0xff;
    _id.copy_out(d + BGPPacket::COMMON_HEADER_LEN + 5);
    d[BGPPacket::COMMON_HEADER_LEN + 9] = _OptParmLen;
    debug_msg("encoding Open, optparmlen = %u\n", (uint32_t)_OptParmLen);

    if (!_parameter_list.empty())  {
	size_t i = BGPPacket::MINOPENPACKET;
	ParameterList::const_iterator pi = _parameter_list.begin();
	while(pi != _parameter_list.end()) {
	    XLOG_ASSERT(i + (*pi)->length() <= len);
	    memcpy(d + i, (*pi)->data(), (*pi)->length());
	    i += (*pi)->length();
	    pi++;
	}
    }

    return true;
}

OpenPacket::OpenPacket(const uint8_t *d, uint16_t l)
	throw(CorruptMessage)
    : _as(AsNum::AS_INVALID)
{
    debug_msg("OpenPacket(const uint8_t *, uint16_t %u) constructor called\n",
	      (uint32_t)l);
    _Type = MESSAGETYPEOPEN;

    _OptParmLen = 0;

    size_t i, myOptParmLen, remaining;

    if (l < BGPPacket::MINOPENPACKET) {
	debug_msg("Open message too short\n");
	xorp_throw(CorruptMessage, "Open message too short",
		   MSGHEADERERR, BADMESSLEN, d + BGPPacket::MARKER_SIZE, 2);
    }
    remaining = l;

    d += BGPPacket::COMMON_HEADER_LEN;		// skip common header
    remaining -= BGPPacket::COMMON_HEADER_LEN;

    _Version = d[0];

    _as = AsNum(d+1);

    _HoldTime = (d[3]<<8) + d[4];

    _id = IPv4(d+5);
    myOptParmLen = i = d[9];
    d += 10;		// skip over fixed open header
    remaining -= 10;

    // we shouldn't trust myOptParmLen yet - make sure it's not
    // greater than the amount of data we actually received.
    if (remaining < myOptParmLen) {
	debug_msg("Open message too short\n");
	xorp_throw(CorruptMessage, "Open message too short",
		   OPENMSGERROR, UNSPECIFIED);
    }

    while (i > 0) {
	size_t len;
	debug_msg("Length of unread parameters : %u\n", XORP_UINT_CAST(i));
	if (remaining < 2) {
	    debug_msg("Open message too short\n");
	    xorp_throw(CorruptMessage, "Parameter is too short",
		       OPENMSGERROR, UNSPECIFIED);
	}

	BGPParameter *p = BGPParameter::create(d, i, len);
	if (p != NULL)
	    add_parameter(p);
	// This assert is safe because if len is bad an exception
	// should already have been thrown.
	XLOG_ASSERT(len > 0);
	XLOG_ASSERT(i >= len);
	i -= len;
	d += len;
    }
    // check to see if the length of the optional parameters defined in the
    // packet is the same as the total length of the decoded parameters.
    if (myOptParmLen != _OptParmLen) {
	xorp_throw(CorruptMessage,  "bad parameters length",
		   OPENMSGERROR, UNSPECIFIED);
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
    // Compare the parameters
    ParameterList::const_iterator me_pi = parameter_list().begin();
    ParameterList::const_iterator him_pi = him.parameter_list().begin();
    for (; me_pi !=  parameter_list().end(); me_pi++) {
	bool match = false;
	for (; him_pi !=  him.parameter_list().end(); him_pi++) {
	    if ((*me_pi)->compare(*(*him_pi))) {
		match = true;
		break;
	    }
	}
	if (match == false)
	    return false;
    }
    return true;
}

void
OpenPacket::add_parameter(const ParameterNode& p)
{
    _parameter_list.push_back(p);
    _OptParmLen = _OptParmLen + p->length();
}
