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

/**
 * A LSA header is a fixed length, the caller should have allocated
 * enough space by calling the length() method.
 */
Lsa_header
Lsa_header::decode(uint8_t *ptr) throw(BadPacket)
{
    OspfTypes::Version version = get_version();

    Lsa_header header(version);

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

    return header;
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
