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

// $XORP: xorp/fea/ifconfig_get.hh,v 1.49 2008/10/02 21:56:47 bms Exp $

#ifndef __FEA_IFCONFIG_GET_HH__
#define __FEA_IFCONFIG_GET_HH__

#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class IfConfig;


class IfConfigGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigGet(FeaDataPlaneManager& fea_data_plane_manager)
	: _is_running(false),
	  _ifconfig(fea_data_plane_manager.ifconfig()),
	  _fea_data_plane_manager(fea_data_plane_manager)
    {}

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigGet() {}
    
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
     * Pull the network interface information from the underlying system.
     * 
     * @param local_config If not NULL, optimized ifconfig-get subclasses
     *  may pull interface config for only interfaces found in local_config.
     *  Set to NULl to pull all information from the kernel.
     * @param iftree the IfTree storage to store the pulled information.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int pull_config(const IfTree* local_config, IfTree& iftree) = 0;

    /** Child classes that *can* do this should over-ride.
     */
    virtual bool can_pull_one() { return false; }

    /** If_index can be -1 if unknown:  We will try to resolve it from ifname.
     * Child classes that can do this should implement the method.
     */
    virtual int pull_config_one(IfTree& iftree, const char* ifname, int if_index) {
	UNUSED(iftree);
	UNUSED(ifname);
	UNUSED(if_index);
	return XORP_ERROR;
    }
    
protected:
    // Misc other state
    bool	_is_running;

private:
    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_IFCONFIG_GET_HH__
