// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2011 XORP, Inc and Others
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





#include "vrrp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "vrrp_target.hh"
#include "vrrp_exception.hh"

const string VrrpTarget::vrrp_target_name   = "vrrp";
const string VrrpTarget::fea_target_name    = "fea";

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

VrrpTarget::VrrpTarget(XrlRouter& rtr)
    : XrlVrrpTargetBase(&rtr),
      _rtr(rtr),
      _running(true),
      _ifmgr(rtr.eventloop(), fea_target_name.c_str(),
	     rtr.finder_address(), rtr.finder_port()),
      _ifmgr_setup(false),
      _rawlink(&rtr),
      _rawipv4(&rtr),
      _fea(&rtr),
      _xrls_pending(0)
{
    _ifmgr.attach_hint_observer(this);

    // When changing MAC, Linux brings the interface down and up.  This will
    // cause VRRP to stop and start, which causes changing MACs yet again.  To
    // avoid this loop, we delay configuration changes so we don't see the
    // interface going down on a MAC change.
    _ifmgr.delay_updates(TimeVal(1, 0));

    start();
}

VrrpTarget::~VrrpTarget()
{
    shutdown();
}

EventLoop&
VrrpTarget::eventloop()
{
    return _rtr.eventloop();
}

void
VrrpTarget::start()
{
    if (_ifmgr.startup() != XORP_OK)
	xorp_throw(VrrpException, "Can't startup Vrrp");
}

bool
VrrpTarget::running() const
{
    return _running || _rtr.pending() || _xrls_pending > 0;
}

void
VrrpTarget::shutdown()
{
    if (_running) {
	_ifmgr.detach_hint_observer(this);
	if (_ifmgr.shutdown() != XORP_OK)
	    xorp_throw(VrrpException, "Can't shutdown fea mirror");
    }

    for (IFS::iterator i = _ifs.begin(); i != _ifs.end(); ++i) {
	VIFS* v = i->second;

	for (VIFS::iterator j = v->begin(); j != v->end(); ++j)
	    delete j->second;

	delete v;
    }
    _ifs.clear();

    _running = false;
}

Vrrp&
VrrpTarget::find_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    Vrrp* v = find_vrid_ptr(ifn, vifn, id);
    if (!v)
	xorp_throw(VrrpException, vrid_error("Cannot find", ifn, vifn, id));

    return *v;
}

Vrrp*
VrrpTarget::find_vrid_ptr(const string& ifn, const string& vifn, uint32_t id)
{
    VrrpVif* x = find_vif(ifn, vifn);
    if (!x)
	return NULL;

    return x->find_vrid(id);
}

void
VrrpTarget::add_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    if (find_vrid_ptr(ifn, vifn, id))
	xorp_throw(VrrpException, vrid_error("Already exists", ifn, vifn, id));

    VrrpVif* x = find_vif(ifn, vifn, true);
    XLOG_ASSERT(x);

    x->add_vrid(id);
}

void
VrrpTarget::delete_vrid(const string& ifn, const string& vifn, uint32_t id)
{
    Vrrp* v = find_vrid_ptr(ifn, vifn, id);
    if (!v)
	xorp_throw(VrrpException, vrid_error("Cannot find", ifn, vifn, id));

    VrrpVif* x = find_vif(ifn, vifn);
    XLOG_ASSERT(x);

    x->delete_vrid(id);
}

VrrpVif*
VrrpTarget::find_vif(const string& ifn, const string& vifn, bool add)
{
    VIFS* v	    = NULL;
    VrrpVif* vif    = NULL;
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

	vif = new VrrpVif(*this, ifn, vifn);
	v->insert(make_pair(vifn, vif));
	added = true;
    } else
	vif = j->second;

    if (added)
	check_interfaces();

    return vif;
}

void
VrrpTarget::tree_complete()
{
    _ifmgr_setup = true;
    check_interfaces();
}

void
VrrpTarget::updates_made()
{
    check_interfaces();
}

void
VrrpTarget::check_interfaces()
{
    XLOG_ASSERT(_ifmgr_setup);

    for (IFS::iterator i = _ifs.begin(); i != _ifs.end(); ++i) {
	VIFS* vifs = i->second;

	for (VIFS::iterator j = vifs->begin(); j != vifs->end(); ++j) {
	    VrrpVif* vif = j->second;

	    vif->configure(_ifmgr.iftree());
	}
    }
}

