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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_iphelper.hh,v 1.9 2007/12/23 08:22:18 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IPHELPER_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IPHELPER_HH__

#include <map>

#include "fea/ifconfig_set.hh"


class IfConfigSetIPHelper : public IfConfigSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigSetIPHelper(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigSetIPHelper();

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

    /**
     * Set the interface status.
     *
     * @param ifname the name of the interface.
     * @param if_index the interface index.
     * @param interface_flags the interface flags.
     * @param is_enabled if true then enable the interface, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_interface_status(const string& ifname, uint32_t if_index,
			     uint32_t interface_flags, bool is_enabled,
			     string& error_msg);

    /**
     * Add an IPv4 address to an interface/vif.
     *
     * @param ifname the name of the interface.
     * @param vifname the name of the vif.
     * @param if_index the interface/vif index.
     * @param addr the address to add.
     * @param prefix_len the prefix length.
     * @param is_broadcast if true this is a broadcast vif.
     * @param broadcast_addr the broadcast address if this is a
     * broadcast vif.
     * @param is_point_to_point if true, this is a point-to-point vif.
     * @param endpoint_addr the endpoint address if this is a
     * point-to-point vif.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_addr(const string& ifname, const string& vifname,
		 uint32_t if_index, const IPv4& addr, uint32_t prefix_len,
		 bool is_broadcast, const IPv4& broadcast_addr,
		 bool is_point_to_point, const IPv4& endpoint_addr,
		 string& error_msg);

    /**
     * Delete an IPv4 address from an interface/vif.
     *
     * @param ifname the name of the interface.
     * @param vifname the name of the vif.
     * @param if_index the interface/vif index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_addr(const string& ifname, const string& vifname,
		    uint32_t if_index, const IPv4& addr, uint32_t prefix_len,
		    string& error_msg);

    /**
     * Add an IPv6 address to an interface/vif.
     *
     * @param ifname the name of the interface.
     * @param vifname the name of the vif.
     * @param if_index the interface/vif index.
     * @param addr the address to add.
     * @param prefix_len the prefix length.
     * @param is_point_to_point if true, this is a point-to-point vif.
     * @param endpoint_addr the endpoint address if this is a
     * point-to-point vif.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_addr(const string& ifname, const string& vifname,
		 uint32_t if_index, const IPv6& addr, uint32_t prefix_len,
		 bool is_point_to_point, const IPv6& endpoint_addr,
		 string& error_msg);

    /**
     * Delete an IPv6 address from an interface/vif.
     *
     * @param ifname the name of the interface.
     * @param vifname the name of the vif.
     * @param if_index the interface/vif index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_addr(const string& ifname, const string& vifname,
		    uint32_t if_index, const IPv6& addr, uint32_t prefix_len,
		    string& error_msg);

#ifdef HOST_OS_WINDOWS
    map<pair<uint32_t, IPAddr>, ULONG>	_nte_map;
#endif
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IPHELPER_HH__
