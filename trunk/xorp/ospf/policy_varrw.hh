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

// $XORP: xorp/ospf/policy_varrw.hh,v 1.1 2005/10/17 22:53:37 atanu Exp $

#ifndef __OSPF_POLICY_VARRRW_HH__
#define __OSPF_POLICY_VARRRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"

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

    OspfVarRW(Lsa::LsaRef lsar, const PolicyTags& policytags);

    void null();

    // SingleVarRW inteface:
    void start_read();
    Element* single_read(const Id& id);
    void single_write(const Id& id, const Element& e);

 private:
    Lsa::LsaRef _lsar;

    // Take a copy of the policy tags, allowing us to reset them.
    PolicyTags _policytags;
//     const PolicyTags& _policytags;

    ElementFactory _ef;

//     map<Id, XorpCallback1<Element*, Id&>::RefPtr> _read_map;

#if	0
    Element* read_policytags();
    Element* read_network();
    Element* read_metric();
    Element* read_e_bit();
    Element* read_tag();
#endif
};

#endif // __OSPF_POLICY_VARRRW_HH__
