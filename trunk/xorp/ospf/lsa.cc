// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/lsa.cc,v 1.106 2007/10/19 01:13:56 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libproto/packet.hh"

#include <map>
#include <list>
#include <set>

#include "ospf.hh"
#include "lsa.hh"
#include "fletcher_checksum.hh"

/**
 * Verify the checksum of an LSA.
 *
 * The cool part of this checksum algorithm is that it is not necessry
 * to compare the computed checksum against the one in the packet; as
 * the computed value should always be zero.
 */
inline
bool
verify_checksum(uint8_t *buf, size_t len, size_t offset)
{
    int32_t x, y;
    fletcher_checksum(buf, len, offset, x, y);
    if (!(255 == x && 255 == y)) {
	return false;
    }

    return true;
}

/**
 * Compute the checksum.
 */
inline
uint16_t
compute_checksum(uint8_t *buf, size_t len, size_t offset)
{
    int32_t x, y;
    fletcher_checksum(buf, len, offset, x, y);

    return (x << 8) | (y);
}

/**
 * Get the length of this LSA and verify that the length is smaller
 * than the buffer and large enough to be a valid LSA. Otherwise throw
 * an exception. Don't modify the value if its greater than the
 * buffer.
 */
inline
size_t
get_lsa_len_from_header(const char *caller, uint8_t *buf, size_t len,
			size_t min_len)
    throw(InvalidPacket)
{
    size_t tlen = Lsa_header::get_lsa_len_from_buffer(buf);
    if (tlen > len) {
	xorp_throw(InvalidPacket,
		   c_format("%s header len %u larger than buffer %u",
			    caller,
			    XORP_UINT_CAST(tlen),
			    XORP_UINT_CAST(len)));
    } else if(tlen < min_len) {
	xorp_throw(InvalidPacket,
		   c_format("%s header len %u smaller than minimum LSA "
			    "of this type %u",
			    caller,
			    XORP_UINT_CAST(tlen),
			    XORP_UINT_CAST(min_len)));
    } else {
	len = tlen;
    }

    return len;
}

uint16_t
Lsa_header::get_lsa_len_from_buffer(uint8_t *ptr)
{
    return extract_16(&ptr[18]);
}

void
Lsa_header::decode(Lsa_header& header, uint8_t *ptr) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

//     Lsa_header header(version);

    header.set_ls_age(extract_16(&ptr[0]));

    switch(version) {
    case OspfTypes::V2:
	header.set_options(ptr[2]);
	header.set_ls_type(ptr[3]);
	break;
    case OspfTypes::V3:
	header.set_ls_type(extract_16(&ptr[2]));
	break;
    }

    header.set_link_state_id(extract_32(&ptr[4]));
    header.set_advertising_router(extract_32(&ptr[8]));
    header.set_ls_sequence_number(extract_32(&ptr[12]));
    header.set_ls_checksum(extract_16(&ptr[16]));
    header.set_length(get_lsa_len_from_buffer(&ptr[0]));

//     return header;
}

