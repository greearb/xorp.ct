// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/auth.cc,v 1.5 2005/11/13 19:25:34 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "auth.hh"

/**
 * RFC 1141 Incremental Updating of the Internet Checksum
 */
inline
uint16_t
incremental_checksum(uint32_t orig, uint32_t add)
{
    // Assumes that the original value of the modified field was zero.
    uint32_t sum = orig + (~add & 0xffff);
    sum = (sum & 0xffff) + (sum >> 16);
    return sum;
}

void
AuthPlainText::generate(vector<uint8_t>& pkt) 
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET], // RFC 1141
	     incremental_checksum(extract_16(&ptr[Packet::CHECKSUM_OFFSET]),
				  AUTH_TYPE));

    memcpy(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_auth[0], sizeof(_auth));
}

inline
string
printable(uint8_t *p)
{
    string output;

    for(size_t i = 0; i < Packet::AUTH_PAYLOAD_SIZE; i++, p++) {
	if ('\0' == *p)
	    break;
	if (xorp_isprint(*p))
	    output += *p;
	else
	    output += c_format("[%#x]", *p);
    }
    
    return output;
}

bool
AuthPlainText::verify(vector<uint8_t>& pkt) 
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];
    OspfTypes::AuType autype = extract_16(&ptr[Packet::AUTH_TYPE_OFFSET]);
    if (AUTH_TYPE != autype) {
	string error =
	    c_format("Authentication type failure expected %u received %u",
		     AUTH_TYPE, autype);
	set_verify_error(error);
	return false;
    }

    if (0 != memcmp(&ptr[Packet::AUTH_PAYLOAD_OFFSET], &_auth[0], 
		    sizeof(_auth))) {
	string error = c_format("Authentication failure password mismatch");
	error += c_format(" Expected <%s> ", printable(&_auth[0]).c_str());
	error += c_format(" Received <%s> ",
			  printable(&ptr[Packet::AUTH_PAYLOAD_OFFSET]).
			  c_str());
	set_verify_error(error);
	return false;
    }
    
    return true;
}

void
AuthPlainText::reset()
{
    size_t len = get_password().size();
    memset(&_auth[0], 0, sizeof(_auth));
    memcpy(&_auth[0], get_password().c_str(),
	   len > sizeof(_auth) ? sizeof(_auth) : len);
}

void
AuthMD5::generate(vector<uint8_t>& pkt) 
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];

    // Set the authentication type and zero out the checksum.
    embed_16(&ptr[Packet::AUTH_TYPE_OFFSET], AUTH_TYPE);
    embed_16(&ptr[Packet::CHECKSUM_OFFSET], 0);

    // Set config in the authentication block.
    embed_16(&ptr[Packet::AUTH_PAYLOAD_OFFSET], 0);
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 2] = KEY_ID;
    ptr[Packet::AUTH_PAYLOAD_OFFSET + 3] = MD5_DIGEST_LENGTH;
    embed_32(&ptr[Packet::AUTH_PAYLOAD_OFFSET + 4], _outbound_seqno++);

    // Add the space for the digest at the end of the packet.
    size_t pend = pkt.size();
    pkt.resize(pend + MD5_DIGEST_LENGTH);
    ptr = &pkt[0];

    memcpy(&ptr[pend], &_key[0], MD5_DIGEST_LENGTH);
    
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pkt.size());
    MD5_Final(&ptr[pend], &ctx);
}

bool
AuthMD5::verify(vector<uint8_t>& pkt) 
{
    XLOG_ASSERT(pkt.size() >= Packet::STANDARD_HEADER_V2);

    uint8_t *ptr = &pkt[0];

    uint32_t seqno = extract_32(&ptr[Packet::AUTH_PAYLOAD_OFFSET + 4]);

    if (seqno < _inbound_seqno) {
	string error =
	    c_format("Authentication failure: seqno %u < expected %u", seqno,
		     _inbound_seqno);
	set_verify_error(error);
	return false;
    }
    
    MD5_CTX ctx;
    uint8_t digest[MD5_DIGEST_LENGTH];

    MD5_Init(&ctx);
    MD5_Update(&ctx, &ptr[0], pkt.size() - MD5_DIGEST_LENGTH);
    MD5_Update(&ctx, &_key[0], MD5_DIGEST_LENGTH);
    MD5_Final(&digest[0], &ctx);

    if (0 != memcmp(&digest[0], &ptr[pkt.size() - MD5_DIGEST_LENGTH],
		    MD5_DIGEST_LENGTH)) {
	string error = c_format("Authentication failure: digest mismatch");
	set_verify_error(error);
	return false;
    }

    _inbound_seqno = seqno;

    return true;
}

void
AuthMD5::reset()
{
    _inbound_seqno = 0;
    _outbound_seqno = 0;

    size_t len = get_password().size();
    memset(&_key[0], 0, sizeof(_key));
    memcpy(&_key[0], get_password().c_str(),
	   len >  MD5_DIGEST_LENGTH ? MD5_DIGEST_LENGTH : len);
}
