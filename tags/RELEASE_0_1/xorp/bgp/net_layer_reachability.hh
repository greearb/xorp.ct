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

// $XORP: xorp/bgp/net_layer_reachability.hh,v 1.13 2002/12/09 18:28:43 hodson Exp $

#ifndef __BGP_NET_LAYER_REACHABILITY_HH__
#define __BGP_NET_LAYER_REACHABILITY_HH__

#include "update_attrib.hh"

class NetLayerReachability : public BGPUpdateAttrib
{
public:
    NetLayerReachability(uint32_t d, uint8_t s);
    NetLayerReachability(const IPv4Net& p);
    string str() const;
    bool NetLayerReachability::operator<(const NetLayerReachability &him) const;
    bool NetLayerReachability::operator==(const NetLayerReachability &him) const;
protected:
private:
};

#endif // __BGP_NET_LAYER_REACHABILITY_HH__
