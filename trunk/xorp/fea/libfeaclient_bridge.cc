// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/libfeaclient_bridge.cc,v 1.20 2007/04/19 21:36:49 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_atoms.hh"
#include "libfeaclient/ifmgr_cmds.hh"
#include "libfeaclient/ifmgr_xrl_replicator.hh"

#include "libfeaclient_bridge.hh"

// ----------------------------------------------------------------------------
// Debug helpers

static const char*
update_name(IfConfigUpdateReporterBase::Update u)
{
    switch (u) {
    case IfConfigUpdateReporterBase::CREATED:
	return "Creation";
    case IfConfigUpdateReporterBase::DELETED:
	return "Deletion";
    case IfConfigUpdateReporterBase::CHANGED:
	break;					// FALLTHROUGH
    }
    return "Change";
}

static const char*
truth_of(bool v)
{
    return v ? "true" : "false";
}

// ----------------------------------------------------------------------------
// LibFeaClientBridge implementation

LibFeaClientBridge::LibFeaClientBridge(XrlRouter& rtr, const IfTree& iftree)
    : _iftree(iftree)
{
    _rm = new IfMgrXrlReplicationManager(rtr);
}

LibFeaClientBridge::~LibFeaClientBridge()
{
    delete _rm;
}

bool
LibFeaClientBridge::add_libfeaclient_mirror(const string& m)
{
    return _rm->add_mirror(m);
}

bool
LibFeaClientBridge::remove_libfeaclient_mirror(const string& m)
{
    return _rm->remove_mirror(m);
}

const IfMgrIfTree&
LibFeaClientBridge::libfeaclient_iftree() const
{
    return _rm->iftree();
}

const IfTree&
LibFeaClientBridge::fea_iftree() const
{
    return _iftree;
}

void
LibFeaClientBridge::interface_update(const string& ifname,
				     const Update& update,
				     bool	   system)
{
    debug_msg("%s update for interface %s (system = %s)\n",
	      update_name(update), ifname.c_str(), truth_of(system));

    if (system) {
	//
	// XXX: We don't propagate interface-related changes that just
	// appear beneath us within the system. I.e., all changes
	// should be through the FEA.
	//
	return;
    }

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIfAdd(ifname));
	break;					// FALLTHROUGH

    case DELETED:
	_rm->push(new IfMgrIfRemove(ifname));
	return;

    case CHANGED:
	break;					// FALLTHROUGH
    }

    //
    // Validate interface is in the FEA iftree we're using and
    // in libfeaclient's equivalent.
    //
    const IfMgrIfAtom* ifa = _rm->iftree().find_if(ifname);
    if (ifa == 0) {
	XLOG_WARNING("Got update for interface not in libfeaclient tree: "
		     "%s\n", ifname.c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	XLOG_WARNING("Got update for interface not in FEA tree: %s\n",
		     ifname.c_str());
	return;
    }

    //
    // Copy interface state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new IfMgrIfSetEnabled(ifname, ii->second.enabled()));
    _rm->push(new IfMgrIfSetDiscard(ifname, ii->second.discard()));
    _rm->push(new IfMgrIfSetMtu(ifname, ii->second.mtu()));
    _rm->push(new IfMgrIfSetMac(ifname, ii->second.mac()));
    _rm->push(new IfMgrIfSetPifIndex(ifname, ii->second.pif_index()));
    _rm->push(new IfMgrIfSetNoCarrier(ifname, ii->second.no_carrier()));

    //
    // XXX TODO / TBD if need doing...
    // _rm->push(new IfMgrIfSetIfFlags(ifname, ii- if_flags - relevant ????
    //
}


