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

// $XORP: xorp/vrrp/arpd.hh,v 1.5 2008/12/18 11:34:51 abittau Exp $

#ifndef __VRRP_ARPD_HH__
#define __VRRP_ARPD_HH__

#include <set>

#include "libxorp/ipv4.hh"
#include "vrrp_packet.hh"
#include "vrrp_interface.hh"

/**
 * @short an ARP daemon.
 *
 * This daemon can be configured to "own" several IPs for which it will send out
 * ARP replies when receiving ARP requests.
 */
class ARPd {
public:
    /**
     * @param vif the VRRP interface on which the daemon runs.
     */
    ARPd(VrrpInterface& vif);
    ~ARPd();

    /**
     * Remove all configured IPs.
     */
    void clear();

    /**
     * Add an IP for which ARP replies should be sent.
     *
     * @param ip the IP to add.
     */
    void insert(const IPv4& ip);

    /**
     * Stop the daemon.
     */
    void stop();

    /**
     * Start the daemon.
     */
    void start();

    /**
     * Using this method the caller notifies ARPd that it has finished
     * manipulating the IP addresses.  This way one can clear and add IPs one by
     * one without causing ARPd to stop (if IPs are cleared) and resume if IPs
     * are added.
     */
    void ips_updated();

    /**
     * This method notifies the reception of an ARP packet.
     *
     * @param mac the source MAC address of the packet.
     * @param payload the ARP header and data.
     */
    void recv(const Mac& src, const PAYLOAD& payload);

    /**
     * Sets the MAC address of the ARP daemon, used when generating replies.
     *
     * @param mac the MAC address.
     */
    void set_mac(const Mac& mac);

private:
    typedef set<IPv4>	IPS;

    /**
     * Use this to notify the interface that we no longer need to receive
     * packets.  This can be used for example when no IPs are configured or when
     * the ARPd has been stopped.
     */
    void start_receiving();

    /**
     * Notify the interface that we desire to receive ARP packets.
     */
    void stop_receiving();

    VrrpInterface&  _vif;
    Mac		    _mac;
    IPS		    _ips;
    bool	    _running;
    bool	    _receiving;
};

#endif // __VRRP_ARPD_HH__
