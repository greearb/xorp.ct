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

// $XORP: xorp/bgp/test_next_hop_resolver.hh,v 1.1 2003/07/03 00:24:43 atanu Exp $

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


#endif // __BGP_TEST_NEXT_HOP_RESOLVER_HH__
