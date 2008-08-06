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

// $XORP: xorp/policy/test_varrw.hh,v 1.1 2008/08/06 08:27:11 abittau Exp $

#ifndef __POLICY_TEST_VARRW_HH__
#define __POLICY_TEST_VARRW_HH__

#include <map>

#include "policy/common/varrw.hh"

class TestVarRW : public VarRW {
public:
    const Element& read(const Id& id);
    void  write(const Id& id, const Element& elem);

private:
    typedef map<Id, const Element*> ELEM;

    ELEM    _elem;
};

#endif // __POLICY_TEST_VARRW_HH__
