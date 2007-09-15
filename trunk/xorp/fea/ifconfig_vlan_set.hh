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

// $XORP: xorp/fea/ifconfig_vlan_set.hh,v 1.1 2007/09/15 00:32:16 pavlin Exp $

#ifndef __FEA_IFCONFIG_VLAN_SET_HH__
#define __FEA_IFCONFIG_VLAN_SET_HH__

#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class IfConfig;


class IfConfigVlanSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     */
    IfConfigVlanSet(FeaDataPlaneManager& fea_data_plane_manager)
	: _is_running(false),
	  _ifconfig(fea_data_plane_manager.ifconfig()),
	  _fea_data_plane_manager(fea_data_plane_manager)
    {}

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigVlanSet() {}

    /**
     * Get the @ref IfConfig instance.
     *
     * @return the @ref IfConfig instance.
     */
    IfConfig&	ifconfig() { return _ifconfig; }

    /**
     * Get the @ref FeaDataPlaneManager instance.
     *
     * @return the @ref FeaDataPlaneManager instance.
     */
    FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

    /**
     * Test whether this instance is running.
     *
     * @return true if the instance is running, otherwise false.
     */
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;

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
    virtual bool push_config(IfTree& iftree) = 0;

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
			 string& error_msg) = 0;

    /**
     * Delete a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vlan(const string& ifname,
			    const string& vifname,
			    string& error_msg) = 0;

    /**
     * Configure a VLAN.
     *
     * @param parent_ifname the parent interface name.
     * @param vlan_name the VLAN vif name.
     * @param vlan_id the VLAN ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vlan(const string& parent_ifname,
			    const string& vlan_name,
			    uint16_t vlan_id,
			    string& error_msg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_IFCONFIG_VLAN_SET_HH__
