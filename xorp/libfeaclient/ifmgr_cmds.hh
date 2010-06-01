// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/libfeaclient/ifmgr_cmds.hh,v 1.26 2008/10/02 21:57:16 bms Exp $

#ifndef __LIBFEACLIENT_IFMGR_CMDS_HH__
#define __LIBFEACLIENT_IFMGR_CMDS_HH__

#include "ifmgr_cmd_base.hh"

/**
 * @short Base class for interface state manipulator commands.
 */
class IfMgrIfCommandBase : public IfMgrCommandBase {
public:
    IfMgrIfCommandBase(const string& ifname)
	: _ifname(ifname)
    {}

    /**
     * @return Interface name command relates to.
     */
    const string& ifname() const		{ return _ifname; }

protected:
    string	_ifname;
};

/**
 * @short Command to add an interface.
 */
class IfMgrIfAdd : public IfMgrIfCommandBase {
public:
    IfMgrIfAdd(const string& ifname)
	: IfMgrIfCommandBase(ifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an interface.
 */
class IfMgrIfRemove : public IfMgrIfCommandBase {
public:
    IfMgrIfRemove(const string& ifname)
	: IfMgrIfCommandBase(ifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set enabled condition on interface.
 */
class IfMgrIfSetEnabled : public IfMgrIfCommandBase {
public:
    IfMgrIfSetEnabled(const string& ifname, bool enabled)
	: IfMgrIfCommandBase(ifname),
	  _enabled(enabled)
    {}

    bool enabled() const			{ return _enabled; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_enabled;
};

/**
 * @short Command to set discard condition on interface.
 */
class IfMgrIfSetDiscard : public IfMgrIfCommandBase {
public:
    IfMgrIfSetDiscard(const string& ifname, bool discard)
	: IfMgrIfCommandBase(ifname),
	  _discard(discard)
    {}

    bool discard() const			{ return _discard; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_discard;
};

/**
 * @short Command to set unreachable condition on interface.
 */
class IfMgrIfSetUnreachable : public IfMgrIfCommandBase {
public:
    IfMgrIfSetUnreachable(const string& ifname, bool unreachable)
	: IfMgrIfCommandBase(ifname),
	  _unreachable(unreachable)
    {}

    bool unreachable() const			{ return _unreachable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_unreachable;
};

/**
 * @short Command to set management condition on interface.
 */
class IfMgrIfSetManagement : public IfMgrIfCommandBase {
public:
    IfMgrIfSetManagement(const string& ifname, bool management)
	: IfMgrIfCommandBase(ifname),
	  _management(management)
    {}

    bool management() const			{ return _management; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_management;
};

/**
 * @short Command to set MTU of interface.
 */
class IfMgrIfSetMtu : public IfMgrIfCommandBase {
public:
    IfMgrIfSetMtu(const string& ifname, uint32_t mtu)
	: IfMgrIfCommandBase(ifname),
	  _mtu(mtu)
    {}

    uint32_t mtu() const			{ return _mtu; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_mtu;
};

/**
 * @short Command to set MAC address of interface.
 */
class IfMgrIfSetMac : public IfMgrIfCommandBase {
public:
    IfMgrIfSetMac(const string& ifname, const Mac& mac)
	: IfMgrIfCommandBase(ifname),
	  _mac(mac)
    {}

    const Mac& mac() const			{ return _mac; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    Mac		_mac;
};

/**
 * @short Command to set physical interface index.
 */
class IfMgrIfSetPifIndex : public IfMgrIfCommandBase {
public:
    IfMgrIfSetPifIndex(const string& ifname, uint32_t pif_index)
	: IfMgrIfCommandBase(ifname),
	  _pif_index(pif_index)
    {}

    uint32_t pif_index() const			{ return _pif_index; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_pif_index;
};

/**
 * @short Command to set the no_carrier flag of interface.
 */
class IfMgrIfSetNoCarrier : public IfMgrIfCommandBase {
public:
    IfMgrIfSetNoCarrier(const string& ifname, bool no_carrier)
	: IfMgrIfCommandBase(ifname),
	  _no_carrier(no_carrier)
    {}

    bool no_carrier() const			{ return _no_carrier; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_no_carrier;
};

/**
 * @short Command to set the baudrate of interface.
 */
class IfMgrIfSetBaudrate : public IfMgrIfCommandBase {
public:
    IfMgrIfSetBaudrate(const string& ifname, uint64_t baudrate)
	: IfMgrIfCommandBase(ifname),
	  _baudrate(baudrate)
    {}

    uint64_t baudrate() const			{ return _baudrate; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint64_t	_baudrate;
};


/**
 * @short Base class for virtual interface state manipulation commands.
 */
class IfMgrVifCommandBase : public IfMgrIfCommandBase {
public:
    IfMgrVifCommandBase(const string& ifname, const string& vifname)
	: IfMgrIfCommandBase(ifname),
	  _vifname(vifname)
    {}

    /**
     * @return Virtual interface name command relates to.
     */
    const string& vifname() const		{ return _vifname; }

protected:
    string	_vifname;
};

/**
 * @short Command to add a virtual interface to an interface.
 */
class IfMgrVifAdd : public IfMgrVifCommandBase {
public:
    IfMgrVifAdd(const string& ifname, const string& vifname)
	: IfMgrVifCommandBase(ifname, vifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove a virtual interface to an interface.
 */
class IfMgrVifRemove : public IfMgrVifCommandBase {
public:
    IfMgrVifRemove(const string& ifname, const string& vifname)
	: IfMgrVifCommandBase(ifname, vifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set enabled condition on a virtual interface.
 */
class IfMgrVifSetEnabled : public IfMgrVifCommandBase {
public:
    IfMgrVifSetEnabled(const string&	ifname,
		       const string&	vifname,
		       bool		enabled)
	: IfMgrVifCommandBase(ifname, vifname), _enabled(enabled)
    {}

    bool enabled() const 			{ return _enabled; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_enabled;
};

/**
 * @short Command to mark virtual interface as multicast capable.
 */
class IfMgrVifSetMulticastCapable : public IfMgrVifCommandBase {
public:
    IfMgrVifSetMulticastCapable(const string&	ifname,
				const string&	vifname,
				bool		multicast_capable)
	: IfMgrVifCommandBase(ifname, vifname),
	  _multicast_capable(multicast_capable)
    {}

    bool multicast_capable() const 	{ return _multicast_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_multicast_capable;
};

/**
 * @short Command to mark virtual interface as broadcast capable.
 */
class IfMgrVifSetBroadcastCapable : public IfMgrVifCommandBase {
public:
    IfMgrVifSetBroadcastCapable(const string&	ifname,
				const string&	vifname,
				bool		broadcast_capable)
	: IfMgrVifCommandBase(ifname, vifname),
	  _broadcast_capable(broadcast_capable)
    {}

    bool broadcast_capable() const 	{ return _broadcast_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_broadcast_capable;
};

/**
 * @short Command to mark virtual interface as point-to-point capable.
 */
class IfMgrVifSetP2PCapable : public IfMgrVifCommandBase {
public:
    IfMgrVifSetP2PCapable(const string&	ifname,
			  const string&	vifname,
			  bool		p2p_capable)
	: IfMgrVifCommandBase(ifname, vifname),
	  _p2p_capable(p2p_capable)
    {}

    bool p2p_capable() const 		{ return _p2p_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_p2p_capable;
};

/**
 * @short Command to mark virtual interface as loopback capable.
 */
class IfMgrVifSetLoopbackCapable : public IfMgrVifCommandBase {
public:
    IfMgrVifSetLoopbackCapable(const string&	ifname,
			       const string&	vifname,
			       bool		loopback_capable)
	: IfMgrVifCommandBase(ifname, vifname),
	  _loopback_capable(loopback_capable)
    {}

    bool loopback_capable() const 	{ return _loopback_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_loopback_capable;
};

/**
 * @short Command to mark virtual interface as PIM Register vif.
 */
class IfMgrVifSetPimRegister : public IfMgrVifCommandBase {
public:
    IfMgrVifSetPimRegister(const string&	ifname,
			   const string&	vifname,
			   bool			pim_register)
	: IfMgrVifCommandBase(ifname, vifname),
	  _pim_register(pim_register)
    {}

    bool pim_register() const 	{ return _pim_register; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_pim_register;
};

/**
 * @short Command to associate a physical interface id with a virtual
 * interface.
 */
class IfMgrVifSetPifIndex : public IfMgrVifCommandBase {
public:
    IfMgrVifSetPifIndex(const string&	ifname,
			const string&	vifname,
			uint32_t		pif_index)
	: IfMgrVifCommandBase(ifname, vifname),
	  _pif_index(pif_index)
    {}

    uint32_t pif_index() const 			{ return _pif_index; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_pif_index;
};

/**
 * @short Command to associate a virtual interface id with a virtual
 * interface.
 */
class IfMgrVifSetVifIndex : public IfMgrVifCommandBase {
public:
    IfMgrVifSetVifIndex(const string&	ifname,
			const string&	vifname,
			uint32_t	vif_index)
	: IfMgrVifCommandBase(ifname, vifname),
	  _vif_index(vif_index)
    {}

    uint32_t vif_index() const 			{ return _vif_index; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_vif_index;
};

/**
 * @short Command to mark virtual interface as VLAN vif.
 */
class IfMgrVifSetIsVlan : public IfMgrVifCommandBase {
public:
    IfMgrVifSetIsVlan(const string&	ifname,
		      const string&	vifname,
		      bool		is_vlan)
	: IfMgrVifCommandBase(ifname, vifname),
	  _is_vlan(is_vlan)
    {}

    bool is_vlan() const 	{ return _is_vlan; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_is_vlan;
};

/**
 * @short Command to set the VLAN ID to a virtual interface.
 */
class IfMgrVifSetVlanId : public IfMgrVifCommandBase {
public:
    IfMgrVifSetVlanId(const string&	ifname,
		      const string&	vifname,
		      uint16_t		vlan_id)
	: IfMgrVifCommandBase(ifname, vifname),
	  _vlan_id(vlan_id)
    {}

    uint16_t vlan_id() const 	{ return _vlan_id; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint16_t	_vlan_id;
};


/**
 * @short Base class for interface IPv4 address data manipulation.
 */
class IfMgrIPv4CommandBase : public IfMgrVifCommandBase {
public:
    IfMgrIPv4CommandBase(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr)
	: IfMgrVifCommandBase(ifname, vifname),
	  _addr(addr)
    {}

    /**
     * @return IPv4 address command relates to.
     */
    const IPv4& addr() const 			{ return _addr; }

protected:
    IPv4	_addr;
};

/**
 * @short Command to add an address to a virtual interface.
 */
class IfMgrIPv4Add : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4Add(const string&	ifname,
		 const string&	vifname,
		 const IPv4&	addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an address to a virtual interface.
 */
class IfMgrIPv4Remove : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4Remove(const string&	ifname,
		    const string&	vifname,
		    const IPv4&		addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set prefix of a virtual interface interface address.
 */
class IfMgrIPv4SetPrefix : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetPrefix(const string&	ifname,
		       const string&	vifname,
		       const IPv4&	addr,
		       uint32_t		prefix_len)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _prefix_len(prefix_len)
    {}

    uint32_t prefix_len() const			{ return _prefix_len; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_prefix_len;
};

/**
 * @short Command to set enabled flag of a virtual interface interface address.
 */
class IfMgrIPv4SetEnabled : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetEnabled(const string&	ifname,
			const string&	vifname,
			const IPv4&	addr,
			bool		enabled)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _enabled(enabled)
    {}

    bool enabled() const				{ return _enabled; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_enabled;
};

/**
 * @short Command to mark virtual interface address as multicast capable.
 */
class IfMgrIPv4SetMulticastCapable : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetMulticastCapable(const string&	ifname,
				 const string&	vifname,
				 const IPv4&	addr,
				 bool		multicast_capable)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _multicast_capable(multicast_capable)
    {}

    bool multicast_capable() const	{ return _multicast_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_multicast_capable;
};

/**
 * @short Command to mark virtual interface address as a loopback address.
 */
class IfMgrIPv4SetLoopback : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetLoopback(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr,
			 bool		loopback)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _loopback(loopback)
    {}

    bool loopback() const			{ return _loopback; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_loopback;
};

/**
 * @short Command to set broadcast address associated with a virtual
 * interface address.
 */
class IfMgrIPv4SetBroadcast : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetBroadcast(const string&	ifname,
			  const string&	vifname,
			  const IPv4&	addr,
			  const IPv4&	broadcast_addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _broadcast_addr(broadcast_addr)
    {}

    const IPv4& broadcast_addr() const	{ return _broadcast_addr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv4	_broadcast_addr;
};

/**
 * @short Command to set endpoint address associated with a virtual
 * interface address.
 */
class IfMgrIPv4SetEndpoint : public IfMgrIPv4CommandBase {
public:
    IfMgrIPv4SetEndpoint(const string&	ifname,
			 const string&	vifname,
			 const IPv4&	addr,
			 const IPv4&	endpoint_addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr),
	  _endpoint_addr(endpoint_addr)
    {}

    const IPv4& endpoint_addr() const		{ return _endpoint_addr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv4	_endpoint_addr;
};



#ifdef HAVE_IPV6

/**
 * @short Base class for interface IPv6 address data manipulation.
 */
class IfMgrIPv6CommandBase : public IfMgrVifCommandBase {
public:
    IfMgrIPv6CommandBase(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr)
	: IfMgrVifCommandBase(ifname, vifname),
	  _addr(addr)
    {}

    /**
     * @return IPv6 address command relates to.
     */
    const IPv6& addr() const 			{ return _addr; }

protected:
    IPv6	_addr;
};

/**
 * @short Command to add an address to a virtual interface.
 */
class IfMgrIPv6Add : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6Add(const string&	ifname,
		 const string&	vifname,
		 const IPv6&	addr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an address to a virtual interface.
 */
class IfMgrIPv6Remove : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6Remove(const string&	ifname,
		    const string&	vifname,
		    const IPv6&		addr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set prefix of a virtual interface interface address.
 */
class IfMgrIPv6SetPrefix : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6SetPrefix(const string&	ifname,
		       const string&	vifname,
		       const IPv6&	addr,
		       uint32_t		prefix_len)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _prefix_len(prefix_len)
    {}

    uint32_t prefix_len() const			{ return _prefix_len; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t	_prefix_len;
};

/**
 * @short Command to set enabled flag of a virtual interface interface address.
 */
class IfMgrIPv6SetEnabled : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6SetEnabled(const string&	ifname,
			const string&	vifname,
			const IPv6&	addr,
			bool		enabled)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _enabled(enabled)
    {}

    bool enabled() const				{ return _enabled; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_enabled;
};

/**
 * @short Command to mark virtual interface address as multicast capable.
 */
class IfMgrIPv6SetMulticastCapable : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6SetMulticastCapable(const string&	ifname,
				 const string&	vifname,
				 const IPv6&	addr,
				 bool		multicast_capable)
	: IfMgrIPv6CommandBase(ifname, vifname, addr),
	  _multicast_capable(multicast_capable)
    {}

    bool multicast_capable() const	{ return _multicast_capable; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_multicast_capable;
};

/**
 * @short Command to mark virtual interface address as a loopback address.
 */
class IfMgrIPv6SetLoopback : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6SetLoopback(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr,
			 bool		loopback)
	: IfMgrIPv6CommandBase(ifname, vifname, addr),
	  _loopback(loopback)
    {}

    bool loopback() const			{ return _loopback; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool	_loopback;
};

/**
 * @short Command to set endpoint address associated with a virtual
 * interface address.
 */
class IfMgrIPv6SetEndpoint : public IfMgrIPv6CommandBase {
public:
    IfMgrIPv6SetEndpoint(const string&	ifname,
			 const string&	vifname,
			 const IPv6&	addr,
			 const IPv6&	endpoint_addr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr),
	  _endpoint_addr(endpoint_addr)
    {}

    const IPv6& endpoint_addr() const	{ return _endpoint_addr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv6	_endpoint_addr;
};
#endif


/**
 * @short Base class for configuration events.
 *
 * These commands serve as hints to remote command recipients, eg complete
 * config tree sent, changes made.
 */
class IfMgrHintCommandBase : public IfMgrCommandBase {
public:
    /**
     * Apply command to local tree.  This is a no-op for this class
     * and its derivatives.
     *
     * @return success indication, always true.
     */
    bool execute(IfMgrIfTree& tree) const;
};

/**
 * @short Hint that configuration tree has been sent in it's entirety.
 */
class IfMgrHintTreeComplete : public IfMgrHintCommandBase {
public:
    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;
    string str() const;
};

/**
 * @short Hint that configuration tree has changed and tree should
 * be examined.
 */
class IfMgrHintUpdatesMade : public IfMgrHintCommandBase {
public:
    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;
    string str() const;
};

#endif // __LIBFEACLIENT_IFMGR_CMDS_HH__
