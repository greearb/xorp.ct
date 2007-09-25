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

// $XORP: xorp/fea/ifconfig_vlan_set.hh,v 1.3 2007/09/15 01:22:35 pavlin Exp $

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
	  _fea_data_plane_manager(fea_data_plane_manager),
	  _is_vif_deleted(false)
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
     * Configure a VLAN.
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
    virtual int config_vlan(const IfTreeInterface* pulled_ifp,
			    const IfTreeVif* pulled_vifp,
			    const IfTreeInterface& config_iface,
			    const IfTreeVif& config_vif,
			    string& error_msg) = 0;

    /**
     * Test whether a vif was deleted.
     *
     * @return if true a vif was deleted.
     */
    bool is_vif_deleted() const { return (_is_vif_deleted); }

    /**
     * Set/reset the flag indicating that a vif was deleted.
     *
     * @param v if true the vif was deleted.
     */
    void set_vif_deleted(bool v) { _is_vif_deleted = v; }

protected:
    // Misc other state
    bool	_is_running;

private:
    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;

    bool _is_vif_deleted;		// If true, a vif was deleted
};

#endif // __FEA_IFCONFIG_VLAN_SET_HH__
