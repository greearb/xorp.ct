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

// $XORP: xorp/xrl/tests/test_tgt.hh,v 1.4 2003/05/07 23:15:24 mjh Exp $

#include <string>

#include "libxipc/xrl_router.hh"
#include "xrl/targets/test_base.hh"

class XrlTestTarget: public XrlTestTargetBase {
public:
    XrlTestTarget(XrlRouter* r) : XrlTestTargetBase(r) {}

protected:
    // Methods to be implemented by derived classes supporting this interface.
    virtual XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    virtual XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    virtual XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    virtual XrlCmdError common_0_1_shutdown();

    virtual XrlCmdError test_1_0_print_hello_world();

    virtual XrlCmdError test_1_0_print_hello_world_and_message(
	// Input values, 
	const string&	msg);

    virtual XrlCmdError test_1_0_get_greeting_count(
	// Output values, 
	int32_t&	num_msgs);

    virtual XrlCmdError test_1_0_get_greeting(
	// Input Values,
	const int32_t&	greeting_no,
	// Output values, 
	string&		greeting);

    virtual XrlCmdError test_1_0_shoot_foot();

private:
    static const string	 greetings[];
    static const int32_t n_greetings;
};
