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

// $XORP: xorp/vrrp/vrrp_interface.hh,v 1.4 2008/12/18 11:34:51 abittau Exp $

#ifndef __VRRP_VRRP_INTERFACE_HH__
#define __VRRP_VRRP_INTERFACE_HH__

#include <string>
#include <set>

#include "libxorp/ipv4.hh"
#include "libxorp/mac.hh"
#include "vrrp_packet.hh"

/**
 * @short A network interface on which VRRP runs.
 *
 * This interface provides the means via which VRRP receives and send packets.
 */
class VrrpInterface {
public:
    virtual ~VrrpInterface() {}

    /**
     * Determines whether the router owns an IP address.  If the router has this
     * IP address configured as the real IP address of one of its interfaces,
     * then it owns it, else it does not.
     *
     * @return whether the router owns the IP address.
     * @param addr the IP address to check for.
     */
    virtual bool	    own(const IPv4& addr) = 0;

    /**
     * Check whether the network interface is up.
     *
     * @return whether the interface is up and ready.
     */
    virtual bool	    ready() const = 0;

    /**
     * Obtain the primary IP address of the interface.  This is the first one
     * configured.
     *
     * @return the primary IP address of the interface.  The first configured.
     */
    virtual const IPv4&	    addr() const = 0;

    /**
     * Transmits a L2 packet.
     *
     * @param src the source MAC address.
     * @param dst the destination MAC address.
     * @param ether the ethernet type.
     * @param payload the data following the MAC header.
     */
    virtual void	    send(const Mac& src, const Mac& dst, uint32_t ether,
				 const PAYLOAD& payload) = 0;

    /**
     * Join the VRRP multicast group on this interface.
     */
    virtual void	    join_mcast() = 0;

    /**
     * Leave the VRRP multicast group on this interface.
     */
    virtual void	    leave_mcast() = 0;

    /**
     * Add a MAC address to this interface.
     *
     * @param mac MAC address to add.
     */
    virtual void	    add_mac(const Mac& mac) = 0;

    /**
     * Delete a MAC address from this interface.
     *
     * @param mac MAC address to delete from this interface.
     */
    virtual void	    delete_mac(const Mac& mac) = 0;

    // XXX these should be handled elsewhere, or differently.  VRRP doesn't have
    // to "know" about ARPs.

    /**
     * Start the reception of ARP packets.
     */
    virtual void	    start_arps() = 0;

    /**
     * Stop the reception of ARP packets.
     */
    virtual void	    stop_arps() = 0;
};

#endif // __VRRP_VRRP_INTERFACE_HH__
