// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
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
// ===========================================================================

#include "wrapper_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "wrapperpolicy.hh"

WrapperVarRW::WrapperVarRW(IPv4Net& network, IPv4& nexthop, uint32_t& metric,
                           IPv4& originator, IPv4& main_addr, uint32_t vtype,
                           PolicyTags& policytags)
    : _network(network), _nexthop(nexthop), _metric(metric),
      _originator(originator), _main_addr(main_addr), _vtype(vtype),
      _policytags(policytags)
{
}

void
WrapperVarRW::start_read()
{
    initialize(_policytags);

    initialize(VAR_NETWORK, _ef.create(ElemIPv4Net::id,
                                       cstring(_network)));
    initialize(VAR_NEXTHOP, _ef.create(ElemIPv4NextHop::id, cstring(_nexthop)));
    initialize(VAR_METRIC, _ef.create(ElemU32::id,
                                      c_format("%u", _metric).c_str()));
    initialize(VAR_ORIGINATOR, _ef.create(ElemIPv4::id, cstring(_originator)));
    initialize(VAR_MAINADDR, _ef.create(ElemIPv4::id,
                                        cstring(_main_addr)));
    initialize(VAR_VTYPE, _ef.create(ElemU32::id,
                                     c_format("%u", _vtype).c_str()));
}

Element*
WrapperVarRW::single_read(const Id& /*id*/)
{
    XLOG_UNREACHABLE();

    return 0;
}

void
WrapperVarRW::single_write(const Id& id, const Element& e)
{
    switch(id) {
    case VAR_NETWORK: {
        const ElemIPv4Net* eip = dynamic_cast<const ElemIPv4Net*>(&e);
        XLOG_ASSERT(eip != NULL);
        _network = IPv4Net(eip->val());
    }
    break;
    case VAR_NEXTHOP: {
        const ElemIPv4NextHop* eip = dynamic_cast<const ElemIPv4NextHop*>(&e);
        XLOG_ASSERT(eip != NULL);
        _nexthop = IPv4(eip->val());
    }
    break;
    case VAR_METRIC: {
        const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
        _metric = u32.val();
    }
    break;
    case VAR_VTYPE: {
        const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
        _vtype = u32.val();
    }
    break;
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

