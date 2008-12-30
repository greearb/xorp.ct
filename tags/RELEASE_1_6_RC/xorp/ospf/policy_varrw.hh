// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/ospf/policy_varrw.hh,v 1.14 2008/08/06 08:24:10 abittau Exp $

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
    };

    OspfVarRW(IPNet<A>& network, A& nexthop, uint32_t& metric, bool& e_bit,
	      uint32_t& tag, bool& tag_set, PolicyTags& policytags);

    // SingleVarRW inteface:
    void	start_read();
    Element*	single_read(const Id& id);
    void	single_write(const Id& id, const Element& e);

 private:
    void start_read_common();
    void single_write_common(const Id& id, const Element& e);

    IPNet<A>&	    _network;
    A&		    _nexthop;
    uint32_t&	    _metric;
    bool&	    _e_bit;
    uint32_t&	    _tag;
    bool&	    _tag_set;
    PolicyTags&	    _policytags;
    ElementFactory  _ef;
};

#endif // __OSPF_POLICY_VARRRW_HH__
