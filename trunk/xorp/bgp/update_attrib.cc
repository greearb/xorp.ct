// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/update_attrib.cc,v 1.20 2007/02/16 22:45:22 pavlin Exp $"

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
            XLOG_WARNING(("Received duplicate " +
			  wr.str("nlri or withdraw") +
			  " in update message\n").c_str());
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
