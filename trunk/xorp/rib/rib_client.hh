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

// $XORP: xorp/rib/rib_client.hh,v 1.4 2003/05/08 05:51:27 mjh Exp $

#ifndef __RIB_RIB_CLIENT_HH__
#define __RIB_RIB_CLIENT_HH__

#include <list>

#include "libxorp/ref_ptr.hh"

#include "route.hh"


class XrlRouter;
class SyncFtiCommand;
typedef ref_ptr<SyncFtiCommand> RibClientTask;

/**
 * @short RibClient handles communication of routes to a RIB client (e.g.,
 * the FEA).
 *
 * RibClient communicates add route and delete requests from the RIBs
 * to a RIB client (e.g., the FEA).
 */
class RibClient {
public:
    /**
     * RibClient constructor.
     *
     * @param xrl_router XRL router instance to use for communication
     * with the RIB client.
     * @param target_name the XRL target name to send the route changes to.
     * @param max_ops the maximum number of operations in a transaction.
     */
    RibClient(XrlRouter& xrl_router, const string& target_name,
	      size_t max_ops = 100);

    /**
     * RibClient destructor
     */
    ~RibClient();

    /**
     * Get the target name.
     * 
     * @return the target name.
     */
    const string& target_name() const { return _target_name; }

    /**
     * Set enabled state.
     *
     * When enabled RibClient attempts to send commands to the RIB client.
     * When disabled it silently ignores the requests.
     */
    void set_enabled(bool en) { _enabled = en; }

    /**
     * Get enabled state.
     */
    bool enabled() const { return (_enabled); }
    
    /**
     * Communicate the addition of a new IPv4 route to the RIB client.
     *
     * @param dest the destination subnet of the route.
     * @param gw the nexthop gateway to be used to forward packets
     * towards the dest network.
     * @param ifname the name of the interface to be used to forward
     * packets towards the dest network.
     * @param vifname the name of the virtual interface to be used to
     * forward packets towards the dest network.
     * @param metric the routing metric toward dest.
     * @param admin_distance the administratively defined distance
     * toward dest.
     * @param protocol_origin the name of the protocol that originated
     * this entry.
     */
    void add_route(const IPv4Net& dest,
		   const IPv4& gw,
		   const string& ifname,
		   const string& vifname,
		   uint32_t metric,
		   uint32_t admin_distance,
		   const string& protocol_origin);

    /**
     * Communicate the deletion of an IPv4 route to the RIB client.
     *
     * @param dest the destination subnet of the route.
     */
    void delete_route(const IPv4Net& dest);

    /**
     * Communicate the addition of a new IPv6 route to the RIB client.
     *
     * @param dest the destination subnet of the route.
     * @param gw the nexthop gateway to be used to forward packets
     * towards the dest network.
     * @param ifname the name of the interface to be used to forward
     * packets towards the dest network.
     * @param vifname the name of the virtual interface to be used to
     * forward packets towards the dest network.
     * @param metric the routing metric toward dest.
     * @param admin_distance the administratively defined distance
     * toward dest.
     * @param protocol_origin the name of the protocol that originated
     * this entry.
     */
    void add_route(const IPv6Net& dest,
		   const IPv6& gw,
		   const string& ifname,
		   const string& vifname,
		   uint32_t metric,
		   uint32_t admin_distance,
		   const string& protocol_origin);

    /**
     * Communicate the deletion of an IPv6 route to the RIB client.
     *
     * @param dest the destination subnet of the route.
     */
    void delete_route(const IPv6Net& dest);

    //
    // The methods below are compatibility methods used by the
    // ExportTable - they are implemented in terms of the methods
    // above.
    //

    /**
     * Communicate the addition of a new IPv4 route to the RIB client.
     *
     * @param re the routing table entry of the new route.
     */
    void add_route(const IPv4RouteEntry& re);

    /**
     * Communicate the deletion of a new IPv4 route to the RIB client.
     *
     * @param re the routing table entry of the route to be deleted.
     */
    void delete_route(const IPv4RouteEntry& re);

    /**
     * Communicate the addition of a new IPv6 route to the RIB client.
     *
     * @param re the routing table entry of the new route.
     */
    void add_route(const IPv6RouteEntry& re);

    /**
     * Communicate the deletion of a new IPv6 route to the RIB client.
     *
     * @param re the routing table entry of the route to be deleted.
     */
    void delete_route(const IPv6RouteEntry& re);

    /**
     * @return the number of route adds and deletes that are
     * currently queued for communication with the RIB client.  
     */
    size_t tasks_count() const;

    /**
     * @return true if there are currently any adds or deletes queued
     * for transmission to the RIB client or awaiting acknowledgement from
     * the RIB client.
     */
    bool tasks_pending() const;

    /**
     * check RibClient failure status
     *
     * @return true if RibClient has suffered a fatal error, true otherwise
     */
    bool failed() const { return _failed;}

protected:
    /**
     * @return The next task or NULL if there isn't one.
     */
    SyncFtiCommand* get_next();

    /**
     * Called when a transaction has completed.
     */
    void transaction_completed(bool fatal_error);

    /**
     * Called to start a transaction.
     */
    void start();

private:
    XrlRouter&	_xrl_router;		// The XRL router to use
    const string _target_name;		// The XRL target name to send the
					// route changes to.
    bool	_busy;			// Transaction in progress
    list<RibClientTask> _tasks;		// List of current task
    list<RibClientTask> _completed_tasks; // Completed tasks are stored here
					  // for later deletion.
    const size_t _max_ops;		// Max. allowed tasks in a transaction
    size_t	_op_count;		// Number of tasks in this transaction
    bool	_enabled;		// True if enabled
    bool        _failed;                // True if this interface has 
                                        // suffered a fatal error.
};

#endif // __RIB_RIB_CLIENT_HH__
