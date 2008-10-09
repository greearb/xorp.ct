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

// $XORP: xorp/vrrp/vrrp.hh,v 1.6 2008/10/09 17:55:51 abittau Exp $

#ifndef __VRRP_VRRP_HH__
#define __VRRP_VRRP_HH__

#include <set>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/timer.hh"
#include "libxorp/mac.hh"
#include "libxorp/eventloop.hh"
#include "vrrp_packet.hh"
#include "arpd.hh"

#define MAX_VRRP_SIZE (20 + VRRP_MAX_PACKET_SIZE)

class VRRP {
public:
    static const Mac mcast_mac;
    static const Mac bcast_mac;

    VRRP(VRRPInterface& vif, EventLoop& e, uint32_t vrid);
    ~VRRP();

    void	    set_priority(uint32_t priority);
    void	    set_interval(uint32_t interval);
    void	    set_preempt(bool preempt);
    void	    set_disable(bool disable);
    void	    add_ip(const IPv4& ip);
    void	    delete_ip(const IPv4& ip);
    bool	    running() const;
    uint32_t	    priority() const;
    void	    start();
    void	    stop();
    void	    check_ownership();
    void	    recv(const IPv4& from, const VRRPHeader& vh);
    ARPd&	    arpd();
    void	    get_info(string& state, IPv4& master) const;

private:
    enum State {
	INITIALIZE = 0,
	MASTER,
	BACKUP
    };

    enum {
	PRIORITY_LEAVE	= 0,
	PRIORITY_OWN	= 255
    };

    typedef set<IPv4>	IPS;

    void setup_intervals();
    void setup_timers(bool skew = false);
    void cancel_timers();
    void send_arps();
    void send_arp(const IPv4& ip);
    void send_advertisement(uint32_t priority);
    void send_advertisement();
    void become_master();
    void become_backup();
    bool master_down_expiry();
    bool adver_expiry();
    void recv_adver_master(const IPv4& from, uint32_t priority);
    void recv_adver_backup(uint32_t priority);
    void prepare_advertisement(uint32_t priority);
    void recv_advertisement(const IPv4& from, uint32_t priority);
    bool check_ips(const VRRPHeader& vh);

    IPv4	    _last_adv;
    VRRPInterface&  _vif;
    uint32_t	    _vrid;
    uint32_t	    _priority;
    uint32_t	    _interval;
    double	    _skew_time;
    double	    _master_down_interval;
    bool	    _preempt;
    IPS		    _ips;
    State	    _state;
    XorpTimer	    _master_down_timer;
    XorpTimer	    _adver_timer;
    bool	    _own;
    bool	    _disable;
    VRRPPacket	    _adv_packet;
    Mac		    _source_mac;
    ARPd	    _arpd; // XXX use OS proxy arp mechanism?
};

#endif // __VRRP_VRRP_HH__
