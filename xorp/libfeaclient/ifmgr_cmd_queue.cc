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



#include "libxorp/xorp.h"
#include "libfeaclient_module.h"
#include "libxorp/xlog.h"
#include "ifmgr_atoms.hh"
#include "ifmgr_cmds.hh"
#include "ifmgr_cmd_queue.hh"

// ----------------------------------------------------------------------------
// IfMgrCommandSinkBase

IfMgrCommandSinkBase::~IfMgrCommandSinkBase()
{
}

// ----------------------------------------------------------------------------
// IfMgrCommandTee

IfMgrCommandTee::IfMgrCommandTee(IfMgrCommandSinkBase& o1,
				 IfMgrCommandSinkBase& o2)
    : _o1(o1), _o2(o2)
{
}

void
IfMgrCommandTee::push(const Cmd& cmd)
{
    _o1.push(cmd);
    _o2.push(cmd);
}

// ----------------------------------------------------------------------------
// IfMgrCommandDispatcher

IfMgrCommandDispatcher::IfMgrCommandDispatcher(IfMgrIfTree& tree)
    : _iftree(tree)
{
}

void
IfMgrCommandDispatcher::push(const Cmd& cmd)
{
    if (_cmd.get() != NULL) {
	XLOG_WARNING("Dropping buffered command.");
    }
    _cmd = cmd;
}

bool
IfMgrCommandDispatcher::execute()
{
    bool success = false;
    if (_cmd.get() != NULL) {
	success = _cmd->execute(_iftree);
	_cmd = 0;
    }
    return success;
}


// ----------------------------------------------------------------------------
// IfMgrCommandFifoQueue

void
IfMgrCommandFifoQueue::push(const Cmd& c)
{
    _fifo.push_back(c);
}

bool
IfMgrCommandFifoQueue::empty() const
{
    return _fifo.empty();
}

IfMgrCommandFifoQueue::Cmd&
IfMgrCommandFifoQueue::front()
{
    return _fifo.front();
}

const IfMgrCommandFifoQueue::Cmd&
IfMgrCommandFifoQueue::front() const
{
    return _fifo.front();
}

void
IfMgrCommandFifoQueue::pop_front()
{
    _fifo.pop_front();
}


// ----------------------------------------------------------------------------
// IfMgrCommandIfClusteringQueue

IfMgrCommandIfClusteringQueue::IfMgrCommandIfClusteringQueue()
    : _current_ifname("rolf harris")
{}

void
IfMgrCommandIfClusteringQueue::push(const Cmd& cmd)
{
    IfMgrIfCommandBase* ifcmd = dynamic_cast<IfMgrIfCommandBase*>(cmd.get());
    XLOG_ASSERT(ifcmd != NULL);
    if (ifcmd->ifname() == _current_ifname) {
	_current_cmds.push_back(cmd);
    } else {
	_future_cmds.push_back(cmd);
	if (_current_cmds.empty()) {
	    change_active_interface();
	}
    }
}

bool
IfMgrCommandIfClusteringQueue::empty() const
{
    return _current_cmds.empty();
}

IfMgrCommandIfClusteringQueue::Cmd&
IfMgrCommandIfClusteringQueue::front()
{
    return _current_cmds.front();
}

const IfMgrCommandIfClusteringQueue::Cmd&
IfMgrCommandIfClusteringQueue::front() const
{
    return _current_cmds.front();
}

void
IfMgrCommandIfClusteringQueue::pop_front()
{

    if (_current_cmds.empty() == false) {
	Cmd& c = _current_cmds.front();
	IfMgrIfCommandBase* ifcmd =
	    dynamic_cast<IfMgrIfCommandBase*>(c.get());
	XLOG_ASSERT(ifcmd != NULL);
	_current_ifname = ifcmd->ifname();
	_current_cmds.pop_front();
    }

    if (_current_cmds.empty()) {
	change_active_interface();
    }
}

//
// Helper predicate class for
// IfMgrCommandIfClusteringQueue::change_active_interface()
//
class InterfaceNameOfQueuedCmdMatches {
public:
    InterfaceNameOfQueuedCmdMatches(const string& ifname)
	: _ifname(ifname)
    {}

    bool operator() (IfMgrCommandIfClusteringQueue::Cmd c) {
	IfMgrIfCommandBase* ifcmd = dynamic_cast<IfMgrIfCommandBase*>(c.get());
	XLOG_ASSERT(ifcmd != NULL);
	return ifcmd->ifname() == _ifname;
    }
protected:
    const string& _ifname;
};

void
IfMgrCommandIfClusteringQueue::change_active_interface()
{
    XLOG_ASSERT(_current_cmds.empty());
    if (_future_cmds.empty()) {
	return;
    }

    // Take first command in future commands and use interface of that
    // as new current interface.
    Cmd& c = _future_cmds.front();
    IfMgrIfCommandBase* ifcmd =
	dynamic_cast<IfMgrIfCommandBase*>(c.get());
    XLOG_ASSERT(ifcmd != NULL);
    _current_ifname = ifcmd->ifname();
    back_insert_iterator<CmdList> bi(_current_cmds);
    remove_copy_if(_future_cmds.begin(), _future_cmds.end(), bi,
		   InterfaceNameOfQueuedCmdMatches(_current_ifname));
}


// ----------------------------------------------------------------------------
// IfMgr*ToCommands

