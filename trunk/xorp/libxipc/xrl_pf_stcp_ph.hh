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

// $XORP: xorp/libxipc/xrl_pf_stcp_ph.hh,v 1.2 2002/12/19 01:29:14 hodson Exp $

#ifndef __XRLPF_STCP_PH_HH__
#define __XRLPF_STCP_PH_HH__

/* major and minor maybe defined on system, undefine them */
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif

// STCP Packet Type enumerations

enum STCPPacketType {
    STCP_PT_HELO	= 0x00,
    STCP_PT_REQUEST	= 0x01,
    STCP_PT_RESPONSE	= 0x02
};

// STCP Packet Header.
struct STCPPacketHeader {
    STCPPacketHeader(uint32_t seqno,
		     STCPPacketType type,
		     const XrlError& err,
		     uint32_t xrl_data_bytes) {
	initialize(seqno, type, err, xrl_data_bytes);
    }
    void initialize(uint32_t seqno, STCPPacketType type,
		    const XrlError& err, uint32_t xrl_data_bytes);

    bool is_valid() const;

    uint32_t fourcc() const;

    uint8_t  major() const;
    uint8_t  minor() const;

    STCPPacketType type() const;

    uint32_t seqno() const;

    uint32_t error_code() const;
    uint32_t error_note_bytes() const;

    uint32_t xrl_data_bytes() const;

    inline uint32_t payload_bytes() const
    {
	return error_note_bytes() + xrl_data_bytes();
    }

private:
    uint8_t _fourcc[4];		  // fourcc 'S' 'T' 'C' 'P'
    uint8_t _major[1];		  // Version
    uint8_t _minor[1];		  // Version
    uint8_t _seqno[4];		  // Sequence number
    uint8_t _type[2];		  // [15:7] Set to zero, [0:1] hello/req./resp.
    uint8_t _error_code[4];	  // XrlError code
    uint8_t _error_note_bytes[4]; // Length of note (if any) assoc. w/ code
    uint8_t _xrl_data_bytes[4];	  // Xrl return args data bytes
};

#endif // __XRLPF_STCP_PH_HH__
