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

#ifndef __WRAPPER_POLICY_VARRRW_HH__
#define __WRAPPER_POLICY_VARRRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "policy/backend/policy_filters.hh"
#include "policy/backend/policytags.hh"

class WrapperVarRW : public SingleVarRW {
public:
    // Some of the following should be treated read-only when
    // importing routes into WRAPPER.
    enum {
        VAR_NETWORK = VAR_PROTOCOL,     // Destination (RO)
        VAR_NEXTHOP,                    // Next-hop (RW)
        VAR_METRIC,                     // Metric (RW)

        VAR_VTYPE,      // Which part of WRAPPER this route came from. (RO)
        VAR_ORIGINATOR, // main address of the advertising node. (RO)
        VAR_MAINADDR,   // main address of the destination. (RO)

        VAR_WRAPPERMAX     // must be last.
    };

    WrapperVarRW(IPv4Net& network, IPv4& nexthop, uint32_t& metric,
                 IPv4& originator, IPv4& main_addr, uint32_t vtype,
                 PolicyTags& policytags);

    // SingleVarRW inteface:
    void start_read();
    Element* single_read(const Id& id);
    void single_write(const Id& id, const Element& e);

private:
    IPv4Net&    _network;
    IPv4&       _nexthop;
    uint32_t&   _metric;

    IPv4&       _originator;
    IPv4&       _main_addr;
    uint32_t&   _vtype;

    PolicyTags& _policytags;

    ElementFactory _ef;
};

#endif // __WRAPPER_POLICY_VARRRW_HH__

