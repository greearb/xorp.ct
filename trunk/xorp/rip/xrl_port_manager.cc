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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#define DEBUG_LOGGING

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/debug.h"

#include "xrl_port_manager.hh"

template <typename A>
void
XrlPortManager<A>::tree_complete()
{
    debug_msg("XrlPortManager<IPv%d>::tree_complete notification\n",
	      A::ip_version());
}

template <typename A>
void
XrlPortManager<A>::updates_made()
{
    debug_msg("XrlPortManager<IPv%d>::updates_made notification\n",
	      A::ip_version());
}

template class XrlPortManager<IPv4>;
template class XrlPortManager<IPv6>;
