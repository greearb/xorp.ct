// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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


#ifndef __RTRMGR_TEST_SAMPLE_CONFIG_HH__
#define __RTRMGR_TEST_SAMPLE_CONFIG_HH__

#include "generic_module_manager.hh"

class Rtrmgr {
public:
    Rtrmgr();
    int run();
    void module_status_changed(const string& module_name,
			       GenericModule::ModuleStatus status);
};

#endif // __RTRMGR_TEST_SAMPLE_CONFIG_HH__