void
IfMgrIfTreeToCommands::convert(IfMgrCommandSinkBase& s) const
{
    const IfMgrIfTree::IfMap& interfaces = _tree.interfaces();
    IfMgrIfTree::IfMap::const_iterator cii;

    for (cii = interfaces.begin(); cii != interfaces.end(); ++cii) {
	IfMgrIfAtomToCommands(cii->second).convert(s);
    }
    s.push(new IfMgrHintTreeComplete());
}

void
IfMgrIfAtomToCommands::convert(IfMgrCommandSinkBase& s) const
{
    s.push(new IfMgrIfAdd(_i.name()));
    s.push(new IfMgrIfSetEnabled(_i.name(), _i.enabled()));
    s.push(new IfMgrIfSetDiscard(_i.name(), _i.discard()));
    s.push(new IfMgrIfSetUnreachable(_i.name(), _i.unreachable()));
    s.push(new IfMgrIfSetManagement(_i.name(), _i.management()));
    s.push(new IfMgrIfSetMtu(_i.name(), _i.mtu()));
    s.push(new IfMgrIfSetMac(_i.name(), _i.mac()));
    s.push(new IfMgrIfSetPifIndex(_i.name(), _i.pif_index()));
    s.push(new IfMgrIfSetNoCarrier(_i.name(), _i.no_carrier()));
    s.push(new IfMgrIfSetBaudrate(_i.name(), _i.baudrate()));
    s.push(new IfMgrIfSetString(_i.name(), _i.parent_ifname(), IF_STRING_PARENT_IFNAME));
    s.push(new IfMgrIfSetString(_i.name(), _i.iface_type(), IF_STRING_IFTYPE));
    s.push(new IfMgrIfSetString(_i.name(), _i.vid(), IF_STRING_VID));

    const IfMgrIfAtom::VifMap& vifs = _i.vifs();
    IfMgrIfAtom::VifMap::const_iterator cvi;
    for (cvi = vifs.begin(); cvi != vifs.end(); ++cvi) {
	IfMgrVifAtomToCommands(_i.name(), cvi->second).convert(s);
    }
}

void
IfMgrVifAtomToCommands::convert(IfMgrCommandSinkBase& s) const
{
    const string& ifn = _ifn;
    const string& vifn = _v.name();

    s.push(new IfMgrVifAdd(ifn, vifn));
    s.push(new IfMgrVifSetEnabled(ifn, vifn, _v.enabled()));
    s.push(new IfMgrVifSetMulticastCapable(ifn, vifn, _v.multicast_capable()));
    s.push(new IfMgrVifSetBroadcastCapable(ifn, vifn, _v.broadcast_capable()));
    s.push(new IfMgrVifSetP2PCapable(ifn, vifn, _v.p2p_capable()));
    s.push(new IfMgrVifSetLoopbackCapable(ifn, vifn, _v.loopback()));
    s.push(new IfMgrVifSetPimRegister(ifn, vifn, _v.pim_register()));
    s.push(new IfMgrVifSetPifIndex(ifn, vifn, _v.pif_index()));
    s.push(new IfMgrVifSetVifIndex(ifn, vifn, _v.vif_index()));

    const IfMgrVifAtom::IPv4Map& v4s = _v.ipv4addrs();
    for (IfMgrVifAtom::IPv4Map::const_iterator cai = v4s.begin();
	 cai != v4s.end(); ++cai) {
	IfMgrIPv4AtomToCommands(ifn, vifn, cai->second).convert(s);
    }

#ifdef HAVE_IPV6
    const IfMgrVifAtom::IPv6Map& v6s = _v.ipv6addrs();
    for (IfMgrVifAtom::IPv6Map::const_iterator cai = v6s.begin();
	 cai != v6s.end(); ++cai) {
	IfMgrIPv6AtomToCommands(ifn, vifn, cai->second).convert(s);
    }
#endif
}

void
IfMgrIPv4AtomToCommands::convert(IfMgrCommandSinkBase& s) const
{
    const string& ifn = _ifn;
    const string& vifn = _vifn;
    IPv4 addr = _a.addr();

    s.push(new IfMgrIPv4Add(ifn, vifn, addr));
    s.push(new IfMgrIPv4SetPrefix(ifn, vifn, addr, _a.prefix_len()));
    s.push(new IfMgrIPv4SetEnabled(ifn, vifn, addr, _a.enabled()));
    s.push(new IfMgrIPv4SetMulticastCapable(ifn, vifn, addr,
					    _a.multicast_capable()));
    s.push(new IfMgrIPv4SetLoopback(ifn, vifn, addr, _a.loopback()));
    s.push(new IfMgrIPv4SetBroadcast(ifn, vifn, addr, _a.broadcast_addr()));
    s.push(new IfMgrIPv4SetEndpoint(ifn, vifn, addr, _a.endpoint_addr()));
}

#ifdef HAVE_IPV6
void
IfMgrIPv6AtomToCommands::convert(IfMgrCommandSinkBase& s) const
{
    const string& ifn = _ifn;
    const string& vifn = _vifn;
    const IPv6& addr = _a.addr();

    s.push(new IfMgrIPv6Add(ifn, vifn, addr));
    s.push(new IfMgrIPv6SetPrefix(ifn, vifn, addr, _a.prefix_len()));
    s.push(new IfMgrIPv6SetEnabled(ifn, vifn, addr, _a.enabled()));
    s.push(new IfMgrIPv6SetMulticastCapable(ifn, vifn, addr,
					    _a.multicast_capable()));
    s.push(new IfMgrIPv6SetLoopback(ifn, vifn, addr, _a.loopback()));
    s.push(new IfMgrIPv6SetEndpoint(ifn, vifn, addr, _a.endpoint_addr()));
}

#endif
