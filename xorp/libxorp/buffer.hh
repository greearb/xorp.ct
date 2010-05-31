// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/buffer.hh,v 1.10 2008/10/02 21:57:28 bms Exp $

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
