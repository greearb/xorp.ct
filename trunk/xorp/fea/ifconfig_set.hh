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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.5 2003/10/11 19:47:37 pavlin Exp $

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

    virtual bool push_config(const IfTree& config);

protected:
    virtual int set_interface_mac_address(const string& ifname,
					  uint16_t if_index,
					  const struct ether_addr& ether_addr,
					  string& reason) = 0;

    virtual int set_interface_mtu(const string& ifname,
				  uint16_t if_index,
				  uint32_t mtu,
				  string& reason) = 0;

    virtual int set_interface_flags(const string& ifname,
				    uint16_t if_index,
				    uint32_t flags,
				    string& reason) = 0;

    virtual int set_vif_address(const string& ifname,
				uint16_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& reason) = 0;

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
			   public NetlinkSocket6,
			   public NetlinkSocketObserver {
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
     * Data has pop-up.
     * 
     * @param data the buffer with the data.
     * @param nbytes the number of bytes in the @ref data buffer.
     */
    virtual void nlsock_data(const uint8_t* data, size_t nbytes);

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

    int		check_request(NetlinkSocket& ns, uint32_t seqno,
			      string& reason);

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of netlink socket data to
					// cache so route lookup via netlink
					// socket can appear synchronous.
    vector<uint8_t> _cache_data;	// Cached netlink socket data.

    int _s4;
    int _s6;
};

#endif // __FEA_IFCONFIG_SET_HH__
