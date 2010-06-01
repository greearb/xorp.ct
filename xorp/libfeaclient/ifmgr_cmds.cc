// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "libxorp/c_format.hh"

#include "ifmgr_atoms.hh"
#include "ifmgr_cmds.hh"

#include "libxipc/xrl_sender.hh"
#include "xrl/interfaces/fea_ifmgr_mirror_xif.hh"

// ----------------------------------------------------------------------------
// Helper functions

/*
 * Makes string starting: IfMgrIfXXX("if0"
 */
static string
if_str_begin(const IfMgrIfCommandBase* i, const char* cmd)
{
    return string("IfMgrIf") + cmd + "(\"" + i->ifname() + "\"";
}

/*
 * Makes string that would complete if_str_begin().
 */
static const char*
if_str_end()
{
    return ")";
}

/*
 * Makes string starting: IfMgrVifXXX("if0", "vif32"
 */
static string
vif_str_begin(const IfMgrVifCommandBase* v, const char* cmd)
{
    return string("IfMgrVif") + cmd +
	"(\"" + v->ifname() + ", \"" + v->vifname() + "\"";
}

/*
 * Makes string that would complete vif_str_begin().
 */
static const char*
vif_str_end()
{
    return ")";
}

/*
 * Makes string starting: IfMgrIPv4XXX("if0", "vif32"
 */
static string
ipv4_str_begin(const IfMgrIPv4CommandBase* i, const char* cmd)
{
    return string("IfMgrIPv4") + cmd +
	"(\"" + i->ifname() + ", \"" + i->vifname() + "\", " + i->addr().str();
}

/*
 * Makes string that would complete ipv4_str_begin().
 */
static const char*
ipv4_str_end()
{
    return ")";
}

#ifdef HAVE_IPV6
/*
 * Makes string starting: IfMgrIPv6XXX("if0", "vif32"
 */
static string
ipv6_str_begin(const IfMgrIPv6CommandBase* i, const char* cmd)
{
    return string("IfMgrIPv6") + cmd +
	"(\"" + i->ifname() + ", \"" + i->vifname() + "\", " + i->addr().str();
}

/*
 * Makes string that would complete ipv6_str_begin().
 */
static const char*
ipv6_str_end()
{
    return ")";
}
#endif


// ----------------------------------------------------------------------------
// Destructor for abstract IfMgrCommandBase class

IfMgrCommandBase::~IfMgrCommandBase()
{
}


// ----------------------------------------------------------------------------
//
// 	I N T E R F A C E   C O N F I G U R A T I O N   C O M M A N D S
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// IfMgrIfCommandAdd

bool
IfMgrIfAdd::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    if (interfaces.find(n) != interfaces.end())
	return true;	// Not a failure to add something that already exists

    pair<IfMgrIfTree::IfMap::iterator, bool> r =
	interfaces.insert( make_pair(n, IfMgrIfAtom(n)) );
    return r.second;
}

bool
IfMgrIfAdd::forward(XrlSender&			sender,
		    const string&		xrl_target,
		    const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_add(xt, ifname(), xcb);
}

string
IfMgrIfAdd::str() const
{
    return if_str_begin(this, "Add") + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfRemove

bool
IfMgrIfRemove::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return true;	// Not a failure if the interface doesn't exist anymore

    interfaces.erase(i);
    return true;
}

bool
IfMgrIfRemove::forward(XrlSender&		sender,
		       const string&		xrl_target,
		       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_remove(xt, ifname(), xcb);
}

string
IfMgrIfRemove::str() const
{
    return if_str_begin(this, "Remove") + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetEnabled

bool
IfMgrIfSetEnabled::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_enabled(enabled());
    return true;
}

bool
IfMgrIfSetEnabled::forward(XrlSender&			sender,
			   const string&		xrl_target,
			   const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_enabled(xt, ifname(), enabled(), xcb);
}

