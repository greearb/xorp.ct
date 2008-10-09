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

#ident "$XORP$"

#include "vrrp_module.h"
#include "libxorp/status_codes.h"
#include "vrrp_target.hh"
#include "vrrp_exception.hh"

const string VRRPTarget::vrrp_target_name = "vrrp";

VRRPTarget::VRRPTarget(XrlRouter& rtr) : XrlVrrpTargetBase(&rtr),
		_running(true)
{
}

VRRPTarget::~VRRPTarget()
{
    for (VRRPS::iterator i = _vrrps.begin(); i != _vrrps.end(); ++i)
	delete i->second;
}

bool
VRRPTarget::running() const
{
    return _running;
}

void
VRRPTarget::shutdown()
{
    _running = false;
}

VRRP&
VRRPTarget::find(const string& ifn, const string& vifn, uint32_t id)
{
    VRRPKey key(ifn, vifn, id);

    VRRP* v = find(key);

    if (!v)
	xorp_throw(VRRPException, string("Cannot find: ") + key.str());

    return *v;
}

VRRP*
VRRPTarget::find(const VRRPKey& k)
{
    VRRPS::iterator i = _vrrps.find(k);

    if (i == _vrrps.end())
	return NULL;

    return i->second;
}

void
VRRPTarget::insert(const string& ifn, const string& vifn, uint32_t id)
{
    VRRPKey key(ifn, vifn, id);

    if (find(key))
	xorp_throw(VRRPException, string("Already exists: ") + key.str());

    _vrrps[key] = new VRRP(key);
}

void
VRRPTarget::erase(const string& ifn, const string& vifn, uint32_t id)
{
    VRRPKey key(ifn, vifn, id);

    VRRP* v = find(key);
    if (!v)
	xorp_throw(VRRPException, string("Cannot find: ") + key.str());

    _vrrps.erase(key);
    delete v;
}

XrlCmdError
VRRPTarget::common_0_1_get_target_name(
        // Output values,
        string& name)
{
    name = vrrp_target_name;

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::common_0_1_get_version(
        // Output values,
        string& version)
{
    version = XORP_MODULE_VERSION;

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason)
{
    if (running()) {
	status = PROC_READY;
	reason = "running";
    } else {
	status = PROC_SHUTDOWN;
	reason = "dying";
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::common_0_1_shutdown()
{
    shutdown();

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_add_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid)
{
    try {
	insert(ifname, vifname, vrid);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_delete_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid)
{
    try {
	erase(ifname, vifname, vrid);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_set_priority(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& priority)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.set_priority(priority);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_set_interval(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& interval)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.set_interval(interval);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_set_preempt(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     preempt)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.set_preempt(preempt);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_set_disable(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     disable)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.set_disable(disable);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_add_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.add_ip(ip);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VRRPTarget::vrrp_0_1_delete_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip)
{
    try {
	VRRP& v = find(ifname, vifname, vrid);

	v.delete_ip(ip);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}
