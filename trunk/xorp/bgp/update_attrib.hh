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

// $XORP: xorp/bgp/update_attrib.hh,v 1.2 2003/01/29 20:32:33 rizzo Exp $

#ifndef __BGP_UPDATE_ATTRIB_HH__
#define __BGP_UPDATE_ATTRIB_HH__

#include "libxorp/ipvxnet.hh"
#include "libxorp/exceptions.hh"
#include "exceptions.hh"

/**
 * Encoding used in BGP update packets to encode prefixes
 * (IPv4 only) for withdrawn routes and NLRI information.
 *
 * The prefixes are passed on the wire in a compressed format:
 *   1 byte:	prefix length L (in bits)
 *   n bytes:	prefix, L bits (n = (L+7)/8)
 *
 * Effectively, this class is just an IPv4Net.
 * We only need methods to encode and decode the objects.
 */
class BGPUpdateAttrib : public IPv4Net
{
public:
    /**
     * construct from an address d and masklength s
     */
    BGPUpdateAttrib(uint32_t d, uint8_t s) : IPv4Net(d, s)	{}

    BGPUpdateAttrib(const IPv4Net& p) : IPv4Net(p)		{}

    /**
     * Construct from wire format
     */
    BGPUpdateAttrib(const uint8_t *d);

    /**
     * store in memory in wire format
     */
    void copy_out(uint8_t *d) const;

    /**
     * total size in encoded format
     */
    size_t wire_size() const					{
	return calc_byte_size() + 1;
    }

    // size of next operand in memory
    static size_t size(const uint8_t *d) throw(CorruptMessage)	{
	if (d[0] > 32)
	    xorp_throw(CorruptMessage,
                c_format("inconsistent length %d", d[0]),
			0, 0);	// XXX change codes!
	return (d[0] + 7)/8 + 1;
    }

    size_t calc_byte_size() const			{
	return (prefix_len() + 7) / 8;
    }

    const IPv4Net& net() const				{
	return (IPv4Net &)(*this);
    }

    virtual string str() const				{
	return "Update Attribute";
    }

protected:
private:
};

#endif // __BGP_UPDATE_ATTRIB_HH__
