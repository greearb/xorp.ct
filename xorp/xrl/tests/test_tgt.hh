// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/xrl/tests/test_tgt.hh,v 1.12 2008/10/02 21:58:55 bms Exp $



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
