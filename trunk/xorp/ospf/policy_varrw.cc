// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <list>
#include <set>

#include "libproto/spt.hh"

#include "ospf.hh"
#include "policy_varrw.hh"

template <typename A>
OspfVarRW<A>::OspfVarRW(IPNet<A>& network, A& nexthop, uint32_t& metric,
			bool& e_bit, uint32_t& tag, bool& tag_set,
			PolicyTags& policytags)
    : _network(network), _nexthop(nexthop), _metric(metric), _e_bit(e_bit),
      _tag(tag), _tag_set(tag_set), _policytags(policytags)
{
}

template <>
void
OspfVarRW<IPv4>::start_read()
{
    initialize(VAR_NETWORK, _ef.create(ElemIPv4Net::id,
				       _network.str().c_str()));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv4NextHop::id,
	       _nexthop.str().c_str()));

    start_read_common();
}

template <>
void
OspfVarRW<IPv6>::start_read()
{
    initialize(VAR_NETWORK, _ef.create(ElemIPv6Net::id,
				       _network.str().c_str()));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv6NextHop::id,
               _nexthop.str().c_str()));

    start_read_common();
}

template <typename A>
void
OspfVarRW<A>::start_read_common()
{
    initialize(VAR_POLICYTAGS, _policytags.element());
    initialize(VAR_METRIC, _ef.create(ElemU32::id,
				      c_format("%u", _metric).c_str()));
    initialize(VAR_EBIT, _ef.create(ElemU32::id,
				    c_format("%u", _e_bit ? 2 : 1).c_str()));

    // XXX which tag wins?
    Element* element = _policytags.element_tag();
    ElemU32* e = dynamic_cast<ElemU32*>(element);
    if (e != NULL && e->val())
	_tag = e->val();

    delete element;

    initialize(VAR_TAG, _ef.create(ElemU32::id,
				   c_format("%u", _tag).c_str()));
}

template <typename A>
Element* 
OspfVarRW<A>::single_read(const Id& /*id*/)
{
    XLOG_UNREACHABLE();

    return 0;
}

template <>
void
OspfVarRW<IPv4>::single_write(const Id& id, const Element& e)
{
    switch (id) {
    case VAR_NETWORK: {
	const ElemIPv4Net* eip = dynamic_cast<const ElemIPv4Net*>(&e);
	XLOG_ASSERT(eip != NULL);
	_network = IPNet<IPv4>(eip->val());
    }
	break;

    case VAR_NEXTHOP: {
	const ElemIPv4NextHop* eip = dynamic_cast<const ElemIPv4NextHop*>(&e);
	XLOG_ASSERT(eip != NULL);
	_nexthop = IPv4(eip->val());
    }
	break;

    default:
	single_write_common(id, e);
	break;
    }
}

template <typename A>
void
OspfVarRW<A>::single_write_common(const Id& id, const Element& e)
{
    switch (id) {
    case VAR_POLICYTAGS:
	_policytags.set_ptags(e);
	break;

    case VAR_METRIC: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_metric = u32.val();
    }
	break;

    case VAR_EBIT: {
	const ElemU32& b = dynamic_cast<const ElemU32&>(e);
	_e_bit = b.val() == 2 ? true : false;
    }

	break;

    case VAR_TAG: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_tag = u32.val();
	_policytags.set_tag(e);
    }
	break;

    default:
	XLOG_WARNING("Unexpected Id %d %s", id, cstring(e));
    }
}

template <>
void
OspfVarRW<IPv6>::single_write(const Id& id, const Element& e)
{
    switch (id) {
    case VAR_NETWORK: {
	const ElemIPv6Net* eip = dynamic_cast<const ElemIPv6Net*>(&e);
	XLOG_ASSERT(eip != NULL);
	_network = IPNet<IPv6>(eip->val());
    }
	break;

    case VAR_NEXTHOP: {
	const ElemIPv6NextHop* eip = dynamic_cast<const ElemIPv6NextHop*>(&e);
	XLOG_ASSERT(eip != NULL);
	_nexthop = IPv6(eip->val());
    }
	break;

    default:
	single_write_common(id, e);
	break;
    }
}

template class OspfVarRW<IPv4>;
template class OspfVarRW<IPv6>;
