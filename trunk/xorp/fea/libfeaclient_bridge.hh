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

// $XORP: xorp/fea/libfeaclient_bridge.hh,v 1.2 2003/10/17 21:03:26 hodson Exp $

#ifndef __FEA_LIBFEACLIENT_BRIDGE_HH__
#define __FEA_LIBFEACLIENT_BRIDGE_HH__

#include "ifconfig.hh"

class XrlRouter;
class IfTree;
class IfMgrXrlReplicationManager;
class IfMgrIfTree;

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
 * is made aware of this object through @ref set_iftree.  Failure to
 * call method before an update is received will cause a fatal error.
 */
class LibFeaClientBridge : public IfConfigUpdateReporterBase {
public:
    LibFeaClientBridge(XrlRouter& rtr);
    ~LibFeaClientBridge();

    /**
     * Set interface configuration tree to read from when an update is
     * recorded through one of the update methods implemented from
     * IfConfigUpdateReporterBase.
     */
    void set_iftree(const IfTree* tree);

    /**
     * Add named Xrl target to list to receive libfeaclient updates.
     *
     * @param xrl_target_name Xrl target instance name.
     * @return true on success, false if target is already on the list.
     */
    bool add_libfeaclient_mirror(const string& xrl_target_name);

    /**
     * Add named Xrl target to list to receive libfeaclient updates.
     *
     * @param xrl_target_name Xrl target instance name.
     * @return true on success, false if named target is not on the list.
     */
    bool remove_libfeaclient_mirror(const string& xrl_target_name);

    /**
     * Get reference to libfeaclient's interface configuration tree.
     *
     * @return reference to tree.
     */
    inline const IfMgrIfTree& libfeaclient_iftree() const;

    /**
     * Get pointer FEA interface configuration tree that is
     * being used to feed data into libfeaclient's interface
     * configuration tree.
     *
     * @return pointer to tree.
     */
    inline const IfTree* fea_iftree() const;

protected:
    void interface_update(const string& ifname,
			  const Update& update,
			  bool 		is_system_interfaces_reportee);

    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& update,
		    bool	  is_system_interfaces_reportee);

    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& update,
			 bool	       is_system_interfaces_reportee);

    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& update,
			 bool	       is_system_interfaces_reportee);

protected:
    IfMgrXrlReplicationManager* _rm;
    const IfTree*		_iftree;
};

/**
 * Check equivalence of interface configuration trees in FEA and
 * libfeaclient.  This is a debugging method.
 *
 * @param fea_iftree reference to an FEA interface configuration tree.
 *
 * @param libfeaclient_iftree reference to a libfeaclient interface
 * configuration tree.
 *
 * @param errlog string to store textual representation of
 * differences.
 *
 * @return true if tree's are equivalent, false otherwise.
 */
bool
equivalent(const IfTree&	fea_iftree,
	   const IfMgrIfTree&	libfeaclient_iftree,
	   string&		errlog);

#endif // __FEA_LIBFEACLIENT_BRIDGE_HH__
