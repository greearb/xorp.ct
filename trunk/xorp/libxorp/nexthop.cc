// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/nexthop.cc,v 1.3 2004/04/01 19:54:10 mjh Exp $"

#include "xorp.h"
#include "nexthop.hh"

template<class A>
IPNextHop<A>::IPNextHop<A>(const A &from_ipaddr) {
    _addr = from_ipaddr;
}

template<class A>
IPPeerNextHop<A>::IPPeerNextHop<A>(const A &from_ipaddr)
    : IPNextHop<A>(from_ipaddr) {
}

template<class A> string
IPPeerNextHop<A>::str() const {
    string nh = "NH:";
    return nh + this->_addr.str();
}

template<class A>
IPEncapsNextHop<A>::IPEncapsNextHop<A>(const A &from_ipaddr)
    : IPNextHop<A>(from_ipaddr) {
}

template<class A> string
IPEncapsNextHop<A>::str() const {
    string enh = "NH->";
    return enh + this->_addr.str();
}

template<class A>
IPExternalNextHop<A>::IPExternalNextHop<A>(const A &from_ipaddr) 
    : IPNextHop<A>(from_ipaddr) 
{
}

template<class A> string
IPExternalNextHop<A>::str() const {
    return string("Ext>") + this->_addr.str();
}

DiscardNextHop::DiscardNextHop() : NextHop() {
}

string
DiscardNextHop::str() const {
    return string("DISCARD");
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
