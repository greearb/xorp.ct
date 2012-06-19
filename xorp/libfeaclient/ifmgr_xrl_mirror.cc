// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
//
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/fea_ifmgr_replicator_xif.hh"
#include "ifmgr_cmds.hh"
#include "ifmgr_xrl_mirror.hh"

//----------------------------------------------------------------------------
// IfMgrXrlMirrorRouterObserver

IfMgrXrlMirrorRouterObserver::~IfMgrXrlMirrorRouterObserver()
{
}

//----------------------------------------------------------------------------
// IfMgrHintObserver

IfMgrHintObserver::~IfMgrHintObserver()
{
}

// ----------------------------------------------------------------------------
// IfMgrXrlMirrorTarget
//
// Class that receives Xrls from Fea IfMgr and converts them into commands
// to be dispatched on IfMgrIfTree.
//
// NB All method definitions are external to class declaration in case
// we later decide to move the declaration to a header.
//

class IfMgrXrlMirrorTarget : protected XrlFeaIfmgrMirrorTargetBase {
public:
    IfMgrXrlMirrorTarget(XrlRouter& rtr, IfMgrCommandDispatcher& dispatcher);

    bool attach(IfMgrHintObserver* o);
    bool detach(IfMgrHintObserver* o);

protected:
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string& name);

    XrlCmdError common_0_1_get_version(
	// Output values,
	string& version);

    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t& status,
	string& reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError fea_ifmgr_mirror_0_1_interface_add(
	// Input values,
	const string&	ifname);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_remove(
	// Input values,
	const string&	ifname);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_enabled(
	// Input values,
	const string&	ifname,
	const bool&	enabled);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_discard(
	// Input values,
	const string&	ifname,
	const bool&	discard);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_unreachable(
	// Input values,
	const string&	ifname,
	const bool&	unreachable);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_management(
	// Input values,
	const string&	ifname,
	const bool&	management);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_mtu(
	// Input values,
	const string&	ifname,
	const uint32_t&	mtu);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_mac(
	// Input values,
	const string&	ifname,
	const Mac&	mac);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_pif_index(
	// Input values,
	const string&	ifname,
	const uint32_t&	pif_index);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_no_carrier(
	// Input values,
	const string&	ifname,
	const bool&	no_carrier);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_baudrate(
	// Input values,
	const string&	ifname,
	const uint64_t&	baudrate);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_parent_ifname(
	// Input values,
	const string&	ifname,
	const string&	parent_ifname);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_iface_type(
	// Input values,
	const string&	ifname,
	const string&	iface_type);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_vid(
	// Input values,
	const string&	ifname,
	const string&	vid);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_add(
	// Input values,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_remove(
	// Input values,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_enabled(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	enabled);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_multicast_capable(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	capable);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_broadcast_capable(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	capable);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_p2p_capable(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	capable);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_loopback(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	loopback);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_pim_register(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const bool&	pim_register);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	pif_index);

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_vif_index(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	vif_index);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_add(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_remove(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_prefix(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t&	prefix_len);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_enabled(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const bool&	enabled);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_multicast_capable(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const bool&	capable);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_loopback(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const bool&	loopback);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_broadcast(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const IPv4&	broadcast_addr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_endpoint(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const IPv4&	endpoint_addr);

#ifdef HAVE_IPV6
    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_add(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_remove(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_set_prefix(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const uint32_t&	prefix_len);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_set_enabled(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const bool&	enabled);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_set_loopback(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const bool&	loopback);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_set_multicast_capable(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const bool&	capable);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv6_set_endpoint(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const IPv6&	endpoint_addr);
#endif

    XrlCmdError fea_ifmgr_mirror_0_1_hint_tree_complete();

    XrlCmdError fea_ifmgr_mirror_0_1_hint_updates_made();

protected:
    // Not implemented
    IfMgrXrlMirrorTarget();
    IfMgrXrlMirrorTarget(const IfMgrXrlMirrorTarget&);
    IfMgrXrlMirrorTarget& operator=(const IfMgrXrlMirrorTarget&);

protected:
    XrlRouter&		    _rtr;
    IfMgrCommandDispatcher& _dispatcher;
    IfMgrHintObserver*	    _hint_observer;
};

// ----------------------------------------------------------------------------
// IfMgrXrlMirrorTarget implementation

static const char* DISPATCH_FAILED = "Local dispatch error";

IfMgrXrlMirrorTarget::IfMgrXrlMirrorTarget(XrlRouter&		 rtr,
					   IfMgrCommandDispatcher& dispatcher)
    : XrlFeaIfmgrMirrorTargetBase(&rtr), _rtr(rtr), _dispatcher(dispatcher),
      _hint_observer(NULL)
{
}

XrlCmdError
IfMgrXrlMirrorTarget::common_0_1_get_target_name(string& name)
{
    name = _rtr.instance_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
IfMgrXrlMirrorTarget::common_0_1_get_version(string& version)
{
    version = "IfMgrMirror/0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
IfMgrXrlMirrorTarget::common_0_1_get_status(uint32_t& status,
					    string&   reason)
{
    // Nothing to do.  Expect container application will get this call
    // at the appropriate level.
    status = uint32_t(PROC_READY);
    reason.erase();

    return XrlCmdError::OKAY();
}

XrlCmdError
IfMgrXrlMirrorTarget::common_0_1_shutdown()
{
    // Nothing to do.  Expect container application will get this call
    // at the appropriate level.
    return XrlCmdError::OKAY();
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_add(const string& ifname)
{
    _dispatcher.push(new IfMgrIfAdd(ifname));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_remove(
	const string& ifname
	)
{
    _dispatcher.push(new IfMgrIfRemove(ifname));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_enabled(
	const string&	ifname,
	const bool&	en
	)
{
    _dispatcher.push(new IfMgrIfSetEnabled(ifname, en));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_discard(
	const string&	ifname,
	const bool&	discard
	)
{
    _dispatcher.push(new IfMgrIfSetDiscard(ifname, discard));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_unreachable(
	const string&	ifname,
	const bool&	unreachable
	)
{
    _dispatcher.push(new IfMgrIfSetUnreachable(ifname, unreachable));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_management(
	const string&	ifname,
	const bool&	management
	)
{
    _dispatcher.push(new IfMgrIfSetManagement(ifname, management));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_mtu(
	const string&	ifname,
	const uint32_t&	mtu
	)
{
    _dispatcher.push(new IfMgrIfSetMtu(ifname, mtu));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_mac(
	const string&	ifname,
	const Mac&	mac
	)
{
    _dispatcher.push(new IfMgrIfSetMac(ifname, mac));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_pif_index(
	const string&	ifname,
	const uint32_t&	pif
	)
{
    _dispatcher.push(new IfMgrIfSetPifIndex(ifname,
					    static_cast<uint32_t>(pif)));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_no_carrier(
	const string&	ifname,
	const bool&	no_carrier
	)
{
    _dispatcher.push(new IfMgrIfSetNoCarrier(ifname, no_carrier));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_baudrate(
	const string&	ifname,
	const uint64_t&	baudrate
	)
{
    _dispatcher.push(new IfMgrIfSetBaudrate(ifname, baudrate));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_parent_ifname(
	const string&	ifname,
	const string&	parent_ifname
	)
{
    _dispatcher.push(new IfMgrIfSetString(ifname, parent_ifname, IF_STRING_PARENT_IFNAME));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_iface_type(
	const string&	ifname,
	const string&	iface_type
	)
{
    _dispatcher.push(new IfMgrIfSetString(ifname, iface_type, IF_STRING_IFTYPE));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_vid(
	const string&	ifname,
	const string&	vid
	)
{
    _dispatcher.push(new IfMgrIfSetString(ifname, vid, IF_STRING_VID));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_add(
	const string& ifname,
	const string& vifname
	)
{
    _dispatcher.push(new IfMgrVifAdd(ifname, vifname));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_remove(
	const string& ifname,
	const string& vifname
	)
{
    _dispatcher.push(new IfMgrVifRemove(ifname, vifname));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_enabled(
	const string& ifname,
	const string& vifname,
	const bool&   en
	)
{
    _dispatcher.push(new IfMgrVifSetEnabled(ifname, vifname, en));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_multicast_capable(
	const string&	ifname,
	const string&	vifname,
	const bool&	cap
	)
{
    _dispatcher.push(new IfMgrVifSetMulticastCapable(ifname, vifname, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_broadcast_capable(
	const string&	ifname,
	const string&	vifname,
	const bool&	cap
	)
{
    _dispatcher.push(new IfMgrVifSetBroadcastCapable(ifname, vifname, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_p2p_capable(
	const string&	ifname,
	const string&	vifname,
	const bool&	cap
	)
{
    _dispatcher.push(new IfMgrVifSetP2PCapable(ifname, vifname, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_loopback(
	const string&	ifname,
	const string&	vifname,
	const bool&	cap
	)
{
    _dispatcher.push(new IfMgrVifSetLoopbackCapable(ifname, vifname, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_pim_register(
	const string&	ifname,
	const string&	vifname,
	const bool&	pim_register
	)
{
    _dispatcher.push(new IfMgrVifSetPimRegister(ifname, vifname, pim_register));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_pif_index(
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	pif_index
	)
{
    _dispatcher.push(new IfMgrVifSetPifIndex(ifname, vifname, pif_index));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_vif_index(
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	vif_index
	)
{
    _dispatcher.push(new IfMgrVifSetVifIndex(ifname, vifname, vif_index));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_add(
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr
)
{
    _dispatcher.push(new IfMgrIPv4Add(ifname, vifname, addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_remove(
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr
)
{
    _dispatcher.push(new IfMgrIPv4Remove(ifname, vifname, addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_prefix(
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t& prefix_len
)
{
    _dispatcher.push(new IfMgrIPv4SetPrefix(ifname, vifname, addr, prefix_len));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_enabled(
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const bool&	en
)
{
    _dispatcher.push(new IfMgrIPv4SetEnabled(ifname, vifname, addr, en));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_multicast_capable(
	const string&	ifn,
	const string&	vifn,
	const IPv4&	addr,
	const bool&	cap
)
{
    _dispatcher.push(new IfMgrIPv4SetMulticastCapable(ifn, vifn, addr, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_loopback(
	const string&	ifn,
	const string&	vifn,
	const IPv4&	addr,
	const bool&	cap
)
{
    _dispatcher.push(new IfMgrIPv4SetLoopback(ifn, vifn, addr, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_broadcast(
	const string&	ifn,
	const string&	vifn,
	const IPv4&	addr,
	const IPv4&	broadcast_addr
)
{
    _dispatcher.push(new IfMgrIPv4SetBroadcast(ifn, vifn, addr,
					       broadcast_addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_endpoint(
	const string&	ifn,
	const string&	vifn,
	const IPv4&	addr,
	const IPv4&	endpoint_addr
)
{
    _dispatcher.push(new IfMgrIPv4SetEndpoint(ifn, vifn, addr, endpoint_addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

#ifdef HAVE_IPV6
XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_add(
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr
)
{
    _dispatcher.push(new IfMgrIPv6Add(ifname, vifname, addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_remove(
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr
)
{
    _dispatcher.push(new IfMgrIPv6Remove(ifname, vifname, addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_prefix(
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const uint32_t& prefix_len
)
{
    _dispatcher.push(new IfMgrIPv6SetPrefix(ifname, vifname, addr, prefix_len));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_enabled(
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const bool&	en
)
{
    _dispatcher.push(new IfMgrIPv6SetEnabled(ifname, vifname, addr, en));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_multicast_capable(
	const string&	ifn,
	const string&	vifn,
	const IPv6&	addr,
	const bool&	cap
)
{
    _dispatcher.push(new IfMgrIPv6SetMulticastCapable(ifn, vifn, addr, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_loopback(
	const string&	ifn,
	const string&	vifn,
	const IPv6&	addr,
	const bool&	cap
)
{
    _dispatcher.push(new IfMgrIPv6SetLoopback(ifn, vifn, addr, cap));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_endpoint(
	const string&	ifn,
	const string&	vifn,
	const IPv6&	addr,
	const IPv6&	endpoint_addr
)
{
    _dispatcher.push(new IfMgrIPv6SetEndpoint(ifn, vifn, addr, endpoint_addr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}
#endif //ipv6

bool
IfMgrXrlMirrorTarget::attach(IfMgrHintObserver* ho)
{
    if (_hint_observer != NULL) {
	return false;
    }
    _hint_observer = ho;
    return true;
}

bool
IfMgrXrlMirrorTarget::detach(IfMgrHintObserver* ho)
{
    if (_hint_observer != ho) {
	return false;
    }
    _hint_observer = NULL;
    return true;
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_hint_tree_complete()
{
    if (_hint_observer)
	_hint_observer->tree_complete();
    return XrlCmdError::OKAY();
}

XrlCmdError
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_hint_updates_made()
{
    if (_hint_observer)
	_hint_observer->updates_made();
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// IfMgrXrlMirrorRouter
//
// Class to act as an XrlRouter for IfMgrXrlMirror
//

class IfMgrXrlMirrorRouter : public XrlStdRouter
{
public:
    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name)
	: XrlStdRouter(e, class_name), _o(NULL)
    {}

    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 IPv4		finder_addr)
	: XrlStdRouter(e, class_name, finder_addr), _o(NULL)
    {}

    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 IPv4		finder_addr,
			 uint16_t	finder_port)
	: XrlStdRouter(e, class_name, finder_addr, finder_port), _o(NULL)
    {}

    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 const char*	finder_hostname,
			 uint16_t	finder_port)
	: XrlStdRouter(e, class_name, finder_hostname, finder_port), _o(NULL)
    {}

    /**
     * Attach an observer to router.  Only one observer is permitted.
     *
     * @param o observer to attach.
     * @return true on success, false if an observer is already registered;
     */
    bool attach(IfMgrXrlMirrorRouterObserver* o)
    {
	if (_o != NULL)
	    return false;
	_o = o;
	return true;
    }

    /**
     * Detach an observer to router.
     *
     * @param o observer to detach.
     * @return true on success, false if another observer is attached;
     */
    bool detach(IfMgrXrlMirrorRouterObserver* o)
    {
	if (o != _o)
	    return false;
	_o = NULL;
	return true;
    }

protected:
    void finder_ready_event(const string& tgt_name)
    {
	if (tgt_name == instance_name() && _o != NULL) {
	    _o->finder_ready_event();
	}
    }

    void finder_disconnect_event()
    {
	if (_o != NULL) {
	    _o->finder_disconnect_event();
	}
    }

protected:
    IfMgrXrlMirrorRouterObserver* _o;
};


// ----------------------------------------------------------------------------
// IfMgrXrlMirror

static const char* CLSNAME = "ifmgr_mirror";

IfMgrXrlMirror::IfMgrXrlMirror(EventLoop&	e,
			       const char*	rtarget,
			       IPv4		finder_addr,
			       uint16_t		finder_port)

    : ServiceBase("FEA Interface Mirror"),
      _e(e), _finder_addr(finder_addr), _finder_port(finder_port),
      _dispatcher(_iftree), _rtarget(rtarget), _rtr(NULL), _xrl_tgt(NULL),
      _updates_delay(TimeVal::ZERO())

{
}

IfMgrXrlMirror::IfMgrXrlMirror(EventLoop&	e,
			       const char*	rtarget,
			       const char*	finder_hostname,
			       uint16_t		finder_port)

    : ServiceBase("FEA Interface Mirror"),
      _e(e), _finder_hostname(finder_hostname), _finder_port(finder_port),
      _dispatcher(_iftree), _rtarget(rtarget), _rtr(NULL), _xrl_tgt(NULL),
      _updates_delay(TimeVal::ZERO())
{
}

IfMgrXrlMirror::~IfMgrXrlMirror()
{
    if (_rtr != NULL) {
	//
	// Both _rtr and _xrl_tgt have been assigned in startup() so
	// if one is NULL then the other must be NULL and we need to
	// clean-up.  We test in case startup() has not been called.
	//
	_xrl_tgt->detach(this);
	_rtr->detach(this);
	delete _xrl_tgt;
	_xrl_tgt = NULL;
	delete _rtr;
	_rtr = NULL;
    }
}

int
IfMgrXrlMirror::startup()
{
    if (status() != SERVICE_READY)
	return (XORP_ERROR);

    if (_rtr == NULL) {
	if (! _finder_hostname.empty()) {
	    _rtr = new IfMgrXrlMirrorRouter(_e, CLSNAME,
					    _finder_hostname.c_str(),
					    _finder_port);
	} else {
	    _rtr = new IfMgrXrlMirrorRouter(_e, CLSNAME,
					    _finder_addr,
					    _finder_port);
	}
	_rtr->attach(this);
    }
    if (_xrl_tgt == NULL) {
	_xrl_tgt = new IfMgrXrlMirrorTarget(*_rtr, _dispatcher);
	_xrl_tgt->attach(this);
    }
    set_status(SERVICE_STARTING, "Initializing Xrl Router.");

    return (XORP_OK);
}

int
IfMgrXrlMirror::shutdown()
{
    if (status() != SERVICE_RUNNING)
	return (XORP_ERROR);

    set_status(SERVICE_SHUTTING_DOWN);
    unregister_with_ifmgr();

    return (XORP_OK);
}

void
IfMgrXrlMirror::finder_disconnect_event()
{
    _iftree.clear();
    if (status() == SERVICE_SHUTTING_DOWN) {
	set_status(SERVICE_SHUTDOWN);
    } else {
	set_status(SERVICE_FAILED);
    }
}

void
IfMgrXrlMirror::finder_ready_event()
{
    register_with_ifmgr();
}

void
IfMgrXrlMirror::tree_complete()
{
    if (status() != SERVICE_STARTING) {
	// XXX In case shutdown is called or a failure occurs before
	// tree_complete message is received.  If either happens we don't
	// want to advertise the tree as complete.
	return;
    }

    set_status(SERVICE_RUNNING);
    list<IfMgrHintObserver*>::const_iterator ci;
    for (ci = _hint_observers.begin(); ci != _hint_observers.end(); ++ci) {
	IfMgrHintObserver* ho = *ci;
	ho->tree_complete();
    }
}

void
IfMgrXrlMirror::delay_updates(const TimeVal& delay)
{
    _updates_delay = delay;
}

void
IfMgrXrlMirror::updates_made()
{
    if (_updates_delay.is_zero()) {
	do_updates();
	return;
    }

    if (_updates_timer.scheduled())
	return;

    _updates_timer = _e.new_oneoff_after(
	_updates_delay,
	callback(this, &IfMgrXrlMirror::do_updates));
}

void
IfMgrXrlMirror::do_updates()
{
    if (status() & ServiceStatus(SERVICE_SHUTTING_DOWN
				 | SERVICE_SHUTDOWN
				 | SERVICE_FAILED))
	return;

    list<IfMgrHintObserver*>::const_iterator ci;
    for (ci = _hint_observers.begin(); ci != _hint_observers.end(); ++ci) {
	IfMgrHintObserver* ho = *ci;
	ho->updates_made();
    }
}

bool
IfMgrXrlMirror::attach_hint_observer(IfMgrHintObserver* o)
{
    if (status() & ServiceStatus(SERVICE_SHUTTING_DOWN
				 | SERVICE_SHUTDOWN
				 | SERVICE_FAILED))
	return false;

    if (find(_hint_observers.begin(), _hint_observers.end(), o)
	!= _hint_observers.end()) {
	return false;
    }
    _hint_observers.push_back(o);
    return true;
}

bool
IfMgrXrlMirror::detach_hint_observer(IfMgrHintObserver* o)
{
    list<IfMgrHintObserver*>::iterator i;
    i = find(_hint_observers.begin(), _hint_observers.end(), o);
    if (i == _hint_observers.end()) {
	return false;
    }
    _hint_observers.erase(i);
    return true;
}

void
IfMgrXrlMirror::register_with_ifmgr()
{
    XrlIfmgrReplicatorV0p1Client c(_rtr);
    if (c.send_register_ifmgr_mirror(_rtarget.c_str(),
				     _rtr->instance_name(),
				     callback(this,
					      &IfMgrXrlMirror::register_cb))
	== false) {
	set_status(SERVICE_FAILED, "Failed to send registration to ifmgr");
	return;
    }
    set_status(SERVICE_STARTING, "Registering with FEA interface manager.");
}

void
IfMgrXrlMirror::register_cb(const XrlError& e)
{
    if (e == XrlError::OKAY()) {
	set_status(SERVICE_STARTING, "Waiting to receive interface data.");
    } else {
	set_status(SERVICE_FAILED, "Failed to send registration to ifmgr");
    }
}

void
IfMgrXrlMirror::unregister_with_ifmgr()
{
    XrlIfmgrReplicatorV0p1Client c(_rtr);
    if (c.send_unregister_ifmgr_mirror(_rtarget.c_str(), _rtr->instance_name(),
		callback(this, &IfMgrXrlMirror::unregister_cb))
	== false) {
	set_status(SERVICE_FAILED, "Failed to send unregister to FEA");
	return;
    }
    set_status(SERVICE_SHUTTING_DOWN,
	       "De-registering with FEA interface manager.");
}

void
IfMgrXrlMirror::unregister_cb(const XrlError& e)
{
    _iftree.clear();

    if (e == XrlError::OKAY()) {
	set_status(SERVICE_SHUTDOWN);
    } else {
	set_status(SERVICE_FAILED, "Failed to de-registration to ifmgr");
    }
}
