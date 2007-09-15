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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.54 2007/07/16 23:54:06 pavlin Exp $

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
     * (@see FeaDataPlaneManager).
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

    /**
     * Test whether the provider mirrors the pulled interface configuration
     * from the system.
     *
     * Note that this method is a short-term solution and might disappear
     * in the future.
     *
     * @return true if the provider mirrors the pulled interface configuration
     * from the system.
     */
    virtual bool is_pulled_config_mirror() const { return (false); }

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
     * Add an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg) = 0;

    /**
     * Add a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg) = 0;

    /**
     * Configure an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param flags the flags to set on the interface.
     * @param is_up if true, the interface is UP, otherwise is DOWN.
     * @param is_deleted if true, the interface is deleted.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg) = 0;

    /**
     * Configure a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param flags the flags to set on the vif.
     * @param is_up if true, the vif is UP, otherwise is DOWN.
     * @param is_deleted if true, the vif is deleted.
     * @param broadcast if true, the vif is broadcast capable.
     * @param loopback if true, the vif corresponds to the loopback interface.
     * @param point_to_point if true, the vif is a point-to-point interface.
     * @param multicast if true, the vif is multicast capable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg) = 0;

    /**
     * Set the MAC address of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param ether_addr the Ethernet MAC address to set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg) = 0;

    /**
     * Set the MTU of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param mtu the MTU to set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg) = 0;

    /**
     * Add an address to a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param is_broadcast true if @ref dst_or_bcast is a broadcast address.
     * @param is_p2p true if dst_or_bcast is a destination/peer address.
     * @param addr the address to add.
     * @param dst_or_bcast the broadcast or the destination/peer address.
     * @param prefix_len the prefix length of the subnet mask.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg) = 0;

    /**
     * Delete an address from a vif. 
     * 
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length of the subnet mask.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    void push_iftree_begin();
    void push_iftree_end();
    void push_interface_begin(const IfTreeInterface& i);
    void push_interface_end(IfTreeInterface& i);
    void push_vif_begin(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_end(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr4& a);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr6& a);

    IfConfig&		_ifconfig;
    FeaDataPlaneManager& _fea_data_plane_manager;
};

#endif // __FEA_IFCONFIG_SET_HH__
