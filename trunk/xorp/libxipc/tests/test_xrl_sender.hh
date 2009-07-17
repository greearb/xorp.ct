// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxipc/test_xrl_sender.hh,v 1.9 2008/10/02 21:57:23 bms Exp $

#ifndef __LIBXIPC_TEST_XRL_SENDER_HH__
#define __LIBXIPC_TEST_XRL_SENDER_HH__

class TestSender {
public:
    TestSender(EventLoop& eventloop, XrlRouter* xrl_router,
	       size_t max_xrl_id);

    ~TestSender();

    bool done() const;
    void print_xrl_sent() const;
    void print_xrl_received() const;
    void clear();
    void start_transmission();
    void start_transmission_process();
    void send_batch();

private:
    typedef void (TestSender::*CB)(const XrlError& err);

    bool transmit_xrl_next(CB cb);
    void start_transmission_cb(const XrlError& xrl_error);
    void send_next();
    void send_next_cb(const XrlError& xrl_error);
    void send_next_start_pipeline();
    void send_next_pipeline();
    void send_next_pipeline_cb(const XrlError& xrl_error);
    void send_single();
    void send_single_cb(const XrlError& err);
    void end_transmission();
    void end_transmission_cb(const XrlError& xrl_error);
    void send_batch_do(unsigned xrls);
    void send_batch_cb(const XrlError& xrl_error);
    bool send_batch_task();

    EventLoop&			_eventloop;
    XrlRouter&			_xrl_router;
    XrlTestXrlsV0p1Client	_test_xrls_client;
    string			_receiver_target;
    XorpTimer			_start_transmission_timer;
    XorpTimer			_end_transmission_timer;
    XorpTimer			_try_again_timer;
    XorpTimer			_exit_timer;
    size_t			_max_xrl_id;
    size_t			_next_xrl_send_id;
    size_t			_next_xrl_recv_id;
    bool			_sent_end_transmission;
    bool			_done;

    // Data to send
    bool			_my_bool;
    int32_t			_my_int;
    IPv4			_my_ipv4;
    IPv4Net			_my_ipv4net;
    IPv6			_my_ipv6;
    IPv6Net			_my_ipv6net;
    Mac				_my_mac;
    string			_my_string;
    vector<uint8_t>		_my_vector;

    // batch state
    XorpTask			_batch_task;
    int				_batch_sent;
    int				_batch_remaining;
    int				_batch_per_run;
    int				_batch_size;
    unsigned			_batch_got;
    unsigned			_batch_errors;
};

#endif // __LIBXIPC_TEST_XRL_SENDER_HH__
