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

#ident "$XORP: xorp/bgp/net_layer_reachability.cc,v 1.19 2002/12/09 18:28:43 hodson Exp $"

#include "bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "net_layer_reachability.hh"

NetLayerReachability::NetLayerReachability(uint32_t d, uint8_t s)
    : BGPUpdateAttrib(d, s)
{
    debug_msg("NetLayerReachability constructor (int) called, s=%d\n", s);
};

NetLayerReachability::NetLayerReachability(const IPv4Net& p) :
    BGPUpdateAttrib(p) {};

string
NetLayerReachability::str() const
{
    return "Network layer reachability " + _net.str();
}

bool
NetLayerReachability::operator<(const NetLayerReachability &him) const
{

    return _net < him.net();
}

bool
NetLayerReachability::operator==(const NetLayerReachability &him) const
{
    return _net == him.net();
}
