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

// $XORP: xorp/rtrmgr/randomness.hh,v 1.3 2002/12/09 18:29:38 hodson Exp $

#ifndef __RTRMGR_RANDOMNESS_HH__
#define __RTRMGR_RANDOMNESS_HH__

#include <stdio.h>

#define RAND_POOL_SIZE 65536

class RandomGen {
public:
    RandomGen();
    void add_event_loop(EventLoop *eventloop);
    void get_random_bytes(size_t len, uint8_t* buf);
private:
    bool read_file(const string& filename);
    bool read_fd(FILE *file);
    void add_buf_to_randomness(uint8_t *buf, int len);

    bool _random_exists;
    bool _urandom_exists;
    uint8_t *_random_data;
    XorpTimer timer;
    EventLoop *_eventloop;
    int _last_loc;
    int _counter;
};

#endif // __RTRMGR_RANDOMNESS_HH__
