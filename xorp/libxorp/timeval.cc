// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#include "timeval.hh"
#include "c_format.hh"

string
TimeVal::str() const
{
	return c_format("%d.%06d", XORP_INT_CAST(_sec), XORP_INT_CAST(_usec));
}

int64_t TimeVal::to_ms() const {
    int64_t ms = _usec / 1000;
    // Round a truncated fraction of <1ms to 1ms, not zero.
    if (_sec == 0 && ms == 0 && _usec != 0)
	ms = 1;
    else {
	ms += (int64_t)(_sec) * 1000LL;
    }
    return (ms);
}

void TimeVal::set_ms(int64_t ms) {
    if (ms == 0) {
	_sec = _usec = 0;
    }
    else {
	_sec = ms / 1000;
	_usec = (ms % 1000) * 1000;
    }
}

string
TimeVal::pretty_print() const
{
	time_t t = static_cast<time_t>(_sec);
	return c_format("%.24s", asctime(localtime(&t)));
}
