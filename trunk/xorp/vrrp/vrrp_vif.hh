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

#ifndef __VRRP_VRRP_VIF_HH__
#define __VRRP_VRRP_VIF_HH__

#include <string>
#include <set>

#include "libxorp/ipv4.hh"
#include "libfeaclient/ifmgr_atoms.hh"
#include "vrrp.hh"

class VRRPVif {
public:
    VRRPVif(const string& ifname, const string& vifname);
    ~VRRPVif();

    bool	    own(const IPv4& addr);
    VRRP*	    find_vrid(uint32_t vrid);
    void	    add_vrid(uint32_t vrid);
    void	    delete_vrid(uint32_t vrid);
    bool	    ready() const;
    void	    configure(const IfMgrIfTree& conf);
    const IPv4&	    addr() const;

private:
    typedef set<IPv4>		    IPS;
    typedef map<uint32_t, VRRP*>    VRRPS;

    void		    set_ready(bool ready);
    template <class T> bool is_enabled(const T* obj);

    string  _ifname;
    string  _vifname;
    bool    _ready;	// is it configured?
    IPS	    _ips;	// IPs assigned to this vif
    VRRPS   _vrrps;	// VRRPs enabled on this vif
};

#endif // __VRRP_VRRP_VIF_HH__
