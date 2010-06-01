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

// $XORP: xorp/vrrp/vrrp.hh,v 1.10 2008/12/18 11:34:51 abittau Exp $

#ifndef __VRRP_VRRP_HH__
#define __VRRP_VRRP_HH__

#include <set>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/timer.hh"
#include "libxorp/mac.hh"
#include "libxorp/eventloop.hh"
#include "vrrp_packet.hh"
#include "vrrp_vif.hh"

//#include "arpd.hh"

#define MAX_VRRP_SIZE (20 + VRRP_MAX_PACKET_SIZE)

/**
 * @short VRRP protocol implementation.
 *
 * This class implements the state machine of the VRRP protocol.  It receive
 * events and responds to them.
 */
class Vrrp {
public:
    static const Mac mcast_mac;

    /**
     * @param vif the VRRP interface on which this instance is running.
     * @param e eventloop.
     * @param vrid the VRRP id of this instance.
     */
    Vrrp(VrrpVif& vif, EventLoop& e, uint32_t vrid);
    ~Vrrp();

    /**
     * Set the priority of the router.
     *
     * @param priority the priority of this router.
     */
    void	    set_priority(uint32_t priority);

    /**
     * Set the advertisement interval.
     * 
     * @param interval the interval for sending advertisements.
     */
    void	    set_interval(uint32_t interval);

    /**
     * Set whether a high priority backup router should preempt backup routers
     * with lower priority acting as masters.
     *
     * @param preempt whether a high priority backup should preempt a low one.
     */
    void	    set_preempt(bool preempt);

    /**
     * Stop or start this VRRP instance.
     *
     * @param disable stop this VRRP instance.
     */
    void	    set_disable(bool disable);

    /**
     * Add an IP to the virtual router.
     *
     * @param ip IP address to add to the virtual router.
     */
    void	    add_ip(const IPv4& ip);

    void set_prefix(const IPv4& ip, uint32_t prefix);

    /**
     * Delete an IP from the virtual router.
     * 
     * @param ip IP address to remove from the virtual router.
     */
    void	    delete_ip(const IPv4& ip);

    /**
     * Check whether this instance is running.
     *
     * @return whether this VRRP instance is running.
     */
    bool	    running() const;

    /**
     * Obtain the priority of this router.
     *
     * @return priority of router.
     */
    uint32_t	    priority() const;

    /**
     * Start VRRP processing.
     */
    void	    start();

    /**
     * Stop the protocol.
     */
    void	    stop();

    /**
     * Check whether the router owns all IPs configured for the virtual router.
     * This must be called after updating IP addresses.
     */
    //void	    check_ownership();

    /**
     * Must be called when a VRRP packet is received.
     *
     * @param from IP address sending the packet.
     * @param vh the VRRP packet received, starting from the VRRP header.
     */
    void	    recv(const IPv4& from, const VrrpHeader& vh);

    /**
     * Obtain an instance to the ARP daemon.
     *
     * @return an instance of the ARP daemon.
     */
    //ARPd&	    arpd();

    /**
     * Returns information about the state of the protocol.
     *
     * @param state the state of the router (output parameter).
     * @param master the IP address of the master (output parameter).
     */
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

    /**
     * Calculate advertisement intervals based on protocol parameters.  This is
     * called when any of these parameters (e.g., whether the router owns the
     * IPs) change.
     */
    void setup_intervals();

    /**
     * Setup timers based on intervals.  Must be called when intervals change.
     *
     * @param skew whether the skew time should be added when setting the timer.
     */
    void setup_timers(bool skew = false);

    /**
     * Stop all timers.
     */
    void cancel_timers();

    /**
     * Send gratuitous ARPs for all the IP addresses of this virtual router.
     */
    void send_arps();

    /**
     * Send a gratitious ARP for a specific IP.
     *
     * @param ip the IP address to send the ARP for.
     */
    void send_arp(const IPv4& ip);

    /**
     * Send an advertisement with given priority.
     *
     * @param priority of the advertisement.
     */
    void send_advertisement(uint32_t priority);

    /**
     * Send a VRRP advertisement.  The parameters are taken from member
     * variables.
     *
     */
    void send_advertisement();

    /**
     * Become the master router.
     */
    void become_master();

    /**
     * Become a backup router.
     */
    void become_backup();

    /**
     * This is called when this router thinks the master router is dead.  That
     * is, when the timer waiting for an advertisement expires.
     *
     * @return whether to reschedule the timer.
     */
    bool master_down_expiry();

    /**
     * This is called when the next advertisement should be sent.
     *
     * @return whether to renew the timer.
     */
    bool adver_expiry();

    /**
     * Called when receiving an advertisement in the master state.
     *
     * @param from the IP that sent the advertisement.
     * @param priority the priority in the VRRP advertisement.
     */
    void recv_adver_master(const IPv4& from, uint32_t priority);

    /**
     * Called when an advertisement is received in the backup state.
     *
     * @param priority the priority within the VRRP advertisement.
     */
    void recv_adver_backup(uint32_t priority);

    /**
     * Create the advertisement packet.
     *
     * @param priority the priority to be announced in the advertisement.
     */
    void prepare_advertisement(uint32_t priority);

    /**
     * Notification of advertisement reception.
     *
     * @param from IP address of sender.
     * @param priority the priority in the VRRP header.
     */
    void recv_advertisement(const IPv4& from, uint32_t priority);

    /**
     * Check whether the IPs in the VRRP packet match those configured
     *
     * @return true on exact matches.
     * @param vh the VRRP header.
     */
    bool check_ips(const VrrpHeader& vh);

    IPv4	    _last_adv;
    VrrpVif&  _vif;
    uint32_t	    _vrid;
    uint32_t	    _priority;
    uint32_t	    _interval;
    double	    _skew_time;
    double	    _master_down_interval;
    bool	    _preempt;
    set<IPv4>       _ips;
    map<uint32_t, uint32_t> _prefixes; // map IP addr to prefix value.
    State	    _state;
    XorpTimer	    _master_down_timer;
    XorpTimer	    _adver_timer;
    //bool	    _own;
    bool	    _disable;
    VrrpPacket	    _adv_packet;
    Mac		    _source_mac;
    //ARPd	    _arpd; // XXX use OS proxy arp mechanism?
};

#endif // __VRRP_VRRP_HH__