string
IfMgrIfSetEnabled::str() const
{
    return if_str_begin(this, "SetEnabled")
	+ "\", " + bool_c_str(enabled()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetDiscard

bool
IfMgrIfSetDiscard::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_discard(discard());
    return true;
}

bool
IfMgrIfSetDiscard::forward(XrlSender&			sender,
			   const string&		xrl_target,
			   const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_discard(xt, ifname(), discard(), xcb);
}

string
IfMgrIfSetDiscard::str() const
{
    return if_str_begin(this, "SetDiscard")
	+ "\", " + bool_c_str(discard()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetUnreachable

bool
IfMgrIfSetUnreachable::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_unreachable(unreachable());
    return true;
}

bool
IfMgrIfSetUnreachable::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_unreachable(xt, ifname(), unreachable(), xcb);
}

string
IfMgrIfSetUnreachable::str() const
{
    return if_str_begin(this, "SetUnreachable")
	+ "\", " + bool_c_str(unreachable()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetManagement

bool
IfMgrIfSetManagement::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_management(management());
    return true;
}

bool
IfMgrIfSetManagement::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_management(xt, ifname(), management(), xcb);
}

string
IfMgrIfSetManagement::str() const
{
    return if_str_begin(this, "SetManagement")
	+ "\", " + bool_c_str(management()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetMtu

bool
IfMgrIfSetMtu::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_mtu(mtu());
    return true;
}

bool
IfMgrIfSetMtu::forward(XrlSender&		sender,
		       const string&		xrl_target,
		       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_mtu(xt, ifname(), mtu(), xcb);
}

string
IfMgrIfSetMtu::str() const
{
    return if_str_begin(this, "Mtu") + ", " +
	c_format("%u", XORP_UINT_CAST(mtu())) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetMac

bool
IfMgrIfSetMac::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_mac(mac());
    return true;
}

bool
IfMgrIfSetMac::forward(XrlSender&		sender,
		       const string&		xrl_target,
		       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_mac(xt, ifname(), mac(), xcb);
}

string
IfMgrIfSetMac::str() const
{
    return if_str_begin(this, "SetMac") + ", " + mac().str() + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetPifIndex

bool
IfMgrIfSetPifIndex::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_pif_index(pif_index());
    return true;
}

bool
IfMgrIfSetPifIndex::forward(XrlSender&			sender,
			    const string&		xrl_target,
			    const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_pif_index(xt, ifname(), pif_index(), xcb);
}

string
IfMgrIfSetPifIndex::str() const
{
    return if_str_begin(this, "SetPifIndex") +
	c_format(", %u", XORP_UINT_CAST(pif_index())) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetNoCarrier

bool
IfMgrIfSetNoCarrier::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_no_carrier(no_carrier());
    return true;
}

bool
IfMgrIfSetNoCarrier::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_no_carrier(xt, ifname(), no_carrier(), xcb);
}

string
IfMgrIfSetNoCarrier::str() const
{
    return if_str_begin(this, "NoCarrier") + ", " +
	c_format("%s", bool_c_str(no_carrier())) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetBaudrate

bool
IfMgrIfSetBaudrate::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& interfaces = t.interfaces();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = interfaces.find(n);
    if (i == interfaces.end())
	return false;

    IfMgrIfAtom& interface = i->second;
    interface.set_baudrate(baudrate());
    return true;
}

bool
IfMgrIfSetBaudrate::forward(XrlSender&			sender,
			    const string&		xrl_target,
			    const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_baudrate(xt, ifname(), baudrate(), xcb);
}

string
IfMgrIfSetBaudrate::str() const
{
    return if_str_begin(this, "Baudrate") + ", " +
	c_format("%u", XORP_UINT_CAST(baudrate())) + if_str_end();
}


// ----------------------------------------------------------------------------
//
//		V I F   C O N F I G U R A T I O N   C O M M A N D S
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// IfMgrVifAdd

bool
IfMgrVifAdd::execute(IfMgrIfTree& tree) const
{
    IfMgrIfAtom* ifa = tree.find_interface(ifname());
    if (ifa == NULL)
	return false;

    IfMgrIfAtom::VifMap& vifs = ifa->vifs();
    const string& n = vifname();

    if (vifs.find(n) != vifs.end())
	return true;	// Not a failure to add something that already exists

    pair<IfMgrIfAtom::VifMap::iterator, bool> r =
	vifs.insert( make_pair(n, IfMgrVifAtom(n)) );
    return r.second;
}

bool
IfMgrVifAdd::forward(XrlSender&			sender,
		     const string&		xrl_target,
		     const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_add(xt, ifname(), vifname(), xscb);
}

string
IfMgrVifAdd::str() const
{
    return vif_str_begin(this, "Add") + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifRemove

bool
IfMgrVifRemove::execute(IfMgrIfTree& tree) const
{
    IfMgrIfAtom* ifa = tree.find_interface(ifname());
    if (ifa == NULL)
	return true;	// Not a failure if the interface doesn't exist anymore

    IfMgrIfAtom::VifMap& vifs = ifa->vifs();
    const string& n = vifname();

    IfMgrIfAtom::VifMap::iterator i = vifs.find(n);
    if (i == vifs.end())
	return true;	// Not a failure if the vif doesn't exist anymore

    vifs.erase(i);
    return true;
}

bool
IfMgrVifRemove::forward(XrlSender&		sender,
			const string&		xrl_target,
			const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_remove(xt, ifname(), vifname(), xscb);
}

string
IfMgrVifRemove::str() const
{
    return vif_str_begin(this, "Remove") + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetEnabled

bool
IfMgrVifSetEnabled::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_enabled(enabled());
    return true;
}

bool
IfMgrVifSetEnabled::forward(XrlSender&			sender,
			    const string&		xrl_target,
			    const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_enabled(xt, ifname(), vifname(), enabled(), xscb);
}

string
IfMgrVifSetEnabled::str() const
{
    return vif_str_begin(this, "SetEnabled")
	+ ", " + bool_c_str(enabled()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetMulticastCapable

bool
IfMgrVifSetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_multicast_capable(multicast_capable());
    return true;
}

bool
IfMgrVifSetMulticastCapable::forward(XrlSender&			sender,
				     const string&		xrl_target,
				     const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_multicast_capable(xt, ifname(), vifname(),
					    multicast_capable(), xscb);
}

string
IfMgrVifSetMulticastCapable::str() const
{
    return vif_str_begin(this, "SetMulticastCapable")
	+ ", " + bool_c_str(multicast_capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetBroadcastCapable

bool
IfMgrVifSetBroadcastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_broadcast_capable(broadcast_capable());
    return true;
}

bool
IfMgrVifSetBroadcastCapable::forward(XrlSender&			sender,
				     const string&		xrl_target,
				     const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_broadcast_capable(xt, ifname(), vifname(),
					    broadcast_capable(), xscb);
}

string
IfMgrVifSetBroadcastCapable::str() const
{
    return vif_str_begin(this, "SetBroadcastCapable")
	+ ", " + bool_c_str(broadcast_capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetP2PCapable

bool
IfMgrVifSetP2PCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_p2p_capable(p2p_capable());
    return true;
}

bool
IfMgrVifSetP2PCapable::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_p2p_capable(xt, ifname(), vifname(), p2p_capable(),
				      xscb);
}

string
IfMgrVifSetP2PCapable::str() const
{
    return vif_str_begin(this, "SetP2PCapable")
	+ ", " + bool_c_str(p2p_capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetLoopbackCapable

bool
IfMgrVifSetLoopbackCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_loopback(loopback_capable());
    return true;
}

bool
IfMgrVifSetLoopbackCapable::forward(XrlSender&			sender,
				    const string&		xrl_target,
				    const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_loopback(xt, ifname(), vifname(), loopback_capable(),
				   xscb);
}

string
IfMgrVifSetLoopbackCapable::str() const
{
    return vif_str_begin(this, "SetLoopbackCapable")
	+ ", " + bool_c_str(loopback_capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetPimRegister

bool
IfMgrVifSetPimRegister::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_pim_register(pim_register());
    return true;
}

bool
IfMgrVifSetPimRegister::forward(XrlSender&		sender,
				const string&		xrl_target,
				const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_pim_register(xt, ifname(), vifname(),
				       pim_register(), xscb);
}

string
IfMgrVifSetPimRegister::str() const
{
    return vif_str_begin(this, "SetPimRegister")
	+ ", " + bool_c_str(pim_register()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetPifIndex

bool
IfMgrVifSetPifIndex::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_pif_index(pif_index());
    return true;
}

bool
IfMgrVifSetPifIndex::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_pif_index(xt, ifname(), vifname(), pif_index(),
				    xscb);
}

string
IfMgrVifSetPifIndex::str() const
{
    return vif_str_begin(this, "SetPifIndex")
	+ ", " + c_format("%u", XORP_UINT_CAST(pif_index())) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetVifIndex

bool
IfMgrVifSetVifIndex::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_vif_index(vif_index());
    return true;
}

bool
IfMgrVifSetVifIndex::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_vif_index(xt, ifname(), vifname(), vif_index(),
				    xscb);
}

string
IfMgrVifSetVifIndex::str() const
{
    return vif_str_begin(this, "SetVifIndex")
	+ ", " + c_format("%u", XORP_UINT_CAST(vif_index())) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetIsVlan

bool
IfMgrVifSetIsVlan::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_vlan(is_vlan());
    return true;
}

bool
IfMgrVifSetIsVlan::forward(XrlSender&			sender,
			   const string&		xrl_target,
			   const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_vlan(xt, ifname(), vifname(), is_vlan(), xscb);
}

string
IfMgrVifSetIsVlan::str() const
{
    return vif_str_begin(this, "SetIsVlan")
	+ ", " + bool_c_str(is_vlan()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetVlanId

bool
IfMgrVifSetVlanId::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    vifa->set_vlan_id(vlan_id());
    return true;
}

bool
IfMgrVifSetVlanId::forward(XrlSender&			sender,
			   const string&		xrl_target,
			   const IfMgrXrlSendCB&	xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_vlan_id(xt, ifname(), vifname(), vlan_id(), xscb);
}

string
IfMgrVifSetVlanId::str() const
{
    return vif_str_begin(this, "SetVlanId")
	+ ", " + c_format("%u", XORP_UINT_CAST(vlan_id())) + vif_str_end();
}


// ----------------------------------------------------------------------------
//
//     I P 4   A D D R E S S   C O N F I G U R A T I O N   C O M M A N D S
//
// ----------------------------------------------------------------------------

bool
IfMgrIPv4Add::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    IfMgrVifAtom::IPv4Map& addrs = vifa->ipv4addrs();
    if (addrs.find(addr()) != addrs.end())
	return true;	// Not a failure to add something that already exists

    pair<IfMgrVifAtom::IPv4Map::iterator, bool> r =
	addrs.insert( make_pair(addr(), IfMgrIPv4Atom(addr())) );
    return r.second;
}

bool
IfMgrIPv4Add::forward(XrlSender&		sender,
		      const string&		xrl_target,
		      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_add(xt, ifname(), vifname(), addr(), xcb);
}

string
IfMgrIPv4Add::str() const
{
    return ipv4_str_begin(this, "Add") + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4Remove

bool
IfMgrIPv4Remove::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return true;	// Not a failure if the vif doesn't exist anymore

    IfMgrVifAtom::IPv4Map& addrs = vifa->ipv4addrs();
    IfMgrVifAtom::IPv4Map::iterator i = addrs.find(addr());
    if (i == addrs.end())
	return true;	// Not a failure if the address doesn't exist anymore

    addrs.erase(i);
    return true;
}

bool
IfMgrIPv4Remove::forward(XrlSender&		sender,
			 const string&		xrl_target,
			 const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_remove(xt, ifname(), vifname(), addr(), xcb);
}

string
IfMgrIPv4Remove::str() const
{
    return ipv4_str_begin(this, "Remove") + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetPrefix

bool
IfMgrIPv4SetPrefix::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_prefix_len(prefix_len());
    return true;
}

bool
IfMgrIPv4SetPrefix::forward(XrlSender&			sender,
			    const string&		xrl_target,
			    const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_prefix(xt, ifname(), vifname(), addr(),
				  prefix_len(), xcb);
}

string
IfMgrIPv4SetPrefix::str() const
{
    return ipv4_str_begin(this, "SetPrefix") + ", "
	+ c_format("%u", XORP_UINT_CAST(prefix_len())) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetEnabled

bool
IfMgrIPv4SetEnabled::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_enabled(enabled());
    return true;
}

bool
IfMgrIPv4SetEnabled::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_enabled(xt, ifname(), vifname(), addr(), enabled(),
				   xcb);
}

string
IfMgrIPv4SetEnabled::str() const
{
    return ipv4_str_begin(this, "SetEnabled") + ", " +
	bool_c_str(enabled()) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetMulticastCapable

bool
IfMgrIPv4SetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_multicast_capable(multicast_capable());
    return true;
}

bool
IfMgrIPv4SetMulticastCapable::forward(XrlSender&		sender,
				      const string&		xrl_target,
				      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_multicast_capable(xt, ifname(), vifname(), addr(),
					     multicast_capable(), xcb);
}

string
IfMgrIPv4SetMulticastCapable::str() const
{
    return ipv4_str_begin(this, "SetMulticastCapable") + ", " +
	bool_c_str(multicast_capable()) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetLoopback

bool
IfMgrIPv4SetLoopback::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_loopback(loopback());
    return true;
}

bool
IfMgrIPv4SetLoopback::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_loopback(xt, ifname(), vifname(), addr(),
				    loopback(), xcb);
}

string
IfMgrIPv4SetLoopback::str() const
{
    return ipv4_str_begin(this, "SetLoopback") + ", " +
	bool_c_str(loopback()) + ipv4_str_end();
}


// ----------------------------------------------------------------------------
// IfMgrIPv4SetBroadcast

bool
IfMgrIPv4SetBroadcast::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_broadcast_addr(broadcast_addr());
    return true;
}

bool
IfMgrIPv4SetBroadcast::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_broadcast(xt, ifname(), vifname(), addr(),
				     broadcast_addr(), xcb);
}

string
IfMgrIPv4SetBroadcast::str() const
{
    return ipv4_str_begin(this, "SetBroadcast") + ", " +
	broadcast_addr().str() + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetEndpoint

bool
IfMgrIPv4SetEndpoint::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_endpoint_addr(endpoint_addr());
    return true;
}

bool
IfMgrIPv4SetEndpoint::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_endpoint(xt, ifname(), vifname(), addr(),
				    endpoint_addr(), xcb);
}

