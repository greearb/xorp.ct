// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/tools/show_interfaces.hh,v 1.11 2008/10/02 21:57:13 bms Exp $

#ifndef __FEA_TOOLS_SHOW_INTERFACES_HH__
#define __FEA_TOOLS_SHOW_INTERFACES_HH__


#include "libxorp/service.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"


class EventLoop;


class InterfaceMonitor : public IfMgrHintObserver,
			 public ServiceBase,
			 public ServiceChangeObserverBase {
public:
    /**
     * InterfaceMonitor constructor
     *
     * @param eventloop this process's EventLoop.
     * @param class_name the XRL class name of this target.
     * @param finder_hostname the finder's host name.
     * @param finder_port the finder's port.
     * @param fea_target the FEA target name.
     */
    InterfaceMonitor(EventLoop&		eventloop,
		     const string&	class_name,
		     const string&	finder_hostname,
		     uint16_t		finder_port,
		     const string&	fea_target);

    /**
     * InterfaceMonitor destructor
     */
    ~InterfaceMonitor();

    /**
     * Startup operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Print information about network interfaces that was received
     * from the FEA.
     *
     * @param print_iface_name the name of the interface to print.
     * If it is the empty string, then print information about all
     * configured interfaces.
     */
    void print_interfaces(const string& print_iface_name) const;

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

    EventLoop&		_eventloop;

    IfMgrXrlMirror	_ifmgr;

    //
    // Status-related state
    //
    size_t		_startup_requests_n;
    size_t		_shutdown_requests_n;
};

#endif // __FEA_TOOLS_SHOW_INTERFACES_HH__
