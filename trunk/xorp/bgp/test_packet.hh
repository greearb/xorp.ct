// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/bgp/test_packet.hh,v 1.11 2008/10/02 21:56:22 bms Exp $

#ifndef __BGP_TEST_PACKET_HH__
#define __BGP_TEST_PACKET_HH__

#include "packet.hh"
#include "socket.hh"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"

class BGPTestPacket
{
public:
    BGPTestPacket();
    int run_tests();
protected:
private:
    KeepAlivePacket* create_keepalive();
    bool test_keepalive();
    UpdatePacket* create_update();
    bool test_update();
    UpdatePacket* create_update_ipv6();
    bool test_update_ipv6();
    OpenPacket* create_open();
    bool test_open();
    NotificationPacket* create_notification();
    bool test_notification();
    bool test_aspath();
};

#endif // __BGP_TEST_PACKET_HH__
