// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rib/fea_client.hh,v 1.1.1.1 2002/12/11 23:56:13 hodson Exp $

#ifndef __RIB_FEA_CLIENT_HH__
#define __RIB_FEA_CLIENT_HH__

#include <list>
#include "libxorp/ref_ptr.hh"
#include "route.hh"

class XrlRouter;

class SyncFtiCommand;
typedef ref_ptr<SyncFtiCommand> FeaClientTask;


/**
 * @short FeaClient handles communication of routes to the FEA.
 *
 * FeaClient communicates add route and delete requests from the RIBs
 * to the FEA process.
 */
class FeaClient
{
public:
    /**
     * FeaClient constructor.
     *
     * @param xrl_router XRL router instance to use for communication
     * with the FEA
     * @param max_ops the maximum number of operations in a transaction.
     */
    FeaClient(XrlRouter& xrl_router, uint32_t	max_ops = 100);

    /**
     * FeaClient destructor
     */
    ~FeaClient();

    /**
     * Communicate the addition of a new IPv4 route to the FEA.
     *
     * @param dest the destination subnet of the route.
     * @param gw the nexthop gateway to be used to forward packets
     * towards the dest network.
     * @param ifname the name of the interface to be used to forward
     * packets towards the dest network.
     * @param vifname the name of the virtual interface to be used to
     * forward packets towards the dest network.
     */
    void add_route(const IPv4Net& dest,
		   const IPv4& gw,
		   const string& ifname,
		   const string& vifname);

    /**
     * Communicate the deletion of an IPv4 route to the FEA.
     *
     * @param dest the destination subnet of the route.
     */
    void delete_route(const IPv4Net&);

    void add_route(const IPv6Net& dest,
		   const IPv6& gw,
		   const string& ifname,
		   const string& vifname);

    /**
     * Communicate the addition of a new IPv6 route to the FEA.
     *
     * @param dest the destination subnet of the route.
     * @param gw the nexthop gateway to be used to forward packets
     * towards the dest network.
     * @param ifname the name of the interface to be used to forward
     * packets towards the dest network.
     * @param vifname the name of the virtual interface to be used to
     * forward packets towards the dest network.
     */

    /**
     * Communicate the deletion of an IPv6 route to the FEA.
     *
     * @param dest the destination subnet of the route.
     */
    void delete_route(const IPv6Net& re);


    // The methods below are compatibility methods used by the
    // ExportTable - they are implemented in terms of the methods
    // above.

    /**
     * Communicate the addition of a new IPv4 route to the FEA.
     *
     * @param re the routing table entry of the new route.
     */
    void add_route(const IPv4RouteEntry& re);

    /**
     * Communicate the deletion of a new IPv4 route to the FEA.
     *
     * @param re the routing table entry of the route to be deleted.
     */
    void delete_route(const IPv4RouteEntry& re);

    /**
     * Communicate the addition of a new IPv6 route to the FEA.
     *
     * @param re the routing table entry of the new route.
     */
    void add_route(const IPv6RouteEntry& re);

    /**
     * Communicate the deletion of a new IPv6 route to the FEA.
     *
     * @param re the routing table entry of the route to be deleted.
     */
    void delete_route(const IPv6RouteEntry& re);

    /**
     * @returns the number of route adds and deletes that are
     * currently queued for communication with the FEA.  
     */
    size_t tasks_count() const;

    /**
     * @returns true if there are currently any adds or deletes queued
     * for transmission to the FEA or awaiting acknowledgement from
     * the FEA
     */
    bool tasks_pending() const;

protected:
    /**
     * @return The next task or 0 if there isn't one.
     */
    SyncFtiCommand *get_next();
    /**
     * Called when a transaction has completed.
     */
    void transaction_completed();
    /**
     * Called to start a transaction.
     */
    void start();

    XrlRouter& _xrl_router;
    bool _busy;			// Transaction in progress.
    list<FeaClientTask> _tasks;	// List of current task.
    list<FeaClientTask> _completed_tasks;	// Completed tasks are
						// stored here. For
						// later deletion.
    const uint32_t _max_ops;	// Maximum allowed tasks in a transaction.
    uint32_t _op_count;		// Number of tasks in this transaction.
};

#endif // __RIB_FEA_CLIENT_HH__
