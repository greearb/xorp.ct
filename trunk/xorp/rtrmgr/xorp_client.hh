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

// $XORP: xorp/rtrmgr/xorp_client.hh,v 1.9 2003/04/24 23:43:48 mjh Exp $


#ifndef __RTRMGR_XORP_CLIENT_HH__
#define __RTRMGR_XORP_CLIENT_HH__

#include <list>
#include <string>
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxipc/xrl_router.hh"

//typedef XrlRouter::XrlCallback XCCommandCallback;

class EventLoop;
class XrlRouter;
class UnexpandedXrl;

class XorpClient  {
public:
    XorpClient(EventLoop& eventloop, XrlRouter& xrlrouter);
    ~XorpClient() {};
    int send_xrl(const UnexpandedXrl &xrl, 
		 XrlRouter::XrlCallback cb,
		 bool do_exec);
    int send_now(const Xrl &xrl, XrlRouter::XrlCallback cb, 
		 const string& expected_response, bool do_exec);
    XrlArgs fake_return_args(const string& xrl_return_spec);
    EventLoop& eventloop() const { return _eventloop; }
private:
    EventLoop& _eventloop;
    XrlRouter& _xrlrouter;
};

#endif // __RTRMGR_XORP_CLIENT_HH__
