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
 * A LSA header is a fixed length assume that we have been passed our
 * 20 bytes.
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

size_t
Lsa_header::copy_out(uint8_t *ptr) const
{
    OspfTypes::Version version = get_version();

    Lsa_header header(version);

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
