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

// $XORP: xorp/rip/xrl_redist_manager.hh,v 1.9 2007/02/16 22:47:18 pavlin Exp $

#ifndef __RIP_XRL_REDIST_MANAGER__
#define __RIP_XRL_REDIST_MANAGER__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"
#include "libxorp/service.hh"

#include <list>
#include <string>

#include "redist.hh"


class EventLoop;
class XrlRouter;
class XrlError;

template <typename A> class RouteDB;
template <typename A> class System;
template <typename A> class RedistJob;

/**
 * Xrl Redistribution class.  This class requests the RIB to start and stop
 * route redistribution
 */
template <typename A>
class XrlRedistManager : public ServiceBase {
public:
    typedef A		Addr;
    typedef IPNet<A>	Net;
    typedef list<RouteRedistributor<A>*> RedistList;

public:
    XrlRedistManager(EventLoop& e, RouteDB<A>& rdb, XrlRouter& router);
    XrlRedistManager(System<A>& system, XrlRouter& router);
    ~XrlRedistManager();

    bool startup();
    bool shutdown();

    void request_redist_for(const string& protocol,
			    uint16_t	  cost,
			    uint16_t	  tag);

    void request_no_redist_for(const string& protocol);

    void add_route(const string& protocol, const Net& net, const Addr& nh,
		   const PolicyTags& policytags);
    
    void add_route(const string& protocol, const Net& net, const Addr& nh,
		   uint16_t cost, uint16_t tag, const PolicyTags& policytags);

    void delete_route(const string& protocol, const Net& net);

    EventLoop&	eventloop()			{ return _e; }
    RouteDB<A>&	route_db()			{ return _rdb; }
    XrlRouter&	xrl_router()			{ return _xr; }

    void job_completed(const RedistJob<A>* job);

protected:
    void run_next_job();

protected:
    EventLoop&		_e;
    RouteDB<A>&		_rdb;
    XrlRouter&		_xr;

    RedistList		_redists;
    RedistList		_dead_redists;
    list<RedistJob<A>*>	_jobs;
};

#endif // __RIP_XRL_REDIST_MANAGER__
