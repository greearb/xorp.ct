// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP$

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_BSD_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_BSD_HH__

#include "fea/ifconfig_set.hh"


class IfConfigVlanSetBsd : public IfConfigVlanSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     */
    IfConfigVlanSetBsd(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigVlanSetBsd();

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
     * Push the VLAN network interface configuration into the underlying
     * system.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param iftree the interface tree configuration to push.
     * @return true on success, otherwise false.
     */
    virtual bool push_config(IfTree& iftree);

    /**
     * Add a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vlan(const string& parent_ifname,
			 const string& vlan_name,
			 string& error_msg);

    /**
     * Delete a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vlan(const string& parent_ifname,
			    const string& vlan_name,
			    string& error_msg);

    /**
     * Configure a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param vlan_tag the VLAN tag.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vlan(const string& parent_ifname,
			    const string& vlan_name,
			    uint16_t vlan_tag,
			    string& error_msg);

private:
    int _s4;
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_VLAN_SET_BSD_HH__
