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

#ident "$XORP: xorp/libfeaclient/xrl_ifmgr_mirror.cc,v 1.2 2003/08/27 17:13:55 hodson Exp $"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_std_router.hh"

#include "ifmgr_cmds.hh"
#include "xrl_ifmgr_mirror.hh"

// ----------------------------------------------------------------------------
// XrlIfMgrMirrorTarget
//
// Class that receives Xrls from Fea IfMgr and converts them into commands
// to be dispatched on IfMgrIfTree.
//
// NB All method definitions are external to class declaration in case
// we later decide to move the declaration to a header.
//

class XrlIfMgrMirrorTarget : protected XrlFeaIfmgrMirrorTargetBase {
public:
    XrlIfMgrMirrorTarget(XrlRouter& rtr, IfMgrCommandDispatcher& dispatcher);

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
	const uint32_t&	prefix);

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
	const uint32_t&	prefix);

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

protected:
    // Not implemented
    XrlIfMgrMirrorTarget();
    XrlIfMgrMirrorTarget(const XrlIfMgrMirrorTarget&);
    XrlIfMgrMirrorTarget& operator=(const XrlIfMgrMirrorTarget&);

protected:
    XrlRouter&		    _rtr;
    IfMgrCommandDispatcher& _dispatcher;
};

// ----------------------------------------------------------------------------
// XrlIfMgrMirrorTarget implementation

static const char* DISPATCH_FAILED = "Local dispatch error";

XrlIfMgrMirrorTarget::XrlIfMgrMirrorTarget(XrlRouter&		 rtr,
					   IfMgrCommandDispatcher& dispatcher)
    : XrlFeaIfmgrMirrorTargetBase(&rtr), _rtr(rtr), _dispatcher(dispatcher)
{
}

XrlCmdError
XrlIfMgrMirrorTarget::common_0_1_get_target_name(string& name)
{
    name = _rtr.instance_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlIfMgrMirrorTarget::common_0_1_get_version(string& version)
{
    version = "IfMgrMirror/0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlIfMgrMirrorTarget::common_0_1_get_status(uint32_t& status,
					    string&   reason)
{
    // Nothing to do.  Expect container application will get this call
    // at the appropriate level.
    status = uint32_t(PROC_READY);
    reason.erase();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlIfMgrMirrorTarget::common_0_1_shutdown()
{
    // Nothing to do.  Expect container application will get this call
    // at the appropriate level.
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_interface_add(const string& ifname)
{
    _dispatcher.push(new IfMgrIfAdd(ifname));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_interface_remove(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_enabled(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_mtu(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_interface_set_mac(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_add(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_remove(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_enabled(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_multicast_capable(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_broadcast_capable(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_p2p_capable(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_loopback(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_vif_set_pif_index(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_add(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_remove(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_prefix(
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t& prefix
)
{
    _dispatcher.push(new IfMgrIPv4SetPrefix(ifname, vifname, addr, prefix));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_enabled(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_multicast_capable(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_loopback(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_broadcast(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv4_set_endpoint(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_add(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_remove(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_prefix(
	const string&	ifname,
	const string&	vifname,
	const IPv6&	addr,
	const uint32_t& prefix
)
{
    _dispatcher.push(new IfMgrIPv6SetPrefix(ifname, vifname, addr, prefix));
    if (_dispatcher.execute() == true) {
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED(DISPATCH_FAILED);
}

XrlCmdError
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_enabled(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_multicast_capable(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_loopback(
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
XrlIfMgrMirrorTarget::fea_ifmgr_mirror_0_1_ipv6_set_endpoint(
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


// ----------------------------------------------------------------------------
// XrlIfMgrMirrorRouter
//
// Class to act as an XrlRouter for XrlIfMgrMirror
//

class XrlIfMgrMirrorRouter : public XrlStdRouter
{
public:
    XrlIfMgrMirrorRouter(EventLoop&	e,
			 const char*	class_name)
	: XrlStdRouter(e, class_name), _o(0)
    {}

    XrlIfMgrMirrorRouter(EventLoop&	e,
			 const char*	class_name,
			 IPv4		finder_addr)
	: XrlStdRouter(e, class_name, finder_addr), _o(0)
    {}

    XrlIfMgrMirrorRouter(EventLoop&	e,
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
    bool attach(XrlIfMgrMirrorRouterObserver* o)
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
    bool detach(XrlIfMgrMirrorRouterObserver* o)
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
    XrlIfMgrMirrorRouterObserver* _o;
};


// ----------------------------------------------------------------------------
// XrlIfMgrMirror

static const char* CLSNAME = "ifmgr_mirror";
const char* XrlIfMgrMirror::DEFAULT_REGISTRATION_TARGET = "XXX tbd";

XrlIfMgrMirror::XrlIfMgrMirror(EventLoop&	e,
			       const char*	rtarget)
    : _e(e), _dispatcher (_iftree), _rtarget(rtarget)
{
    _rtr = new XrlIfMgrMirrorRouter(e, CLSNAME);
    _xrl_tgt = new XrlIfMgrMirrorTarget(*_rtr, _dispatcher);
    _rtr->attach(this);
}

XrlIfMgrMirror::XrlIfMgrMirror(EventLoop&	e,
			       IPv4		finder_addr,
			       const char*	rtarget)
    : _e(e), _dispatcher (_iftree), _rtarget(rtarget)
{
    _rtr = new XrlIfMgrMirrorRouter(e, CLSNAME, finder_addr);
    _xrl_tgt = new XrlIfMgrMirrorTarget(*_rtr, _dispatcher);
    _rtr->attach(this);
}

XrlIfMgrMirror::XrlIfMgrMirror(EventLoop&	e,
			       IPv4		finder_addr,
			       uint16_t		finder_port,
			       const char*	rtarget)
    : _e(e), _dispatcher (_iftree), _rtarget(rtarget)
{
    _rtr = new XrlIfMgrMirrorRouter(e, CLSNAME, finder_addr, finder_port);
    _xrl_tgt = new XrlIfMgrMirrorTarget(*_rtr, _dispatcher);
    _rtr->attach(this);
}

XrlIfMgrMirror::~XrlIfMgrMirror()
{
    _rtr->detach(this);
    delete _xrl_tgt;
    delete _rtr;
}

void
XrlIfMgrMirror::finder_disconnect_event()
{
    _status = NO_FINDER;
    _iftree.clear();
}

void
XrlIfMgrMirror::finder_ready_event()
{
    register_with_ifmgr();
}

void
XrlIfMgrMirror::register_with_ifmgr()
{
}
