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

// $XORP: xorp/bgp/update_attrib.hh,v 1.11 2002/12/09 18:28:50 hodson Exp $

#ifndef __BGP_UPDATE_ATTRIB_HH__
#define __BGP_UPDATE_ATTRIB_HH__

#include "libxorp/ipvxnet.hh"

class BGPUpdateAttrib
{
public:
    BGPUpdateAttrib(uint32_t d, uint8_t s);
    BGPUpdateAttrib(const IPv4Net& p);
    const char* get_data() const;
    const uint8_t* get_size() const;
    uint8_t  get_byte_size() const;
    void set_next(BGPUpdateAttrib* n);
    BGPUpdateAttrib* get_next() const;
    void dump();
    uint8_t calc_byte_size(uint8_t a);
    const IPv4Net& net() const {return _net;}
    virtual string str() const;
protected:
    uint32_t _data; //UpdateAttribute is defined by the spec as IPv4 only
    uint8_t _size; //prefix_length in bits
    uint8_t _byte_size; 
    BGPUpdateAttrib* _next;	
    IPv4Net _net;
private:
};

#endif // __BGP_UPDATE_ATTRIB_HH__
