// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/mrt/mifset.cc,v 1.3 2003/03/30 03:50:45 pavlin Exp $"


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
