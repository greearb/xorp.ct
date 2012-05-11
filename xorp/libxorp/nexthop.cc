// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
//
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "libxorp_module.h"

#include "xorp.h"
#include "nexthop.hh"

ostream& operator<<(ostream& os, const NextHop& rhs)
{
	os << rhs.str() << endl;
	return os;
}

string
NextHop::type_str(int type)
{
    static map<int, string> nexthop_names;
    if (nexthop_names.empty()) {
	nexthop_names[GENERIC_NEXTHOP] = " ";
	nexthop_names[PEER_NEXTHOP] = "NH: ";
	nexthop_names[ENCAPS_NEXTHOP] = "NH-> ";
	nexthop_names[EXTERNAL_NEXTHOP] = "Ext> ";
	nexthop_names[DISCARD_NEXTHOP] = "DISCARD ";
	nexthop_names[UNREACHABLE_NEXTHOP] = "UNREACHABLE ";
    }
    map<int, string>::iterator i = nexthop_names.find(type);
    if (i == nexthop_names.end())
	return " ";
    else
	return i->second;
}

template<class A>
IPNextHop<A>::IPNextHop(const A& from_ipaddr)
    : _addr(from_ipaddr)
{
}

template<class A>
string
IPNextHop<A>::str() const
{
    return (NextHop::type_str(type()) + this->_addr.str());
}

template<class A>
IPPeerNextHop<A>::IPPeerNextHop(const A& from_ipaddr)
    : IPNextHop<A>(from_ipaddr)
{
}

template<class A>
IPEncapsNextHop<A>::IPEncapsNextHop(const A& from_ipaddr)
    : IPNextHop<A>(from_ipaddr)
{
}

template<class A>
IPExternalNextHop<A>::IPExternalNextHop(const A& from_ipaddr)
    : IPNextHop<A>(from_ipaddr)
{
}

DiscardNextHop::DiscardNextHop()
    : NextHop()
{
}

UnreachableNextHop::UnreachableNextHop()
    : NextHop()
{
}

template class IPNextHop<IPv4>;
template class IPNextHop<IPv6>;
template class IPNextHop<IPvX>;

template class IPPeerNextHop<IPv4>;
template class IPPeerNextHop<IPv6>;
template class IPPeerNextHop<IPvX>;

template class IPExternalNextHop<IPv4>;
template class IPExternalNextHop<IPv6>;
template class IPExternalNextHop<IPvX>;

template class IPEncapsNextHop<IPv4>;
template class IPEncapsNextHop<IPv6>;
template class IPEncapsNextHop<IPvX>;
