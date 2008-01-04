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

// $XORP: xorp/ospf/policy_varrw.hh,v 1.10 2007/02/23 21:08:08 atanu Exp $

#ifndef __OSPF_POLICY_VARRRW_HH__
#define __OSPF_POLICY_VARRRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "policy/backend/policy_filters.hh"
#include "policy/backend/policytags.hh"

template <typename A>
class OspfVarRW : public SingleVarRW {
 public:
    enum {
	VAR_NETWORK = VAR_PROTOCOL,
	VAR_NEXTHOP,
	VAR_METRIC,
	VAR_EBIT,
	VAR_TAG
    };

    OspfVarRW(IPNet<A>& network, A& nexthop, uint32_t& metric, bool& e_bit,
	      uint32_t& tag, bool& tag_set, PolicyTags& policytags);

    void null();

    // SingleVarRW inteface:
    void start_read();
    Element* single_read(const Id& id);
    void single_write(const Id& id, const Element& e);

 private:
    IPNet<A>& _network;
    A& _nexthop;
    uint32_t& _metric;
    bool& _e_bit;
    uint32_t& _tag;
    bool& _tag_set;

    PolicyTags& _policytags;

    ElementFactory _ef;
};

#endif // __OSPF_POLICY_VARRRW_HH__
