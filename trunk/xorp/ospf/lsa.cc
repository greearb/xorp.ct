// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

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
    if (!(0 == x && 0 == y)) {
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

void
Lsa_header::decode(Lsa_header& header, uint8_t *ptr) const throw(BadPacket)
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
    header.set_length(extract_16(&ptr[18]));

//     return header;
}

/**
 * A LSA header is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
Lsa_header
Lsa_header::decode(uint8_t *ptr) const throw(BadPacket)
{
     Lsa_header header(get_version());
     decode(header, ptr);

     return header;
}

void
Lsa_header::decode_inline(uint8_t *ptr) throw(BadPacket)
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

    output = c_format("LS age %u", get_ls_age());

    switch(get_version()) {
    case OspfTypes::V2:
	output += c_format(" Options %#x", get_options());
	break;
    case OspfTypes::V3:
	break;
    }

    output += c_format(" LS type %u", get_ls_type());
    output += c_format(" Link State ID %u", get_link_state_id());
    output += c_format(" Advertising Router %#x", get_advertising_router());
    output += c_format(" LS sequence number %u", get_ls_sequence_number());
    output += c_format(" LS checksum %#x", get_ls_checksum());
    output += c_format(" length %u", get_length());
    
    return output;
}

/**
 * A link state request is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
Ls_request
Ls_request::decode(uint8_t *ptr) throw(BadPacket)
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

    output = c_format(" LS type %u", get_ls_type());
    output += c_format(" Link State ID %u", get_link_state_id());
    output += c_format(" Advertising Router %#x", get_advertising_router());
    
    return output;
}

size_t
RouterLink::length() const
{
    switch (get_version()) {
    case OspfTypes::V2:
	return 16;
	break;
    case OspfTypes::V3:
	return 16;
	break;
    }
    XLOG_UNREACHABLE();
    return 0;
}

RouterLink
RouterLink::decode(uint8_t *ptr, size_t& len) throw(BadPacket)
{
    if (len < length())
	xorp_throw(BadPacket,
		   c_format("RouterLink too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(length())));

    OspfTypes::Version version = get_version();

    RouterLink link(version);

    uint8_t type;
    uint8_t tos_number = 0;

    switch (version) {
    case OspfTypes::V2:
	set_link_id(extract_32(&ptr[0]));
	set_link_data(extract_32(&ptr[4]));

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
	    xorp_throw(BadPacket,
		   c_format("RouterLink illegal type should be 0..4 not %u",
			    XORP_UINT_CAST(type)));
		break;
	}
	link.set_metric(extract_16(&ptr[10]));
	// XXX - This LSA may be carrying more metric info for other
	// TOS. We are going to ignore them.
	tos_number = ptr[9];
	if (0 == tos_number)
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
	    xorp_throw(BadPacket,
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
    output += c_format(" Metric %u", get_metric());

    switch(get_version()) {
    case OspfTypes::V2:
	output += c_format(" Link ID %#x", get_link_id());
	output += c_format(" Link Data %#x", get_link_data());
	break;
    case OspfTypes::V3:
	output += c_format(" Interface ID %#x", get_interface_id());
	output += c_format(" Neighbour Interface ID %#x",
			   get_neighbour_interface_id());
	output += c_format(" Neighbour Router ID %#x",
			   get_neighbour_router_id());
	break;
    }

    return output;
}

Lsa::LsaRef
RouterLsa::decode(uint8_t *buf, size_t& len) const throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    size_t header_length = _header.length();
    size_t required = header_length + min_length();

    if (len < required)
	xorp_throw(BadPacket,
		   c_format("RouterLSA too short %u, must be at least %u",
			    XORP_UINT_CAST(len),
			    XORP_UINT_CAST(required)));

    // Verify the checksum.
    if (!verify_checksum(buf + 2, len - 2, 16 - 2))
	xorp_throw(BadPacket, c_format("LSA Checksum failed"));

    RouterLsa *lsa;
    try {
	lsa = new RouterLsa(version, buf, len);
	size_t nlinks = 0;	// Number of Links OSPF V2 Only

	// Decode the LSA Header.
	lsa->_header.decode_inline(buf);
	
	uint8_t flag = buf[header_length];
	switch(version) {
	case OspfTypes::V2:
	    lsa->set_v_bit(flag & 0x4);
	    lsa->set_e_bit(flag & 0x2);
	    lsa->set_b_bit(flag & 0x1);
	    nlinks = extract_16(&buf[header_length + 2]);
	    break;
	case OspfTypes::V3:
	    lsa->set_w_bit(flag & 0x8);
	    lsa->set_v_bit(flag & 0x4);
	    lsa->set_e_bit(flag & 0x2);
	    lsa->set_b_bit(flag & 0x1);
	    lsa->set_options(extract_32(&buf[header_length + 1]) & 0xffffff);
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
		xorp_throw(BadPacket,
			   c_format(
				    "RouterLSA mismatch in router links"
				    " expected %u received %u",
				    nlinks,
				    lsa->get_router_links().size()));
	    break;
	case OspfTypes::V3:
	    break;
	}

    } catch(BadPacket& e) {
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
	if (get_v_bit())
	    flag |= 0x4;
	if (get_e_bit())
	    flag |= 0x2;
	if (get_b_bit())
	    flag |= 0x1;
	embed_16(&ptr[header_length + 2], _router_links.size());
	break;
    case OspfTypes::V3:
	if (get_w_bit())
	    flag |= 0x8;
	if (get_v_bit())
	    flag |= 0x4;
	if (get_e_bit())
	    flag |= 0x2;
	if (get_b_bit())
	    flag |= 0x1;
	// Careful Options occupy 3 bytes, four bytes are written out.
	embed_32(&ptr[header_length], get_options());
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
    output += _header.str();

    output += "\n";

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	output += c_format("\tW-bit %s\n", get_w_bit() ? "true" : "false");
	break;
    }

    output += c_format("\tV-bit %s\n", get_v_bit() ? "true" : "false");
    output += c_format("\tE-bit %s\n", get_e_bit() ? "true" : "false");
    output += c_format("\tB-bit %s", get_b_bit() ? "true" : "false");

    switch(version) {
    case OspfTypes::V2:
	// # links we don't bother to store this info.
	break;
    case OspfTypes::V3:
	output += c_format("\n\tOptions %#x", get_options());
	break;
    }

    const list<RouterLink> &rl = _router_links;
    list<RouterLink>::const_iterator i = rl.begin();

    for (; i != rl.end(); i++) {
	output += "\n\t" + (*i).str();
    }

    return output;
}
