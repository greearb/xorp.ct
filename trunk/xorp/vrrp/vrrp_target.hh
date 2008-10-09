// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/vrrp/vrrp_target.hh,v 1.9 2008/10/09 18:03:50 abittau Exp $

#ifndef __VRRP_VRRP_TARGET_HH__
#define __VRRP_VRRP_TARGET_HH__

#include "libxipc/xrl_router.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "xrl/targets/vrrp_base.hh"
#include "xrl/interfaces/fea_rawlink_xif.hh"
#include "xrl/interfaces/fea_rawpkt4_xif.hh"
#include "xrl/interfaces/fea_ifmgr_xif.hh"
#include "vrrp.hh"
#include "vrrp_vif.hh"

class VrrpTarget : public XrlVrrpTargetBase, public IfMgrHintObserver {
public:
    static const string vrrp_target_name;
    static const string fea_target_name;

    VrrpTarget(XrlRouter& rtr);
    ~VrrpTarget();

    bool       running() const;
    void       tree_complete();
    void       updates_made();
    void       send(const string& ifname, const string& vifname,
		    const Mac& src, const Mac& dst, uint32_t ether,
	            const PAYLOAD& payload);
    void       join_mcast(const string& ifname, const string& vifname);
    void       leave_mcast(const string& ifname, const string& vifname);
    void       start_arps(const string& ifname, const string& vifname);
    void       stop_arps(const string& ifname, const string& vifname);
    void       add_mac(const string& ifname, const Mac& mac);
    void       delete_mac(const string& ifname, const Mac& mac);
    EventLoop& eventloop();

protected:
    XrlCmdError common_0_1_get_target_name(
        // Output values,
        string& name);

    XrlCmdError common_0_1_get_version(
        // Output values,
        string& version);

    XrlCmdError common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError vrrp_0_1_add_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid);

    XrlCmdError vrrp_0_1_delete_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid);

    XrlCmdError vrrp_0_1_set_priority(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& priority);

    XrlCmdError vrrp_0_1_set_interval(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& interval);

    XrlCmdError vrrp_0_1_set_preempt(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     preempt);

    XrlCmdError vrrp_0_1_set_disable(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     disable);

    XrlCmdError vrrp_0_1_add_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip);

    XrlCmdError vrrp_0_1_delete_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip);

    XrlCmdError vrrp_0_1_get_vrid_info(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        // Output values,
        string& state,
        IPv4&   master);

    XrlCmdError vrrp_0_1_get_vrids(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        // Output values,
        XrlAtomList&    vrids);

    XrlCmdError vrrp_0_1_get_ifs(
        // Output values,
        XrlAtomList&    ifs);

    XrlCmdError vrrp_0_1_get_vifs(
        // Input values,
        const string&   ifname,
        // Output values,
        XrlAtomList&    vifs);

    XrlCmdError raw_packet4_client_0_1_recv(
        // Input values,
        const string&   if_name,
        const string&   vif_name,
        const IPv4&     src_address,
        const IPv4&     dst_address,
        const uint32_t& ip_protocol,
        const int32_t&  ip_ttl,
        const int32_t&  ip_tos,
        const bool&     ip_router_alert,
        const bool&     ip_internet_control,
        const vector<uint8_t>&  payload);

    XrlCmdError raw_link_client_0_1_recv(
        // Input values,
        const string&   if_name,
        const string&   vif_name,
        const Mac&      src_address,
        const Mac&      dst_address,
        const uint32_t& ether_type,
        const vector<uint8_t>&  payload);

private:
    typedef map<string, VrrpVif*>   VIFS; // vifname -> VrrpVif
    typedef map<string, VIFS*>	    IFS;  // ifname -> VIFS

    void	shutdown();
    void	start();
    void	check_interfaces();
    void	check_vif(VrrpVif& vif);
    void	add_vrid(const string& ifn, const string& vifn, uint32_t id);
    void	delete_vrid(const string& ifn, const string& vifn, uint32_t id);
    Vrrp&	find_vrid(const string& ifn, const string& vifn, uint32_t id);
    Vrrp*	find_vrid_ptr(const string& ifn, const string& vifn,
			      uint32_t id);
    VrrpVif*	find_vif(const string& ifn, const string& vifn,
			 bool add = false);
    void	xrl_cb(const XrlError& xrl_error);

    XrlRouter&		    _rtr;
    bool		    _running;
    IFS			    _ifs;
    IfMgrXrlMirror	    _ifmgr;
    bool		    _ifmgr_setup;
    XrlRawLinkV0p1Client    _rawlink;
    XrlRawPacket4V0p1Client _rawipv4;
    XrlIfmgrV0p1Client	    _fea;
    int			    _xrls_pending;
};

#endif // __VRRP_VRRP_TARGET_HH__
