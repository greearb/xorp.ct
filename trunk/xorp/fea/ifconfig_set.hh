// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.20 2004/10/21 00:44:22 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__


#include "iftree.hh"

class IfConfig;

class IfConfigSet {
public:
    IfConfigSet(IfConfig& ifc);
    
    virtual ~IfConfigSet();
    
    IfConfig&	ifc() { return _ifc; }
    
    virtual void register_ifc_primary();
    virtual void register_ifc_secondary();
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start() = 0;
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop() = 0;

    /**
     * Push the network interface configuration into the underlying system.
     *
     * @param config the interface tree configuration to push.
     * @return true on success, otherwise false.
     */
    virtual bool push_config(const IfTree& config);

    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const = 0;

protected:
    /**
     * Start the configuration.
     *
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_begin(string& errmsg) = 0;

    /**
     * Complete the configuration.
     *
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_end(string& errmsg) = 0;

    /**
     * Add an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_interface(const string& ifname,
			      uint16_t if_index,
			      string& errmsg) = 0;

    /**
     * Add a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint16_t if_index,
			string& errmsg) = 0;

    /**
     * Configure an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param flags the flags to set on the interface.
     * @param is_up if true, the interface is UP, otherwise is DOWN.
     * @param is_deleted if true, the interface is deleted.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface(const string& ifname,
				 uint16_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& errmsg) = 0;

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
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint16_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& errmsg) = 0;

    /**
     * Set the MAC address of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param ether_addr the Ethernet MAC address to set.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& errmsg) = 0;

    /**
     * Set the MTU of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param mtu the MTU to set.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& errmsg) = 0;

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
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& errmsg) = 0;

    /**
     * Delete an address from a vif. 
     * 
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length of the subnet mask.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& errmsg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    void push_iftree_begin();
    void push_iftree_end();
    void push_interface_begin(const IfTreeInterface& i);
    void push_interface_end(const IfTreeInterface& i);
    void push_vif_begin(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_end(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr4& a);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr6& a);

    IfConfig&	_ifc;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class IfConfigSetDummy : public IfConfigSet {
public:
    IfConfigSetDummy(IfConfig& ifc);
    virtual ~IfConfigSetDummy();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Push the network interface configuration into the underlying system.
     *
     * @param config the interface tree configuration to push.
     * @return true on success, otherwise false.
     */
    virtual bool push_config(const IfTree& config);

    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;

private:
    virtual int config_begin(string& errmsg);
    virtual int config_end(string& errmsg);
    virtual int add_interface(const string& ifname,
			      uint16_t if_index,
			      string& errmsg);
    virtual int add_vif(const string& vifname,
			const string& vifname,
			uint16_t if_index,
			string& errmsg);
    virtual int config_interface(const string& ifname,
				 uint16_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& errmsg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint16_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& errmsg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& errmsg);
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& errmsg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& errmsg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& errmsg);
};

class IfConfigSetIoctl : public IfConfigSet {
public:
    IfConfigSetIoctl(IfConfig& ifc);
    virtual ~IfConfigSetIoctl();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;

private:
    virtual int config_begin(string& errmsg);
    virtual int config_end(string& errmsg);
    virtual int add_interface(const string& ifname,
			      uint16_t if_index,
			      string& errmsg);
    virtual int add_vif(const string& vifname,
			const string& vifname,
			uint16_t if_index,
			string& errmsg);
    virtual int config_interface(const string& ifname,
				 uint16_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& errmsg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint16_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& errmsg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& errmsg);
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& errmsg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& errmsg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& errmsg);

    virtual int add_vif_address4(const string& ifname,
				 const string& vifname,
				 uint16_t if_index,
				 bool is_broadcast,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst_or_bcast,
				 uint32_t prefix_len,
				 string& errmsg);
    virtual int add_vif_address6(const string& ifname,
				 const string& vifname,
				 uint16_t if_index,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst,
				 uint32_t prefix_len,
				 string& errmsg);

    int _s4;
    int _s6;
};

class IfConfigSetNetlink : public IfConfigSet,
			   public NetlinkSocket4,
			   public NetlinkSocket6 {
public:
    IfConfigSetNetlink(IfConfig& ifc);
    virtual ~IfConfigSetNetlink();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();

    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;

private:
    virtual int config_begin(string& errmsg);
    virtual int config_end(string& errmsg);
    virtual int add_interface(const string& ifname,
			      uint16_t if_index,
			      string& errmsg);
    virtual int add_vif(const string& vifname,
			const string& vifname,
			uint16_t if_index,
			string& errmsg);
    virtual int config_interface(const string& ifname,
				 uint16_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& errmsg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint16_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& errmsg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& errmsg);
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& errmsg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& errmsg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& errmsg);

    NetlinkSocketReader	_ns_reader;
};

class IfConfigSetClick : public IfConfigSet,
			 public ClickSocket {
public:
    IfConfigSetClick(IfConfig& ifc);
    virtual ~IfConfigSetClick();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();

    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();

    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;

private:
    virtual int config_begin(string& errmsg);
    virtual int config_end(string& errmsg);
    virtual int add_interface(const string& ifname,
			      uint16_t if_index,
			      string& errmsg);
    virtual int add_vif(const string& vifname,
			const string& vifname,
			uint16_t if_index,
			string& errmsg);
    virtual int config_interface(const string& ifname,
				 uint16_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& errmsg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint16_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& errmsg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& errmsg);
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& errmsg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& errmsg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& errmsg);

    virtual string generate_config();

    ClickSocketReader	_cs_reader;
    IfTree		_iftree;
};

#endif // __FEA_IFCONFIG_SET_HH__
