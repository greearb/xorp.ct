// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2009 XORP, Inc.
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

// $XORP: xorp/vrrp/vrrp_target.hh,v 1.11 2008/12/18 11:34:51 abittau Exp $

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

/**
 * @short The VRRP XORP process.
 *
 * This class manages all VRRP instances and provides a link between XRLs and
 * the VRRP state machine.
 */
class VrrpTarget : public XrlVrrpTargetBase, public IfMgrHintObserver {
public:
    static const string vrrp_target_name;
    static const string fea_target_name;

    /**
     * @param rtr the XRL router to use.
     */
    VrrpTarget(XrlRouter& rtr);
    ~VrrpTarget();

    /**
     * Check whether VRRP is running.
     * 
     * @return whether the VRRP protocol is running.
     */
    bool       running() const;

    /**
     * Called when the rtrmgr configuration is first received.
     */
    void       tree_complete();

    /**
     * Called when the rtrmgr configuration changed.
     */
    void       updates_made();

    /**
     * Transmit a L2 packet.
     *
     * @param ifname the physical interface on which to transmit.
     * @param vifname the logical interface on which to transmit.
     * @param src the source MAC address.
     * @param dst the destination MAC address.
     * @param ether the 802.3 ethernet type.
     * @param payload the data to follow the MAC header.
     */
    void       send(const string& ifname, const string& vifname,
		    const Mac& src, const Mac& dst, uint32_t ether,
	            const PAYLOAD& payload);

    /**
     * Join the VRRP multicast group.
     *
     * @param ifname the interface on which to join.
     * @param vifname the vif on which to join.
     */
    void       join_mcast(const string& ifname, const string& vifname);

    /**
     * Leave the VRRP multicast group.
     *
     * @param ifname the interface on which to join.
     * @param vifname the vif on which to join.
     */
    void       leave_mcast(const string& ifname, const string& vifname);

    /**
     * Start the reception of ARP packets on an interface.
     *
     * @param ifname the interface on which to receive ARPs.
     * @param vifname the vif on which to get ARPs.
     */
    void       start_arps(const string& ifname, const string& vifname);

    /**
     * Stop the reception of ARPs.
     *
     * @param ifname the interface on which to stop reception.
     * @param vifname the vif on which to stop reception.
     */
    void       stop_arps(const string& ifname, const string& vifname);

    /**
     * Add a MAC address to the router.
     *
     * @param ifname the interface on which to add the MAC.
     * @param mac the MAC address.
     */
    void add_mac(const string& ifname, const Mac& mac);

    /**
     * Delete MAC address from the router.
     *
     * @param ifname the interface on which the MAC should be deleted.
     * @param mac the MAC to remove.
     */
    void delete_mac(const string& ifname, const Mac& mac);

    void add_ip(const string& ifname, const IPv4& ip, uint32_t prefix);
    void delete_ip(const string& ifname, const IPv4& ip);


    /**
     * @return an instance of the eventloop.
     */
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
    XrlCmdError common_0_1_startup();

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

    XrlCmdError vrrp_0_1_set_prefix (
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip,
	const uint32_t& prefix_len);

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

    /**
     * Terminate the VRRP process.
     */
    void	shutdown();

    /**
     * Start VRRP.
     */
    void	start();

    /**
     * Check whether rtrmgr configuration changes affect VRRP.
     */
    void	check_interfaces();

    /**
     * Check whether configuration changes affect a specific interface.
     *
     * @param vif the interface to check.
     */
    void	check_vif(VrrpVif& vif);
    
    /**
     * Add a VRRP instance.
     *
     * @param ifn interface to run on.
     * @param vifn vif to run on.
     * @param id the VRRP router id.
     */
    void	add_vrid(const string& ifn, const string& vifn, uint32_t id);

    /**
     * Delete a VRRP instance.
     *
     * @param ifn the interface on which the instance is running.
     * @param vifn the vif on which it's running.
     * @param id the VRRP router id.
     */
    void	delete_vrid(const string& ifn, const string& vifn, uint32_t id);

    /**
     * Get a reference to a VRRP instance.  An exception is thrown if the
     * instance does not exist.
     *
     * @return the VRRP instance.
     * @param ifn the interface on which it is running.
     * @param vifn the vif on which it is running.
     * @param id the VRRP id.
     */
    Vrrp&	find_vrid(const string& ifn, const string& vifn, uint32_t id);

    /**
     * Get a pointer to a VRRP instance.  Null is return if it does not exist.
     *
     * @return the VRRP instance or null if it is not present.
     * @param ifn the interface on which the instance is running.
     * @param vifn the vif on which it is running.
     * @param id the VRRP id.
     */
    Vrrp*	find_vrid_ptr(const string& ifn, const string& vifn,
			      uint32_t id);

    /**
     * Get a pointer to a network interface.  Null is returned if none is found
     * and add is false.  If none is found and add is true, the interface is
     * created and a pointer to it is returned.
     *
     * @return the interface, or null if it is not found and add is false.
     * @param ifn the physical interface name.
     * @param vifn the logical interface name.
     * @param add whether the interface should be added if it is not found.
     */
    VrrpVif*	find_vif(const string& ifn, const string& vifn,
			 bool add = false);

    /**
     * The callback for all XRL operations.
     *
     * @param xrl_error the error (if any) of the operation.
     */
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
