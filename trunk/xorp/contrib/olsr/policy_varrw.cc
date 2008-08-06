// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/policy_varrw.cc,v 1.2 2008/07/23 05:09:52 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "olsr.hh"
#include "policy_varrw.hh"

OlsrVarRW::OlsrVarRW(IPv4Net& network, IPv4& nexthop, uint32_t& metric,
		     IPv4& originator, IPv4& main_addr, uint32_t vtype,
		     PolicyTags& policytags)
    : _network(network), _nexthop(nexthop), _metric(metric),
      _originator(originator), _main_addr(main_addr), _vtype(vtype),
      _policytags(policytags)
{
}

void
OlsrVarRW::start_read()
{
    initialize(_policytags);

    initialize(VAR_NETWORK, _ef.create(ElemIPv4Net::id,
				       cstring(_network)));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv4::id, cstring(_nexthop)));
    initialize(VAR_METRIC, _ef.create(ElemU32::id,
				      c_format("%u", _metric).c_str()));
    initialize(VAR_ORIGINATOR, _ef.create(ElemIPv4::id, cstring(_originator)));
    initialize(VAR_MAINADDR, _ef.create(ElemIPv4::id,
					cstring(_main_addr)));
    initialize(VAR_VTYPE, _ef.create(ElemU32::id,
				    c_format("%u", _vtype).c_str()));
}

Element* 
OlsrVarRW::single_read(const Id& /*id*/)
{
    XLOG_UNREACHABLE();

    return 0;
}

void
OlsrVarRW::single_write(const Id& id, const Element& e)
{
    switch(id) {
    case VAR_NETWORK: {
	const ElemIPv4Net* eip = dynamic_cast<const ElemIPv4Net*>(&e);
	XLOG_ASSERT(eip != NULL);
	_network = IPv4Net(eip->val());
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
#if 1
    case VAR_VTYPE: {
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_vtype = u32.val();
    }
	break;
#endif
    case VAR_ORIGINATOR: {
	const ElemIPv4* eip = dynamic_cast<const ElemIPv4*>(&e);
	XLOG_ASSERT(eip != NULL);
	_originator = IPv4(eip->val());
    }
	break;
    case VAR_MAINADDR: {
	const ElemIPv4* eip = dynamic_cast<const ElemIPv4*>(&e);
	XLOG_ASSERT(eip != NULL);
	_main_addr = IPv4(eip->val());
    }
	break;
    default:
	XLOG_WARNING("Unexpected Id %d %s", id, cstring(e));
    }
}
