// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/internal_message.cc,v 1.5 2004/05/19 10:25:21 mjh Exp $"

#include <string>
#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "internal_message.hh"

template<class A>
InternalMessage<A>::InternalMessage(const SubnetRoute<A> *rte,
				       const PeerHandler *origin,
				       uint32_t genid)
{
    XLOG_ASSERT(rte);

    _subnet_route = rte;
    _origin_peer = origin;
    _changed = false;
    _push = false;
    _from_previous_peering = false;
    _genid = genid;
}

template<class A>
InternalMessage<A>::~InternalMessage()
{
}

template<class A>
const IPNet<A>&
InternalMessage<A>::net() const
{
    return _subnet_route->net();
}

template<class A>
string
InternalMessage<A>::str() const
{
    string s;
    s += c_format("GenID is %d\n", _genid);
    if (_changed)
	s += "CHANGED flag is set\n";
    if (_push)
	s += "PUSH flag is set\n";
    if (_from_previous_peering)
	s += "FROM_PREVIOUS_PEERING flag is set\n";
    s += _subnet_route->str();
    return s;
}

template class InternalMessage<IPv4>;
template class InternalMessage<IPv6>;
