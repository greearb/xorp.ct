// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_set_linux.hh,v 1.4 2008/01/04 03:16:12 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_LINUX_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_LINUX_HH__

#include "fea/ifconfig_vlan_set.hh"


class IfConfigVlanSetLinux : public IfConfigVlanSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigVlanSetLinux(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigVlanSetLinux();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Add a VLAN.
     *
     * If an entry for the same VLAN already exists, is is overwritten
     * with the new information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_add_vlan(const IfTreeInterface* pulled_ifp,
				const IfTreeVif* pulled_vifp,
				const IfTreeInterface& config_iface,
				const IfTreeVif& config_vif,
				string& error_msg);

    /**
     * Delete a VLAN.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_delete_vlan(const IfTreeInterface* pulled_ifp,
				   const IfTreeVif* pulled_vifp,
				   const IfTreeInterface& config_iface,
				   const IfTreeVif& config_vif,
				   string& error_msg);

private:
    /**
     * Add a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param vlan_id the VLAN ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_vlan(const string& parent_ifname,
		 const string& vlan_name,
		 uint16_t vlan_id,
		 string& error_msg);

    /**
     * Delete a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_vlan(const string& parent_ifname,
		    const string& vlan_name,
		    string& error_msg);

    int _s4;
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_LINUX_HH__
