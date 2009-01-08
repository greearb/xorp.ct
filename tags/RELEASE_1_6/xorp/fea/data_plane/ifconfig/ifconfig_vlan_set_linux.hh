// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_vlan_set_linux.hh,v 1.6 2008/10/02 21:57:09 bms Exp $

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
