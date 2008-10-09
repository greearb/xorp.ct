// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/vrrp/vrrp.hh,v 1.3 2008/10/09 17:46:27 abittau Exp $

#ifndef __VRRP_VRRP_HH__
#define __VRRP_VRRP_HH__

#include <set>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/timer.hh"
#include "libxorp/mac.hh"
#include "vrrp_packet.hh"
#include "arpd.hh"

class VRRPVif;

#define MAX_VRRP_SIZE (20 + VRRP_MAX_PACKET_SIZE)

class VRRP {
public:
    static const Mac mcast_mac;
    static const Mac bcast_mac;

    VRRP(VRRPVif& vif, uint32_t vrid);
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

    VRRPVif&	_vif;
    uint32_t	_vrid;
    uint32_t	_priority;
    uint32_t	_interval;
    double	_skew_time;
    double	_master_down_interval;
    bool	_preempt;
    IPS		_ips;
    State	_state;
    XorpTimer	_master_down_timer;
    XorpTimer	_adver_timer;
    bool	_own;
    bool	_disable;
    VRRPPacket	_adv_packet;
    Mac		_source_mac;
    ARPd	_arpd; // XXX use OS proxy arp mechanism?
};

#endif // __VRRP_VRRP_HH__
