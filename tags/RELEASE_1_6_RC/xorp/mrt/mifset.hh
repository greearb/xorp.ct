// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/mrt/mifset.hh,v 1.10 2008/07/23 05:11:05 pavlin Exp $


#ifndef __MRT_MIFSET_HH__
#define __MRT_MIFSET_HH__


//
// Multicast interface bitmap-based classes.
//


#include <sys/types.h>

#include <bitset>
#include <vector>

#include "libxorp/xorp.h"
#include "max_vifs.h"



//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

// Interface array bitmask

typedef bitset<MAX_VIFS> Mifset;


//
// Global variables
//

//
// Global functions prototypes
//
void mifset_to_array(const Mifset& mifset, uint8_t *array);
void array_to_mifset(const uint8_t *array, Mifset& mifset);
void mifset_to_vector(const Mifset& mifset, vector<uint8_t>& vector);
void vector_to_mifset(const vector<uint8_t>& vector, Mifset& mifset);

#endif // __MRT_MIFSET_HH__