/**
 * A LSA header is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
Lsa_header
Lsa_header::decode(uint8_t *ptr) const throw(InvalidPacket)
{
     Lsa_header header(get_version());
     decode(header, ptr);

     return header;
}

void
Lsa_header::decode_inline(uint8_t *ptr) throw(InvalidPacket)
{
    decode(*this, ptr);
}

/**
 * A LSA header is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
size_t
Lsa_header::copy_out(uint8_t *ptr) const
{
    OspfTypes::Version version = get_version();

    embed_16(&ptr[0], get_ls_age());

    switch(version) {
    case OspfTypes::V2:
	ptr[2] = get_options();
	ptr[3] = get_ls_type();
	break;
    case OspfTypes::V3:
	embed_16(&ptr[2], get_ls_type());
	break;
    }

    embed_32(&ptr[4], get_link_state_id());
    embed_32(&ptr[8], get_advertising_router());
    embed_32(&ptr[12], get_ls_sequence_number());
    embed_16(&ptr[16], get_ls_checksum());
    embed_16(&ptr[18], get_length());

    return 20;
}

string
Lsa_header::str() const
{
    string output;

    output = c_format("LS age %4u", get_ls_age());

    switch(get_version()) {
    case OspfTypes::V2:
	output += c_format(" Options %#4x %s", get_options(),
			   cstring(Options(get_version(), get_options())));
	break;
    case OspfTypes::V3:
	break;
    }

    output += c_format(" LS type %#x", get_ls_type());
    output += c_format(" Link State ID %s",
		       pr_id(get_link_state_id()).c_str());
    output += c_format(" Advertising Router %s",
		       pr_id(get_advertising_router()).c_str());
    output += c_format(" LS sequence number %#x", get_ls_sequence_number());
    output += c_format(" LS checksum %#x", get_ls_checksum());
    output += c_format(" length %u", get_length());
    
    return output;
}

inline
uint16_t
add_age(uint16_t current, uint16_t delta)
{
    uint16_t age = current + delta;

    // The largest acceptable age for an LSA is MaxAge.
    return age < OspfTypes::MaxAge ? age : OspfTypes::MaxAge;
}

void
Lsa::revive(const TimeVal& now)
{
    Lsa_header& h = get_header();

    XLOG_ASSERT(get_self_originating());
    XLOG_ASSERT(h.get_ls_age() == OspfTypes::MaxAge);
    XLOG_ASSERT(h.get_ls_sequence_number() == OspfTypes::MaxSequenceNumber);

    set_transmitted(false);
    h.set_ls_sequence_number(OspfTypes::InitialSequenceNumber);
    get_header().set_ls_age(0);
    record_creation_time(now);

    encode();
}

void
Lsa::update_age_and_seqno(const TimeVal& now)
{
    XLOG_ASSERT(get_self_originating());
//     XLOG_ASSERT(get_header().get_ls_age() != OspfTypes::MaxAge);

    // If this LSA has been transmitted then its okay to bump the
    // sequence number.
    if (get_transmitted()) {
	set_transmitted(false);
	increment_sequence_number();
    }
    get_header().set_ls_age(0);
    record_creation_time(now);

    encode();
}

void
Lsa::update_age(TimeVal now)
{
    // Compute the new age value based on the current time.
    TimeVal tdiff = now - _creation;
    uint16_t age = add_age(_initial_age, tdiff.sec());

    set_ls_age(age);
}

void
Lsa::update_age_inftransdelay(uint8_t *ptr, uint16_t inftransdelay)
{
    uint16_t age;
    
    age = extract_16(ptr);

    debug_msg("Current age %u\n", age);

    age = add_age(age, inftransdelay);

    debug_msg("Age with InfTransDelay added %u\n", age);

    embed_16(&ptr[0], age);
}

void
Lsa::set_maxage()
{
    set_ls_age(OspfTypes::MaxAge);
}

bool
Lsa::maxage() const
{
    return OspfTypes::MaxAge == _header.get_ls_age();
}

bool
Lsa::max_sequence_number() const
{
    return OspfTypes::MaxSequenceNumber == _header.get_ls_sequence_number();
}

void
Lsa::set_ls_age(uint16_t age)
{
    XLOG_ASSERT(age <= OspfTypes::MaxAge);

    if (OspfTypes::MaxAge == _header.get_ls_age())
	XLOG_FATAL("Age already MaxAge(%d) being set to %d\n%s",
		     OspfTypes::MaxAge, age, str().c_str());

    // Update the stored age value.
    _header.set_ls_age(age);

    // If a stored packet exists update it as well. The age field is
    // not covered by the checksum so this is safe.
    if (_pkt.size() < sizeof(uint16_t))
	return;

    // Update the age in the stored LSA itself.
    uint8_t *ptr = &_pkt[0];
    embed_16(&ptr[0], _header.get_ls_age());
}

/**
 * A link state request is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
Ls_request
Ls_request::decode(uint8_t *ptr) throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    Ls_request header(version);

    switch(version) {
    case OspfTypes::V2:
	header.set_ls_type(extract_32(&ptr[0]));
	break;
    case OspfTypes::V3:
	header.set_ls_type(extract_16(&ptr[2]));
	break;
    }

    header.set_link_state_id(extract_32(&ptr[4]));
    header.set_advertising_router(extract_32(&ptr[8]));

    return header;
}

/**
 * A link state request is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
size_t
Ls_request::copy_out(uint8_t *ptr) const
{
    OspfTypes::Version version = get_version();

    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[0], get_ls_type());
	break;
    case OspfTypes::V3:
	embed_16(&ptr[2], get_ls_type());
	break;
    }

    embed_32(&ptr[4], get_link_state_id());
    embed_32(&ptr[8], get_advertising_router());

    return 20;
}

string
Ls_request::str() const
{
    string output;

    output = c_format(" LS type %#x", get_ls_type());
    output += c_format(" Link State ID %s", 
		       pr_id(get_link_state_id()).c_str());
    output += c_format(" Advertising Router %s",
		       pr_id(get_advertising_router()).c_str());
    
    return output;
}

/* LsaDecoder */

LsaDecoder::~LsaDecoder()
{
    // Free all the stored decoder packets.
    map<uint16_t, Lsa *>::iterator i;

    for(i = _lsa_decoders.begin(); i != _lsa_decoders.end(); i++)
	delete i->second;

    delete _unknown_lsa_decoder;
}

void
LsaDecoder::register_decoder(Lsa *lsa)
{
    // Don't allow a registration to be overwritten.
    XLOG_ASSERT(0 == _lsa_decoders.count(lsa->get_ls_type()));
    _lsa_decoders[lsa->get_ls_type()] = lsa;

    // Keep a record of the smallest LSA that may be decoded.
    // This will be useful as sanity check in the packet decoder.
    if (0 == _min_lsa_length)
	_min_lsa_length = lsa->min_length();
    else if (_min_lsa_length > lsa->min_length())
	_min_lsa_length = lsa->min_length();
}

void
LsaDecoder::register_unknown_decoder(Lsa *lsa)
{
    switch(get_version()) {
    case OspfTypes::V2:
	XLOG_FATAL("OSPFv2 does not have an Unknown-LSA decoder");
	break;
    case OspfTypes::V3:
	break;
    }
    // Don't allow a registration to be overwritten.
    XLOG_ASSERT(0 == _unknown_lsa_decoder);
    _unknown_lsa_decoder = lsa;
}

