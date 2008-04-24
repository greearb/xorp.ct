// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP$

#ifndef __OLSR_TEST_COMMON_HH__
#define __OLSR_TEST_COMMON_HH__

/**
 * The type fields when saving an OLSR instance's link state database.
 */
enum TLV {
    TLV_VERSION = 1,	  // The first entry in a file 4 byte version number.
    TLV_SYSTEM_INFO = 2,  // A string defining the creation system.
    TLV_OLSR_VERSION = 3, // The OLSR version that the database came from.
    TLV_N1 = 4,		  // A one-hop neighbor entry.
    TLV_N2 = 5,		  // A two-hop neighbor entry.
    TLV_TC = 6,		  // A topology control entry.
    TLV_MID = 7,	  // A Multiple Interface entry.

    TLV_START = TLV_VERSION, // Lower bound.
    TLV_END = TLV_MID	     // Upper bound.
};
const uint32_t TLV_CURRENT_VERSION = 1;	// Current version number

#endif // __OLSR_TEST_COMMON_HH__
