// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/ospf/test_common.hh,v 1.7 2008/07/23 05:11:09 pavlin Exp $

#ifndef __OSPF_TEST_COMMON_HH__
#define __OSPF_TEST_COMMON_HH__

/**
 * Compute legal values for the options fields.
 */
inline
uint32_t
compute_options(OspfTypes::Version version, OspfTypes::AreaType area_type)
{
    // Set/UnSet E-Bit.
    Options options(version, 0);
    switch(area_type) {
    case OspfTypes::NORMAL:
	options.set_e_bit(true);
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	options.set_e_bit(false);
	break;
    }

    switch (version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	options.set_v6_bit(true);
	break;
    }

    return options.get_options();
}

/**
 * The type fields when saving an LSA database.
 */
enum TLV {
    TLV_VERSION = 1,	// The first entry in a file 4 byte version number.
    TLV_SYSTEM_INFO = 2,// A string defining the creation system. 
    TLV_OSPF_VERSION = 3,// The OSPF version that the database came from.
    TLV_AREA = 4,	// AREA that the following LSAs belong to 4 bytes
    TLV_LSA = 5		// Binary LSA.
};
const uint32_t TLV_CURRENT_VERSION = 1;	// Current version number

#endif // __OSPF_TEST_COMMON_HH__
