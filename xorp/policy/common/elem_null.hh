// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/common/elem_null.hh,v 1.10 2008/10/02 21:58:06 bms Exp $

#ifndef __POLICY_COMMON_ELEM_NULL_HH__
#define __POLICY_COMMON_ELEM_NULL_HH__

#include "element_base.hh"


/**
 * @short An element representing nothing. Null.
 *
 * This is used by VarRW when an element being read is not available. For
 * example, if the route is IPv4, but the IPv6 representation is being asked
 * for.
 *
 * The dispatcher also treats Null arguments as a special case by returning a
 * Null.
 */
class ElemNull : public Element {
public:
    static const char* id;
    static Hash _hash;
            
    ElemNull() : Element(_hash) {}
    ElemNull(const char* /* c_str */) : Element(_hash) {}
                
    string str() const { return "null"; }

    const char* type() const { return id; }
};

#endif // __POLICY_COMMON_ELEM_NULL_HH__
