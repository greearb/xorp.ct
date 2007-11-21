// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/rip/xrl_redist_manager.hh,v 1.12 2007/11/21 08:29:40 pavlin Exp $

#ifndef __RIP_XRL_REDIST_MANAGER__
#define __RIP_XRL_REDIST_MANAGER__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"
#include "libxorp/service.hh"

#include <list>
#include <string>

#include "redist.hh"

template <typename A> class System;

/**
 * Xrl Route redistribution manager.
 */
template <typename A>
class XrlRedistManager : public ServiceBase {
public:
    typedef A		Addr;
    typedef IPNet<A>	Net;

public:
    XrlRedistManager(System<A>& system);
    ~XrlRedistManager();

    int startup();
    int shutdown();

    void add_route(const Net& net, const Addr& nh, const string& ifname,
		   const string& vifname, uint16_t cost, uint16_t tag,
		   const PolicyTags& policytags);

    void delete_route(const Net& net);

protected:
    RouteRedistributor<A> _rr;
};

#endif // __RIP_XRL_REDIST_MANAGER__
