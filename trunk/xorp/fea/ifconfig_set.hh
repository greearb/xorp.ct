// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.9 2003/10/13 23:32:41 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__


class IfConfig;
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IfTreeAddr4;
class IfTreeAddr6;

class IfConfigSet {
public:
    IfConfigSet(IfConfig& ifc);
    
    virtual ~IfConfigSet();
    
    IfConfig&	ifc() { return _ifc; }
    
    virtual void register_ifc();

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

protected:
    /**
     * Set the interface MAC address.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param ether_addr the Ethernet MAC address to set.
     * @param reason the human-readable reason for any failure.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& reason) = 0;

    /**
     * Set the interface MTU address.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param mtu the MTU to set.
     * @param reason the human-readable reason for any failure.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& reason) = 0;

    /**
     * Set the interface flags.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param flags the flags to set.
     * @param reason the human-readable reason for any failure.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_flags(const string& ifname,
				    uint16_t if_index,
				    uint32_t flags,
				    string& reason) = 0;

    /**
     * Set an address on an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param is_broadcast true if @ref dst_or_bcast is a broadcast address.
     * @param is_p2p true if dst_or_bcast is a destination/peer address.
     * @param addr the address to set.
     * @param dst_or_bcast the broadcast or the destination/peer address.
     * @param prefix_len the prefix length of the subnet mask.
     * @param reason the human-readable reason for any failure.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_vif_address(const string& ifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& reason) = 0;

    /**
     * Delete an address from an interface. 
     * 
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length of the subnet mask.
     * @param reason the human-readable reason for any failure.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& ifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& reason) = 0;

private:
    void push_interface(const IfTreeInterface& i);
    void push_vif(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr4& a);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr6& a);

    IfConfig&	_ifc;
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

private:
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& reason);

    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& reason);

    virtual int set_interface_flags(const string& ifname,
				    uint16_t if_index,
				    uint32_t flags,
				    string& reason);

    virtual int set_vif_address(const string& ifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& reason);

    virtual int delete_vif_address(const string& ifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& reason);
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
    
private:
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& reason);

    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& reason);

    virtual int set_interface_flags(const string& ifname,
				    uint16_t if_index,
				    uint32_t flags,
				    string& reason);

    virtual int set_vif_address(const string& ifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& reason);

    virtual int delete_vif_address(const string& ifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& reason);

    virtual int set_vif_address4(const string& ifname,
				 uint16_t if_index,
				 bool is_broadcast,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst_or_bcast,
				 uint32_t prefix_len,
				 string& reason);

    virtual int set_vif_address6(const string& ifname,
				 uint16_t if_index,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst,
				 uint32_t prefix_len,
				 string& reason);

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

private:
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& reason);

    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& reason);

    virtual int set_interface_flags(const string& ifname,
				    uint16_t if_index,
				    uint32_t flags,
				    string& reason);

    virtual int set_vif_address(const string& ifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& reason);

    virtual int delete_vif_address(const string& ifname,
				   uint16_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& reason);

    NetlinkSocketReader	_ns_reader;
};

#endif // __FEA_IFCONFIG_SET_HH__
