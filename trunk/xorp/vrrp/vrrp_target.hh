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

// $XORP: xorp/vrrp/vrrp_target.hh,v 1.1 2008/10/09 17:40:58 abittau Exp $

#ifndef __VRRP_VRRP_TARGET_HH__
#define __VRRP_VRRP_TARGET_HH__

#include "libxipc/xrl_router.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "xrl/targets/vrrp_base.hh"
#include "vrrp.hh"
#include "vrrp_vif.hh"

class VRRPTarget : public XrlVrrpTargetBase, public IfMgrHintObserver {
public:
    static const string vrrp_target_name;
    static const string fea_target_name;

    VRRPTarget(XrlRouter& rtr);
    ~VRRPTarget();

    bool running() const;
    void tree_complete();
    void updates_made();

    static EventLoop& eventloop();

protected:
    XrlCmdError common_0_1_get_target_name(
        // Output values,
        string& name);

    XrlCmdError common_0_1_get_version(
        // Output values,
        string& version);

    XrlCmdError common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError vrrp_0_1_add_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid);

    XrlCmdError vrrp_0_1_delete_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid);

    XrlCmdError vrrp_0_1_set_priority(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& priority);

    XrlCmdError vrrp_0_1_set_interval(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& interval);

    XrlCmdError vrrp_0_1_set_preempt(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     preempt);

    XrlCmdError vrrp_0_1_set_disable(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     disable);

    XrlCmdError vrrp_0_1_add_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip);

    XrlCmdError vrrp_0_1_delete_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip);

private:
    typedef map<string, VRRPVif*>   VIFS; // vifname -> VRRPVif
    typedef map<string, VIFS*>	    IFS;  // ifname -> VIFS

    void	shutdown();
    void	start();
    void	check_interfaces();
    void	check_vif(VRRPVif& vif);
    void	add_vrid(const string& ifn, const string& vifn, uint32_t id);
    void	delete_vrid(const string& ifn, const string& vifn, uint32_t id);
    VRRP&	find_vrid(const string& ifn, const string& vifn, uint32_t id);
    VRRP*	find_vrid_ptr(const string& ifn, const string& vifn,
			      uint32_t id);
    VRRPVif*	find_vif(const string& ifn, const string& vifn,
			 bool add = false);

    bool		    _running;
    IFS			    _ifs;
    IfMgrXrlMirror	    _ifmgr;
    bool		    _ifmgr_setup;
    static EventLoop*	    _eventloop;
};

#endif // __VRRP_VRRP_TARGET_HH__
