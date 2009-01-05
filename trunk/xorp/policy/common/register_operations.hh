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

// $XORP: xorp/policy/common/register_operations.hh,v 1.8 2008/10/02 21:58:07 bms Exp $

#ifndef __POLICY_COMMON_REGISTER_OPERATIONS_HH__
#define __POLICY_COMMON_REGISTER_OPERATIONS_HH__

#include "element.hh"

/**
 * @short Do initial registration of dispatcher callbacks.
 *
 * The sole purpose of this class is to register the callbacks before the
 * dispatcher is actually used, and register them only once at startup.
 */
class RegisterOperations {
public:
    /**
     * Constructor which performs registrations
     *
     * In essence, this is where the grammar lives.
     */
    RegisterOperations();
};

namespace operations {

/**
 * Maybe be used to construct elements.  Also for casting! 
 *
 * @param type the string representation of typename.
 * @param arg the string representation of value.
 * @return element of wanted type representing arg.
 */
Element* ctr(const ElemStr& type, const Element& arg);

}

#endif // __POLICY_COMMON_REGISTER_OPERATIONS_HH__
