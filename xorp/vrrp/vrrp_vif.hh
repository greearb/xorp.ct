// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2010 XORP, Inc and Others
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

// $XORP: xorp/vrrp/vrrp_vif.hh,v 1.10 2008/12/18 11:34:52 abittau Exp $

#ifndef __VRRP_VRRP_VIF_HH__
#define __VRRP_VRRP_VIF_HH__

#include <string>
#include <set>

#include "libxorp/ipv4.hh"
#include "libfeaclient/ifmgr_atoms.hh"
#include "libxipc/xrl_error.hh"

//#include "vrrp.hh"
//#include "vrrp_interface.hh"

class VrrpTarget;
class Vrrp;

/**
 * @short Implementation of a VRRP network interface.
 *
 * This class links the VRRP state machine to the actual network.
 */
class VrrpVif {
public:
    typedef set<uint8_t>    VRIDS;

    /**
     * @param vt the VRRP target.
     * @param ifname the name of the physical interface.
     * @param vifname the name of the logical interface.
     */
    VrrpVif(VrrpTarget& vt, const string& ifname, const string& vifname);
    ~VrrpVif();

    VrrpTarget& get_vrrp_target() { return _vt; }

    /**
     * Check whether an IP address is configured on this interface.
     *
     * @return whether the given IP address is configured on the interface.
     * @param addr the IP address tocheck for.
     */
    bool	    own(const IPv4& addr);

    /**
     * Look for a VRRP instance on this interface.
     *
     * @return a VRRP instance configured on this interface.
     * @param vrid the VRRP ID to look for.
     */
    Vrrp*	    find_vrid(uint32_t vrid);

    /**
     * Add a VRRP instance on this interface.
     *
     * @param vrid the router ID of this VRRP instance.
     */
    void	    add_vrid(uint32_t vrid);

    /**
     * Delete a VRRP instance from this interface.
     *
     * @param vrid the router ID of the instance.
     */
    void	    delete_vrid(uint32_t vrid);

    /**
     * Check whether the interface is up.
     *
     * @return whether the interface is enabled.
     */
    bool	    ready() const;

    /**
     * Change the interface's configuration.
     *
     * @param conf the new configuration of the interface.
     */
    void	    configure(const IfMgrIfTree& conf);

    /**
     * Obtain the interface's primary IP address.
     *
     * @return the primary IP address.
     */
    const IPv4&	    addr() const;

    /**
     * Send a L2 packet.
     *
     * @param src the source MAC address.
     * @param dst the destination MAC address.
     * @param ether the Ethernet type.
     * @param payload the data following the MAC header.
     */
    void	    send(const Mac& src, const Mac& dst, uint32_t ether,
			 const vector<uint8_t>& payload);

    /**
     * Join the VRRP multicast group.
     */
    void	    join_mcast();

    /**
     * Leave the VRRP multicast group.
     */
    void	    leave_mcast();

    /**
     * Receive an IP packet.
     *
     * @param from the source IP address.
     * @param payload the IP payload.
     */
    void recv(const IPv4& from, const vector<uint8_t>& payload);

    /**
     * Add a MAC address to this interface.
     *
     * @param mac MAC address to add.
     */
    void add_mac(const Mac& mac);
    void add_ip(const IPv4& ip, uint32_t prefix);

    /**
     * Delete a MAC address from this interface.
     *
     * @param mac MAC address to remove.
     */
    void delete_mac(const Mac& mac);

    void delete_ip(const IPv4& ip);

    /**
     * Start the reception of ARP packets.
     */
    void	    start_arps();

    /**
     * Stop the reception of ARP packets.
     */
    void	    stop_arps();

    /**
     * Notify the reception of an ARP packet.
     *
     * @param src the source MAC address.
     * @param payload the ARP header and data.
     */
    void recv_arp(const Mac& src, const vector<uint8_t>& payload);

    /**
     * Obtain a list of VRRP instance configured on this interface.
     *
     * @param vrids the VRRP instances on this interface (output parameter).
     */
    void	    get_vrids(VRIDS& vrids);

    /**
     * Callback on XRL error caused by this interface.
     *
     * @param xrl_error the error (if any).
     */
    void	    xrl_cb(const XrlError& xrl_error);

private:
    typedef set<IPv4>		    IPS;
    typedef map<uint32_t, Vrrp*>    VRRPS;

    /**
     * Enable or disable the interface.  This will start / stop all VRRP
     * instances running on this interface.
     *
     * @param ready if true, enable the interface, otherwise disable it.
     */
    void		    set_ready(bool ready);

    /**
     * Check whether a rtrmgr configuration element (e.g., an interface) is
     * enabled.
     *
     * @return true if the element is enabled, false otherwise.
     * @param obj element to check.
     */
    template <class T> bool is_enabled(const T* obj);

    VrrpTarget&	_vt;
    string	_ifname;
    string	_vifname;
    bool	_ready;	// is it configured?
    IPS		_ips;	// IPs assigned to this vif
    VRRPS	_vrrps;	// VRRPs enabled on this vif
    uint32_t	_join;
    uint32_t	_arps;
};

#endif // __VRRP_VRRP_VIF_HH__
