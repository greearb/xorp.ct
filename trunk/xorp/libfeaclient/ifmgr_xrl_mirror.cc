// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libfeaclient/ifmgr_xrl_mirror.cc,v 1.2 2003/09/30 03:07:57 pavlin Exp $"

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

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_mtu(
	// Input values,
	const string&	ifname,
	const uint32_t&	mtu_bytes);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_mac(
	// Input values,
	const string&	ifname,
	const Mac&	mac);

    XrlCmdError fea_ifmgr_mirror_0_1_interface_set_pif_index(
	// Input values,
	const string&	ifname,
	const uint32_t&	pif_index);

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

    XrlCmdError fea_ifmgr_mirror_0_1_vif_set_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	index);

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
	const IPv4&	oaddr);

    XrlCmdError fea_ifmgr_mirror_0_1_ipv4_set_endpoint(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const IPv4&	oaddr);

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
	const IPv6&	oaddr);

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
      _hint_observer(0)
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
IfMgrXrlMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_mtu(
	const string&	ifname,
	const uint32_t&	mtu_bytes
	)
{
    _dispatcher.push(new IfMgrIfSetMtu(ifname, mtu_bytes));
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
					    static_cast<uint16_t>(pif)));
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
	const IPv4&	oaddr
)
{
    _dispatcher.push(new IfMgrIPv4SetBroadcast(ifn, vifn, addr, oaddr));
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
	const IPv4&	oaddr
)
{
    _dispatcher.push(new IfMgrIPv4SetEndpoint(ifn, vifn, addr, oaddr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

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
	const IPv6&	oaddr
)
{
    _dispatcher.push(new IfMgrIPv6SetEndpoint(ifn, vifn, addr, oaddr));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

bool
IfMgrXrlMirrorTarget::attach(IfMgrHintObserver* ho)
{
    if (_hint_observer != 0) {
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
    _hint_observer = 0;
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
	: XrlStdRouter(e, class_name), _o(0)
    {}

    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 IPv4		finder_addr)
	: XrlStdRouter(e, class_name, finder_addr), _o(0)
    {}

    IfMgrXrlMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 IPv4		finder_addr,
			 uint16_t	finder_port)
	: XrlStdRouter(e, class_name, finder_addr, finder_port), _o(0)
    {}

    /**
     * Attach an observer to router.  Only one observer is permitted.
     *
     * @param o observer to attach.
     * @return true on success, false if an observer is already registered;
     */
    bool attach(IfMgrXrlMirrorRouterObserver* o)
    {
	if (_o)
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
	_o = 0;
	return true;
    }

protected:
    void finder_ready_event(const string& tgt_name)
    {
	if (tgt_name == instance_name() && _o != 0) {
	    _o->finder_ready_event();
	}
    }

    void finder_disconnect_event()
    {
	if (_o != 0) {
	    _o->finder_disconnect_event();
	}
    }

protected:
    IfMgrXrlMirrorRouterObserver* _o;
};


// ----------------------------------------------------------------------------
// IfMgrXrlMirror

static const char* CLSNAME = "ifmgr_mirror";
const char* IfMgrXrlMirror::DEFAULT_REGISTRATION_TARGET = "XXX tbd";

IfMgrXrlMirror::IfMgrXrlMirror(EventLoop&	e,
			       const char*	rtarget)
    : _e(e), _dispatcher(_iftree), _rtarget(rtarget), _status(NO_FINDER)
{
    _rtr = new IfMgrXrlMirrorRouter(e, CLSNAME);
    _xrl_tgt = new IfMgrXrlMirrorTarget(*_rtr, _dispatcher);

    _rtr->attach(this);
    _xrl_tgt->attach(this);
}

IfMgrXrlMirror::IfMgrXrlMirror(EventLoop&	e,
			       IPv4		finder_addr,
			       const char*	rtarget)
    : _e(e), _dispatcher(_iftree), _rtarget(rtarget), _status(NO_FINDER)
{
    _rtr = new IfMgrXrlMirrorRouter(e, CLSNAME, finder_addr);
    _xrl_tgt = new IfMgrXrlMirrorTarget(*_rtr, _dispatcher);

    _rtr->attach(this);
    _xrl_tgt->attach(this);
}

IfMgrXrlMirror::IfMgrXrlMirror(EventLoop&	e,
			       IPv4		finder_addr,
			       uint16_t		finder_port,
			       const char*	rtarget)
    : _e(e), _dispatcher(_iftree), _rtarget(rtarget), _status(NO_FINDER)
{
    _rtr = new IfMgrXrlMirrorRouter(e, CLSNAME, finder_addr, finder_port);
    _xrl_tgt = new IfMgrXrlMirrorTarget(*_rtr, _dispatcher);

    _rtr->attach(this);
    _xrl_tgt->attach(this);
}

IfMgrXrlMirror::~IfMgrXrlMirror()
{
    _xrl_tgt->detach(this);
    _rtr->detach(this);
    delete _xrl_tgt;
    delete _rtr;
}

void
IfMgrXrlMirror::finder_disconnect_event()
{
    set_status(NO_FINDER);
    _iftree.clear();
}

void
IfMgrXrlMirror::finder_ready_event()
{
    register_with_ifmgr();
}

void
IfMgrXrlMirror::tree_complete()
{
    set_status(READY);
    list<IfMgrHintObserver*>::const_iterator ci;
    for (ci = _hint_observers.begin(); ci != _hint_observers.end(); ++ci) {
	IfMgrHintObserver* ho = *ci;
	ho->tree_complete();
    }
}

void
IfMgrXrlMirror::updates_made()
{
    list<IfMgrHintObserver*>::const_iterator ci;
    for (ci = _hint_observers.begin(); ci != _hint_observers.end(); ++ci) {
	IfMgrHintObserver* ho = *ci;
	ho->updates_made();
    }
}

bool
IfMgrXrlMirror::attach_hint_observer(IfMgrHintObserver* o)
{
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
	XLOG_FATAL("Failed to send registration to ifmgr");
    }
    set_status(REGISTERING_WITH_FEA);
}

void
IfMgrXrlMirror::register_cb(const XrlError& e)
{
    if (e == XrlError::OKAY()) {
	set_status(WAITING_FOR_TREE_IMAGE);
    } else {
	XLOG_FATAL("Failed to register with ifmgr: \"%s\"", e.str().c_str());
    }
}

IfMgrXrlMirror::Status
IfMgrXrlMirror::status() const
{
    return _status;
}

void
IfMgrXrlMirror::set_status(Status s)
{
    _status = s;
}
