// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fibconfig_table_get.cc,v 1.1 2007/04/26 22:29:50 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fibconfig.hh"
#include "fibconfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//


FibConfigTableGet::FibConfigTableGet(FibConfig& fibconfig)
    : _s4(-1),
      _s6(-1),
      _is_running(false),
      _fibconfig(fibconfig),
      _is_primary(true)
{
    
}

FibConfigTableGet::~FibConfigTableGet()
{
    if (_s4 >= 0) {
	comm_close(_s4);
	_s4 = -1;
    }
    if (_s6 >= 0) {
	comm_close(_s6);
	_s6 = -1;
    }
}

void
FibConfigTableGet::register_fibconfig_primary()
{
    _fibconfig.register_fibconfig_table_get_primary(this);
}

void
FibConfigTableGet::register_fibconfig_secondary()
{
    _fibconfig.register_fibconfig_table_get_secondary(this);
}

int
FibConfigTableGet::sock(int family)
{
    switch (family) {
    case AF_INET:
	return _s4;
#ifdef HAVE_IPV6
    case AF_INET6:
	return _s6;
#endif // HAVE_IPV6
    default:
	XLOG_FATAL("Unknown address family %d", family);
    }
    return (-1);
}
