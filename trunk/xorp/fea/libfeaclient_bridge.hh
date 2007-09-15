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

// $XORP: xorp/fea/libfeaclient_bridge.hh,v 1.14 2007/08/16 01:12:25 pavlin Exp $

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
