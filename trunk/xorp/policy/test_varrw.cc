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

#ident "$XORP: xorp/policy/test_varrw.cc,v 1.1 2008/08/06 08:27:11 abittau Exp $"

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
