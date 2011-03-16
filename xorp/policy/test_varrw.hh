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

// $XORP: xorp/policy/test_varrw.hh,v 1.3 2008/10/02 21:58:01 bms Exp $

#ifndef __POLICY_TEST_VARRW_HH__
#define __POLICY_TEST_VARRW_HH__



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
