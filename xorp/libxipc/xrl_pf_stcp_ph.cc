// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



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
      _flags(_data + _flags_offset),
      _type(_data + _type_offset),
      _error_code(_data + _error_code_offset),
      _error_note_bytes(_data + _error_note_bytes_offset),
      _xrl_data_bytes(_data + _xrl_data_bytes_offset)
{
    x_static_assert(STCPPacketHeader::SIZE == _fourcc_sizeof + _major_sizeof
		  + _minor_sizeof + _seqno_sizeof + _flags_sizeof + _type_sizeof
		  + _error_code_sizeof + _error_note_bytes_sizeof
		  + _xrl_data_bytes_sizeof);
    x_static_assert(STCPPacketHeader::SIZE ==
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
    embed_8(_flags, 0);
    embed_8(_type, type);
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
    return STCPPacketType(extract_8(_type));
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

bool
STCPPacketHeader::batch() const
{
    return extract_8(_flags) & FLAG_BATCH_MASK;
}

void
STCPPacketHeader::set_batch(bool batch)
{
    uint8_t flags = extract_8(_flags);

    flags &= ~(1 << FLAG_BATCH_SHIFT);
    flags |= batch << FLAG_BATCH_SHIFT;

    embed_8(_flags, flags);
}
