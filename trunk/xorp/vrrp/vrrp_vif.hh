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

// $XORP: xorp/vrrp/vrrp_vif.hh,v 1.7 2008/10/09 18:03:50 abittau Exp $

#ifndef __VRRP_VRRP_VIF_HH__
#define __VRRP_VRRP_VIF_HH__

#include <string>
#include <set>

#include "libxorp/ipv4.hh"
#include "libfeaclient/ifmgr_atoms.hh"
#include "vrrp.hh"
#include "vrrp_interface.hh"

class VrrpTarget;

class VrrpVif : public VrrpInterface {
public:
    typedef set<uint8_t>    VRIDS;

    VrrpVif(VrrpTarget& vt, const string& ifname, const string& vifname);
    ~VrrpVif();

    bool	    own(const IPv4& addr);
    Vrrp*	    find_vrid(uint32_t vrid);
    void	    add_vrid(uint32_t vrid);
    void	    delete_vrid(uint32_t vrid);
    bool	    ready() const;
    void	    configure(const IfMgrIfTree& conf);
    const IPv4&	    addr() const;
    void	    send(const Mac& src, const Mac& dst, uint32_t ether,
			 const PAYLOAD& payload);
    void	    join_mcast();
    void	    leave_mcast();
    void	    recv(const IPv4& from, const PAYLOAD& payload);
    void	    add_mac(const Mac& mac);
    void	    delete_mac(const Mac& mac);
    void	    start_arps();
    void	    stop_arps();
    void	    recv_arp(const Mac& src, const PAYLOAD& payload);
    void	    get_vrids(VRIDS& vrids);

private:
    typedef set<IPv4>		    IPS;
    typedef map<uint32_t, Vrrp*>    VRRPS;

    void		    set_ready(bool ready);
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
