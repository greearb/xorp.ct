// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others-2009 XORP, Inc.
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


#ifndef __RTRMGR_XORP_CLIENT_HH__
#define __RTRMGR_XORP_CLIENT_HH__


#include "libxipc/xrl_router.hh"


class EventLoop;
class UnexpandedXrl;
class XrlRouter;

class XorpClient  {
public:
    XorpClient(EventLoop& eventloop, XrlRouter& xrl_router);
    ~XorpClient();

    void send_now(const Xrl& xrl, XrlRouter::XrlCallback cb, 
		 const string& expected_response, bool do_exec);
    void fake_send_done(string xrl_return_spec, XrlRouter::XrlCallback cb);
    XrlArgs fake_return_args(const string& xrl_return_spec);
    EventLoop& eventloop() const { return _eventloop; }

private:
    EventLoop&	_eventloop;
    XrlRouter&	_xrl_router;
    XorpTimer	_delay_timer;
};

#endif // __RTRMGR_XORP_CLIENT_HH__
