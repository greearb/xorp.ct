// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/mrt/mifset.hh,v 1.17 2002/12/09 18:29:22 hodson Exp $


#ifndef __MRT_MIFSET_HH__
#define __MRT_MIFSET_HH__


//
// Multicast interface bitmap-based classes.
//


#include <sys/types.h>
#include <sys/time.h>

#include <bitset>

#include "max_vifs.h"
#include "timer.hh"



//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

// Interface array bitmask

typedef bitset<MAX_VIFS> Mifset;


// Interface array timers

/**
 * @short Class for handling timers per virtual network interface.
 */
class MifsetTimers {
public:
    /**
     * Default constructor
     */
    MifsetTimers();
    
    /**
     * Destructor
     */
    ~MifsetTimers();
    
    /**
     * Test if the timer is set
     * 
     * Test if the timer for a given virtual interface is set (i.e., running).
     * 
     * @param vif_index the index of the virtual interface to test if its timer
     * is set.
     * @return true if the timer is set, otherwise false.
     */
    bool	mif_timer_is_set(size_t vif_index) const {
	return (_mifset_timers[vif_index].is_set());
    }
    
    /**
     * Get the remaining expiration time
     * 
     * Compute the expiration time for the timer for a given virtual interface.
     * 
     * @param vif_index the index of the virtual interface to compute the
     * expiration time of its timer.
     * @param timeval_diff the timeval structure to store the result. Note that
     * if the timer is not set, the timeval structure will be reset to
     * (0, 0); i.e., zero seconds and zero microseconds.
     * @return the number of seconds until the timer expire. Note that
     * if the timer is not set, the return value is 0 (zero seconds).
     */
    uint32_t	mif_timer_remain(size_t vif_index, struct timeval& timeval_diff) const;
    
    /**
     * Set the timer
     * 
     * This method will start a one-time timer. If the timer should be
     * periodic, it should be restarted by the timeout function.
     * 
     * @param vif_index the index of the virtual interface to set the timer of.
     * @param delay_sec the number of seconds to the timer expiration.
     * @param delay_usec the number of microseconds (in addition to
     * the number of seconds @ref delay_sec) to the timer expiration.
     * @param func the timeout function to call when the timer expires.
     * @param data_pointer the data to supply to the timeout function @ref func
     * when the timer expires.
     */
    void	set_mif_timer(size_t vif_index, uint32_t delay_sec,
			      uint32_t delay_usec, cfunc_t func,
			      void *data_pointer);
    
    /**
     * Cancel the timer
     * 
     * @param vif_index the index of the virtual interface of the timer to
     * cancel.
     */
    void	cancel_mif_timer(size_t vif_index);
    
private:
    Timer	_mifset_timers[MAX_VIFS];	// The array with the timers
};


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
