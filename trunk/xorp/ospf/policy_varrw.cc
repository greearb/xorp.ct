// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/policy_varrw.cc,v 1.13 2007/05/23 04:08:28 pavlin Exp $"

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

template <typename A>
void
OspfVarRW<A>::null()
{
    initialize(VAR_NETWORK, NULL);
    initialize(VAR_NEXTHOP, NULL);
    initialize(VAR_METRIC, NULL);
    initialize(VAR_EBIT, NULL);
    initialize(VAR_TAG, NULL);
}

template <>
void
OspfVarRW<IPv4>::start_read()
{
    initialize(VAR_POLICYTAGS, _policytags.element());
    initialize(VAR_NETWORK, _ef.create(ElemIPv4Net::id,
				       _network.str().c_str()));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv4::id, _nexthop.str().c_str()));
    initialize(VAR_METRIC, _ef.create(ElemU32::id,
				      c_format("%u", _metric).c_str()));
    initialize(VAR_EBIT, _ef.create(ElemBool::id, bool_c_str(_e_bit)));
    initialize(VAR_TAG, _ef.create(ElemU32::id,
				   c_format("%u", _tag).c_str()));
}

template <>
void
OspfVarRW<IPv6>::start_read()
{
    initialize(VAR_POLICYTAGS, _policytags.element());
    initialize(VAR_NETWORK, _ef.create(ElemIPv6Net::id,
				       _network.str().c_str()));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv6::id, _nexthop.str().c_str()));
    initialize(VAR_METRIC, _ef.create(ElemU32::id,
				      c_format("%u", _metric).c_str()));
    initialize(VAR_EBIT, _ef.create(ElemBool::id, bool_c_str(_e_bit)));
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
    switch(id) {
    case VAR_POLICYTAGS:
	_policytags = e;
	break;
    case VAR_NETWORK: {
	const ElemIPv4Net* eip = dynamic_cast<const ElemIPv4Net*>(&e);
	XLOG_ASSERT(eip != NULL);
	_network = IPNet<IPv4>(eip->val());
    }
	break;
    case VAR_NEXTHOP: {
	const ElemIPv4* eip = dynamic_cast<const ElemIPv4*>(&e);
	XLOG_ASSERT(eip != NULL);
	_nexthop = IPv4(eip->val());
    }
	break;
    case VAR_METRIC: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_metric = u32.val();
    }
	break;
    case VAR_EBIT: {
	const ElemBool& b = dynamic_cast<const ElemBool&>(e);
	_e_bit = b.val();
    }
	break;
    case VAR_TAG: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_tag = u32.val();
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
    switch(id) {
    case VAR_POLICYTAGS:
	_policytags = e;
	break;
    case VAR_NETWORK: {
	const ElemIPv6Net* eip = dynamic_cast<const ElemIPv6Net*>(&e);
	XLOG_ASSERT(eip != NULL);
	_network = IPNet<IPv6>(eip->val());
    }
	break;
    case VAR_NEXTHOP: {
	const ElemIPv6* eip = dynamic_cast<const ElemIPv6*>(&e);
	XLOG_ASSERT(eip != NULL);
	_nexthop = IPv6(eip->val());
    }
	break;
    case VAR_METRIC: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_metric = u32.val();
    }
	break;
    case VAR_EBIT: {
	const ElemBool& b = dynamic_cast<const ElemBool&>(e);
	_e_bit = b.val();
    }
	break;
    case VAR_TAG: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_tag = u32.val();
	_tag_set = true;
    }
	break;
    default:
	XLOG_WARNING("Unexpected Id %d %s", id, cstring(e));
    }
}

template class OspfVarRW<IPv4>;
template class OspfVarRW<IPv6>;
