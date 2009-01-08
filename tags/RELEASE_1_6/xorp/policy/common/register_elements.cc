// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/common/register_elements.cc,v 1.16 2008/10/02 21:58:07 bms Exp $"

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
