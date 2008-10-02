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

// $XORP: xorp/fea/libfeaclient_bridge.hh,v 1.17 2008/07/23 05:10:10 pavlin Exp $

#ifndef __FEA_LIBFEACLIENT_BRIDGE_HH__
#define __FEA_LIBFEACLIENT_BRIDGE_HH__

#include "ifconfig_reporter.hh"

class IfConfigUpdateReplicator;
class IfMgrXrlReplicationManager;
class IfTree;
class XrlRouter;

/**
 * @short Bridge class to intervene between the FEA's interface
 * manager and libfeaclient.
 *
 * The LibFeaClientBridge takes updates received from the FEA's
 * interface manager and forwards them to registered remote
 * libfeaclient users.  For each update received, the bridge gets the
 * all the state associated with the item being updated, and pushes it
 * into a contained instance of an @ref IfMgrXrlReplicationManager.
 * If the data pushed into the IfMgrXrlReplicationManager triggers
 * state changes in it's internal interface config representation, it
 * forwards the changes to remote observers.
 *
 * In addition to arranging to plumb the LibFeaClientBridge into the
 * FEA to receive updates, it is imperative that the underlying IfTree
 * object used represent state be available to the bridge.  The bridge
 * is made aware of this object through @ref iftree.  Failure to
 * call method before an update is received will cause a fatal error.
 */
class LibFeaClientBridge : public IfConfigUpdateReporterBase {
public:
    LibFeaClientBridge(XrlRouter& rtr,
		       IfConfigUpdateReplicator& update_replicator);
    ~LibFeaClientBridge();

    /**
     * Add named Xrl target to list to receive libfeaclient updates.
     *
     * @param xrl_target_instance_name Xrl target instance name.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_libfeaclient_mirror(const string& xrl_target_instance_name);

    /**
     * Remove named Xrl target from the list to receive libfeaclient updates.
     *
     * @param xrl_target_instance_name Xrl target instance name.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_libfeaclient_mirror(const string& xrl_target_instance_name);

protected:
    void interface_update(const string& ifname,
			  const Update& update);

    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& update);

    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& update);

    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& update);

    void updates_completed();

protected:
    IfMgrXrlReplicationManager* _rm;
};

#endif // __FEA_LIBFEACLIENT_BRIDGE_HH__
