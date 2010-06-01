// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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


#ifndef __LIBXIPC_TEST_RECEIVER_HH__
#define __LIBXIPC_TEST_RECEIVER_HH__

class TestReceiver : public XrlTestXrlsTargetBase {
public:
    TestReceiver(EventLoop& eventloop, XrlRouter* xrl_router,
		 FILE* output);
    ~TestReceiver();

    bool done() const;
    void print_xrl_received() const;
    void enable_sampler();

private:
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    XrlCmdError common_0_1_shutdown();
    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError test_xrls_0_1_start_transmission();

    XrlCmdError test_xrls_0_1_end_transmission();

    XrlCmdError test_xrls_0_1_add_xrl0();

    XrlCmdError test_xrls_0_1_add_xrl1(
	// Input values,
	const string&	data1);

    XrlCmdError test_xrls_0_1_add_xrl2(
	// Input values,
	const string&	data1,
	const string&	data2);

    XrlCmdError test_xrls_0_1_add_xrl9(
	// Input values,
	const bool&	data1,
	const int32_t&	data2,
	const IPv4&	data3,
	const IPv4Net&	data4,
	const IPv6&	data5,
	const IPv6Net&	data6,
	const Mac&	data7,
	const string&	data8,
	const vector<uint8_t>& data9);

    XrlCmdError test_xrls_0_1_add_xrlx(const XrlAtomList &);

    void print_statistics();

    EventLoop&	_eventloop;
    FILE*	_output;
    TimeVal	_start_time;
    TimeVal	_end_time;
    size_t	_received_xrls;
    XorpTimer	_exit_timer;
    bool	_done;
    bool	_sample;
};

#endif // __LIBXIPC_TEST_RECEIVER_HH__
