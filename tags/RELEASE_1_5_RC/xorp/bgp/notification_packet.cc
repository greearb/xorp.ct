// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/notification_packet.cc,v 1.33 2008/01/04 03:15:20 pavlin Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libproto/packet.hh"

#include "packet.hh"


#define DEBUG_BGPNotificationPacket

/* **************** BGPNotificationPacket *********************** */

NotificationPacket::NotificationPacket(uint8_t		ec,
				       uint8_t		esc,
				       const uint8_t*	ed,
				       size_t		elen)
{
    debug_msg("Error code %d sub code %d data %p len %u\n", ec, esc,
	      ed, XORP_UINT_CAST(elen));

    if (elen == 0)
	ed = 0;
    if (ed == 0)
	elen = 0;
    // elen is the length of the error_data to carry
    _Length = BGPPacket::MINNOTIFICATIONPACKET + elen;

    _Type = MESSAGETYPENOTIFICATION;
    _error_code = ec;
    _error_subcode = esc;
    if (ed) {
	uint8_t *error_data = new uint8_t[elen];
	memcpy(error_data, ed, elen);
	_error_data = error_data;
    } else
	_error_data = 0;

    debug_msg("%s", str().c_str());
}

NotificationPacket::NotificationPacket(const uint8_t *d, uint16_t l)
    throw(CorruptMessage)
{
    debug_msg("Data %p len %d\n", d, l);
    if (l < BGPPacket::MINNOTIFICATIONPACKET)
	xorp_throw(CorruptMessage,
		   c_format("Notification message too short %d", l),
		   MSGHEADERERR, BADMESSLEN, d + BGPPacket::MARKER_SIZE, 2);

    _Length = l;
    _Type = MESSAGETYPENOTIFICATION;
    d += BGPPacket::COMMON_HEADER_LEN;		// skip header
    _error_code = d[0];
    _error_subcode = d[1];
    int error_data_len = _Length - BGPPacket::MINNOTIFICATIONPACKET;
    if (error_data_len > 0) {
	uint8_t *ed = new uint8_t[error_data_len];
	memcpy(ed, d + 2, error_data_len);
	_error_data = ed;
	debug_msg("%s", str().c_str());
    } else
	_error_data = NULL;
    return;
}

bool
NotificationPacket::encode(uint8_t *buf, size_t& len, const BGPPeerData *peerdata) const
{
    UNUSED(peerdata);
    XLOG_ASSERT(buf != 0);
    debug_msg("Encode in NotificationPacket called (%u)\n",
	      XORP_UINT_CAST(_Length));

    if (len < _Length)
	return false;

    len = _Length;

    // allocate buffer, set length, fill up header
    buf = basic_encode(len, buf);
    buf[BGPPacket::COMMON_HEADER_LEN] = _error_code;
    buf[BGPPacket::COMMON_HEADER_LEN + 1] = _error_subcode;
    if (_error_data != 0)
	memcpy(buf + BGPPacket::MINNOTIFICATIONPACKET, _error_data,
		len - BGPPacket::MINNOTIFICATIONPACKET);
    return true;
}

bool
NotificationPacket::validate_error_code(const int error,
					const int subcode)
{
    bool good_error_code = true;
    bool good_error_subcode = false;
    switch (error) {
    case MSGHEADERERR:
	switch (subcode) {
	case CONNNOTSYNC:
	case BADMESSLEN:
	case BADMESSTYPE:
	    good_error_subcode = true;
	    break;
	}
	break;
    case OPENMSGERROR:
	switch (subcode) {
	case UNSUPVERNUM:
	case BADASPEER:
	case BADBGPIDENT:
	case UNSUPOPTPAR:
	case AUTHFAIL:
	case UNACCEPTHOLDTIME:
	    good_error_subcode = true;
	    break;
	}
	break;
    case UPDATEMSGERR:
	switch (subcode) {
	case MALATTRLIST:
	case UNRECOGWATTR:
	case MISSWATTR:
	case ATTRFLAGS:
	case ATTRLEN:
	case INVALORGATTR:
	case INVALNHATTR:
	case OPTATTR:
	case INVALNETFIELD:
	case MALASPATH:
	    good_error_subcode = true;
	    break;
	}
	break;
    case HOLDTIMEEXP:
	break;
    case FSMERROR:
	break;
    case CEASE:
	break;
    default:
	good_error_code = false;
    }

    if (!good_error_subcode && 0 == subcode)
	good_error_subcode = true;

    return good_error_code && good_error_subcode;
}

