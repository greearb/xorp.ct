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

#ident "$XORP: xorp/bgp/open_packet.cc,v 1.7 2003/01/28 22:06:57 rizzo Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "packet.hh"

extern void dump_bytes(const uint8_t *d, size_t l);

/* **************** OpenPacket *********************** */


OpenPacket::OpenPacket(const AsNum& as,
			     const IPv4& bgpid,
			     const uint16_t holdtime) :
    _BGPIdentifier(bgpid),
    _AutonomousSystemNumber(as),
    _HoldTime(holdtime),
    _Version(BGPVERSION)
{
    debug_msg("OpenPacket(BGPLocalData *d) constructor called\n");

    _Type = MESSAGETYPEOPEN;
    _OptParmLen = 0;
    _num_parameters = 0;
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
    _AutonomousSystemNumber.copy_out(d + BGP_COMMON_HEADER_LEN + 1);

    d[BGP_COMMON_HEADER_LEN + 3] = (_HoldTime >> 8) & 0xff;
    d[BGP_COMMON_HEADER_LEN + 4] = _HoldTime & 0xff;
    _BGPIdentifier.copy_out(d + BGP_COMMON_HEADER_LEN + 5);
    d[BGP_COMMON_HEADER_LEN + 9] = _OptParmLen;

    if (_num_parameters > 0)  {
	size_t i = MINOPENPACKET;
	list <BGPParameter*>::const_iterator pi = _parameter_list.begin();
	while(pi != _parameter_list.end()) {
	    memcpy(d + i, (*pi)->data(), (*pi)->length());
	    i += (*pi)->length();
	    pi++;
	}
    }

    return d;
}

OpenPacket::OpenPacket(const uint8_t *data, uint16_t l)
	throw(CorruptMessage)
    // XXX assigning dummy value is bad practice
    : _AutonomousSystemNumber(AsNum::AS_INVALID)
{
    debug_msg("OpenPacket(const uint8_t *, uint16_t) constructor called\n");
    _Type = MESSAGETYPEOPEN;

    _OptParmLen = 0;
    _num_parameters = 0;

    ParamType param_type;
    int param_len;
    int prev_len;
    int count = 0;
    int remaining_param_len;
    BGPParameter *p;
    uint8_t myOptParmLen;

    if (l < MINOPENPACKET) {
	debug_msg("Open message too short\n");
	xorp_throw(CorruptMessage, "Open message too short",
		   MSGHEADERERR, BADMESSLEN);
    }

    /* shift data by common header length */
    data = data + BGP_COMMON_HEADER_LEN;

    _Version = (uint8_t &)(*data);

    data++;
    _AutonomousSystemNumber = AsNum(data);
    data += 2;

    _HoldTime = ntohs((uint16_t &)(*data));
    data += 2;

    _BGPIdentifier = IPv4((const uint8_t *)(data));
    data += 4;

    remaining_param_len = (uint8_t)(* data);
    myOptParmLen = remaining_param_len;
    data++;

    while(remaining_param_len > 0) {
	count ++;
	p = NULL;
	if (count == 5)
	    assert(1 == 2);

	debug_msg("Length of unread parameters : %d\n", remaining_param_len);

	param_type = (ParamType)(* (data));
	param_len = (uint8_t)(* (data+1));

	prev_len = remaining_param_len;

	remaining_param_len -= (param_len + 2);

	if (prev_len < remaining_param_len || param_len == 0) {
	    debug_msg("Badly constructed Parameters\n");
	    debug_msg("Throw exception\n");
	    debug_msg("Send bad packet\n");
	    // XXX there doesn't seem to be a good error code for this.
	    xorp_throw(CorruptMessage, "Badly constructed Parameters\n",
		       OPENMSGERROR, 0);
	}

	debug_msg("param_type %d param_len %d\n", param_type, param_len);

	switch (param_type) {
	case PARAMTYPEAUTH:
	    p = new BGPAuthParameter(param_len+2, data);
	    break;
	case PARAMTYPECAP: {
	    CapType cap_type = (CapType)*(data+2);
	    switch (cap_type) { // This is the capability type
	    case CAPABILITYMULTIPROTOCOL:
		p = new BGPMultiProtocolCapability(param_len+2 , data);
		break;
	    case CAPABILITYREFRESH:
	    case CAPABILITYREFRESHOLD:
		p = new BGPRefreshCapability(param_len+2 , data);
		break;
	    default:
		p = new BGPUnknownCapability(param_len+2 , data);
		// abort();
	    }
	    break;
	}
	default :
	    debug_msg("Some other type\n");
	    debug_msg("Throw exception\n");
	    debug_msg("Send invalid packet");
	    xorp_throw(CorruptMessage,
		       c_format("Unrecognised optional parameter %d\n",
				param_type),
		       OPENMSGERROR, UNSUPOPTPAR);
	}
	if (p != NULL)
	    add_parameter(p);
	data = data + param_len +2;
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
		  _AutonomousSystemNumber.str().c_str(),
		  _HoldTime,
		  _BGPIdentifier.str().c_str());

    list <BGPParameter*>::const_iterator pi = parameter_list().begin();
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
    if (_AutonomousSystemNumber != him.AutonomousSystemNumber())
	return false;
    // hold time
    if (_HoldTime != him.HoldTime())
	return false;
    // bgp id
    if (_BGPIdentifier != him.BGPIdentifier())
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
    BGPParameter *parameter;

    debug_msg("Open packet add_parameter called\n");

    switch (p->type()) {
	case PARAMTYPEAUTH:
	    parameter = new BGPAuthParameter((const BGPAuthParameter&)*p);
	    break;
	case PARAMTYPECAP: {
	    const BGPCapParameter* cparam =
		static_cast<const BGPCapParameter*>(p);
	    switch (cparam->cap_code())	{
	    case CAPABILITYMULTIPROTOCOL:
		parameter =
	 new BGPMultiProtocolCapability((const BGPMultiProtocolCapability&)*p);
		break;
	    case CAPABILITYREFRESHOLD:
		// need to look at this further.....
		// what do we do about the old capability
		abort();
		break;
	    case CAPABILITYREFRESH:
		parameter =
		    new BGPRefreshCapability((const BGPRefreshCapability&)*p);
		break;
	    case CAPABILITYMULTIROUTE:
		parameter =
	       new BGPMultiRouteCapability((const BGPMultiRouteCapability&)*p);
		break;
	    case CAPABILITYUNKNOWN:
		parameter =
		    new BGPUnknownCapability((const BGPUnknownCapability&)*p);
		break;
	    default:
		debug_msg("Don't understand parameter type %d and "
			  "capability code %d.\n",
			  p->type(), cparam->cap_code());
		abort();
	    }
	}
	    break;
    default:
	// unknown parameter type
	abort();
    }

    _parameter_list.push_back(parameter);
    // p.dump_contents();
    _num_parameters++;
    // TODO add bounds checking

    _OptParmLen = _OptParmLen + p->length();
}
