// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/test_next_hop_resolver.hh,v 1.9 2008/07/23 05:09:39 pavlin Exp $

#ifndef __BGP_TEST_NEXT_HOP_RESOLVER_HH__
#define __BGP_TEST_NEXT_HOP_RESOLVER_HH__

#include "libxorp/test_main.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

template <class A>
bool
nhr_test1(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test2(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg);

template <class A>
bool
nhr_test3(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg);

template <class A>
bool
nhr_test4(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test5(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test6(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test7(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test8(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet);

template <class A>
bool
nhr_test9(TestInfo& info, A nexthop, A real_nexthop, IPNet<A> subnet, int reg);

#endif // __BGP_TEST_NEXT_HOP_RESOLVER_HH__
