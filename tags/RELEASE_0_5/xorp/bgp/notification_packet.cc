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

#ident "$XORP: xorp/bgp/notification_packet.cc,v 1.15 2003/10/03 00:26:58 atanu Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "packet.hh"
#define DEBUG_BGPNotificationPacket

/* **************** BGPNotificationPacket *********************** */

NotificationPacket::NotificationPacket(uint8_t		ec,
				       uint8_t		esc,
				       const uint8_t*	ed,
				       size_t		elen)
{
    debug_msg("Error code %d sub code %d data %#x len %u\n", ec, esc,
	      (int)ed, (uint32_t)elen);

    if (elen == 0)
	ed = 0;
    if (ed == 0)
	elen = 0;
    // elen is the length of the error_data to carry
    _Length = MINNOTIFICATIONPACKET + elen;

    _Type = MESSAGETYPENOTIFICATION;
    _error_code = ec;
    _error_subcode = esc;
    if (ed) {
	uint8_t *error_data = new uint8_t[elen];
	memcpy(error_data, ed, elen);
	_error_data = error_data;
    } else
	_error_data = 0;

    debug_msg(str().c_str());
}

NotificationPacket::NotificationPacket(const uint8_t *d, uint16_t l)
    throw(InvalidPacket)
{
    debug_msg("Data %#x len %d\n", (int)d, l);
    if (l < MINNOTIFICATIONPACKET)
	xorp_throw(InvalidPacket, "Truncated Notification Message");

    _Length = l;
    _Type = MESSAGETYPENOTIFICATION;
    d += BGP_COMMON_HEADER_LEN;	// skip header
    _error_code = d[0];
    _error_subcode = d[1];
    int error_data_len = _Length - MINNOTIFICATIONPACKET;
    if (error_data_len > 0) {
	uint8_t *ed = new uint8_t[error_data_len];
	memcpy(ed, d + 2, error_data_len);
	_error_data = ed;
	debug_msg("%s", str().c_str());
    } else
	_error_data = NULL;
    return;
}

const uint8_t *
NotificationPacket::encode(size_t& len, uint8_t *buf) const
{
    debug_msg("Encode in NotificationPacket called (%u)\n", (uint32_t)_Length);
    if (buf != 0)		// have a buffer, check length
	assert(len >= _Length);	// XXX should become an exception

    len = _Length;

    // allocate buffer, set length, fill up header
    buf = basic_encode(len, buf);
    buf[BGP_COMMON_HEADER_LEN] = _error_code;
    buf[BGP_COMMON_HEADER_LEN+1] = _error_subcode;
    if (_error_data != 0)
	memcpy(buf + MINNOTIFICATIONPACKET, _error_data,
		len - MINNOTIFICATIONPACKET);
    return buf;
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
					    const uint8_t* error_data)
{
    string s;

    switch (error) {
    case MSGHEADERERR:
	s += "Message Header Error: ";
	switch (subcode) {
	case CONNNOTSYNC:
	    s += "Connection Not Synchronized";
	    break;
	case BADMESSLEN:
	    if (error_data != NULL)
		s += c_format("Bad Message Length - field: %d",
			      ntohs((uint16_t &)*error_data));
	    else
		s += "Bad Message Length: ";
	    break;
	case BADMESSTYPE:
	    if (error_data != NULL)
		s += c_format("Bad Message Type - field : %d",
			      (uint8_t &) *error_data);
	    else
		s += "Bad Message Type";
	    break;
	}
	break;
    case OPENMSGERROR:
	s += "OPEN Message Error: ";
	switch (subcode) {
	case UNSUPVERNUM:
	    if (error_data != NULL)
		s += c_format("Unsupported Version Number. Min supported Version is %d",
			      ntohs((uint16_t &)*error_data));
	    else
		s += "Unsupported Version Number.";
	    break;
	case BADASPEER:
	    s += "Bad Peer AS.";
	    break;
	case BADBGPIDENT:
	    s += "Bad BGP Identifier.";
	    break;
	case UNSUPOPTPAR:
	    s += "Unsupported Optional Parameter.";
	    break;
	case AUTHFAIL:
	    s += "Authentication Failure.";
	    break;
	case UNACCEPTHOLDTIME:
	    s += "Unacceptable Hold Time.";
	    break;
	case UNSUPCAPABILITY:
	    s += "Unsuported Capability";
	    break;
	}
	break;
    case UPDATEMSGERR:
	s += "UPDATE Message Error: ";
	switch (subcode) {
	case MALATTRLIST:
	    s += "Malformed Attribute List.";
	    break;
	case UNRECOGWATTR:
	    if (error_data != NULL)
		s += c_format("Unrecognized Well-known Attribute, Type %d",
			      error_data[0]);
	    else
		s += "Unrecognized Well-known Attribute, NO ERROR DATA SUPPLIED";
	    break;
	case MISSWATTR:
	    if (error_data != NULL)
		s += c_format("Missing Well-known Attribute, Type %d",
			      error_data[0]);
	    else
		s += "Missing Well-known Attribute, NO ERROR DATA SUPPLIED";
	    break;
	case ATTRFLAGS:
	    s += "Attribute Flags Error.";
	    break;
	case ATTRLEN:
	    s += "Attribute Length Error.";
	    break;
	case INVALORGATTR:
	    s += "Invalid ORIGIN Attribute.";
	    break;
	case INVALNHATTR:
	    s += "Invalid NEXT_HOP Attribute.";
	    break;
	case OPTATTR:
	    s += "Optional Attribute Error.";
	    break;
	case INVALNETFIELD:
	    s += "Invalid Network Field.";
	    break;
	case MALASPATH:
	    s += "Malformed AS_PATH.";
	    break;
	}
	break;
    case HOLDTIMEEXP:
	s += "Hold Timer Expired.";
	break;
    case FSMERROR:
	s += "Finite State Machine Error.";
	break;
    case CEASE:
	s += "Cease.";
	break;
    }

    return s;
}

string
NotificationPacket::str() const
{
    return "Notification Packet: " +
	pretty_print_error_code(_error_code, _error_subcode, _error_data) +
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
		   _Length - MINNOTIFICATIONPACKET))
	return false;

    return true;
}
