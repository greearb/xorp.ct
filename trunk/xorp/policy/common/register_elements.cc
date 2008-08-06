// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/common/register_elements.cc,v 1.14 2008/07/23 05:11:27 pavlin Exp $"

#include "libxorp/xorp.h"

#include "register_elements.hh"
#include "element_factory.hh"
#include "element.hh"
#include "elem_set.hh"
#include "elem_null.hh"
#include "elem_bgp.hh"


RegisterElements::RegisterElements()
{
    register_element<ElemInt32>();
    register_element<ElemU32>();
    register_element<ElemU32Range>();
    register_element<ElemStr>();
    register_element<ElemBool>();
    register_element<ElemNull>();
    register_element<ElemIPv4>();
    register_element<ElemIPv4Net>();
    register_element<ElemIPv4Range>();
    register_element<ElemIPv6>();
    register_element<ElemIPv6Range>();
    register_element<ElemIPv6Net>();
    register_element<ElemSetU32>();
    register_element<ElemSetCom32>();
    register_element<ElemSetIPv4Net>();
    register_element<ElemSetIPv6Net>();
    register_element<ElemSetStr>();
    register_element<ElemASPath>();
    register_element<ElemIPv4NextHop>();
    register_element<ElemIPv6NextHop>();
}

// I love templates =D [and C++]
template <class T>
void
RegisterElements::register_element()
{
    static ElementFactory ef;

    struct Local {
	static Element* create(const char* x)
	{
	    return new T(x);
	}
    };

    ef.add(T::id, &Local::create);
}
