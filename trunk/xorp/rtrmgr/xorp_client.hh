// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/xorp_client.hh,v 1.14 2004/01/15 08:51:59 pavlin Exp $


#ifndef __RTRMGR_XORP_CLIENT_HH__
#define __RTRMGR_XORP_CLIENT_HH__


#include "libxipc/xrl_router.hh"


class EventLoop;
class UnexpandedXrl;
class XrlRouter;

class XorpClient  {
public:
    XorpClient(EventLoop& eventloop, XrlRouter& xrlrouter);
    ~XorpClient() {};

#if 0
    int send_xrl(const UnexpandedXrl& xrl, string& errmsg,
		 XrlRouter::XrlCallback cb, bool do_exec);
#endif
    void send_now(const Xrl& xrl, XrlRouter::XrlCallback cb, 
		 const string& expected_response, bool do_exec);
    void fake_send_done(string xrl_return_spec, XrlRouter::XrlCallback cb);
    XrlArgs fake_return_args(const string& xrl_return_spec);
    EventLoop& eventloop() const { return _eventloop; }

private:
    EventLoop&	_eventloop;
    XrlRouter&	_xrlrouter;
    XorpTimer	_delay_timer;
};

#endif // __RTRMGR_XORP_CLIENT_HH__
