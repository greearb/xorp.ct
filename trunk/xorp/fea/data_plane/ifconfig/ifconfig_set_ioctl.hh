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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_ioctl.hh,v 1.2 2007/06/06 19:55:54 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IOCTL_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IOCTL_HH__

#include "fea/ifconfig_set.hh"


class IfConfigSetIoctl : public IfConfigSet {
public:
    IfConfigSetIoctl(IfConfig& ifconfig);
    virtual ~IfConfigSetIoctl();

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
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
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
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);

    virtual int add_vif_address4(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_broadcast,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst_or_bcast,
				 uint32_t prefix_len,
				 string& error_msg);
    virtual int add_vif_address6(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst,
				 uint32_t prefix_len,
				 string& error_msg);

    int _s4;
    int _s6;
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_IOCTL_HH__
