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

#ident "$XORP: xorp/bgp/update_attrib.cc,v 1.1.1.1 2002/12/11 23:55:50 hodson Exp $"

#include "bgp_module.h"
#include "config.h"
#include "update_attrib.hh"
#include "libxorp/ipv4net.hh"

#include <string.h>
#include <stdlib.h>
#include "libxorp/debug.h"

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
