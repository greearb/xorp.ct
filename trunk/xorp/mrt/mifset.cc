// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/mrt/mifset.cc,v 1.1.1.1 2002/12/11 23:56:07 hodson Exp $"


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


MifsetTimers::MifsetTimers()
{
    
}

MifsetTimers::~MifsetTimers()
{
    
}

void
MifsetTimers::set_mif_timer(size_t vif_index, uint32_t delay_sec,
			    uint32_t delay_usec, cfunc_t func,
			    void *data_pointer)
{
    _mifset_timers[vif_index].start(delay_sec, delay_usec, func, data_pointer);
}

void
MifsetTimers::cancel_mif_timer(size_t vif_index)
{
    _mifset_timers[vif_index].cancel();
}

uint32_t
MifsetTimers::mif_timer_remain(size_t vif_index,
			       struct timeval& timeval_diff) const
{
    return (_mifset_timers[vif_index].left_timeval(&timeval_diff));
}

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
