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

#ident "$XORP: xorp/libfeaclient/ifmgr_cmds.cc,v 1.6 2003/09/30 03:07:57 pavlin Exp $"

#include "libxorp/c_format.hh"

#include "ifmgr_atoms.hh"
#include "ifmgr_cmds.hh"

#include "libxipc/xrl_sender.hh"
#include "xrl/interfaces/fea_ifmgr_mirror_xif.hh"

// ----------------------------------------------------------------------------
// Helper functions

static const char*
bool2str(bool t)
{
    return t ? "true" : "false";
}

/*
 * Makes string starting: IfMgrIfCommandXXX("if0"
 */
static string
if_str_begin(const IfMgrIfCommandBase* i, const char* cmd)
{
    return string("IfMgrIfCommand") + cmd + "(\"" + i->ifname() + "\"";
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
 * Makes string starting: IfMgrVifCommandXXX("if0", "vif32"
 */
static string
vif_str_begin(const IfMgrVifCommandBase* v, const char* cmd)
{
    return string("IfMgrVifCommand") + cmd +
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
 * Makes string starting: IfMgrIPv4CommandXXX("if0", "vif32"
 */
static string
ipv4_str_begin(const IfMgrIPv4CommandBase* i, const char* cmd)
{
    return string("IfMgrIPv4Command") + cmd +
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

/*
 * Makes string starting: IfMgrIPv6CommandXXX("if0", "vif32"
 */
static string
ipv6_str_begin(const IfMgrIPv6CommandBase* i, const char* cmd)
{
    return string("IfMgrIPv6Command") + cmd +
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
    IfMgrIfTree::IfMap& ifs = t.ifs();
    const string& n = ifname();

    if (ifs.find(n) != ifs.end()) {
	return true;	// Not a failure to add something that already exists
    }
    pair<IfMgrIfTree::IfMap::iterator, bool> r =
	ifs.insert( make_pair(n, IfMgrIfAtom(n)) );
    return r.second;
}

bool
IfMgrIfAdd::forward(XrlSender&		  sender,
		    const string&	  xrl_target,
		    const IfMgrXrlSendCB& xcb) const
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
    IfMgrIfTree::IfMap& ifs = t.ifs();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = ifs.find(n);
    if (i != ifs.end()) {
	ifs.erase(i);
	return true;
    }
    return false;
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
    IfMgrIfTree::IfMap& ifs = t.ifs();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = ifs.find(n);
    if (i != ifs.end()) {
	IfMgrIfAtom& interface = i->second;
	interface.set_enabled(en());
	return true;
    }
    return false;
}

bool
IfMgrIfSetEnabled::forward(XrlSender&		 sender,
			   const string&	 xrl_target,
			   const IfMgrXrlSendCB& xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_enabled(xt, ifname(), en(), xcb);
}

string
IfMgrIfSetEnabled::str() const
{
    return if_str_begin(this, "SetEnabled")
	+ "\", " + bool2str(en()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetMtu

bool
IfMgrIfSetMtu::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& ifs = t.ifs();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = ifs.find(n);
    if (i != ifs.end()) {
	IfMgrIfAtom& interface = i->second;
	interface.set_mtu_bytes(mtu_bytes());
	return true;
    }
    return false;
}

bool
IfMgrIfSetMtu::forward(XrlSender&	     sender,
		       const string&	     xrl_target,
		       const IfMgrXrlSendCB& xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_interface_set_mtu(xt, ifname(), mtu_bytes(), xcb);
}

string
IfMgrIfSetMtu::str() const
{
    return if_str_begin(this, "Mtu") + ", " +
	c_format("%u", mtu_bytes()) + if_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIfSetMac

bool
IfMgrIfSetMac::execute(IfMgrIfTree& t) const
{
    IfMgrIfTree::IfMap& ifs = t.ifs();
    const string& n = ifname();

    IfMgrIfTree::IfMap::iterator i = ifs.find(n);
    if (i != ifs.end()) {
	IfMgrIfAtom& interface = i->second;
	interface.set_mac(mac());
	return true;
    }
    return false;
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
    return if_str_begin(this, "Mac") + ", " + mac().str() + if_str_end();
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
    IfMgrIfAtom* ifa = tree.find_if(ifname());
    if (ifa == 0)
	return false;

    IfMgrIfAtom::VifMap& vifs = ifa->vifs();
    const string& n = vifname();

    if (vifs.find(n) != vifs.end()) {
	return true;	// Not a failure to add something that already exists
    }
    pair<IfMgrIfAtom::VifMap::iterator, bool> r =
	vifs.insert( make_pair(n, IfMgrVifAtom(n)) );
    return r.second;
}

bool
IfMgrVifAdd::forward(XrlSender&		   sender,
		     const string&	   xrl_target,
		     const IfMgrXrlSendCB& xscb) const
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
    IfMgrIfAtom* ifa = tree.find_if(ifname());
    if (ifa == 0)
	return false;

    IfMgrIfAtom::VifMap& vifs = ifa->vifs();
    const string& n = vifname();

    IfMgrIfAtom::VifMap::iterator i = vifs.find(n);
    if (i != vifs.end()) {
	vifs.erase(i);
	return true;
    }
    return false;
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
    if (vifa == 0)
	return false;

    vifa->set_enabled(en());
    return true;
}

bool
IfMgrVifSetEnabled::forward(XrlSender&		  sender,
			    const string&	  xrl_target,
			    const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_vif_set_enabled(xt, ifname(), vifname(), en(), xscb);
}

string
IfMgrVifSetEnabled::str() const
{
    return vif_str_begin(this, "SetEnabled")
	+ ", " + bool2str(en()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetMulticastCapable

bool
IfMgrVifSetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    vifa->set_multicast_capable(capable());
    return true;
}

bool
IfMgrVifSetMulticastCapable::forward(XrlSender&		   sender,
				     const string&	   xrl_target,
				     const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& in = ifname();
    const string& vn = vifname();
    return c.send_vif_set_multicast_capable(xt, in, vn, capable(), xscb);
}

string
IfMgrVifSetMulticastCapable::str() const
{
    return vif_str_begin(this, "SetMulticastCapable")
	+ ", " + bool2str(capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetBroadcastCapable

bool
IfMgrVifSetBroadcastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    vifa->set_broadcast_capable(capable());
    return true;
}

bool
IfMgrVifSetBroadcastCapable::forward(XrlSender&		   sender,
				     const string&	   xrl_target,
				     const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& in = ifname();
    const string& vn = vifname();
    return c.send_vif_set_broadcast_capable(xt, in, vn, capable(), xscb);
}

string
IfMgrVifSetBroadcastCapable::str() const
{
    return vif_str_begin(this, "SetBroadcastCapable")
	+ ", " + bool2str(capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetP2PCapable

bool
IfMgrVifSetP2PCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    vifa->set_p2p_capable(capable());
    return true;
}

bool
IfMgrVifSetP2PCapable::forward(XrlSender&	     sender,
			       const string&	     xrl_target,
			       const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const bool& cap = capable();
    return c.send_vif_set_p2p_capable(xt, ifname(), vifname(), cap, xscb);
}

string
IfMgrVifSetP2PCapable::str() const
{
    return vif_str_begin(this, "SetP2PCapable")
	+ ", " + bool2str(capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetLoopbackCapable

bool
IfMgrVifSetLoopbackCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    vifa->set_loopback(capable());
    return true;
}

bool
IfMgrVifSetLoopbackCapable::forward(XrlSender&		  sender,
				    const string&	  xrl_target,
				    const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const bool& cap = capable();
    return c.send_vif_set_loopback(xt, ifname(), vifname(), cap, xscb);
}

string
IfMgrVifSetLoopbackCapable::str() const
{
    return vif_str_begin(this, "SetLoopbackCapable")
	+ ", " + bool2str(capable()) + vif_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrVifSetPifIndex

bool
IfMgrVifSetPifIndex::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    vifa->set_pif_index(pif_index());
    return true;
}

bool
IfMgrVifSetPifIndex::forward(XrlSender&		   sender,
			     const string&	   xrl_target,
			     const IfMgrXrlSendCB& xscb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& in = ifname();
    const string& vn = vifname();
    return c.send_vif_set_pif_index(xt, in, vn, pif_index(), xscb);
}

string
IfMgrVifSetPifIndex::str() const
{
    return vif_str_begin(this, "SetPifIndex")
	+ ", " + c_format("%u", pif_index()) + vif_str_end();
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
    if (vifa == 0)
	return false;

    IfMgrVifAtom::V4Map& addrs = vifa->ipv4addrs();
    if (addrs.find(addr()) != addrs.end()) {
	return true;	// Not a failure to add something that already exists
    }
    pair<IfMgrVifAtom::V4Map::iterator, bool> r =
	addrs.insert( make_pair(addr(), IfMgrIPv4Atom(addr())) );
    return r.second;
}

bool
IfMgrIPv4Add::forward(XrlSender&	    sender,
		      const string&	    xrl_target,
		      const IfMgrXrlSendCB& xcb) const
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
    if (vifa == 0)
	return false;

    IfMgrVifAtom::V4Map& addrs = vifa->ipv4addrs();
    IfMgrVifAtom::V4Map::iterator i = addrs.find(addr());
    if (i != addrs.end()) {
	addrs.erase(i);
	return true;
    }
    return false;
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
    if (a == 0)
	return false;

    a->set_prefix_len(prefix_len());
    return true;
}

bool
IfMgrIPv4SetPrefix::forward(XrlSender&		  sender,
			    const string&	  xrl_target,
			    const IfMgrXrlSendCB& xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv4_set_prefix(xt, ifn, vifn, addr(), prefix_len(), xcb);
}

string
IfMgrIPv4SetPrefix::str() const
{
    return ipv4_str_begin(this, "SetPrefix") + ", "
	+ c_format("%u", prefix_len()) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetEnabled

bool
IfMgrIPv4SetEnabled::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_enabled(en());
    return true;
}

bool
IfMgrIPv4SetEnabled::forward(XrlSender&		   sender,
			     const string&	   xrl_target,
			     const IfMgrXrlSendCB& xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv4_set_enabled(xt, ifname(), vifname(), addr(), en(), xcb);
}

string
IfMgrIPv4SetEnabled::str() const
{
    return ipv4_str_begin(this, "SetEnabled") + ", " +
	bool2str(en()) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetMulticastCapable

bool
IfMgrIPv4SetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_multicast_capable(capable());
    return true;
}

bool
IfMgrIPv4SetMulticastCapable::forward(XrlSender&		sender,
				      const string&		xrl_target,
				      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    IPv4 a = addr();
    return c.send_ipv4_set_multicast_capable(xt, ifn, vifn, a, capable(), xcb);
}

string
IfMgrIPv4SetMulticastCapable::str() const
{
    return ipv4_str_begin(this, "SetMulticastCapable") + ", " +
	bool2str(capable()) + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetLoopback

bool
IfMgrIPv4SetLoopback::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
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
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv4_set_loopback(xt, ifn, vifn, addr(), loopback(), xcb);
}

string
IfMgrIPv4SetLoopback::str() const
{
    return ipv4_str_begin(this, "SetLoopback") + ", " +
	bool2str(loopback()) + ipv4_str_end();
}


// ----------------------------------------------------------------------------
// IfMgrIPv4SetBroadcast

bool
IfMgrIPv4SetBroadcast::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_broadcast_addr(oaddr());
    return true;
}

bool
IfMgrIPv4SetBroadcast::forward(XrlSender&		sender,
			       const string&		xrl_target,
			       const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv4_set_broadcast(xt, ifn, vifn, addr(), oaddr(), xcb);
}

string
IfMgrIPv4SetBroadcast::str() const
{
    return ipv4_str_begin(this, "SetBroadcast") + ", " +
	oaddr().str() + ipv4_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv4SetEndpoint

bool
IfMgrIPv4SetEndpoint::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv4Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_endpoint_addr(oaddr());
    return true;
}

bool
IfMgrIPv4SetEndpoint::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv4_set_endpoint(xt, ifn, vifn, addr(), oaddr(), xcb);
}

string
IfMgrIPv4SetEndpoint::str() const
{
    return ipv4_str_begin(this, "SetEndpoint") + ", " +
	oaddr().str() + ipv4_str_end();
}

// ----------------------------------------------------------------------------
//
//     I P 6   A D D R E S S   C O N F I G U R A T I O N   C O M M A N D S
//
// ----------------------------------------------------------------------------

bool
IfMgrIPv6Add::execute(IfMgrIfTree& tree) const
{
    IfMgrVifAtom* vifa = tree.find_vif(ifname(), vifname());
    if (vifa == 0)
	return false;

    IfMgrVifAtom::V6Map& addrs = vifa->ipv6addrs();
    if (addrs.find(addr()) != addrs.end()) {
	return true;	// Not a failure to add something that already exists
    }
    pair<IfMgrVifAtom::V6Map::iterator, bool> r =
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
    if (vifa == 0)
	return false;

    IfMgrVifAtom::V6Map& addrs = vifa->ipv6addrs();
    IfMgrVifAtom::V6Map::iterator i = addrs.find(addr());
    if (i != addrs.end()) {
	addrs.erase(i);
	return true;
    }
    return false;
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
    if (a == 0)
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
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv6_set_prefix(xt, ifn, vifn, addr(), prefix_len(), xcb);
}

string
IfMgrIPv6SetPrefix::str() const
{
    return ipv6_str_begin(this, "SetPrefix") + ", "
	+ c_format("%u", prefix_len()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetEnabled

bool
IfMgrIPv6SetEnabled::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_enabled(en());
    return true;
}

bool
IfMgrIPv6SetEnabled::forward(XrlSender&			sender,
			     const string&		xrl_target,
			     const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    return c.send_ipv6_set_enabled(xt, ifname(), vifname(), addr(), en(), xcb);
}

string
IfMgrIPv6SetEnabled::str() const
{
    return ipv6_str_begin(this, "SetEnabled") + ", " +
	bool2str(en()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetMulticastCapable

bool
IfMgrIPv6SetMulticastCapable::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_multicast_capable(capable());
    return true;
}

bool
IfMgrIPv6SetMulticastCapable::forward(XrlSender&		sender,
				      const string&		xrl_target,
				      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    const IPv6& a = addr();
    return c.send_ipv6_set_multicast_capable(xt, ifn, vifn, a, capable(), xcb);
}

string
IfMgrIPv6SetMulticastCapable::str() const
{
    return ipv6_str_begin(this, "SetMulticastCapable") + ", " +
	bool2str(capable()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetLoopback

bool
IfMgrIPv6SetLoopback::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
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
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv6_set_loopback(xt, ifn, vifn, addr(), loopback(), xcb);
}

string
IfMgrIPv6SetLoopback::str() const
{
    return ipv6_str_begin(this, "SetLoopback") + ", " +
	bool2str(loopback()) + ipv6_str_end();
}

// ----------------------------------------------------------------------------
// IfMgrIPv6SetEndpoint

bool
IfMgrIPv6SetEndpoint::execute(IfMgrIfTree& tree) const
{
    IfMgrIPv6Atom* a = tree.find_addr(ifname(), vifname(), addr());
    if (a == 0)
	return false;

    a->set_endpoint_addr(oaddr());
    return true;
}

bool
IfMgrIPv6SetEndpoint::forward(XrlSender&		sender,
			      const string&		xrl_target,
			      const IfMgrXrlSendCB&	xcb) const
{
    XrlFeaIfmgrMirrorV0p1Client c(&sender);
    const char* xt = xrl_target.c_str();
    const string& ifn = ifname();
    const string& vifn = vifname();
    return c.send_ipv6_set_endpoint(xt, ifn, vifn, addr(), oaddr(), xcb);
}

string
IfMgrIPv6SetEndpoint::str() const
{
    return ipv6_str_begin(this, "SetEndpoint") + ", " +
	oaddr().str() + ipv6_str_end();
}


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
