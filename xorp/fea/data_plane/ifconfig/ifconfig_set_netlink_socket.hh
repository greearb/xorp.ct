// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_netlink_socket.hh,v 1.14 2008/12/18 02:06:57 abittau Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_NETLINK_SOCKET_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_NETLINK_SOCKET_HH__

#include <xorp_config.h>

#ifdef HAVE_NETLINK_SOCKETS

#include "fea/ifconfig_set.hh"
#include "fea/data_plane/control_socket/netlink_socket.hh"


class IfConfigSetNetlinkSocket : public IfConfigSet,
				 public NetlinkSocket {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigSetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigSetNetlinkSocket();

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
    int set_interface_status(const string& ifname,
			     uint32_t if_index,
			     uint32_t interface_flags,
			     bool is_enabled,
			     string& error_msg);

    /**
     * Busy waits until the interface status changes.
     *
     * @param ifp the interface to check.
     * @param is_enabled if true, will wait for interface to become enabled.
     */
    void wait_interface_status(const IfTreeInterface* ifp, bool is_enabled);

    /**
     * Set the interface MAC address.
     *
     * @param ifname the name of the interface.
     * @param if_index the interface index.
     * @param mac the interface MAC address.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_interface_mac_address(const string& ifname, uint32_t if_index,
				  const Mac& mac, string& error_msg);

    /**
     * Set the interface MTU.
     *
     * @param ifname the name of the interface.
     * @param if_index the interface index.
     * @param mac the interface MTU.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_interface_mtu(const string& ifname, uint32_t if_index,
			  uint32_t mtu, string& error_msg);

    /**
     * Add an IPvX address to an interface/vif.
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
		 uint32_t if_index, const IPvX& addr, uint32_t prefix_len,
		 bool is_broadcast, const IPvX& broadcast_addr,
		 bool is_point_to_point, const IPvX& endpoint_addr,
		 string& error_msg);

    /**
     * Delete an IPvX address from an interface/vif.
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
		    uint32_t if_index, const IPvX& addr, uint32_t prefix_len,
		    string& error_msg);

    NetlinkSocketReader	_ns_reader;
};

#endif
#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_NETLINK_SOCKET_HH__
