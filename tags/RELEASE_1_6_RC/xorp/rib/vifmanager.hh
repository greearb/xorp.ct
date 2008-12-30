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

// $XORP: xorp/rib/vifmanager.hh,v 1.23 2008/07/23 05:11:33 pavlin Exp $

#ifndef __RIB_VIFMANAGER_HH__
#define __RIB_VIFMANAGER_HH__

#include "libproto/proto_state.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"


class EventLoop;
class RibManager;
class XrlRouter;

/**
 * @short VifManager keeps track of the VIFs currently enabled in the FEA.
 *
 * The RIB process has a single VifManager instance, which registers
 * with the FEA process to discover the VIFs on this router and their
 * IP addresses and prefixes. When the VIFs or their configuration in
 * the FEA change, the VifManager will be notified, and it will update
 * the RIBs appropriately. The RIBs need to know about VIFs and VIF
 * addresses to decide which routes have nexthops that are on directly
 * connected subnets, and which are nexthops that need to be resolved
 * using other routes to figure out where next to send the packet.
 * Only routes with nexthops that are on directly connected subnets
 * can be sent to the FEA.
 */
class VifManager : public IfMgrHintObserver,
		   public ServiceChangeObserverBase,
		   public ProtoState {
public:
    /**
     * VifManager constructor
     *
     * @param xrl_router this process's XRL router.
     * @param eventloop this process's EventLoop.
     * @param rib_manager this class contains the actual RIBs for IPv4
     * and IPv6, unicast and multicast.
     * @param fea_target the FEA target name.
     */
    VifManager(XrlRouter& xrl_router, EventLoop& eventloop,
	       RibManager* rib_manager, const string& fea_target);

    /**
     * VifManager destructor
     */
    ~VifManager();

    /**
     * Start operation.
     * 
     * Start the process of registering with the FEA, etc.
     * After the startup operations are completed,
     * @ref VifManager::final_start() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	start();

    /**
     * Stop operation.
     * 
     * Gracefully stop operation.
     * After the shutdown operations are completed,
     * @ref VifManager::final_stop() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	stop();

    /**
     * Completely start the node operation.
     * 
     * This method should be called internally after @ref VifManager::start()
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_start();

    /**
     * Completely stop the node operation.
     * 
     * This method should be called internally after @ref VifManager::stop()
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_stop();

protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

    void incr_startup_requests_n();
    void decr_startup_requests_n();
    void incr_shutdown_requests_n();
    void decr_shutdown_requests_n();
    void update_status();

private:
    /**
     * A method invoked when the status of a service changes.
     * 
     * @param service the service whose status has changed.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status);

    /**
     * Get a reference to the service base of the interface manager.
     * 
     * @return a reference to the service base of the interface manager.
     */
    const ServiceBase* ifmgr_mirror_service_base() const { return dynamic_cast<const ServiceBase*>(&_ifmgr); }

    /**
     * Get a reference to the interface manager tree.
     * 
     * @return a reference to the interface manager tree.
     */
    const IfMgrIfTree&	ifmgr_iftree() const { return _ifmgr.iftree(); }

    /**
     * Initiate startup of the interface manager.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int ifmgr_startup();

    /**
     * Initiate shutdown of the interface manager.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int ifmgr_shutdown();

    XrlRouter&		_xrl_router;
    EventLoop&		_eventloop;
    RibManager*		_rib_manager;

    IfMgrXrlMirror	_ifmgr;
    IfMgrIfTree		_iftree;
    IfMgrIfTree		_old_iftree;

    //
    // Status-related state
    //
    size_t		_startup_requests_n;
    size_t		_shutdown_requests_n;
};

#endif // __RIB_VIFMANAGER_HH__
