// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_dummy.hh,v 1.13 2008/05/14 06:59:54 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_DUMMY_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_DUMMY_HH__

#include "fea/ifconfig_set.hh"


class IfConfigSetDummy : public IfConfigSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigSetDummy(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigSetDummy();

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
     * Get a reference to the @ref IfTree instance.
     *
     * @return a reference to the @ref IfTree instance.
     */
    const IfTree& iftree() const { return _iftree; }

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

private:
    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;

    /**
     * Determine if the interface's underlying provider implements unreachable
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if unreachable semantics are emulated.
     */
    virtual bool is_unreachable_emulated(const IfTreeInterface& i) const;

    /**
     * Start the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_begin(string& error_msg);

    /**
     * Complete the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_end(string& error_msg);

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
				       string& error_msg);

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
				     string& error_msg);

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
				 string& error_msg);

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
			       string& error_msg);

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
				   string& error_msg);

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
				      string& error_msg);

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
				   string& error_msg);

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
				      string& error_msg);

    IfTree		_iftree;		// The configured IfTree
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_DUMMY_HH__
