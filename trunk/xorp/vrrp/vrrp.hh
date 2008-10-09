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

// $XORP: xorp/devnotes/template.hh,v 1.10 2008/07/23 05:09:59 pavlin Exp $

#ifndef __VRRP_VRRP_HH__
#define __VRRP_VRRP_HH__

#include <set>
#include <string>

#include "libxorp/ipv4.hh"

class VRRPKey {
public:
    VRRPKey(const string& ifname, const string& vifname, uint32_t id);

    bool    operator==(const VRRPKey& rhs) const;
    bool    operator<(const VRRPKey& lfs) const;
    string  str() const;

private:
    string	_ifname;
    string	_vifname;
    uint32_t	_vrid;
};

class VRRP {
public:
    VRRP(const VRRPKey& key);

    void set_priority(uint32_t priority);
    void set_interval(uint32_t interval);
    void set_preempt(bool preempt);
    void set_disable(bool disable);
    void add_ip(const IPv4& ip);
    void delete_ip(const IPv4& ip);

private:
    typedef set<IPv4>	IPS;

    VRRPKey	_key;
    uint32_t	_priority;
    uint32_t	_interval;
    bool	_preempt;
    bool	_disable;
    IPS		_ips;
};

#endif // __VRRP_VRRP_HH__
