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

#ident "$XORP: xorp/bgp/notification_packet.cc,v 1.37 2002/12/09 18:28:43 hodson Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"

#include "packet.hh"
#define DEBUG_BGPNotificationPacket

/* **************** BGPNotificationPacket *********************** */

NotificationPacket::NotificationPacket(uint8_t ec)
{
    debug_msg("Error code %d\n", ec);

    _Length = BGP_COMMON_HEADER_LEN + BGP_NOTIFY_HEADER_LEN;
    _Type = MESSAGETYPENOTIFICATION;
    _error_code = ec;
    _error_subcode = 0;
    _Data = NULL;
    _error_data = NULL;

    debug_msg(str().c_str());
}

NotificationPacket::NotificationPacket(uint8_t ec, uint8_t esc)
{

    debug_msg("Error code %d sub code %d\n", ec, esc);

    _Length = BGP_COMMON_HEADER_LEN + BGP_NOTIFY_HEADER_LEN;
    _Type = MESSAGETYPENOTIFICATION;
    _error_code = ec;
    _error_subcode = esc;
    _Data = NULL;
    _error_data = NULL;

    debug_msg(str().c_str());
}

NotificationPacket::NotificationPacket(const uint8_t *ed, uint8_t ec,
				       uint8_t esc, uint16_t elen)
{
    debug_msg("Error code %d sub code %d data %#x len %d\n", ec, esc,
	      (int)ed, elen);

    // elen is the length of the error_data to carry
    _Length = BGP_COMMON_HEADER_LEN + BGP_NOTIFY_HEADER_LEN + elen;

    _Type = MESSAGETYPENOTIFICATION;
    _error_code = ec;
    _error_subcode = esc;
    uint8_t *error_data = new uint8_t[elen];
    memcpy(error_data, ed, elen);
    _error_data = error_data;
    _Data = NULL;

    debug_msg(str().c_str());
}

NotificationPacket::NotificationPacket(const uint8_t *d, uint16_t l)
    throw(InvalidPacket)
{
    debug_msg("Data %#x len %d\n", (int)d, l);

    _Length = l;
    _Type = MESSAGETYPENOTIFICATION;
    int data_len = _Length;
    if (data_len < BGP_NOTIFY_HEADER_LEN) {
	xorp_throw(InvalidPacket, "Truncated Notification Message");
    }
    uint8_t *data = new uint8_t[data_len];
    memcpy(data, d, data_len);
    _Data = data;
    _error_data = NULL;
}

NotificationPacket::~NotificationPacket()
{
    if (_Data != NULL)
	delete _Data;
    if (_error_data != NULL)
	delete _error_data;
}

void
NotificationPacket::decode() throw(InvalidPacket)
{
    const uint8_t *data = _Data + BGP_COMMON_HEADER_LEN;

    _error_code = data[0];
    _error_subcode = data[1];
    int error_data_len = _Length - (BGP_COMMON_HEADER_LEN +
				    BGP_NOTIFY_HEADER_LEN);
    if (error_data_len > 0) {
	uint8_t *ed = new uint8_t[error_data_len];
	memcpy(ed, data + 2, error_data_len);
	_error_data = ed;
	debug_msg("%s", str().c_str());
    } else if (error_data_len < 0) {
	_error_data = NULL;
	xorp_throw(InvalidPacket, "Truncated notification packet received");
    } else {
	_error_data = NULL;
    }
    return;
}

const uint8_t *
NotificationPacket::encode(int& len) const
{
    struct iovec io[6];

    uint16_t k = 0;

    debug_msg("Encode in NotificationPacket called (%d)\n", _Length);

    io[0].iov_base = (char *)_Marker;
    io[0].iov_len = 16;
    k = htons(_Length);

    io[1].iov_base = (char *)&k;
    io[1].iov_len = 2;

    io[2].iov_base = const_cast<char*>((const char *)&_Type);
    io[2].iov_len = 1;

    io[3].iov_base = const_cast<char*>((const char *)&_error_code);
    io[3].iov_len = 1;

    io[4].iov_base = const_cast<char*>((const char *)&_error_subcode);
    io[4].iov_len = 1;

    if (_error_data == NULL) {
	return flatten(&io[0], 5, len);
    } else {
	io[5].iov_base = const_cast<char*>((const char *)_error_data);
	io[5].iov_len = _Length - (BGP_COMMON_HEADER_LEN +
				   BGP_NOTIFY_HEADER_LEN);
	return flatten(&io[0], 6, len);
    }
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
NotificationPacket::str() const
{
    string s;
    s = "Notification Packet: ";

    switch (_error_code) {
    case MSGHEADERERR:
	s += "Message Header Error: ";
	switch (_error_subcode) {
	case CONNNOTSYNC:
	    s += "Connection Not Synchronized";
	    break;
	case BADMESSLEN:
	    if (_error_data != NULL)
		s += c_format("Bad Message Length - field: %d",
			      ntohs((uint16_t &)*_error_data));
	    else
		s += "Bad Message Length: ";
	    break;
	case BADMESSTYPE:
	    if (_error_data != NULL)
		s += c_format("Bad Message Type - field : %d",
			      (uint8_t &) *_error_data);
	    else
		s += "Bad Message Type";
	    break;
	}
	break;
    case OPENMSGERROR:
	s += "OPEN Message Error: ";
	switch (_error_subcode) {
	case UNSUPVERNUM:
	    if (_error_data != NULL)
		s += c_format("Unsupported Version Number. Min supported Version is %d",
			      ntohs((uint16_t &)*_error_data));
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
	}
	break;
    case UPDATEMSGERR:
	s += "UPDATE Message Error: ";
	switch (_error_subcode) {
	case MALATTRLIST:
	    s += "Malformed Attribute List.";
	    break;
	case UNRECOGWATTR:
	    if (_error_data != NULL)
		s += c_format("Unrecognized Well-known Attribute, Type %d",
			      _error_data[0]);
	    else
		s += "Unrecognized Well-known Attribute, NO ERROR DATA SUPPLIED";
	    break;
	case MISSWATTR:
	    if (_error_data != NULL)
		s += c_format("Missing Well-known Attribute, Type %d",
			      _error_data[0]);
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
    return s + "\n";
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

    if (_Length != him.get_length())
	return false;

    if (0 != memcmp(_error_data, him.error_data(),
		   _Length - (BGP_COMMON_HEADER_LEN + BGP_NOTIFY_HEADER_LEN)))
	return false;

    return true;
}
