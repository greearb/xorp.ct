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

// $XORP: xorp/rib/vifmanager.hh,v 1.14 2004/04/10 07:46:41 pavlin Exp $

#ifndef __RIB_VIFMANAGER_HH__
#define __RIB_VIFMANAGER_HH__

#include "libproto/proto_state.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"


class EventLoop;
class RibManager;
class Vif;
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
     * @return true on success, false on failure.
     */
    bool ifmgr_startup();

    /**
     * Initiate shutdown of the interface manager.
     * 
     * @return true on success, false on failure.
     */
    bool ifmgr_shutdown();

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