Lsa::LsaRef
LsaDecoder::decode(uint8_t *ptr, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();
    Lsa_header header(version);

    if (len < header.length())
	xorp_throw(InvalidPacket,
		   c_format("LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(header.length())));

    // XXX
    // The LSA header is going to be decoder here and again in the
    // actual LSA code. Could consider passing in the already decoded header.
    header.decode_inline(ptr);

    map<uint16_t, Lsa *>::const_iterator i;
    uint16_t type = header.get_ls_type();
    i = _lsa_decoders.find(type);
    if (i == _lsa_decoders.end()) {
	if (0 != _unknown_lsa_decoder)
	    return _unknown_lsa_decoder->decode(ptr, len);
	xorp_throw(InvalidPacket,
		   c_format("OSPF Version %u Unknown LSA Type %#x",
			    version, type));
    }
    
    Lsa *lsa = i->second;

    return lsa->decode(ptr, len);
}

/* IPv6Prefix */

size_t
IPv6Prefix::length() const
{
    XLOG_ASSERT(OspfTypes::V3 == get_version());

    return bytes_per_prefix(get_network().prefix_len());
}

IPv6Prefix
IPv6Prefix::decode(uint8_t *ptr, size_t& len, uint8_t prefixlen,
		   uint8_t option) const
    throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    IPv6Prefix prefix(version, use_metric());
    prefix.set_prefix_options(option);
    
    uint8_t addr[IPv6::ADDR_BYTELEN];
    uint32_t bytes = bytes_per_prefix(prefixlen);
    if (bytes > sizeof(addr)) 
	xorp_throw(InvalidPacket,
		   c_format("Prefix length %u larger than %u", bytes,
			    XORP_UINT_CAST(sizeof(addr))));

    if (bytes > len)
	xorp_throw(InvalidPacket,
		   c_format("Prefix length %u larger than packet %u", bytes,
			    XORP_UINT_CAST(len)));

    memset(&addr[0], 0, IPv6::ADDR_BYTELEN);
    memcpy(&addr[0], ptr, bytes);
    IPv6 v6;
    v6.set_addr(&addr[0]);
    IPNet<IPv6> v6net(v6, prefixlen);
    
    prefix.set_network(v6net);
    len = bytes;

    return prefix;
}

/**
 * The caller should have called length() to pre-allocate the space
 * required.
 */
size_t
IPv6Prefix::copy_out(uint8_t *ptr) const
{
    XLOG_ASSERT(OspfTypes::V3 == get_version());

    IPv6 v6 = get_network().masked_addr();
    uint8_t buf[IPv6::ADDR_BYTELEN];
    v6.copy_out(&buf[0]);
    size_t bytes = bytes_per_prefix(get_network().prefix_len());
    memcpy(ptr, &buf[0], bytes);

    return bytes;
}

string
IPv6Prefix::str() const
{
    XLOG_ASSERT(OspfTypes::V3 == get_version());

    string output;

    output = c_format("Options %#4x", get_prefix_options());
    output += c_format(" DN-bit: %d", get_dn_bit());
    output += c_format(" P-bit: %d", get_p_bit());
    output += c_format(" MC-bit: %d", get_mc_bit());
    output += c_format(" LA-bit: %d", get_la_bit());
    output += c_format(" NU-bit: %d", get_nu_bit());
    if (use_metric())
	output += c_format(" Metric %u", get_metric());
    output += c_format(" Address %s", cstring(get_network()));

    return output;
}

/* RouterLink */

size_t
RouterLink::length() const
{
    switch (get_version()) {
    case OspfTypes::V2:
	return 12;
	break;
    case OspfTypes::V3:
	return 16;
	break;
    }
    XLOG_UNREACHABLE();
    return 0;
}

RouterLink
RouterLink::decode(uint8_t *ptr, size_t& len) const throw(InvalidPacket)
{
    if (len < length())
	xorp_throw(InvalidPacket,
		   c_format("RouterLink too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(length())));

    OspfTypes::Version version = get_version();

    RouterLink link(version);

    uint8_t type;
    uint8_t tos_number = 0;

    switch (version) {
    case OspfTypes::V2:
	link.set_link_id(extract_32(&ptr[0]));
	link.set_link_data(extract_32(&ptr[4]));

	type = ptr[8];
	switch (type) {
	case p2p:
	    link.set_type(p2p);
	    break;
	case transit:
	    link.set_type(transit);
	    break;
	case stub:
	    link.set_type(stub);
	    break;
	case vlink:
	    link.set_type(vlink);
	    break;
	default:
	    xorp_throw(InvalidPacket,
		   c_format("RouterLink illegal type should be 0..4 not %u",
			    XORP_UINT_CAST(type)));
		break;
	}
	link.set_metric(extract_16(&ptr[10]));
	// XXX - This LSA may be carrying more metric info for other
	// TOS. We are going to ignore them.
	tos_number = ptr[9];
	if (0 != tos_number)
	    XLOG_INFO("Non zero number of TOS %u", tos_number);
	break;
    case OspfTypes::V3:
	type = ptr[0];
	switch (type) {
	case p2p:
	    link.set_type(p2p);
	    break;
	case transit:
	    link.set_type(transit);
	    break;
	case vlink:
	    link.set_type(vlink);
	    break;
	default:
	    xorp_throw(InvalidPacket,
	    c_format("RouterLink illegal type should be 1,2 or 4 not %u",
			    XORP_UINT_CAST(type)));
		break;
	}
	if (0 != ptr[1])
	    XLOG_INFO("RouterLink field that should be zero is %u", ptr[1]);
	link.set_metric(extract_16(&ptr[2]));
	link.set_interface_id(extract_32(&ptr[4]));
	link.set_neighbour_interface_id(extract_32(&ptr[8]));
	link.set_neighbour_router_id(extract_32(&ptr[12]));
	break;
    }

    len = length() + tos_number * 4;

    return link;
}

/**
 * The caller should have called length() to pre-allocate the space
 * required.
 */
size_t
RouterLink::copy_out(uint8_t *ptr) const
{
    OspfTypes::Version version = get_version();

    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[0], get_link_id());
	embed_32(&ptr[4], get_link_data());
	ptr[8] = get_type();
	ptr[9] = 0;	// TOS
	embed_16(&ptr[10], get_metric());
	break;
    case OspfTypes::V3:
	ptr[0] = get_type();
	ptr[1] = 0;
	embed_16(&ptr[2], get_metric());
	embed_32(&ptr[4], get_interface_id());
	embed_32(&ptr[8], get_neighbour_interface_id());
	embed_32(&ptr[12], get_neighbour_router_id());
	break;
    }

    return length();
}

string
RouterLink::str() const
{
    string output;

    output = c_format("Type %u", get_type());

    switch(get_type()) {
    case p2p:
	output += c_format(" Point-to-point");
	break;
    case transit:
	output += c_format(" Transit network");
	break;
    case stub:
	output += c_format(" Stub network");
	break;
    case vlink:
	output += c_format(" Virtual Link");
	break;
    }

    switch(get_version()) {
    case OspfTypes::V2:
	switch(get_type()) {
	case p2p:
	    output += c_format(" Neighbours Router ID %s",
			       pr_id(get_link_id()).c_str());
	    output += c_format(" Routers interface address %s",
			       pr_id(get_link_data()).c_str());
	    break;
	case transit:
	    output += c_format(" IP address of Designated router %s",
			       pr_id(get_link_id()).c_str());
	    output += c_format(" Routers interface address %s",
			       pr_id(get_link_data()).c_str());
	    break;
	case stub:
	    output += c_format(" Subnet number %s",
			       pr_id(get_link_id()).c_str());
	    output += c_format(" Mask %s", pr_id(get_link_data()).c_str());
	    break;
	case vlink:
	    output += c_format(" Neighbours Router ID %s",
			       pr_id(get_link_id()).c_str());
	    output += c_format(" Routers interface address %s",
			       pr_id(get_link_data()).c_str());
	    break;
	}
	break;
    case OspfTypes::V3:
	output += c_format(" Interface ID %u", get_interface_id());
	switch(get_type()) {
	case transit:
	    output += c_format(" Designated Router Interface ID %u",
			       get_neighbour_interface_id());
	    output += c_format(" Designated Router ID %s",
			       pr_id(get_neighbour_router_id()).c_str());
	    break;
	default:
	    output += c_format(" Neighbour Interface ID %u",
			       get_neighbour_interface_id());
	    output += c_format(" Neighbour Router ID %s",
			       pr_id(get_neighbour_router_id()).c_str());
	    break;
	}
	break;
    }

    output += c_format(" Metric %u", get_metric());

    return output;
}

Lsa::LsaRef
UnknownLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Unknown-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Unknown-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    UnknownLsa *lsa = 0;
    try {
	lsa = new UnknownLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	
    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
UnknownLsa::encode()
{
    XLOG_FATAL("Can't encode an Unknown-LSA");

    return true;
}

string
UnknownLsa::str() const
{
    string output;

    output += "Unknown-LSA:\n";
    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    return output;
}

Lsa::LsaRef
RouterLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Router-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Router-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    RouterLsa *lsa = 0;
    try {
	lsa = new RouterLsa(version, buf, len);
	size_t nlinks = 0;	// Number of Links OSPFv2 Only

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	
	uint8_t flag = buf[header_length];
	switch(version) {
	case OspfTypes::V2:
	    lsa->set_nt_bit(flag & 0x10);
	    lsa->set_v_bit(flag & 0x4);
	    lsa->set_e_bit(flag & 0x2);
	    lsa->set_b_bit(flag & 0x1);
	    nlinks = extract_16(&buf[header_length + 2]);
	    break;
	case OspfTypes::V3:
	    lsa->set_nt_bit(flag & 0x10);
	    lsa->set_w_bit(flag & 0x8);
	    lsa->set_v_bit(flag & 0x4);
	    lsa->set_e_bit(flag & 0x2);
	    lsa->set_b_bit(flag & 0x1);
	    lsa->set_options(extract_24(&buf[header_length + 1]));
	    break;
	}

	// Extract the router links
	RouterLink rl(version);
	uint8_t *start = &buf[header_length + 4];
	uint8_t *end = &buf[len];
	while(start < end) {
	    size_t link_len = end - start;
	    lsa->get_router_links().push_back(rl.decode(start, link_len));
	    XLOG_ASSERT(0 != link_len);
	    start += link_len;
	}

	switch(version) {
	case OspfTypes::V2:
	    if (nlinks != lsa->get_router_links().size())
		xorp_throw(InvalidPacket,
			   c_format(
				    "Router-LSA mismatch in router links"
				    " expected %u received %u",
				    XORP_UINT_CAST(nlinks),
				    XORP_UINT_CAST(lsa->
						   get_router_links().size())));
	    break;
	case OspfTypes::V3:
	    break;
	}

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
RouterLsa::encode()
{
    OspfTypes::Version version = get_version();

    size_t router_link_len = RouterLink(version).length();
    size_t len = _header.length() + 4 + _router_links.size() * router_link_len;

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    uint8_t flag = 0;
    switch(version) {
    case OspfTypes::V2:
	if (get_nt_bit())
	    flag |= 0x10;
	if (get_v_bit())
	    flag |= 0x4;
	if (get_e_bit())
	    flag |= 0x2;
	if (get_b_bit())
	    flag |= 0x1;
	embed_16(&ptr[header_length + 2], _router_links.size());
	break;
    case OspfTypes::V3:
	if (get_nt_bit())
	    flag |= 0x10;
	if (get_w_bit())
	    flag |= 0x8;
	if (get_v_bit())
	    flag |= 0x4;
	if (get_e_bit())
	    flag |= 0x2;
	if (get_b_bit())
	    flag |= 0x1;
	embed_24(&ptr[header_length + 1], get_options());
	break;
    }
    ptr[header_length] = flag;

    // Copy out the router links.
    list<RouterLink> &rl = get_router_links();
    list<RouterLink>::iterator i = rl.begin();
    size_t index = header_length + 4;
    for (; i != rl.end(); i++, index += router_link_len) {
	(*i).copy_out(&ptr[index]);
    }

    XLOG_ASSERT(index == len);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
RouterLsa::str() const
{
    OspfTypes::Version version = get_version();

    string output;

    output += "Router-LSA:\n";
    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    output += "\n";

    output += c_format("\tbit Nt %s\n", bool_c_str(get_nt_bit()));

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	output += c_format("\tbit W %s\n", bool_c_str(get_w_bit()));
	break;
    }

    output += c_format("\tbit V %s\n", bool_c_str(get_v_bit()));
    output += c_format("\tbit E %s\n", bool_c_str(get_e_bit()));
    output += c_format("\tbit B %s", bool_c_str(get_b_bit()));

    switch(version) {
    case OspfTypes::V2:
	// # links, don't bother to store this info.
	break;
    case OspfTypes::V3:
	output += c_format("\n\tOptions %#x %s", get_options(),
			   cstring(Options(get_version(), get_options())));
	break;
    }

    const list<RouterLink> &rl = _router_links;
    list<RouterLink>::const_iterator i = rl.begin();

    for (; i != rl.end(); i++) {
	output += "\n\t" + (*i).str();
    }

    return output;
}

Lsa::LsaRef
NetworkLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Network-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Network-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    NetworkLsa *lsa = 0;
    try {
	lsa = new NetworkLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	uint8_t *start = 0;
	switch(version) {
	case OspfTypes::V2:
	    lsa->set_network_mask(extract_32(&buf[header_length]));
	    start = &buf[header_length + 4];
	    break;
	case OspfTypes::V3:
	    lsa->set_options(extract_24(&buf[header_length + 1]));
	    start = &buf[header_length + 4];
	    break;
	}

	uint8_t *end = &buf[len];
	while(start < end) {
	    if (!(start < end))
		xorp_throw(InvalidPacket, c_format("Network-LSA too short"));
	    lsa->get_attached_routers().push_back(extract_32(start));
	    start += 4;
	}

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
NetworkLsa::encode()
{
    OspfTypes::Version version = get_version();

    size_t len = 0;

    switch(version) {
    case OspfTypes::V2:
 	len = _header.length() + 4 + 4 * get_attached_routers().size();
	break;
    case OspfTypes::V3:
	len = _header.length() + 4 + 4 * get_attached_routers().size();
	break;
    }

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    size_t index = 0;
    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[header_length], get_network_mask());
	index = header_length + 4;
	break;
    case OspfTypes::V3:
	embed_24(&ptr[header_length + 1], get_options());
	index = header_length + 4;
	break;
    }

    // Copy out the attached router state.
    list<OspfTypes::RouterID> &ars = get_attached_routers();
    list<OspfTypes::RouterID>::iterator i = ars.begin();
    for (; i != ars.end(); i++) {
	switch(version) {
	case OspfTypes::V2:
	    embed_32(&ptr[index], *i);
	    index += 4;
	    break;
	case OspfTypes::V3:
	    embed_32(&ptr[index], *i);
	    index += 4;
	break;
	}
    }

    XLOG_ASSERT(index == len);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
NetworkLsa::str() const
{
    OspfTypes::Version version = get_version();

    string output;

    output += "Network-LSA:\n";
    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    switch(version) {
    case OspfTypes::V2:
	output += c_format("\n\tNetwork Mask %#x", get_network_mask());
	break;
    case OspfTypes::V3:
	output += c_format("\n\tOptions %#x %s", get_options(),
			   cstring(Options(get_version(), get_options())));
	break;
    }

    list<OspfTypes::RouterID> ars = _attached_routers;
    list<OspfTypes::RouterID>::iterator i = ars.begin();
    for (; i != ars.end(); i++) {
	switch(version) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    break;
	}
	output += "\n\tAttached Router " + pr_id(*i);
    }

    return output;
}

Lsa::LsaRef
SummaryNetworkLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Summary-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Summary-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    SummaryNetworkLsa *lsa = 0;
    try {
	lsa = new SummaryNetworkLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	switch(version) {
	case OspfTypes::V2:
	    lsa->set_network_mask(extract_32(&buf[header_length]));
	    lsa->set_metric(extract_24(&buf[header_length + 5]));
	    break;
	case OspfTypes::V3:
	    lsa->set_metric(extract_24(&buf[header_length + 1]));
	    IPv6Prefix prefix(version);
	    size_t space = len - IPV6_PREFIX_OFFSET;
	    IPv6Prefix prefix_decoder(version);
	    prefix = prefix_decoder.decode(&buf[header_length + 8],
					   space,
					   buf[header_length + 4],
					   buf[header_length + 5]);
	    size_t space_left = (len - (IPV6_PREFIX_OFFSET + space));
	    if (0 != space_left) 
		xorp_throw(InvalidPacket,
			   c_format("Space left in LSA %u bytes",
				    XORP_UINT_CAST(space_left)));
	    lsa->set_ipv6prefix(prefix);
	    break;
	}

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
SummaryNetworkLsa::encode()
{
    OspfTypes::Version version = get_version();

    size_t len = 0;

    switch(version) {
    case OspfTypes::V2:
 	len = _header.length() + 8;
	break;
    case OspfTypes::V3:
	len = _header.length() + 8 + get_ipv6prefix().length();
	break;
    }

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    size_t index = 0;
    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[header_length], get_network_mask());
	embed_24(&ptr[header_length + 5], get_metric());
	index = header_length + 8;
	break;
    case OspfTypes::V3:
	embed_24(&ptr[header_length + 1], get_metric());
	IPv6Prefix prefix = get_ipv6prefix();
	ptr[header_length + 4] = prefix.get_network().prefix_len();
	ptr[header_length + 5] = prefix.get_prefix_options();
	index = header_length + 8 + prefix.copy_out(&ptr[header_length + 8]);
	break;
    }

    XLOG_ASSERT(len == index);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
SummaryNetworkLsa::str() const
{
    OspfTypes::Version version = get_version();

    string output;

    switch(version) {
    case OspfTypes::V2:
	output = "Summary-LSA:\n";
	break;
    case OspfTypes::V3:
	output = "Inter-Area-Prefix-LSA:\n";
	break;
    }

    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    switch(version) {
    case OspfTypes::V2:
	output += c_format("\n\tNetwork Mask %#x", get_network_mask());
	output += c_format("\n\tMetric %d", get_metric());
	break;
    case OspfTypes::V3:
	output += c_format("\n\tMetric %d", get_metric());
	output += c_format("\n\tIPv6Prefix %s", cstring(get_ipv6prefix()));
	break;
    }

    return output;
}

Lsa::LsaRef
SummaryRouterLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Summary-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Summary-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    SummaryRouterLsa *lsa = 0;
    try {
	lsa = new SummaryRouterLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	switch(version) {
	case OspfTypes::V2:
	    lsa->set_network_mask(extract_32(&buf[header_length]));
	    lsa->set_metric(extract_24(&buf[header_length + 5]));
	    break;
	case OspfTypes::V3:
	    lsa->set_options(extract_24(&buf[header_length + 1]));
	    lsa->set_metric(extract_24(&buf[header_length + 5]));
	    lsa->set_destination_id(extract_32(&buf[header_length + 8]));
	    break;
	}

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
SummaryRouterLsa::encode()
{
    OspfTypes::Version version = get_version();

    size_t len = 0;

    switch(version) {
    case OspfTypes::V2:
 	len = _header.length() + 8;
	break;
    case OspfTypes::V3:
	len = _header.length() + 12;
	break;
    }

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    size_t index = 0;
    switch(version) {
    case OspfTypes::V2:
	embed_32(&ptr[header_length], get_network_mask());
	embed_24(&ptr[header_length + 5], get_metric());
	index = header_length + 8;
	break;
    case OspfTypes::V3:
	embed_24(&ptr[header_length + 1], get_options());
	embed_24(&ptr[header_length + 5], get_metric());
	embed_32(&ptr[header_length + 8], get_destination_id());
	index = header_length + 12;
	break;
    }

    XLOG_ASSERT(len == index);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
SummaryRouterLsa::str() const
{
    OspfTypes::Version version = get_version();

    string output;

    switch(version) {
    case OspfTypes::V2:
	output = "Summary-LSA:\n";
	break;
    case OspfTypes::V3:
	output = "Inter-Area-Router-LSA:\n";
	break;
    }

    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    switch(version) {
    case OspfTypes::V2:
	output += c_format("\n\tNetwork Mask %#x", get_network_mask());
	output += c_format("\n\tMetric %d", get_metric());
	break;
    case OspfTypes::V3:
	output += c_format("\n\tOptions %#x %s", get_options(),
			   cstring(Options(get_version(), get_options())));
	output += c_format("\n\tMetric %d", get_metric());
	output += c_format("\n\tDestination Router ID %s",
			   pr_id(get_destination_id()).c_str());
	break;
    }

    return output;
}

Lsa::LsaRef
ASExternalLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("AS-External-LSA too short %u, "
			    "must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("AS-External-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    ASExternalLsa *lsa = 0;
    try {
	// lsa = new this(version, buf, len);
	lsa = donew(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	uint8_t flag;
	switch(version) {
	case OspfTypes::V2: {
	    lsa->set_network_mask(extract_32(&buf[header_length]));
	    flag = buf[header_length + 4];
	    lsa->set_e_bit(flag & 0x80);
	    lsa->set_metric(extract_24(&buf[header_length + 5]));
	    IPv4 forwarding_address;
	    forwarding_address.copy_in(&buf[header_length + 8]);
	    lsa->set_forwarding_address_ipv4(forwarding_address);
	    lsa->set_external_route_tag(extract_32(&buf[header_length + 12]));
	}
	    break;
	case OspfTypes::V3:
	    flag = buf[header_length];
	    lsa->set_e_bit(flag & 0x4);
	    lsa->set_f_bit(flag & 0x2);
	    lsa->set_t_bit(flag & 0x1);
	    lsa->set_metric(extract_24(&buf[header_length + 1]));
 	    lsa->set_referenced_ls_type(extract_16(&buf[header_length + 6]));
	    size_t space = len - IPV6_PREFIX_OFFSET;
	    IPv6Prefix prefix_decoder(version);
	    lsa->set_ipv6prefix(prefix_decoder.decode(&buf[header_length + 8],
						      space,
						      buf[header_length + 4],
						      buf[header_length + 5]));
	    size_t index = header_length + 8 + space;
	    if (lsa->get_f_bit()) {
		if (index + IPv6::ADDR_BYTELEN > len)
		    xorp_throw(InvalidPacket,
			       c_format("AS-External-LSA"
					" bit F set, packet too short"));
		IPv6 address;
		address.copy_in(&buf[index]);
		lsa->set_forwarding_address_ipv6(address);
		index += IPv6::ADDR_BYTELEN;
	    }
	    if (lsa->get_t_bit()) {
		if (index + 4 > len)
		    xorp_throw(InvalidPacket,
			       c_format("AS-External-LSA"
					" bit T set, packet too short"));
		lsa->set_external_route_tag(extract_32(&buf[index]));
		index += 4;
	    }
	    if (0 != lsa->get_referenced_ls_type()) {
		if (index + 4 > len)
		    xorp_throw(InvalidPacket,
			       c_format("AS-External-LSA"
					" Referenced LS Type set, "
					"packet too short"));
		lsa->set_referenced_link_state_id(extract_32(&buf[index]));
	    }
	    break;
	}

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
ASExternalLsa::encode()
{
    OspfTypes::Version version = get_version();

    size_t len = 0;

    switch(version) {
    case OspfTypes::V2:
 	len = _header.length() + 16;
	break;
    case OspfTypes::V3:
	len = _header.length() + 8 +
	    get_ipv6prefix().length() +
	    (get_f_bit() ? IPv6::ADDR_BYTELEN : 0) +
	    (get_t_bit() ? 4 : 0) +
	    (0 != get_referenced_ls_type() ? 4 : 0);
	break;
    }

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    size_t index = 0;
    uint8_t flag = 0;
    switch(version) {
    case OspfTypes::V2: {
	embed_32(&ptr[header_length], get_network_mask());
	if (get_e_bit())
	    flag |= 0x80;
	ptr[header_length + 4] = flag;
	embed_24(&ptr[header_length + 5], get_metric());
	IPv4 forwarding_address = get_forwarding_address_ipv4();
	forwarding_address.copy_out(&ptr[header_length + 8]);
	embed_32(&ptr[header_length + 12], get_external_route_tag());
	index = header_length + 16;
    }
	break;
    case OspfTypes::V3:
	if (get_e_bit())
	    flag |= 0x4;
	if (get_f_bit())
	    flag |= 0x2;
	if (get_t_bit())
	    flag |= 0x1;
	ptr[header_length] = flag;
	embed_24(&ptr[header_length + 1], get_metric());
  	embed_16(&ptr[header_length + 6], get_referenced_ls_type());
	IPv6Prefix prefix = get_ipv6prefix();
	ptr[header_length + 4] = prefix.get_network().prefix_len();
	ptr[header_length + 5] = prefix.get_prefix_options();
	index = header_length + 8 + prefix.copy_out(&ptr[header_length + 8]);
	if (get_f_bit()) {
	    IPv6 forwarding_address = get_forwarding_address_ipv6();
	    forwarding_address.copy_out(&ptr[index]);
	    index += IPv6::ADDR_BYTELEN;
	}
	if (get_t_bit()) {
	    embed_32(&ptr[index], get_external_route_tag());
	    index += 4;
	}
	if (0 != get_referenced_ls_type()) {
	    embed_32(&ptr[index], get_referenced_link_state_id());
	    index += 4;
	}
	break;
    }

    XLOG_ASSERT(len == index);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
ASExternalLsa::str() const
{
    OspfTypes::Version version = get_version();

    string output;

    output = str_name() + ":\n";

    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    switch(version) {
    case OspfTypes::V2:
	output += c_format("\n\tNetwork Mask %#x", get_network_mask());
	output += c_format("\n\tbit E %s", bool_c_str(get_e_bit()));
	output += c_format("\n\tMetric %d %#x", get_metric(), get_metric());
	if (get_metric() == OspfTypes::LSInfinity)
	    output += c_format(" LSInfinity");
	output += c_format("\n\tForwarding address %s",
			   cstring(get_forwarding_address_ipv4()));
	output += c_format("\n\tExternal Route Tag %#x",
			   get_external_route_tag());
	break;
    case OspfTypes::V3:
	output += c_format("\n\tbit E %s", bool_c_str(get_e_bit()));
	output += c_format("\n\tbit F %s", bool_c_str(get_f_bit()));
	output += c_format("\n\tbit T %s", bool_c_str(get_t_bit()));
	output += c_format("\n\tMetric %d %#x", get_metric(), get_metric());
	if (get_metric() == OspfTypes::LSInfinity)
	    output += c_format(" LSInfinity");
	output += c_format("\n\tIPv6Prefix %s", cstring(get_ipv6prefix()));
	output += c_format("\n\tReferenced LS Type %#x",
			   get_referenced_ls_type());
	if (get_f_bit())
	    output += c_format("\n\tForwarding address %s",
			       cstring(get_forwarding_address_ipv6()));
	if (get_t_bit())
	    output += c_format("\n\tExternal Route Tag %#x",
			       get_external_route_tag());
	if (0 != get_referenced_ls_type())
	    output += c_format("\n\tReferenced Link State ID %s",
			       pr_id(get_referenced_link_state_id()).c_str());
	    
	break;
    }

    return output;
}

Lsa::LsaRef
LinkLsa::decode(uint8_t *buf, size_t& len) const throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Link-LSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Link-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    LinkLsa *lsa = 0;
    try {
	lsa = new LinkLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	uint8_t *start = 0;

	lsa->set_rtr_priority(extract_8(&buf[header_length + 0]));
	lsa->set_options(extract_24(&buf[header_length + 1]));
	IPv6 address;
	address.copy_in(&buf[header_length + 4]);
	lsa->set_link_local_address(address);
	size_t prefix_num = extract_32(&buf[header_length + 4 +
					    IPv6::ADDR_BYTELEN]);

	start = &buf[header_length + 4 + IPv6::ADDR_BYTELEN + 4];
	uint8_t *end = &buf[len];
	IPv6Prefix decoder(version);
	while(start < end) {
	    if (!(start + 2 < end))
		xorp_throw(InvalidPacket, c_format("Link-LSA too short"));
	    size_t space = end - (start + 4);
	    IPv6Prefix prefix = decoder.decode(start + 4, space,
					       extract_8(start),
					       extract_8(start + 1));
 	    lsa->get_prefixes().push_back(prefix);
	    start += (space + 4);
	    if (0 == --prefix_num) {
		if (start != end)
		    xorp_throw(InvalidPacket,
			       c_format("Link-LSA # prefixes read data left"));
		break;
	    }
	}
	if (0 != prefix_num)
	    if (start != end)
		xorp_throw(InvalidPacket,
			   c_format("Link-LSA # %d left buffer depleted",
				    XORP_UINT_CAST(prefix_num)));

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
LinkLsa::encode()
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    size_t len = _header.length() + 4 + IPv6::ADDR_BYTELEN + 4;

    list<IPv6Prefix> &ps = get_prefixes();
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++)
	len += i->length() + 4;

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    size_t index = 0;

    embed_8(&ptr[header_length], get_rtr_priority());
    embed_24(&ptr[header_length + 1], get_options());
    get_link_local_address().copy_out(&ptr[header_length + 4]);
    embed_32(&ptr[header_length + 4 + IPv6::ADDR_BYTELEN], ps.size());

    index = header_length + 4 + IPv6::ADDR_BYTELEN + 4;

    // Copy out the IPv6 Prefixes.
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++) {
	embed_8(&ptr[index], i->get_network().prefix_len());
	embed_8(&ptr[index + 1], i->get_prefix_options());
	index += i->copy_out(&ptr[index + 4]) + 4;
    }

    XLOG_ASSERT(index == len);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
LinkLsa::str() const
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    string output;

    output += "Link-LSA:\n";
    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    output += c_format("\n\tRtr Priority %d", get_rtr_priority());
    output += c_format("\n\tOptions %#x %s", get_options(),
		       cstring(Options(get_version(), get_options())));
    output += c_format("\n\tLink-local Interface Address %s",
		       cstring(get_link_local_address()));

    list<IPv6Prefix> ps = _prefixes;
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++)
	output += "\n\tIPv6 Prefix " + i->str();

    return output;
}

Lsa::LsaRef
IntraAreaPrefixLsa::decode(uint8_t *buf, size_t& len) const 
    throw(InvalidPacket)
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(InvalidPacket,
		   c_format("Intra-Area-Prefix-LSA too short %u, "
			    "must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // This guy throws an exception of there is a problem.
    len = get_lsa_len_from_header("Intra-Area-Prefix-LSA", buf, len, required);

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(InvalidPacket, c_format("LSA Checksum failed"));

    IntraAreaPrefixLsa *lsa = 0;
    try {
	lsa = new IntraAreaPrefixLsa(version, buf, len);

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	uint8_t *start = 0;

	size_t prefix_num = extract_16(&buf[header_length]);
	lsa->set_referenced_ls_type(extract_16(&buf[header_length + 2]));
	lsa->set_referenced_link_state_id(extract_32(&buf[header_length + 4]));
	lsa->set_referenced_advertising_router(extract_32(&buf[header_length +
							       8]));

	start = &buf[header_length + 2 + 2 + 4 + 4];
	uint8_t *end = &buf[len];
	IPv6Prefix decoder(version, true);
	while(start < end) {
	    if (!(start + 2 < end))
		xorp_throw(InvalidPacket, c_format("Intra-Area-Prefix-LSA "
						   "too short"));
	    size_t space = end - (start + 4);
	    IPv6Prefix prefix = decoder.decode(start + 4, space,
					       extract_8(start),
					       extract_8(start + 1));
	    prefix.set_metric(extract_16(start + 2));
 	    lsa->get_prefixes().push_back(prefix);
	    start += (space + 4);
	    if (0 == --prefix_num) {
		if (start != end)
		    xorp_throw(InvalidPacket,
			       c_format("Intra-Area-Prefix-LSA # prefixes "
					"read data left"));
		break;
	    }
	}
	if (0 != prefix_num)
	    if (start != end)
		xorp_throw(InvalidPacket,
			   c_format("Intra-Area-Prefix-LSA # %d left "
				    "buffer depleted",
				    XORP_UINT_CAST(prefix_num)));

    } catch(InvalidPacket& e) {
	delete lsa;
	throw e;
    }

    return Lsa::LsaRef(lsa);
}

bool
IntraAreaPrefixLsa::encode()
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    size_t len = _header.length() + 2 + 2 + 4 + 4;

    list<IPv6Prefix> &ps = get_prefixes();
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++)
	len += i->length() + 4;

    _pkt.resize(len);
    uint8_t *ptr = &_pkt[0];
//     uint8_t *ptr = new uint8_t[len];
    memset(ptr, 0, len);

    // Copy the header into the packet
    _header.set_ls_checksum(0);
    _header.set_length(len);
    size_t header_length = _header.copy_out(ptr);
    XLOG_ASSERT(len > header_length);

    embed_16(&ptr[header_length], ps.size());
    embed_16(&ptr[header_length + 2], get_referenced_ls_type());
    embed_32(&ptr[header_length + 4], get_referenced_link_state_id());
    embed_32(&ptr[header_length + 8], get_referenced_advertising_router());

    size_t index = header_length + 2 + 2 + 4 + 4;

    // Copy out the IPv6 Prefixes.
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++) {
	embed_8(&ptr[index], i->get_network().prefix_len());
	embed_8(&ptr[index + 1], i->get_prefix_options());
	embed_16(&ptr[index + 2], i->get_metric());
	index += i->copy_out(&ptr[index + 4]) + 4;
    }

    XLOG_ASSERT(index == len);

    // Compute the checksum and write the whole header out again.
    _header.set_ls_checksum(compute_checksum(ptr + 2, len - 2, 16 - 2));
    _header.copy_out(ptr);

    return true;
}

string
IntraAreaPrefixLsa::str() const
{
    OspfTypes::Version version = get_version();
    XLOG_ASSERT(OspfTypes::V3 == version);

    string output;

    output += "Intra-Area-Prefix-LSA:\n";
    if (!valid())
	output += "INVALID\n";
    output += _header.str();

    output += c_format("\n\tReferenced LS type %#x", get_referenced_ls_type());
    if (get_referenced_ls_type() == RouterLsa(version).get_ls_type()) {
	output += c_format(" Router-LSA");
    } else if (get_referenced_ls_type() == NetworkLsa(version).get_ls_type()) {
	output += c_format(" Network-LSA");
    } else {
	output += c_format(" Unknown");
    }
    output += c_format("\n\tReferenced Link State ID %s",
		       pr_id(get_referenced_link_state_id()).c_str());
    output += c_format("\n\tReferenced Advertising Router %s",
		       pr_id(get_referenced_advertising_router()).c_str());

    list<IPv6Prefix> ps = _prefixes;
    for(list<IPv6Prefix>::iterator i = ps.begin(); i != ps.end(); i++)
	output += "\n\tIPv6 Prefix " + i->str();

    return output;
}
