// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/ospf/xrl_io.cc,v 1.5 2005/08/05 04:49:14 atanu Exp $"

#include "config.h"
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "xrl_io.hh"

template <typename A>
bool
XrlIO<A>::send(const string& interface, const string& vif,
	       A dst, A src,
	       uint8_t* data, uint32_t len)
{
    debug_msg("send(%s,%s,%s,%s,%p,%d\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::register_receive(typename IO<A>::ReceiveCallback cb)
{
    _cb = cb;

    return true;
}

template <typename A>
bool 
XrlIO<A>::enable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::disable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::join_multicast_group(const string& interface, const string& vif,
			       A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::leave_multicast_group(const string& interface, const string& vif,
			       A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::add_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		    bool discard)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false");

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::replace_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
			bool discard)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s\n",
	      cstring(net), cstring(nexthop), metric, equal ? "true" : "false",
	      discard ? "true" : "false");

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
XrlIO<A>::delete_route(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    XLOG_UNFINISHED();

    return true;
}

template class XrlIO<IPv4>;
template class XrlIO<IPv6>;
