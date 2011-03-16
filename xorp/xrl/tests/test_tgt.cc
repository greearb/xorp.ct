// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net





#include "test_tgt.hh"
#include "libxorp/status_codes.h"

const string XrlTestTarget::greetings[] = {
    "Hi", "Howdy Partner", "You again"
};

const int32_t XrlTestTarget::n_greetings = sizeof(XrlTestTarget::greetings) / 
sizeof(XrlTestTarget::greetings[0]);

XrlCmdError
XrlTestTarget::common_0_1_get_target_name(string& name)
{
    name = "test_generated";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::common_0_1_get_version(string& version)
{
    version = "1.0";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::common_0_1_get_status(uint32_t& status, 
				     string& reason)
{
    status = PROC_READY;
    reason = "Test Reason";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::common_0_1_shutdown()
{
    exit(0);
}

XrlCmdError
XrlTestTarget::test_1_0_print_hello_world()
{
    cout << "Hello World" << endl;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::test_1_0_print_hello_world_and_message(const string& msg)
{
    cout << "Hello World, " << msg << endl;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::test_1_0_get_greeting_count(int32_t& num_msgs)
{
    num_msgs = n_greetings;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestTarget::test_1_0_get_greeting(const int32_t& n, 
				     string& greeting)
{
    if (n >=0 && n < n_greetings) {
	greeting = greetings[n];
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::BAD_ARGS();
}

XrlCmdError
XrlTestTarget::test_1_0_shoot_foot()
{
    return XrlCmdError::COMMAND_FAILED("no gun to shoot foot with.");
}