void
LibFeaClientBridge::vif_update(const string& ifname,
			       const string& vifname,
			       const Update& update,
			       bool	     system)
{
    debug_msg("%s update for vif %s/%s (system = %s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      truth_of(system));

    if (system) {
	//
	// XXX: We don't propagate interface-related changes that just
	// appear beneath us within the system. I.e., all changes
	// should be through the FEA.
	//
	return;
    }

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrVifAdd(ifname, vifname));
	break;					// FALLTHROUGH

    case DELETED:
	_rm->push(new IfMgrVifRemove(ifname, vifname));
	return;

    case CHANGED:
	break;					// FALLTHROUGH
    }

    //
    // Validate vif is in the FEA iftree we're using and in
    // libfeaclient's equivalent.
    //
    const IfMgrVifAtom* ifa = _rm->iftree().find_vif(ifname, vifname);
    if (ifa == 0) {
	XLOG_WARNING("Got update for vif not in libfeaclient tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	XLOG_WARNING("Got update for vif on interface not in tree:"
		     "%s/(%s)", ifname.c_str(), vifname.c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for vif not in FEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    //
    // Copy vif state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    const IfTreeVif& vif = vi->second;
    _rm->push(new
	      IfMgrVifSetEnabled(ifname, vifname, vif.enabled())
	      );
    _rm->push(new
	      IfMgrVifSetBroadcastCapable(ifname, vifname, vif.broadcast())
	      );
    _rm->push(new
	      IfMgrVifSetLoopbackCapable(ifname, vifname, vif.loopback())
	      );
    _rm->push(new
	      IfMgrVifSetP2PCapable(ifname, vifname, vif.point_to_point())
	      );
    _rm->push(new
	      IfMgrVifSetMulticastCapable(ifname, vifname, vif.multicast())
	      );
    _rm->push(new
	      IfMgrVifSetPifIndex(ifname, vifname, vif.pif_index())
	      );
}


void
LibFeaClientBridge::vifaddr4_update(const string& ifname,
				    const string& vifname,
				    const IPv4&   addr,
				    const Update& update,
				    bool	  system)
{
    debug_msg("%s update for address %s/%s/%s (system = %s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str(), truth_of(system));

    if (system) {
	//
	// XXX: We don't propagate interface-related changes that just
	// appear beneath us within the system. I.e., all changes
	// should be through the FEA.
	//
	return;
    }

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIPv4Add(ifname, vifname, addr));
	break;					// FALLTHROUGH

    case DELETED:
	_rm->push(new IfMgrIPv4Remove(ifname, vifname, addr));
	return;

    case CHANGED:
	break;					// FALLTHROUGH
    }

    //
    // Validate vif address is in the FEA iftree we're using and in
    // libfeaclient's equivalent
    //
    const IfMgrIPv4Atom* ifa = _rm->iftree().find_addr(ifname,
						       vifname,
						       addr);
    if (ifa == 0) {
	XLOG_WARNING("Got update for address no in libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	XLOG_WARNING("Got update for address on interface not in tree: "
		     "%s/(%s/%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for address on vif not in FEA tree: "
		     "%s/%s/(%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif& vif = vi->second;
    IfTreeVif::IPv4Map::const_iterator ai = vif.get_addr(addr);
    if (ai == vif.ipv4addrs().end()) {
	XLOG_WARNING("Got update for address not in FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr4& a4 = ai->second;

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv4SetEnabled(ifname, vifname, addr, a4.enabled())
	      );
    _rm->push(new
	      IfMgrIPv4SetLoopback(ifname, vifname, addr, a4.loopback())
	      );
    _rm->push(new
	      IfMgrIPv4SetMulticastCapable(ifname, vifname, addr,
					   a4.multicast())
	      );
    _rm->push(new
	      IfMgrIPv4SetPrefix(ifname, vifname, addr, a4.prefix_len())
	      );
    if (a4.point_to_point()) {
	const IPv4& end = a4.endpoint();
	_rm->push(new IfMgrIPv4SetEndpoint(ifname, vifname, addr, end));
    } else {
	// NB if IfTreeAddr4::bcast() will return IPv4::ZERO() if
	// broadcast is not supported.  This happens to be the
	// correct argument for libfeaclient to signify broadcast
	// is not supported.
	const IPv4& bcast = a4.bcast();
	_rm->push(new IfMgrIPv4SetBroadcast(ifname, vifname, addr, bcast));
    }
}


void
LibFeaClientBridge::vifaddr6_update(const string& ifname,
				    const string& vifname,
				    const IPv6&	  addr,
				    const Update& update,
				    bool	  system)
{
    debug_msg("%s update for address %s/%s/%s (system = %s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str(), truth_of(system));

    if (system) {
	//
	// XXX: We don't propagate interface-related changes that just
	// appear beneath us within the system. I.e., all changes
	// should be through the FEA.
	//
	return;
    }

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIPv6Add(ifname, vifname, addr));
	break; 					// FALLTHROUGH

    case DELETED:
	_rm->push(new IfMgrIPv6Remove(ifname, vifname, addr));
	return;

    case CHANGED:
	break; 					// FALLTHROUGH
    }

    //
    // Validate vif address is in the FEA iftree we're using and in
    // libfeaclient's equivalent
    //
    const IfMgrIPv6Atom* ifa = _rm->iftree().find_addr(ifname,
						       vifname,
						       addr);
    if (ifa == 0) {
	XLOG_WARNING("Got update for address no in libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	XLOG_WARNING("Got update for address on interface not in tree: "
		     "%s/(%s/%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for address on vif not in FEA tree: "
		     "%s/%s/(%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif& vif = vi->second;
    IfTreeVif::IPv6Map::const_iterator ai = vif.get_addr(addr);
    if (ai == vif.ipv6addrs().end()) {
	XLOG_WARNING("Got update for address not in FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr6& a6 = ai->second;

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv6SetEnabled(ifname, vifname, addr, a6.enabled())
	      );
    _rm->push(new
	      IfMgrIPv6SetLoopback(ifname, vifname, addr, a6.loopback())
	      );
    _rm->push(new
	      IfMgrIPv6SetMulticastCapable(ifname, vifname, addr,
					   a6.multicast())
	      );
    _rm->push(new
	      IfMgrIPv6SetPrefix(ifname, vifname, addr, a6.prefix_len())
	      );
    _rm->push(new
	      IfMgrIPv6SetEndpoint(ifname, vifname, addr, a6.endpoint()));
}


void
LibFeaClientBridge::updates_completed(bool	  system)
{
    debug_msg("Updates completed (system = %s)\n", truth_of(system));

    if (system) {
	//
	// XXX: We don't propagate interface-related changes that just
	// appear beneath us within the system. I.e., all changes
	// should be through the FEA.
	//
	return;
    }

    _rm->push(new IfMgrHintUpdatesMade());
}