void
VrrpTarget::send(const string& ifname, const string& vifname, const Mac& src,
		 const Mac& dest, uint32_t ether, const PAYLOAD& payload)
{
    VrrpVif* vif = find_vif(ifname, vifname);
    XLOG_ASSERT(vif);

    bool rc = _rawlink.send_send(fea_target_name.c_str(),
				 ifname, vifname, src, dest, ether, payload,
				 callback(vif, &VrrpVif::xrl_cb));

    if (!rc)
	XLOG_FATAL("Cannot send raw data");
}

void
VrrpTarget::join_mcast(const string& ifname, const string& vifname)
{
    bool rc;
    XrlRawPacket4V0p1Client::RegisterReceiverCB cb =
				callback(this, &VrrpTarget::xrl_cb);

    uint32_t proto = IPPROTO_VRRP;
    const IPv4& ip = VrrpPacket::mcast_group;

    rc = _rawipv4.send_register_receiver(fea_target_name.c_str(),
				         _rtr.instance_name(), ifname,
					 vifname, proto, false, cb);
    if (!rc) {
	XLOG_FATAL("Cannot register receiver");
	return;
    }
    _xrls_pending++;

    rc = _rawipv4.send_join_multicast_group(fea_target_name.c_str(),
					    _rtr.instance_name(), ifname,
					    vifname, proto, ip, cb);
    if (!rc)
	XLOG_FATAL("Cannot join mcast group");

    _xrls_pending++;
}

void
VrrpTarget::leave_mcast(const string& ifname, const string& vifname)
{
    bool rc;
    XrlRawPacket4V0p1Client::RegisterReceiverCB cb =
				callback(this, &VrrpTarget::xrl_cb);

    uint32_t proto = IPPROTO_VRRP;
    const IPv4& ip = VrrpPacket::mcast_group;

    rc = _rawipv4.send_leave_multicast_group(fea_target_name.c_str(),
					     _rtr.instance_name(), ifname,
					     vifname, proto, ip, cb);
    if (!rc) {
	XLOG_FATAL("Cannot leave mcast group");
	return;
    }
    _xrls_pending++;

    rc = _rawipv4.send_unregister_receiver(fea_target_name.c_str(),
					   _rtr.instance_name(), ifname,
					       vifname, proto, cb);
    if (!rc)
	XLOG_FATAL("Cannot unregister receiver");

    _xrls_pending++;
}

