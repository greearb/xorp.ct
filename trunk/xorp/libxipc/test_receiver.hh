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

// $XORP: xorp/libxipc/test_receiver.hh,v 1.5 2008/09/23 19:55:37 abittau Exp $

#ifndef __LIBXIPC_TEST_RECEIVER_HH__
#define __LIBXIPC_TEST_RECEIVER_HH__

class TestReceiver : public XrlTestXrlsTargetBase {
public:
    TestReceiver(EventLoop& eventloop, XrlRouter* xrl_router);
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
    TimeVal	_start_time;
    TimeVal	_end_time;
    size_t	_received_xrls;
    XorpTimer	_exit_timer;
    bool	_done;
    bool	_sample;
};

#endif // __LIBXIPC_TEST_RECEIVER_HH__
