// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/xrl_pf_stcp_ph.cc,v 1.17 2008/01/04 03:16:29 pavlin Exp $"

#include "xrl_module.h"
#include "libxorp/xorp.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <strings.h>

#include "libxorp/debug.h"

#include "libproto/packet.hh"

#include "xrl_error.hh"
#include "xrl_pf_stcp_ph.hh"


static const uint32_t PROTO_FOURCC = (('S' << 24) | ('T' << 16) |
				      ('C' <<  8) | ('P' <<  0));
static const uint8_t PROTO_MAJOR = 1;
static const uint8_t PROTO_MINOR = 1;


STCPPacketHeader::STCPPacketHeader(uint8_t* data)
    : _data(data),
      _fourcc(_data + _fourcc_offset),
      _major(_data + _major_offset),
      _minor(_data + _minor_offset),
      _seqno(_data + _seqno_offset),
      _type(_data + _type_offset),
      _error_code(_data + _error_code_offset),
      _error_note_bytes(_data + _error_note_bytes_offset),
      _xrl_data_bytes(_data + _xrl_data_bytes_offset)
{
    static_assert(STCPPacketHeader::SIZE == _fourcc_sizeof + _major_sizeof
		  + _minor_sizeof + _seqno_sizeof + _type_sizeof
		  + _error_code_sizeof + _error_note_bytes_sizeof
		  + _xrl_data_bytes_sizeof);
    static_assert(STCPPacketHeader::SIZE ==
		  _xrl_data_bytes_offset + _xrl_data_bytes_sizeof);
}

void
STCPPacketHeader::initialize(uint32_t		seqno,
			     STCPPacketType	type,
			     const XrlError&	xrl_err,
			     uint32_t		xrl_data_bytes)
{
    embed_32(_fourcc, PROTO_FOURCC);
    embed_8(_major, PROTO_MAJOR);
    embed_8(_minor, PROTO_MINOR);
    embed_16(_type, type);
    embed_32(_seqno, seqno);
    embed_32(_error_code, xrl_err.error_code());
    embed_32(_error_note_bytes, xrl_err.note().size());
    embed_32(_xrl_data_bytes, xrl_data_bytes);
}

static bool
stcp_packet_type_valid(uint8_t t)
{
    switch (STCPPacketType(t)) {
    case STCP_PT_HELO:
    case STCP_PT_HELO_ACK:
    case STCP_PT_REQUEST:
    case STCP_PT_RESPONSE:
	return true;
    }
    return false;
}

bool
STCPPacketHeader::is_valid() const
{
    if (fourcc() != PROTO_FOURCC) {
	debug_msg("invalid fourcc %c %c %c %c\n",
		  _fourcc[0], _fourcc[1], _fourcc[2], _fourcc[3]);
	return false;
    }
    if (major() != PROTO_MAJOR || minor() != PROTO_MINOR) {
	debug_msg("invalid proto (%u.%u)\n", _major[0], _minor[0]);
	return false;
    }
    if (stcp_packet_type_valid(type()) == false) {
	debug_msg("invalid type/padding (%u.%u)\n", _type[0], _type[0]);
	return false;
    }
    return true;
}

uint32_t
STCPPacketHeader::fourcc() const
{
    return extract_32(_fourcc);
}

uint8_t
STCPPacketHeader::major() const
{
    return extract_8(_major);
}

uint8_t
STCPPacketHeader::minor() const
{
    return extract_8(_minor);
}

STCPPacketType
STCPPacketHeader::type() const
{
    return STCPPacketType(extract_16(_type));
}

uint32_t
STCPPacketHeader::seqno() const
{
    return extract_32(_seqno);
}

uint32_t
STCPPacketHeader::error_code() const
{
    return extract_32(_error_code);
}

uint32_t
STCPPacketHeader::error_note_bytes() const
{
    return extract_32(_error_note_bytes);
}

uint32_t
STCPPacketHeader::xrl_data_bytes() const
{
    return extract_32(_xrl_data_bytes);
}

uint32_t
STCPPacketHeader::payload_bytes() const
{
    return xrl_data_bytes();
}

uint32_t
STCPPacketHeader::frame_bytes() const
{
    return header_size() + error_note_bytes() + payload_bytes();
}
