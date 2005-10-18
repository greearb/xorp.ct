// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/policy_varrw.cc,v 1.1 2005/10/17 22:53:37 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "policy_varrw.hh"

template <typename A>
OspfVarRW<A>::OspfVarRW(Lsa::LsaRef lsar, const PolicyTags& policytags)
    : _lsar(lsar), _policytags(policytags)
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
    ASExternalLsa *aselsa;

    if (0 == (aselsa = dynamic_cast<ASExternalLsa *>(_lsar.get()))) {
	null();
	return;
    }

    IPv4 mask = IPv4(htonl(aselsa->get_network_mask()));
    uint32_t lsid = aselsa->get_header().get_link_state_id();
    IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());

    initialize(VAR_POLICYTAGS, _policytags.element());
    initialize(VAR_NETWORK, _ef.create(ElemIPv4Net::id, n.str().c_str()));
    initialize(VAR_NEXTHOP,
	       _ef.create(ElemIPv4::id,
			  aselsa->
			  get_forwarding_address_ipv4().str().c_str()));
    initialize(VAR_METRIC,
	       _ef.create(ElemInt32::id,
			  c_format("%d", aselsa->get_metric()).c_str()));
    initialize(VAR_EBIT,
	       _ef.create(ElemBool::id, pb(aselsa->get_e_bit())));
    initialize(VAR_TAG,
	       _ef.create(ElemInt32::id,
			  c_format("%d",
				   aselsa->get_external_route_tag()).c_str()));
}

template <>
void
OspfVarRW<IPv6>::start_read()
{
    XLOG_WARNING("TBD - read policy vars");

    null();
}

template <typename A>
Element* 
OspfVarRW<A>::single_read(const Id& /*id*/)
{
    return 0;
}

template <>
void
OspfVarRW<IPv4>::single_write(const Id& id, const Element& /*e*/)
{
    switch(id) {
    case VAR_POLICYTAGS:
	break;
    case VAR_NETWORK:
	break;
    case VAR_NEXTHOP:
	break;
    case VAR_METRIC:
	break;
    case VAR_EBIT:
	break;
    case VAR_TAG:
	break;
    }
}

template <>
void
OspfVarRW<IPv6>::single_write(const Id& /*id*/, const Element& /*e*/)
{
    XLOG_WARNING("TBD - write policy vars");
}

#if	0
template <typename A>
Element* 
OspfVarRW<A>::read_policytags()
{
    return 0;
}

template <>
Element* 
OspfVarRW<IPv4>::read_network()
{
    return 0;
}

template <>
Element* 
OspfVarRW<IPv6>::read_network()
{
    return 0;
}

template <typename A>
Element* 
OspfVarRW<A>::read_metric()
{
    return 0;
}

template <typename A>
Element* 
OspfVarRW<A>::read_tag()
{
    return 0;
}
#endif

template class OspfVarRW<IPv4>;
template class OspfVarRW<IPv6>;


