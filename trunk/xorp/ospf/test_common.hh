// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/test_common.hh,v 1.6 2008/01/04 03:16:58 pavlin Exp $

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
