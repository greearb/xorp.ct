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

// $XORP: xorp/contrib/olsr/policy_varrw.hh,v 1.1 2008/04/24 15:19:54 bms Exp $

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

    void null();

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
