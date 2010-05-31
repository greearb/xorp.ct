// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/test_common.hh,v 1.3 2008/10/02 21:56:36 bms Exp $

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
