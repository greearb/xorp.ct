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

#ident "$XORP: xorp/bgp/update_attrib.cc,v 1.18 2002/12/09 18:28:50 hodson Exp $"

#include "bgp_module.h"
#include "config.h"
#include "update_attrib.hh"
#include "libxorp/ipv4net.hh"

#include <string.h>
#include <stdlib.h>
#include "libxorp/debug.h"

BGPUpdateAttrib::BGPUpdateAttrib(uint32_t d, uint8_t s)
    : _net(d, s)
{
	_data = d;
	_size = s;
	_byte_size = calc_byte_size(s);
}


BGPUpdateAttrib::BGPUpdateAttrib(const IPv4Net& p)
    : _net(p)
{
    //XXXX what's going on here?
    //set_attrib((uint8_t *)p.masked_addr().addr(),(uint8_t)p.prefix_len());
    _data = p.masked_addr().addr();
    _size = p.prefix_len();
    _byte_size = calc_byte_size(_size);
}

const char *BGPUpdateAttrib::get_data() const
{
    return (const char *)&_data;
}

const uint8_t * BGPUpdateAttrib::get_size() const
{
    return (const uint8_t *)&_size;
}

void BGPUpdateAttrib::dump()
{
    //IPv4 *net = new IPv4((unsigned int)_data);
	//printf("%s\n", cstring(*net));
}

uint8_t  BGPUpdateAttrib::get_byte_size() const
{
	return _byte_size;
}

void BGPUpdateAttrib::set_next(BGPUpdateAttrib* n)
{
	_next = n;
}

BGPUpdateAttrib* BGPUpdateAttrib::get_next() const
{
	return _next;
}

uint8_t BGPUpdateAttrib::calc_byte_size(uint8_t s)
{
	return (s+7)/8;
}

string BGPUpdateAttrib::str() const
{
    string s;
    s = "Update Attribute";
    return s;
}






