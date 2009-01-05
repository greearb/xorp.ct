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

// $XORP: xorp/policy/test/filter_manager_fake.hh,v 1.7 2008/10/02 21:58:09 bms Exp $

#ifndef __POLICY_TEST_FILTER_MANAGER_FAKE_HH__
#define __POLICY_TEST_FILTER_MANAGER_FAKE_HH__

#include "policy/filter_manager_base.hh"

class FilterManagerFake : public FilterManagerBase {
public:
    void update_filter(const Code::Target& t);
    void flush_updates(uint32_t msec);
};

#endif // __POLICY_TEST_FILTER_MANAGER_FAKE_HH__
