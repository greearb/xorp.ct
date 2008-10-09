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

// $XORP: xorp/vrrp/arpd.hh,v 1.1 2008/10/09 17:49:57 abittau Exp $

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
