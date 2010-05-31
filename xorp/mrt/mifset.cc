// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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




//
// Multicast interface bitmap-based classes implementation.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "mifset.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


void
mifset_to_array(const Mifset& mifset, uint8_t *array)
{
    size_t i;
    
    // Reset the array
    for (i = 0; i < mifset.size() / sizeof(array[0]); i++)
	array[i] = 0;
    if (mifset.size() % sizeof(array[0]))
	array[i] = 0;		// XXX: the extra, semi-filled byte
    
    // Set the bits
    for (size_t i = 0; i < mifset.size(); i++) {
	size_t byte = i / sizeof(array[0]);
	size_t bit = i % sizeof(array[0]);
	if (mifset.test(i))
	    array[byte] |= (1 << bit);
    }
}

void
array_to_mifset(const uint8_t *array, Mifset& mifset)
{
    // Reset the mifset
    mifset.reset();
    
    // Set the bits
    for (size_t i = 0; i < mifset.size(); i++) {
	size_t byte = i / sizeof(array[0]);
	size_t bit = i % sizeof(array[0]);
	if (array[byte] & (1 << bit))
	    mifset.set(i);
    }
}

void
mifset_to_vector(const Mifset& mifset, vector<uint8_t>& vector)
{
    size_t i;
    
    // Reset the vector
    for (i = 0; i < vector.size(); i++)
	vector[i] = 0;
    
    // Set the bits
    for (size_t i = 0; i < mifset.size(); i++) {
	size_t byte = i / sizeof(vector[0]);
	size_t bit = i % sizeof(vector[0]);
	if (mifset.test(i))
	    vector[byte] |= (1 << bit);
    }
}

void vector_to_mifset(const vector<uint8_t>& vector, Mifset& mifset)
{
    // Reset the mifset
    mifset.reset();
    
    // Set the bits
    for (size_t i = 0; i < mifset.size(); i++) {
	size_t byte = i / sizeof(vector[0]);
	size_t bit = i % sizeof(vector[0]);
	if (vector[byte] & (1 << bit))
	    mifset.set(i);
    }
}
