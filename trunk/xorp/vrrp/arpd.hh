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

// $XORP: xorp/vrrp/arpd.hh,v 1.2 2008/10/09 17:55:51 abittau Exp $

#ifndef __VRRP_ARPD_HH__
#define __VRRP_ARPD_HH__

#include <set>

#include "libxorp/ipv4.hh"
#include "vrrp_packet.hh"
#include "vrrp_interface.hh"

class ARPd {
public:
    ARPd(VRRPInterface& vif);
    ~ARPd();

    void clear();
    void insert(const IPv4& ip);
    void stop();
    void start();
    void ips_updated();
    void recv(const Mac& src, const PAYLOAD& payload);
    void set_mac(const Mac& mac);

private:
    typedef set<IPv4>	IPS;

    void start_receiving();
    void stop_receiving();

    VRRPInterface&  _vif;
    Mac		    _mac;
    IPS		    _ips;
    bool	    _running;
    bool	    _receiving;
};

#endif // __VRRP_ARPD_HH__
