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

#ident "$XORP: xorp/rib/rt_tab_base.cc,v 1.4 2004/02/11 08:48:47 pavlin Exp $"

#include "rib_module.h"

#include "rt_tab_base.hh"

template <typename A>
RouteTable<A>::~RouteTable()
{
}

template <typename A>
void
RouteTable<A>::set_next_table(RouteTable<A>* next_table)
{
    _next_table = next_table;
}

template class RouteTable<IPv4>;
typedef RouteTable<IPv4> IPv4RouteTable;

template class RouteTable<IPv6>;
typedef RouteTable<IPv6> IPv6RouteTable;

template class RouteRange<IPv4>;
template class RouteRange<IPv6>;
