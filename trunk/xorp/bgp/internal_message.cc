// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/internal_message.cc,v 1.15 2008/07/23 05:09:32 pavlin Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
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
    s += c_format("GenID is %d\n", XORP_INT_CAST(_genid));
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