string
NotificationPacket::pretty_print_error_code(const int error, const int subcode,
					    const uint8_t* error_data,
					    const size_t len)
{
    string s;

    switch (error) {
    case MSGHEADERERR:
	s += c_format("Message Header Error(%d): ", MSGHEADERERR);
	switch (subcode) {
	case CONNNOTSYNC:
	    s += c_format("Connection Not Synchronized(%d)", CONNNOTSYNC);
	    break;
	case BADMESSLEN:
	    if (error_data != NULL) {
		s += c_format("Bad Message Length(%d) - field: %d",
			      BADMESSLEN, extract_16(error_data));
	    } else {
		s += c_format("Bad Message Length(%d) - "
			      "NO ERROR DATA SUPPLIED", BADMESSLEN);
	    }
	    break;
	case BADMESSTYPE:
	    if (error_data != NULL)
		s += c_format("Bad Message Type(%d) - field : %d",
			      BADMESSTYPE, error_data[0]);
	    else
		s += c_format("Bad Message Type(%d) - "
			      "NO ERROR DATA SUPPLIED", BADMESSTYPE);
	    break;
	}
	break;
    case OPENMSGERROR:
	s += c_format("OPEN Message Error(%d): ", OPENMSGERROR);
	switch (subcode) {
	case UNSUPVERNUM:
	    if (error_data != NULL) {
		s += c_format("Unsupported Version Number(%d) - "
			      "Min supported Version is %d",
			      UNSUPVERNUM, extract_16(error_data));
	    } else {
		s += c_format("Unsupported Version Number(%d) - "
			      "NO ERROR DATA SUPPLIED", UNSUPVERNUM);
	    }
	    break;
	case BADASPEER:
	    s += c_format("Bad Peer AS(%d)", BADASPEER);
	    break;
	case BADBGPIDENT:
	    s += c_format("Bad BGP Identifier(%d)", BADBGPIDENT);
	    break;
	case UNSUPOPTPAR:
	    s += c_format("Unsupported Optional Parameter(%d)", UNSUPOPTPAR);
	    break;
	case AUTHFAIL:
	    s += c_format("Authentication Failure(%d)", AUTHFAIL);
	    break;
	case UNACCEPTHOLDTIME:
	    s += c_format("Unacceptable Hold Time(%d)", UNACCEPTHOLDTIME);
	    break;
	case UNSUPCAPABILITY:
	    s += c_format("Unsuported Capability(%d)", UNSUPCAPABILITY);
	    break;
	}
	break;
    case UPDATEMSGERR:
	s += c_format("UPDATE Message Error(%d): ", UPDATEMSGERR);
	switch (subcode) {
	case MALATTRLIST:
	    s += c_format("Malformed Attribute List(%d)", MALATTRLIST);
	    break;
	case UNRECOGWATTR:
	    if (error_data != NULL)
		s += c_format("Unrecognized Well-known Attribute(%d) - ",
			      UNRECOGWATTR);
	    else
		s += c_format("Unrecognized Well-known Attribute(%d) - "
			      "NO ERROR DATA SUPPLIED", UNRECOGWATTR);
	    break;
	case MISSWATTR:
	    if (error_data != NULL)
		s += c_format("Missing Well-known Attribute(%d) - Type %d",
			      MISSWATTR, error_data[0]);
	    else
		s += c_format("Missing Well-known Attribute(%d) - "
			      "NO ERROR DATA SUPPLIED", MISSWATTR);
	    break;
	case ATTRFLAGS:
	    s += c_format("Attribute Flags Error(%d)", ATTRFLAGS);
	    break;
	case ATTRLEN:
	    s += c_format("Attribute Length Error(%d)", ATTRLEN);
	    break;
	case INVALORGATTR:
	    s += c_format("Invalid ORIGIN Attribute(%d)", INVALORGATTR);
	    break;
	case INVALNHATTR:
	    s += c_format("Invalid NEXT_HOP Attribute(%d)", INVALNHATTR);
	    break;
	case OPTATTR:
	    s += c_format("Optional Attribute Error(%d)", OPTATTR);
	    break;
	case INVALNETFIELD:
	    s += c_format("Invalid Network Field(%d)", INVALNETFIELD);
	    break;
	case MALASPATH:
	    s += c_format("Malformed AS_PATH(%d)", MALASPATH);
	    break;
	}
	break;
    case HOLDTIMEEXP:
	s += c_format("Hold Timer Expired(%d)", HOLDTIMEEXP);
	break;
    case FSMERROR:
	s += c_format("Finite State Machine Error(%d)", FSMERROR);
	break;
    case CEASE:
	s += c_format("Cease(%d)", CEASE);
	break;
    }

    if (error_data != NULL) {
	s += c_format(" [");
	for (size_t i = 0; i < len; i++)
	    s += c_format("%s%#x", i != 0 ? " " : "", error_data[i]);
	s += c_format("]");
    }

    return s;
}

string
NotificationPacket::str() const
{
    return "Notification Packet: " +
	pretty_print_error_code(_error_code, _error_subcode, _error_data,
				_Length - BGPPacket::MINNOTIFICATIONPACKET) +
	"\n";
}

bool
NotificationPacket::operator==(const NotificationPacket& him ) const
{

    if (_error_code != him.error_code())
	return false;
    if (_error_subcode != him.error_subcode())
	return false;

    // common packet headers apply to both length, so only need to compare raw
    // packet length

    if (_Length != him._Length)
	return false;

    if (0 != memcmp(_error_data, him.error_data(),
		   _Length - BGPPacket::MINNOTIFICATIONPACKET))
	return false;

    return true;
}
