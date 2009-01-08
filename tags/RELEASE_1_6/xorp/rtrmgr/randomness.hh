// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/randomness.hh,v 1.12 2008/10/02 21:58:24 bms Exp $

#ifndef __RTRMGR_RANDOMNESS_HH__
#define __RTRMGR_RANDOMNESS_HH__

class RandomGen {
public:
    RandomGen();
    ~RandomGen();

    void get_random_bytes(size_t len, uint8_t* buf);

private:
    bool read_file(const string& filename);
    bool read_fd(FILE* file);
    void add_buf_to_randomness(uint8_t* buf, size_t len);

    static const size_t RAND_POOL_SIZE = 65536;

    bool	_random_exists;
    bool	_urandom_exists;
    uint8_t*	_random_data;
    uint32_t	_counter;
};

#endif // __RTRMGR_RANDOMNESS_HH__
