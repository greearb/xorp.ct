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

#ident "$XORP: xorp/bgp/update_attrib.cc,v 1.3 2003/02/07 05:53:06 rizzo Exp $"

#include "bgp_module.h"
#include "config.h"
#include "update_attrib.hh"
#include "libxorp/ipv4net.hh"

#include <string.h>
#include <stdlib.h>
#include "libxorp/debug.h"
#include "packet.hh"
#include <set>

BGPUpdateAttrib::BGPUpdateAttrib(const uint8_t *d)
{
    _prefix_len = d[0];
    uint32_t a = d[1] << 24;

    if (_prefix_len > 8)
	a |= (d[2] <<16);
    if (_prefix_len > 16)
	a |= (d[3] <<8 );
    if (_prefix_len > 24)
	a |= d[4];
    
    _masked_addr = IPv4(htonl(a));
}

void
BGPUpdateAttrib::copy_out(uint8_t *d) const
{
    uint32_t a = ntohl(masked_addr().addr());
    d[0] = prefix_len();
    d[1] = (a >> 24) & 0xff;
    if (d[0] > 8)
	d[2] = (a >> 16) & 0xff;
    if (d[0] > 16)
	d[3] = (a >> 8) & 0xff;
    if (d[0] > 24)
	d[4] = a & 0xff;
}

size_t
BGPUpdateAttribList::wire_size() const
{
    size_t len = 0;

    for (const_iterator uai = begin() ; uai != end(); ++uai)
        len += uai->wire_size();
    debug_msg("BGPUpdateAttribList::wire_size %p is %u (list size %u)\n",
	this, (uint32_t)len, (uint32_t)size());
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
            XLOG_WARNING(("Received duplicate " + wr.str() +
                       " in update message\n").c_str());
    }
    if (len != 0)
        xorp_throw(CorruptMessage,
                   c_format("leftover bytes %u", (uint32_t)len),
                   UPDATEMSGERR, ATTRLEN);
}

string
BGPUpdateAttribList::str() const
{
    string s = "";
    for (const_iterator i = begin(); i != end(); ++i)
        s += " - " + i->str() + "\n";
    return s;
}
