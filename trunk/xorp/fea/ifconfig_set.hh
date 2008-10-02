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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.62 2008/07/23 05:10:09 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__

#include "iftree.hh"
#include "fea_data_plane_manager.hh"

class IfConfig;
class IPvX;


class IfConfigSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigSet(FeaDataPlaneManager& fea_data_plane_manager)
	: _is_running(false),
	  _ifconfig(fea_data_plane_manager.ifconfig()),
	  _fea_data_plane_manager(fea_data_plane_manager)
    {}

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigSet() {}

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
     * Push the network interface configuration into the underlying system.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param iftree the interface tree configuration to push.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int push_config(IfTree& iftree);

protected:
    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const = 0;

    /**
     * Determine if the interface's underlying provider implements unreachable
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if unreachable semantics are emulated.
     */
    virtual bool is_unreachable_emulated(const IfTreeInterface& i) const = 0;

    /**
     * Start the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_begin(string& error_msg) = 0;

    /**
     * Complete the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_end(string& error_msg) = 0;

    /**
     * Begin the interface configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface_begin(const IfTreeInterface* pulled_ifp,
				       IfTreeInterface& config_iface,
				       string& error_msg) = 0;

    /**
     * End the interface configuration.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface_end(const IfTreeInterface* pulled_ifp,
				     const IfTreeInterface& config_iface,
				     string& error_msg) = 0;

    /**
     * Begin the vif configuration.
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
    virtual int config_vif_begin(const IfTreeInterface* pulled_ifp,
				 const IfTreeVif* pulled_vifp,
				 const IfTreeInterface& config_iface,
				 const IfTreeVif& config_vif,
				 string& error_msg) = 0;

    /**
     * End the vif configuration.
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
    virtual int config_vif_end(const IfTreeInterface* pulled_ifp,
			       const IfTreeVif* pulled_vifp,
			       const IfTreeInterface& config_iface,
			       const IfTreeVif& config_vif,
			       string& error_msg) = 0;

    /**
     * Add IPv4 address information.
     *
     * If an entry for the same address already exists, is is overwritten
     * with the new information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_add_address(const IfTreeInterface* pulled_ifp,
				   const IfTreeVif* pulled_vifp,
				   const IfTreeAddr4* pulled_addrp,
				   const IfTreeInterface& config_iface,
				   const IfTreeVif& config_vif,
				   const IfTreeAddr4& config_addr,
				   string& error_msg) = 0;

    /**
     * Delete IPv4 address information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_delete_address(const IfTreeInterface* pulled_ifp,
				      const IfTreeVif* pulled_vifp,
				      const IfTreeAddr4* pulled_addrp,
				      const IfTreeInterface& config_iface,
				      const IfTreeVif& config_vif,
				      const IfTreeAddr4& config_addr,
				      string& error_msg) = 0;

    /**
     * Add IPv6 address information.
     *
     * If an entry for the same address already exists, is is overwritten
     * with the new information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_add_address(const IfTreeInterface* pulled_ifp,
				   const IfTreeVif* pulled_vifp,
				   const IfTreeAddr6* pulled_addrp,
				   const IfTreeInterface& config_iface,
				   const IfTreeVif& config_vif,
				   const IfTreeAddr6& config_addr,
				   string& error_msg) = 0;

    /**
     * Delete IPv6 address information.
     *
     * @param pulled_ifp pointer to the interface information pulled from
     * the system.
     * @param pulled_vifp pointer to the vif information pulled from
     * the system.
     * @param pulled_addrp pointer to the address information pulled from
     * the system.
     * @param config_iface reference to the interface with the information
     * to configure.
     * @param config_vif reference to the vif with the information
     * to configure.
     * @param config_addr reference to the address with the information
     * to configure.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_delete_address(const IfTreeInterface* pulled_ifp,
				      const IfTreeVif* pulled_vifp,
				      const IfTreeAddr6* pulled_addrp,
				      const IfTreeInterface& config_iface,
				      const IfTreeVif& config_vif,
				      const IfTreeAddr6& config_addr,
				      string& error_msg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    void push_iftree_begin(IfTree& iftree);
    void push_iftree_end(IfTree& iftree);
    void push_interface_begin(const IfTreeInterface* pulled_ifp,
			      IfTreeInterface& config_iface);
    void push_interface_end(const IfTreeInterface* pulled_ifp,
			    IfTreeInterface& config_iface);
    void push_vif_creation(const IfTreeInterface* pulled_ifp,
			   const IfTreeVif* pulled_vifp,
			   IfTreeInterface& config_iface,
			   IfTreeVif& config_vif);
    void push_vif_begin(const IfTreeInterface* pulled_ifp,
			const IfTreeVif* pulled_vifp,
			IfTreeInterface& config_iface,
			IfTreeVif& config_vif);
    void push_vif_end(const IfTreeInterface* pulled_ifp,
		      const IfTreeVif* pulled_vifp,
		      IfTreeInterface& config_iface,
		      IfTreeVif& config_vif);
    void push_vif_address(const IfTreeInterface* pulled_ifp,
			  const IfTreeVif* pulled_vifp,
			  const IfTreeAddr4* pulled_addrp,
			  IfTreeInterface& config_iface,
			  IfTreeVif& config_vif,
			  IfTreeAddr4& config_addr);
    void push_vif_address(const IfTreeInterface* pulled_ifp,
			  const IfTreeVif* pulled_vifp,
			  const IfTreeAddr6* pulled_addrp,
			  IfTreeInterface& config_iface,
			  IfTreeVif& config_vif,
			  IfTreeAddr6& config_addr);

    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_IFCONFIG_SET_HH__
