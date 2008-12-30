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

#ident "$XORP: xorp/policy/test_varrw.cc,v 1.2 2008/08/06 08:30:56 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "test_varrw.hh"
#include "common/policy_exception.hh"

const Element& 
TestVarRW::read(const Id& id) {
    ELEM::iterator i = _elem.find(id);

    if (i == _elem.end())
	xorp_throw(PolicyException, "Reading uninitialized attribute");

    const Element* e = i->second;

    return *e;
}

void 
TestVarRW::write(const Id& id, const Element& elem) {
    _elem[id] = &elem;
}
