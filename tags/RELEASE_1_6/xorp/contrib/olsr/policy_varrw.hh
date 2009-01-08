// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/policy_varrw.hh,v 1.4 2008/10/02 21:56:35 bms Exp $

#ifndef __OLSR_POLICY_VARRRW_HH__
#define __OLSR_POLICY_VARRRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "policy/backend/policy_filters.hh"
#include "policy/backend/policytags.hh"

class OlsrVarRW : public SingleVarRW {
 public:
    // Some of the following should be treated read-only when
    // importing routes into OLSR.
    enum {
	VAR_NETWORK = VAR_PROTOCOL,	// Destination (RO)
	VAR_NEXTHOP,			// Next-hop (RW)
	VAR_METRIC,			// Metric (RW)

	VAR_VTYPE,	// Which part of OLSR this route came from. (RO)
	VAR_ORIGINATOR,	// OLSR main address of the advertising node. (RO)
	VAR_MAINADDR,	// OLSR main address of the destination. (RO)

	VAR_OLSRMAX	// must be last.
    };

    OlsrVarRW(IPv4Net& network, IPv4& nexthop, uint32_t& metric,
	      IPv4& originator, IPv4& main_addr, uint32_t vtype,
	      PolicyTags& policytags);

    // SingleVarRW inteface:
    void start_read();
    Element* single_read(const Id& id);
    void single_write(const Id& id, const Element& e);

 private:
    IPv4Net&	_network;
    IPv4&	_nexthop;
    uint32_t&	_metric;

    IPv4&	_originator;
    IPv4&	_main_addr;
    uint32_t&	_vtype;	    // actually OlsrTypes::VertexType.

    PolicyTags&	_policytags;

    ElementFactory _ef;
};

#endif // __OLSR_POLICY_VARRRW_HH__
