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

#ident "$XORP: xorp/bgp/tools/print_routes.cc,v 1.8 2004/05/15 18:31:39 atanu Exp $"


#include "print_routes.hh"

template <>
void
PrintRoutes<IPv4>::get_route_list_start(bool unicast, bool multicast)
{
    _active_requests = 0;
    send_get_v4_route_list_start("bgp", unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv6>::get_route_list_start(bool unicast, bool multicast)
{
    _active_requests = 0;
    send_get_v6_route_list_start("bgp", unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv4>::get_route_list_next()
{
    send_get_v4_route_list_next("bgp",	_token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

template <>
void
PrintRoutes<IPv6>::get_route_list_next()
{
    send_get_v6_route_list_next("bgp", _token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

