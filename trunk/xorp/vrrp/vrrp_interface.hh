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

// $XORP$

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
