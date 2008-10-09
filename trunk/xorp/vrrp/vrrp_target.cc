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

#ident "$XORP: xorp/vrrp/vrrp_target.cc,v 1.3 2008/10/09 17:46:27 abittau Exp $"

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
		_rtr(rtr),
		_running(true),
		_ifmgr(rtr.eventloop(), fea_target_name.c_str(),
		       rtr.finder_address(), rtr.finder_port()),
		_ifmgr_setup(false),
		_rawlink(&rtr),
		_rawipv4(&rtr),
		_fea(&rtr)
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

	vif = new VRRPVif(*this, ifn, vifn);
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

void
VRRPTarget::send(const string& ifname, const string& vifname, const Mac& src,
		 const Mac& dest, uint32_t ether, const PAYLOAD& payload)
{
    bool rc = _rawlink.send_send(fea_target_name.c_str(),
				 ifname, vifname, src, dest, ether, payload,
				 callback(this, &VRRPTarget::xrl_cb));

    if (!rc)
	XLOG_FATAL("Cannot send raw data");
}

void
VRRPTarget::join_mcast(const string& ifname, const string& vifname)
{
    bool rc;
    XrlRawPacket4V0p1Client::RegisterReceiverCB cb =
				callback(this, &VRRPTarget::xrl_cb);

    uint32_t proto = VRRPPacket::IPPROTO_VRRP;
    const IPv4& ip = VRRPPacket::mcast_group;

    rc = _rawipv4.send_register_receiver(fea_target_name.c_str(),
				         _rtr.instance_name(), ifname,
					 vifname, proto, false, cb);
    if (!rc) {
	XLOG_FATAL("Cannot register receiver");
	return;
    }
    rc = _rawipv4.send_join_multicast_group(fea_target_name.c_str(),
					    _rtr.instance_name(), ifname,
					    vifname, proto, ip, cb);
    if (!rc)
	XLOG_FATAL("Cannot join mcast group");
}

void
VRRPTarget::leave_mcast(const string& ifname, const string& vifname)
{
    bool rc;
    XrlRawPacket4V0p1Client::RegisterReceiverCB cb =
				callback(this, &VRRPTarget::xrl_cb);

    uint32_t proto = VRRPPacket::IPPROTO_VRRP;
    const IPv4& ip = VRRPPacket::mcast_group;

    rc = _rawipv4.send_leave_multicast_group(fea_target_name.c_str(),
					     _rtr.instance_name(), ifname,
					     vifname, proto, ip, cb);
    if (!rc) {
	XLOG_FATAL("Cannot leave mcast group");
	return;
    }
    rc = _rawipv4.send_unregister_receiver(fea_target_name.c_str(),
					   _rtr.instance_name(), ifname,
					       vifname, proto, cb);
    if (!rc)
	XLOG_FATAL("Cannot unregister receiver");
}

void
VRRPTarget::add_mac(const string& ifname, const Mac& mac)
{
    if (!_fea.send_create_mac(fea_target_name.c_str(), ifname, mac,
			      callback(this, &VRRPTarget::xrl_cb)))
	XLOG_FATAL("Cannot add MAC");
}

void
VRRPTarget::delete_mac(const string& ifname, const Mac& mac)
{
    if (!_fea.send_delete_mac(fea_target_name.c_str(), ifname, mac,
			      callback(this, &VRRPTarget::xrl_cb)))
	XLOG_FATAL("Cannot delete MAC");
}

void
VRRPTarget::xrl_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY())
	XLOG_FATAL("XRL error: %s", xrl_error.str().c_str());
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

XrlCmdError
VRRPTarget::raw_packet4_client_0_1_recv(
        // Input values,
        const string&   if_name,
        const string&   vif_name,
        const IPv4&     src_address,
        const IPv4&     dst_address,
        const uint32_t& ip_protocol,
        const int32_t&  ip_ttl,
        const int32_t&  ip_tos,
        const bool&     ip_router_alert,
        const bool&     ip_internet_control,
        const vector<uint8_t>&  payload)
{
    UNUSED(ip_tos);
    UNUSED(ip_router_alert);
    UNUSED(ip_internet_control);

    VRRPVif* vif = find_vif(if_name, vif_name);
    if (!vif) {
	XLOG_WARNING("Cannot find IF %s VIF %s",
		     if_name.c_str(), vif_name.c_str());

	return XrlCmdError::OKAY();
    }

    if (dst_address != VRRPPacket::mcast_group) {
	XLOG_WARNING("Received stuff for unknown IP %s",
		     dst_address.str().c_str());

	return XrlCmdError::OKAY();
    }

    if (ip_protocol != VRRPPacket::IPPROTO_VRRP) {
	XLOG_WARNING("Unknown protocol %u", ip_protocol);

	return XrlCmdError::OKAY();
    }

    if (ip_ttl != 255) {
	XLOG_WARNING("Bad TTL %d", ip_ttl);

	return XrlCmdError::OKAY();
    }

    vif->recv(src_address, payload);

    return XrlCmdError::OKAY();
}
