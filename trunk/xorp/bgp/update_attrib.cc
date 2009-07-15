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



#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4net.hh"

#include <set>

#include "update_attrib.hh"
#include "packet.hh"


BGPUpdateAttrib::BGPUpdateAttrib(const uint8_t *d)
{
    uint8_t plen = d[0];
    union {
	uint8_t		a8[4];
	uint32_t	a32;
    } a;

    a.a32 = 0;		// Reset the original value;

    // Calculate the number of bytes to copy
    uint8_t bytes = plen / 8;
    if (plen % 8)
	bytes++;

    // Copy the address
    if (bytes > 0)
	memcpy(a.a8, d + 1, bytes);

    // Set the address
    IPv4Net& net = *this;
    net = IPv4Net(IPv4(a.a32), plen);
}

void
BGPUpdateAttrib::copy_out(uint8_t *d) const
{
    uint8_t plen = prefix_len();
    union {
	uint8_t		a8[4];
	uint32_t	a32;
    } a;

    // Get the address
    a.a32 = masked_addr().addr();

    // Calculate the number of bytes to copy
    uint8_t bytes = plen / 8;
    if (plen % 8)
	bytes++;

    // Copy the address
    d[0] = plen;
    if (bytes > 0)
	memcpy(d + 1, a.a8, bytes);
}

size_t
BGPUpdateAttrib::size(const uint8_t *d) throw(CorruptMessage)
{
    if (d[0] > 32)
	xorp_throw(CorruptMessage,
		   c_format("inconsistent length %d", d[0]),
		   UPDATEMSGERR, INVALNETFIELD);
    return (d[0] + 7)/8 + 1;
}

size_t
BGPUpdateAttribList::wire_size() const
{
    size_t len = 0;

    for (const_iterator uai = begin() ; uai != end(); ++uai)
        len += uai->wire_size();
    debug_msg("BGPUpdateAttribList::wire_size %p is %u (list size %u)\n",
	      this, XORP_UINT_CAST(len), XORP_UINT_CAST(size()));
    return len;
}

uint8_t *
BGPUpdateAttribList::encode(size_t &l, uint8_t *d) const
{
    size_t want = wire_size();
    if (d != 0) {       // have a buffer, check length
        // XXX this should become an exception
        assert (l >= want);
    } else
	d = new uint8_t[want];
    l = want;

    size_t i=0;
    for (const_iterator uai = begin() ; uai != end(); ++uai) {
        uai->copy_out(d+i);
        i += uai->wire_size();
    }
    return d;
}

#if 0
void
BGPUpdateAttribList::add(const BGPUpdateAttrib &x)
{
	// XXX fill it up
}
#endif

void
BGPUpdateAttribList::decode(const uint8_t *d, size_t len)
	throw(CorruptMessage)
{
    clear();
    set <IPv4Net> x_set;

    while (len >0 && len >= BGPUpdateAttrib::size(d)) {
        BGPUpdateAttrib wr(d);
        len -= BGPUpdateAttrib::size(d);
        d += BGPUpdateAttrib::size(d);
        if (x_set.find(wr.net()) == x_set.end()) {
            push_back(wr);
            x_set.insert(wr.net());
        } else
            XLOG_WARNING("Received duplicate %s in update message",
			 wr.str("nlri or withdraw").c_str());
    }
    if (len != 0)
        xorp_throw(CorruptMessage,
                   c_format("leftover bytes %u", XORP_UINT_CAST(len)),
                   UPDATEMSGERR, ATTRLEN);
}


string
BGPUpdateAttribList::str(string nlri_or_withdraw) const
{
    string s = "";
    for (const_iterator i = begin(); i != end(); ++i)
        s += " - " + i->str(nlri_or_withdraw) + "\n";
    return s;
}
