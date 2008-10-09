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

// $XORP: xorp/vrrp/vrrp_interface.hh,v 1.1 2008/10/09 17:55:52 abittau Exp $

#ifndef __VRRP_VRRP_INTERFACE_HH__
#define __VRRP_VRRP_INTERFACE_HH__

#include <string>
#include <set>

#include "libxorp/ipv4.hh"
#include "libxorp/mac.hh"
#include "vrrp_packet.hh"

class VRRPInterface {
public:
    virtual ~VRRPInterface() {}

    virtual bool	    own(const IPv4& addr) = 0;
    virtual bool	    ready() const = 0;
    virtual const IPv4&	    addr() const = 0;
    virtual void	    send(const Mac& src, const Mac& dst, uint32_t ether,
				 const PAYLOAD& payload) = 0;
    virtual void	    join_mcast() = 0;
    virtual void	    leave_mcast() = 0;
    virtual void	    add_mac(const Mac& mac) = 0;
    virtual void	    delete_mac(const Mac& mac) = 0;

    // XXX these should be handled elsewhere, or differently.  VRRP doesn't have
    // to "know" about ARPs.
    virtual void	    start_arps() = 0;
    virtual void	    stop_arps() = 0;
};

#endif // __VRRP_VRRP_INTERFACE_HH__
