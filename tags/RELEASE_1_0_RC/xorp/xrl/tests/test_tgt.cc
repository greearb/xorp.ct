// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/xrl/tests/test_tgt.cc,v 1.3 2003/05/07 23:15:24 mjh Exp $"

#include <iostream>

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