string
IfMgrIPv4SetEndpoint::str() const
{
    return ipv4_str_begin(this, "SetEndpoint") + ", " +
	endpoint_addr().str() + ipv4_str_end();
}

#ifdef HAVE_IPV6
// ----------------------------------------------------------------------------
//
//     I P 6   A D D R E S S   C O N F I G U R A T I O N   C O M M A N D S
//
// ----------------------------------------------------------------------------

bool
IfMgrIPv6Add::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return false;

    IfMgrVifAtom::IPv6Map& addrs = vifa->ipv6addrs();
    if (addrs.find(addr()) != addrs.end())
	return true;	// Not a failure to add something that already exists

    pair<IfMgrVifAtom::IPv6Map::iterator, bool> r =
	addrs.insert( make_pair(addr(), IfMgrIPv6Atom(addr())) );
    return r.second;
}

bool
IfMgrIPv6Add::forward(XrlSender&		sender,
		      const string&		xrl_target,
		      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_add(xt, ifname(), vifname(), addr(), xcb);
}

string
IfMgrIPv6Add::str() const
{
    return ipv6_str_begin(this, "Add") + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6Remove

bool
IfMgrIPv6Remove::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == NULL)
	return true;	// Not a failure if the vif doesn't exist anymore

    IfMgrVifAtom::IPv6Map& addrs = vifa->ipv6addrs();
    IfMgrVifAtom::IPv6Map::iterator i = addrs.find(addr());
    if (i == addrs.end())
	return true;	// Not a failure if the address doesn't exist anymore

    addrs.erase(i);
    return true;
}

