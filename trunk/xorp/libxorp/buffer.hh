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

// $XORP: xorp/libxorp/buffer.hh,v 1.1.1.1 2002/12/11 23:56:04 hodson Exp $

#ifndef __LIBXORP_BUFFER_HH__
#define __LIBXORP_BUFFER_HH__


#include <string.h>
#include <new>

#include "xorp.h"
#include "exceptions.hh"


/**
 * @short A class for storing buffered data.
 *
 * This class can be used to conveniently store data.
 * Note: currently it has limited functionalities; more will be added
 * in the future.
 */
class Buffer {
public:
    /**
     * Constructor of a buffer of specified size.
     *
     * @param init_max_size the maximum amount of data that can be stored
     * in the buffer.
     */
    explicit Buffer(size_t init_max_size) : _max_size(init_max_size) {
	_data = new uint8_t[_max_size];
	reset();
    }
    
    /**
     * Destructor
     */
    ~Buffer() { delete[] _data; }
    
    /**
     * Reset/remove the data in the buffer.
     */
    void reset() { _cur_size = 0; memset(_data, 0, _max_size); }
    
    /**
     * @return the maximum amount of data that can be stored in the buffer.
     */
    size_t max_size() const { return (_max_size); }
    
    /**
     * @return the amount of data in the buffer.
     */
    size_t data_size() const { return (_cur_size); }
    
    /**
     * Get the data value of the octet at the specified offset.
     *
     * @param offset the data offset from the beginning of the buffer.
     * @return the data value at @ref offset.
     */
    uint8_t data(size_t offset) const throw (InvalidBufferOffset) {
	if (offset >= _max_size)
	    xorp_throw(InvalidBufferOffset, "invalid get data() offset");
	return (_data[offset]);
    }
    
    /**
     * @return a pointer to the data in the buffer.
     */
    uint8_t *data() { return (_data); }
    
    /**
     * Add a data octet to the buffer.
     * 
     * @param value the value of the data octet to add to the buffer.
     * @return @ref XORP_OK on success, otherwise @ref XORP_ERROR.
     */
    int add_data(uint8_t value) {
	if (is_full())
	    return (XORP_ERROR);
	_data[_cur_size++] = value;
	    return (XORP_OK);
    }
    
    /**
     * Test if the buffer is full.
     * 
     * @return true if the buffer is full, otherwise false.
     */
    bool is_full() const { return (_cur_size >= _max_size); }
    
private:
    size_t _max_size;		// The maximum amount of data to store
    size_t _cur_size;		// The current amount of stored data
    uint8_t *_data;		// Pointer to the buffer with the data
};


#endif // __LIBXORP_BUFFER_HH__
