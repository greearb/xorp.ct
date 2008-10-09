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

#ident "$XORP: xorp/vrrp/vrrp_target.cc,v 1.1 2008/10/09 17:40:58 abittau Exp $"

#include <sstream>

#include "vrrp_module.h"
#include "libxorp/status_codes.h"
#include "vrrp_target.hh"
#include "vrrp_exception.hh"

const string VRRPTarget::vrrp_target_name   = "vrrp";
const string VRRPTarget::fea_target_name    = "fea";
EventLoop*   VRRPTarget::_eventloop	    = NULL;

namespace {

string
vrid_error(const string& msg, const string& ifn, const string& vifn,
	   uint32_t id)
{
    ostringstream oss;

    oss << msg
        << " (ifname " << ifn
	<< " vifname " << vifn
	<< " vrid " << id
	<< ")"
	;

    return oss.str();
}

} // anonymous namespace

VRRPTarget::VRRPTarget(XrlRouter& rtr) : XrlVrrpTargetBase(&rtr),
		_running(true),
		_ifmgr(rtr.eventloop(), fea_target_name.c_str(),
		       rtr.finder_address(), rtr.finder_port()),
		_ifmgr_setup(false)
{
    _eventloop = &rtr.eventloop();

    _ifmgr.attach_hint_observer(this);

    start();
}

VRRPTarget::~VRRPTarget()
{
    _ifmgr.detach_hint_observer(this);

    for (IFS::iterator i = _ifs.begin(); i != _ifs.end(); ++i) {
	VIFS* v = i->second;

	for (VIFS::iterator j = v->begin(); j != v->end(); ++j)
	    delete j->second;

	delete v;
    }
}

EventLoop&
VRRPTarget::eventloop()
{
    if (!_eventloop)
	xorp_throw(VRRPException, "no eventloop");

    return *_eventloop;
}

void
VRRPTarget::start()
{
    if (_ifmgr.startup() != XORP_OK)
	xorp_throw(VRRPException, "Can't startup fea mirror");
}

bool
VRRPTarget::running() const
{
    return _running;
}

void
VRRPTarget::shutdown()
{
    if (_ifmgr.shutdown() != XORP_OK)
	xorp_throw(VRRPException, "Can't shutdown fea mirror");

    _running = false;
}

VRRP&
VRRPTarget::find_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    VRRP* v = find_vrid_ptr(ifn, vifn, id);
    if (!v)
	xorp_throw(VRRPException, vrid_error("Cannot find", ifn, vifn, id));

    return *v;
}

VRRP*
VRRPTarget::find_vrid_ptr(const string& ifn, const string& vifn, uint32_t id)
{
    VRRPVif* x = find_vif(ifn, vifn);
    if (!x)
	return NULL;

    return x->find_vrid(id);
}

void
VRRPTarget::add_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    if (find_vrid_ptr(ifn, vifn, id))
	xorp_throw(VRRPException, vrid_error("Already exists", ifn, vifn, id));

    VRRPVif* x = find_vif(ifn, vifn, true);
    XLOG_ASSERT(x);

    x->add_vrid(id);
}

void
VRRPTarget::delete_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    VRRP* v = find_vrid_ptr(ifn, vifn, id);
    if (!v)
	xorp_throw(VRRPException, vrid_error("Cannot find", ifn, vifn, id));

    VRRPVif* x = find_vif(ifn, vifn);
    XLOG_ASSERT(x);

    x->delete_vrid(id);
}

VRRPVif*
VRRPTarget::find_vif(const string& ifn, const string& vifn, bool add)
{
    VIFS* v	    = NULL;
    VRRPVif* vif    = NULL;
    bool added	    = false;

    IFS::iterator i = _ifs.find(ifn);
    if (i == _ifs.end()) {
	if (!add)
	    return NULL;

	v = new VIFS;
	_ifs[ifn] = v;
	added = true;
    } else
	v = i->second;

    VIFS::iterator j = v->find(vifn);
    if (j == v->end()) {
	if (!add)
	    return NULL;

	vif = new VRRPVif(ifn, vifn);
	v->insert(make_pair(vifn, vif));
	added = true;
    } else
	vif = j->second;

    if (added)
	check_interfaces();

    return vif;
}

void
VRRPTarget::tree_complete()
{
    _ifmgr_setup = true;
    check_interfaces();
}

void
VRRPTarget::updates_made()
{
    check_interfaces();
}

void
VRRPTarget::check_interfaces()
{
    XLOG_ASSERT(_ifmgr_setup);

    for (IFS::iterator i = _ifs.begin(); i != _ifs.end(); ++i) {
	VIFS* vifs = i->second;

	for (VIFS::iterator j = vifs->begin(); j != vifs->end(); ++j) {
	    VRRPVif* vif = j->second;

	    vif->configure(_ifmgr.iftree());
	}
    }
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
	add_vrid(ifname, vifname, vrid);
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
	delete_vrid(ifname, vifname, vrid);
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
	VRRP& v = find_vrid(ifname, vifname, vrid);

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
	VRRP& v = find_vrid(ifname, vifname, vrid);

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
	VRRP& v = find_vrid(ifname, vifname, vrid);

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
	VRRP& v = find_vrid(ifname, vifname, vrid);

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
	VRRP& v = find_vrid(ifname, vifname, vrid);

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
	VRRP& v = find_vrid(ifname, vifname, vrid);

	v.delete_ip(ip);
    } catch(const VRRPException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}