bool
IfMgrIPv6Remove::forward(XrlSender&		sender,
			 const string&		xrl_target,
			 const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_remove(xt, ifname(), vifname(), addr(), xcb);
}

string
IfMgrIPv6Remove::str() const
{
    return ipv6_str_begin(this, "Remove") + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetPrefix

bool
IfMgrIPv6SetPrefix::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_prefix_len(prefix_len());
    return true;
}

bool
IfMgrIPv6SetPrefix::forward(XrlSender&			sender,
			    const string&		xrl_target,
			    const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_prefix(xt, ifname(), vifname(), addr(),
				  prefix_len(), xcb);
}

string
IfMgrIPv6SetPrefix::str() const
{
    return ipv6_str_begin(this, "SetPrefix") + ", "
	+ c_format("%u", XORP_UINT_CAST(prefix_len())) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetEnabled

bool
IfMgrIPv6SetEnabled::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_enabled(enabled());
    return true;
}

bool
IfMgrIPv6SetEnabled::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_enabled(xt, ifname(), vifname(), addr(), enabled(),
				   xcb);
}

string
IfMgrIPv6SetEnabled::str() const
{
    return ipv6_str_begin(this, "SetEnabled") + ", " +
	bool_c_str(enabled()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetMulticastCapable

bool
IfMgrIPv6SetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_multicast_capable(multicast_capable());
    return true;
}

bool
IfMgrIPv6SetMulticastCapable::forward(XrlSender&		sender,
				      const string&		xrl_target,
				      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_multicast_capable(xt, ifname(), vifname(), addr(),
					     multicast_capable(), xcb);
}

string
IfMgrIPv6SetMulticastCapable::str() const
{
    return ipv6_str_begin(this, "SetMulticastCapable") + ", " +
	bool_c_str(multicast_capable()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetLoopback

bool
IfMgrIPv6SetLoopback::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_loopback(loopback());
    return true;
}

bool
IfMgrIPv6SetLoopback::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_loopback(xt, ifname(), vifname(), addr(),
				    loopback(), xcb);
}

string
IfMgrIPv6SetLoopback::str() const
{
    return ipv6_str_begin(this, "SetLoopback") + ", " +
	bool_c_str(loopback()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetEndpoint

bool
IfMgrIPv6SetEndpoint::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == NULL)
	return false;

    a->set_endpoint_addr(endpoint_addr());
    return true;
}

bool
IfMgrIPv6SetEndpoint::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_endpoint(xt, ifname(), vifname(), addr(),
				    endpoint_addr(), xcb);
}

string
IfMgrIPv6SetEndpoint::str() const
{
    return ipv6_str_begin(this, "SetEndpoint") + ", " +
	endpoint_addr().str() + ipv6_str_end();
}
#endif


// ----------------------------------------------------------------------------
//
// 	 		   H I N T   C O M M A N D S
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// IfMgrHintCommandBase

bool
IfMgrHintCommandBase::execute(IfMgrIfTree&) const
{
    return true;
}

// ----------------------------------------------------------------------------
// IfMgrHintTreeComplete

bool
IfMgrHintTreeComplete::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_hint_tree_complete(xt, xcb);
}

string
IfMgrHintTreeComplete::str() const
{
    return "IfMgrHintTreeComplete";
}

// ----------------------------------------------------------------------------
// IfMgrHintUpdatesMade

bool
IfMgrHintUpdatesMade::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_hint_updates_made(xt, xcb);
}

string
IfMgrHintUpdatesMade::str() const
{
    return "IfMgrHintUpdatesMade";
}
