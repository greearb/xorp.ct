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

#ident "$XORP: xorp/libxipc/xrl_pf_stcp_ph.cc,v 1.1 2002/12/14 23:43:02 hodson Exp $"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <strings.h>

#include "xrl_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "xrl_error.hh"
#include "xrl_pf_stcp_ph.hh"

static const uint32_t PROTO_FOURCC = (('S' << 24) | ('T' << 16) |
				      ('C' <<  8) | ('P' <<  0));
static const uint8_t PROTO_MAJOR = 1;
static const uint8_t PROTO_MINOR = 1;

// ----------------------------------------------------------------------------
// Utility byte packing and unpacking functions.
//
// NB memcpy used as one means of not caring about alignment.
//

static inline void pack4(uint32_t data, uint8_t* dst)
{
    data = htonl(data);
    memcpy(dst, &data, sizeof(data));
}

static inline void pack2(uint16_t data, uint8_t* dst)
{
    data = htons(data);
    memcpy(dst, &data, sizeof(data));
}

static inline void pack1(uint8_t data, uint8_t* dst)
{
    *dst = data;
}

static inline uint32_t unpack4(const uint8_t* src)
{
    uint32_t t;
    memcpy(&t, src, sizeof(t));
    return ntohl(t);
}

static inline uint16_t unpack2(const uint8_t* src)
{
    uint16_t t;
    memcpy(&t, src, sizeof(t));
    return ntohs(t);
}

static inline uint8_t unpack1(const uint8_t* src)
{
    return *src;
}

void
STCPPacketHeader::initialize(uint32_t seqno,
			     STCPPacketType type,
			     const XrlError& xrl_err,
			     uint32_t xrl_data_bytes) {
    pack4(PROTO_FOURCC, _fourcc);
    pack1(PROTO_MAJOR, _major);
    pack1(PROTO_MINOR, _minor);
    pack2(type, _type);
    pack4(seqno, _seqno);
    pack4(xrl_err.error_code(), _error_code);
    pack4(xrl_err.note().size(), _error_note_bytes);
    pack4(xrl_data_bytes, _xrl_data_bytes);
}

static bool
stcp_packet_type_valid(uint8_t t)
{
    switch (STCPPacketType(t)) {
    case STCP_PT_HELO:
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
    return unpack4(_fourcc);
}

uint8_t
STCPPacketHeader::major() const
{
    return unpack1(_major);
}

uint8_t
STCPPacketHeader::minor() const
{
    return unpack1(_minor);
}

STCPPacketType
STCPPacketHeader::type() const
{
    return STCPPacketType(unpack2(_type));
}

uint32_t
STCPPacketHeader::seqno() const
{
    return unpack4(_seqno);
}

uint32_t
STCPPacketHeader::error_code() const
{
    return unpack4(_error_code);
}

uint32_t
STCPPacketHeader::error_note_bytes() const
{
    return unpack4(_error_note_bytes);
}

uint32_t
STCPPacketHeader::xrl_data_bytes() const
{
    return unpack4(_xrl_data_bytes);
}
