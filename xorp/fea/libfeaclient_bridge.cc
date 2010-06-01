// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_atoms.hh"
#include "libfeaclient/ifmgr_cmds.hh"
#include "libfeaclient/ifmgr_xrl_replicator.hh"

#include "iftree.hh"
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


// ----------------------------------------------------------------------------
// LibFeaClientBridge implementation

LibFeaClientBridge::LibFeaClientBridge(XrlRouter& rtr,
				       IfConfigUpdateReplicator& update_replicator)
    : IfConfigUpdateReporterBase(update_replicator)
{
    _rm = new IfMgrXrlReplicationManager(rtr);
    add_to_replicator();
}

LibFeaClientBridge::~LibFeaClientBridge()
{
    delete _rm;
}

int
LibFeaClientBridge::add_libfeaclient_mirror(const string& m)
{
    if (_rm->add_mirror(m) != true)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
LibFeaClientBridge::remove_libfeaclient_mirror(const string& m)
{
    if (_rm->remove_mirror(m) != true)
	return (XORP_ERROR);

    return (XORP_OK);
}

void
LibFeaClientBridge::interface_update(const string& ifname,
				     const Update& update)
{
    debug_msg("%s update for interface %s\n",
	      update_name(update), ifname.c_str());

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
    const IfMgrIfAtom* ifa = _rm->iftree().find_interface(ifname);
    if (ifa == NULL) {
	XLOG_WARNING("Got update for interface not in the libfeaclient tree: %s",
		     ifname.c_str());
	return;
    }

    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for interface not in the FEA tree: %s",
		     ifname.c_str());
	return;
    }

    //
    // Copy interface state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new IfMgrIfSetEnabled(ifname, ifp->enabled()));
    _rm->push(new IfMgrIfSetDiscard(ifname, ifp->discard()));
    _rm->push(new IfMgrIfSetUnreachable(ifname, ifp->unreachable()));
    _rm->push(new IfMgrIfSetManagement(ifname, ifp->management()));
    _rm->push(new IfMgrIfSetMtu(ifname, ifp->mtu()));
    _rm->push(new IfMgrIfSetMac(ifname, ifp->mac()));
    _rm->push(new IfMgrIfSetPifIndex(ifname, ifp->pif_index()));
    _rm->push(new IfMgrIfSetNoCarrier(ifname, ifp->no_carrier()));
    _rm->push(new IfMgrIfSetBaudrate(ifname, ifp->baudrate()));
}


void
LibFeaClientBridge::vif_update(const string& ifname,
			       const string& vifname,
			       const Update& update)
{
    debug_msg("%s update for vif %s/%s\n",
	      update_name(update), ifname.c_str(), vifname.c_str());

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
    if (ifa == NULL) {
	XLOG_WARNING("Got update for vif not in the libfeaclient tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for vif on interface not in the FEA tree: "
		     "%s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for vif not in the FEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    //
    // Copy vif state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrVifSetEnabled(ifname, vifname, vifp->enabled())
	      );
    _rm->push(new
	      IfMgrVifSetBroadcastCapable(ifname, vifname, vifp->broadcast())
	      );
    _rm->push(new
	      IfMgrVifSetLoopbackCapable(ifname, vifname, vifp->loopback())
	      );
    _rm->push(new
	      IfMgrVifSetP2PCapable(ifname, vifname, vifp->point_to_point())
	      );
    _rm->push(new
	      IfMgrVifSetMulticastCapable(ifname, vifname, vifp->multicast())
	      );
    _rm->push(new
	      IfMgrVifSetPifIndex(ifname, vifname, vifp->pif_index())
	      );
    _rm->push(new
	      IfMgrVifSetVifIndex(ifname, vifname, vifp->vif_index())
	      );
    _rm->push(new
	      IfMgrVifSetPimRegister(ifname, vifname, vifp->pim_register())
	      );
    _rm->push(new
	      IfMgrVifSetIsVlan(ifname, vifname, vifp->is_vlan())
	      );
    _rm->push(new
	      IfMgrVifSetVlanId(ifname, vifname, vifp->vlan_id())
	      );
}


void
LibFeaClientBridge::vifaddr4_update(const string& ifname,
				    const string& vifname,
				    const IPv4&   addr,
				    const Update& update)
{
    debug_msg("%s update for address %s/%s/%s\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str());

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
    if (ifa == NULL) {
	XLOG_WARNING("Got update for address no in the libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for address on interface not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for address on vif not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr4* ap = vifp->find_addr(addr);
    if (ap == NULL) {
	XLOG_WARNING("Got update for address not in the FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv4SetEnabled(ifname, vifname, addr, ap->enabled())
	      );
    _rm->push(new
	      IfMgrIPv4SetLoopback(ifname, vifname, addr, ap->loopback())
	      );
    _rm->push(new
	      IfMgrIPv4SetMulticastCapable(ifname, vifname, addr,
					   ap->multicast())
	      );
    _rm->push(new
	      IfMgrIPv4SetPrefix(ifname, vifname, addr, ap->prefix_len())
	      );
    if (ap->point_to_point()) {
	const IPv4& end = ap->endpoint();
	_rm->push(new IfMgrIPv4SetEndpoint(ifname, vifname, addr, end));
    } else {
	// XXX: Method IfTreeAddr4::bcast() will return IPv4::ZERO() if
	// broadcast is not supported.  This happens to be the
	// correct argument for libfeaclient to signify broadcast
	// is not supported.
	const IPv4& bcast = ap->bcast();
	_rm->push(new IfMgrIPv4SetBroadcast(ifname, vifname, addr, bcast));
    }
}


#ifdef HAVE_IPV6
void
LibFeaClientBridge::vifaddr6_update(const string& ifname,
				    const string& vifname,
				    const IPv6&	  addr,
				    const Update& update)
{
    debug_msg("%s update for address %s/%s/%s\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str());

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
    if (ifa == NULL) {
	XLOG_WARNING("Got update for address no in the libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface* ifp = observed_iftree().find_interface(ifname);
    if (ifp == NULL) {
	XLOG_WARNING("Got update for address on interface not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif* vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	XLOG_WARNING("Got update for address on vif not in the FEA tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr6* ap = vifp->find_addr(addr);
    if (ap == NULL) {
	XLOG_WARNING("Got update for address not in the FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv6SetEnabled(ifname, vifname, addr, ap->enabled())
	      );
    _rm->push(new
	      IfMgrIPv6SetLoopback(ifname, vifname, addr, ap->loopback())
	      );
    _rm->push(new
	      IfMgrIPv6SetMulticastCapable(ifname, vifname, addr,
					   ap->multicast())
	      );
    _rm->push(new
	      IfMgrIPv6SetPrefix(ifname, vifname, addr, ap->prefix_len())
	      );
    _rm->push(new
	      IfMgrIPv6SetEndpoint(ifname, vifname, addr, ap->endpoint()));
}
#endif


void
LibFeaClientBridge::updates_completed()
{
    debug_msg("Updates completed\n");

    _rm->push(new IfMgrHintUpdatesMade());
}