void
VrrpTarget::start_arps(const string& ifname, const string& vifname)
{
    string filter;

    if (!_rawlink.send_register_receiver(fea_target_name.c_str(),
					 _rtr.instance_name(),
					 ifname, vifname, ETHERTYPE_ARP,
					 filter, false, 
					 callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot register arp receiver");

    _xrls_pending++;
}

void
VrrpTarget::stop_arps(const string& ifname, const string& vifname)
{
    string filter;

    if (!_rawlink.send_unregister_receiver(fea_target_name.c_str(),
					   _rtr.instance_name(),
					   ifname, vifname, ETHERTYPE_ARP,
					   filter,
					   callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot unregister arp receiver");

    _xrls_pending++;
}

void
VrrpTarget::add_mac(const string& ifname, const Mac& mac)
{
    if (!_fea.send_create_mac(fea_target_name.c_str(), ifname, mac,
			      callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot add MAC");
    _xrls_pending++;
}

void
VrrpTarget::add_ip(const string& ifname, const IPv4& ip, const uint32_t prefix)
{
    if (!_fea.send_create_address_atomic(fea_target_name.c_str(), ifname, ifname,
					 ip, prefix,
					 callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot add IP");

    _xrls_pending++;
}

void
VrrpTarget::delete_mac(const string& ifname, const Mac& mac)
{
    if (!_fea.send_delete_mac(fea_target_name.c_str(), ifname, mac,
			      callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot delete MAC");

    _xrls_pending++;
}

void
VrrpTarget::delete_ip(const string& ifname, const IPv4& ip)
{
    if (!_fea.send_delete_address_atomic(fea_target_name.c_str(), ifname, ifname, ip,
					 callback(this, &VrrpTarget::xrl_cb)))
	XLOG_FATAL("Cannot delete IP");

    _xrls_pending++;
}

void
VrrpTarget::xrl_cb(const XrlError& xrl_error)
{
    _xrls_pending--;
    XLOG_ASSERT(_xrls_pending >= 0);

    if (xrl_error != XrlError::OKAY())
	XLOG_FATAL("XRL error: %s", xrl_error.str().c_str());
}

XrlCmdError
VrrpTarget::common_0_1_get_target_name(
        // Output values,
        string& name)
{
    name = vrrp_target_name;

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::common_0_1_get_version(
        // Output values,
        string& version)
{
    version = XORP_MODULE_VERSION;

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string& reason)
{
    if (_running) {
	status = PROC_READY;
	reason = "running";
    } else {
	status = PROC_SHUTDOWN;
	reason = "dying";
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::common_0_1_shutdown()
{
    shutdown();

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::common_0_1_startup()
{
    start();
    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_add_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid)
{
    try {
	add_vrid(ifname, vifname, vrid);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_delete_vrid(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid)
{
    try {
	delete_vrid(ifname, vifname, vrid);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_set_priority(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& priority)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.set_priority(priority);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_set_interval(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const uint32_t& interval)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.set_interval(interval);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_set_preempt(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     preempt)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.set_preempt(preempt);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_set_disable(
        // Input values,
        const string&   ifname,
        const string&   vifname,
	const uint32_t& vrid,
        const bool&     disable)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.set_disable(disable);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_add_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.add_ip(ip);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_set_prefix (
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip,
	const uint32_t& prefix_len) {
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.set_prefix(ip, prefix_len);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    return XrlCmdError::OKAY();
}


XrlCmdError
VrrpTarget::vrrp_0_1_delete_ip(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        const IPv4&     ip)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.delete_ip(ip);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_get_vrid_info(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        const uint32_t& vrid,
        // Output values,
        string& state,
        IPv4&   master)
{
    try {
	Vrrp& v = find_vrid(ifname, vifname, vrid);

	v.get_info(state, master);
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_get_vrids(
        // Input values,      
        const string&   ifname,
        const string&   vifname,
        // Output values,     
        XrlAtomList&    vrids)
{
    try {
	VrrpVif* vif = find_vif(ifname, vifname);
	if (!vif)
	    xorp_throw(VrrpException, "unknown vif");

	typedef VrrpVif::VRIDS VRIDS;

	VRIDS tmp;
	vif->get_vrids(tmp);

	for (VRIDS::iterator i = tmp.begin(); i != tmp.end(); ++i) {
	    uint32_t vrid = *i;

	    vrids.append(XrlAtom(vrid));
	}
    } catch(const VrrpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::vrrp_0_1_get_ifs(
        // Output values,
        XrlAtomList&    ifs)
{
    for (IFS::iterator i = _ifs.begin(); i != _ifs.end(); ++i)
	ifs.append(XrlAtom(i->first));

    return XrlCmdError::OKAY();
}
                            
XrlCmdError
VrrpTarget::vrrp_0_1_get_vifs(
        // Input values,    
        const string&   ifname,
        // Output values,
        XrlAtomList&    vifs)
{
    IFS::iterator i = _ifs.find(ifname);
    if (i == _ifs.end())
	return XrlCmdError::COMMAND_FAILED("Can't find interface");

    VIFS* v = i->second;

    for (VIFS::iterator i = v->begin(); i != v->end(); ++i)
	vifs.append(XrlAtom(i->first));

    return XrlCmdError::OKAY();
}

XrlCmdError
VrrpTarget::raw_packet4_client_0_1_recv(
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

    VrrpVif* vif = find_vif(if_name, vif_name);
    if (!vif) {
	XLOG_WARNING("Cannot find IF %s VIF %s",
		     if_name.c_str(), vif_name.c_str());

	return XrlCmdError::OKAY();
    }

    if (dst_address != VrrpPacket::mcast_group) {
	XLOG_WARNING("Received stuff for unknown IP %s",
		     dst_address.str().c_str());

	return XrlCmdError::OKAY();
    }

    if (ip_protocol != IPPROTO_VRRP) {
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

XrlCmdError
VrrpTarget::raw_link_client_0_1_recv(
        // Input values,
        const string&   if_name,
        const string&   vif_name,
        const Mac&      src_address,
        const Mac&      dst_address,
        const uint32_t& ether_type,
        const vector<uint8_t>&  payload)
{
    VrrpVif* vif = find_vif(if_name, vif_name);
    if (!vif) {
	XLOG_WARNING("Can't find VIF %s", if_name.c_str());

	return XrlCmdError::OKAY();
    }

    if (ether_type != ETHERTYPE_ARP) {
	XLOG_WARNING("Unknown ethertype %u", ether_type);

	return XrlCmdError::OKAY();
    }

    // only arp requests for now
    if (dst_address != Mac::BROADCAST())
	return XrlCmdError::OKAY();

    vif->recv_arp(src_address, payload);

    return XrlCmdError::OKAY();
}
