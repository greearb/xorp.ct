// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/fea/xrl_target.cc,v 1.2 2002/12/14 23:42:51 hodson Exp $"

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"

#include "net/if.h"			/* Included for IFF_ definitions */

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl_target.hh"

XrlFeaTarget::XrlFeaTarget(EventLoop&		 	e,
			   XrlRouter&		 	r,
			   Fti&       		 	fti,
			   InterfaceManager&	 	ifmgr,
			   XrlIfConfigUpdateReporter&	xifcur,
			   XrlRawSocket4Manager*	xrsm)
    : XrlFeaTargetBase(&r), _xftm(e, fti), _xifmgr(e, ifmgr),
      _xifcur(xifcur), _xrsm(xrsm)
{
}

XrlCmdError
XrlFeaTarget::common_0_1_get_target_name(
					 // Output values,
					 string&	name)
{
    name = "fea";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_version(
				     // Output values,
				     string&   version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_all_interface_names(
						// Output values,
						XrlAtomList&	ifnames)
{
    IfTree local;
    const IfTree& it = _xifmgr.ifconfig().pull_config(local);

    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second.ifname()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_names(
						       // Output values,
						       XrlAtomList&	ifnames)
{
    const IfTree& it = _xifmgr.iftree();

    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second.ifname()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_all_vif_names(
					  const string& ifname,
					  // Output values,
					  XrlAtomList&	vifnames)
{
    const IfTreeInterface* fip = 0;
    XrlCmdError e = _xifmgr.pull_config_get_if(ifname, fip);

    if (e == XrlCmdError::OKAY()) {
	for (IfTreeInterface::VifMap::const_iterator vi = fip->vifs().begin();
	     vi != fip->vifs().end(); ++vi) {
	    vifnames.append(XrlAtom(vi->second.vifname()));
	}
    }

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_names(
						 const string& ifname,
						 // Output values,
						 XrlAtomList&  vifnames)
{
    const IfTreeInterface* fip = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fip);

    if (e == XrlCmdError::OKAY()) {
	for (IfTreeInterface::VifMap::const_iterator vi = fip->vifs().begin();
	     vi != fip->vifs().end(); ++vi) {
	    vifnames.append(XrlAtom(vi->second.vifname()));
	}
    }

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_interface_enabled(
					      // Input values,
					      const string&	ifname,
					      // Output values,
					      bool&		enabled)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	enabled = fi->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_mac(
				// Input values,
				const string& ifname,
				Mac&	    mac)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mac = fi->mac();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_mtu(
				// Input values,
				const string&	ifname,
				uint32_t&		mtu)
{
    const IfTreeInterface* fi = 0;
    XrlCmdError e = _xifmgr.get_if(ifname, fi);

    if (e == XrlCmdError::OKAY())
	mtu = fi->mtu();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_vif_enabled(
					// Input values,
					const string& ifname,
					const string& vifname,
					// Output values,
					bool&	    enabled)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vifname, fv);

    if (e == XrlCmdError::OKAY())
	enabled = fv->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_prefix4(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv4&	addr,
				    // Output values,
				    uint32_t&	prefix)
{
    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix = fa->prefix();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_broadcast4(
				       // Input values,
				       const string&	ifname,
				       const string&	vifname,
				       const IPv4&	addr,
				       // Output values,
				       IPv4&		broadcast)
{
    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    broadcast = fa->bcast();
    return _xifmgr.addr_valid(ifname, vifname, "broadcast", broadcast);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_endpoint4(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv4&	addr,
				      // Output values,
				      IPv4&		endpoint)
{
    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return _xifmgr.addr_valid(ifname, vifname, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_prefix6(
				    // Input values,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv6&	addr,
				    // Output values,
				    uint32_t&	prefix)
{
    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e == XrlCmdError::OKAY())
	prefix = fa->prefix();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_endpoint6(
				      // Input values,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv6&		addr,
				      // Output values,
				      IPv6&		endpoint)
{
    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, addr, fa);

    if (e != XrlCmdError::OKAY())
	return e;

    endpoint = fa->endpoint();
    return  _xifmgr.addr_valid(ifname, vifname, "endpoint", endpoint);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_all_vif_addresses6(
					       // Input values,
					       const string&	ifname,
					       const string&	vif,
					       // Output values,
					       XrlAtomList&	addrs)
{
    /* XXX This should do a pull up */
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V6Map::const_iterator ai = fv->v6addrs().begin();
	 ai != fv->v6addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses6(
						      // Input values,
						      const string&	ifname,
						      const string&	vif,
						      // Output values,
						      XrlAtomList&	addrs)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V6Map::const_iterator ai = fv->v6addrs().begin();
	 ai != fv->v6addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_vif_flags4(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv4&   address,
				       // Output values
				       bool& up,
				       bool& broadcast,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    const IfTreeAddr4* fa;
    XrlCmdError e = _xifmgr.get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    uint32_t flags = fa->flags();
    up = flags & IFF_UP;
    broadcast = flags & IFF_BROADCAST;
    loopback = flags & IFF_LOOPBACK;
    point_to_point = flags & IFF_POINTOPOINT;
    multicast = flags & IFF_MULTICAST;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_vif_flags6(
				       // Input values
				       const string& ifname,
				       const string& vif,
				       const IPv6&   address,
				       // Output values
				       bool& up,
				       bool& loopback,
				       bool& point_to_point,
				       bool& multicast)
{
    const IfTreeAddr6* fa;
    XrlCmdError e = _xifmgr.get_addr(ifname, vif, address, fa);
    if (e != XrlCmdError::OKAY())
	return e;

    uint32_t flags = fa->flags();
    up = flags & IFF_UP;
    loopback = flags & IFF_LOOPBACK;
    point_to_point = flags & IFF_POINTOPOINT;
    multicast = flags & IFF_MULTICAST;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_all_vif_addresses4(
					       // Input values,
					       const string&	ifname,
					       const string&	vif,
					       // Output values,
					       XrlAtomList&	addrs)
{
    /* XXX This should do a pull up */
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.pull_config_get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V4Map::const_iterator ai = fv->v4addrs().begin();
	 ai != fv->v4addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses4(
						      // Input values,
						      const string&	ifname,
						      const string&	vif,
						      // Output values,
						      XrlAtomList&	addrs)
{
    const IfTreeVif* fv = 0;
    XrlCmdError e = _xifmgr.get_vif(ifname, vif, fv);
    if (e != XrlCmdError::OKAY())
	return e;

    for (IfTreeVif::V4Map::const_iterator ai = fv->v4addrs().begin();
	 ai != fv->v4addrs().end(); ++ai) {
	addrs.append(XrlAtom(ai->second.addr()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_start_transaction(
					  // Output values,
					  uint32_t& tid)
{
    return _xifmgr.start_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_commit_transaction(
					   // Input values,
					   const uint32_t& tid)
{
    return _xifmgr.commit_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_abort_transaction(
					  // Input values,
					  const uint32_t& tid)
{
    return _xifmgr.abort_transaction(tid);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_interface(
					 // Input values,
					 const uint32_t&	tid,
					 const string&	ifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddInterface(it, ifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_interface(
					 // Input values,
					 const uint32_t&	tid,
					 const string&	ifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveInterface(it, ifname));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_enabled(
					      // Input values,
					      const uint32_t&	tid,
					      const string&	ifname,
					      const bool&	enabled)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceEnabled(it, ifname, enabled));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mac(
				// Input values,
				const uint32_t&	tid,
				const string&	ifname,
				const Mac&	mac)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceMAC(it, ifname, mac));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mtu(
				// Input values,
				const uint32_t&	tid,
				const string&	ifname,
				const uint32_t&	mtu)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetInterfaceMTU(it, ifname, mtu));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_vif(
				   // Input values,
				   const uint32_t&	tid,
				   const string&	ifname,
				   const string&	vifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddInterfaceVif(it, ifname, vifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_vif(
				   // Input values,
				   const uint32_t&	tid,
				   const string&	ifname,
				   const string&	vifname)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveInterfaceVif(it, ifname, vifname));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_vif_enabled(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const bool&	enabled)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetVifEnabled(it, ifname, vifname, enabled));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address4(
					// Input values,
					const uint32_t& tid,
					const string&   ifname,
					const string&   vifname,
					const IPv4&	address)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddAddr4(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address4(
					// Input values,
					const uint32_t& tid,
					const string&   ifname,
					const string&   vifname,
					const IPv4&     address)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveAddr4(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled4(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv4&	address,
					const bool&	en)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr4Enabled(it, ifname, vifname, address, en));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_address_enabled4(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv4&	address,
					     bool&		enabled)
{
    const IfTreeAddr4* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix4(
				    // Input values,
				    const uint32_t&	tid,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv4&		address,
				    const uint32_t&	prefix)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Prefix(it, ifname, vifname, address,
					       prefix));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_broadcast4(
				       // Input values,
				       const uint32_t&	tid,
				       const string&	ifname,
				       const string&	vifname,
				       const IPv4&	address,
				       const IPv4&	broadcast)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Broadcast(it, ifname, vifname, address,
						  broadcast));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint4(
				      // Input values,
				      const uint32_t&	tid,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv4&	address,
				      const IPv4&	endpoint)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new SetAddr4Endpoint(it, ifname, vifname, address,
						 endpoint));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address6(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv6&	address)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new AddAddr6(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address6(
					// Input values,
					const uint32_t&	tid,
					const string&	ifname,
					const string&	vifname,
					const IPv6&	address)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid, new RemoveAddr6(it, ifname, vifname, address));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled6(
					     // Input values,
					     const uint32_t&	tid,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv6&	address,
					     const bool&	en)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Enabled(it, ifname, vifname, address, en));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_address_enabled6(
					     // Input values,
					     const string&	ifname,
					     const string&	vifname,
					     const IPv6&	address,
					     bool&		enabled)
{
    const IfTreeAddr6* fa = 0;
    XrlCmdError e = _xifmgr.get_addr(ifname, vifname, address, fa);
    if (e == XrlCmdError::OKAY())
	enabled = fa->enabled();

    return e;
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix6(
				    // Input values,
				    const uint32_t&	tid,
				    const string&	ifname,
				    const string&	vifname,
				    const IPv6&	  	address,
				    const uint32_t&	prefix)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Prefix(it, ifname, vifname, address,
					  prefix));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint6(
				      // Input values,
				      const uint32_t&	tid,
				      const string&	ifname,
				      const string&	vifname,
				      const IPv6&	address,
				      const IPv6&	endpoint)
{
    IfTree& it = _xifmgr.iftree();
    return _xifmgr.add(tid,
		       new SetAddr6Endpoint(it, ifname, vifname, address,
					    endpoint));
}


XrlCmdError
XrlFeaTarget::ifmgr_0_1_register_client(const string& client)
{
    if (_xifcur.add_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" already registered."));
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_unregister_client(const string& client)
{
    if (_xifcur.remove_reportee(client))
	return XrlCmdError::OKAY();
    return XrlCmdError::COMMAND_FAILED(client +
				       string(" not registered."));
}

// ----------------------------------------------------------------------------
// FTI Related

XrlCmdError
XrlFeaTarget::fti_0_1_start_transaction(
	// Output values,
	uint32_t& tid)
{
    return _xftm.start_transaction(tid);
}

XrlCmdError
XrlFeaTarget::fti_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid)
{
    return _xftm.commit_transaction(tid);
}

XrlCmdError
XrlFeaTarget::fti_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid)
{
    return _xftm.abort_transaction(tid);
}

XrlCmdError
XrlFeaTarget::fti_0_1_add_entry4(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	gw,
	const string&	ifname,
	const string&	vifname)
{
    FtiTransactionManager::Operation op(
	new FtiAddEntry4(_xftm.fti(), dst, gw, ifname, vifname)
	);
    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_delete_entry4(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiDeleteEntry4(_xftm.fti(), dst)
	);
    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_delete_all_entries4(
	// Input values,
	const uint32_t&	tid)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.

    FtiTransactionManager::Operation op(
	new FtiDeleteAllEntries4(_xftm.fti())
	);
    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_lookup_route4(
	// Input values,
	const IPv4&	dst,
	// Output values,
	IPv4Net&	netmask,
	IPv4&		gateway,
	string&		ifname,
	string&		vifname)
{
    Fte4 fte;
    if (_xftm.fti().lookup_route4(dst, fte)) {
	netmask = fte.net();
	gateway = fte.gateway();
	ifname = fte.ifname();
	vifname = fte.vifname();
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_1_lookup_entry4(
	// Input values,
	const IPv4Net&	dst,
	// Output values,
	IPv4&		gateway,
	string&		ifname,
	string&		vifname)
{
    Fte4 fte;
    if (_xftm.fti().lookup_entry4(dst, fte)) {
	gateway = fte.gateway();
	ifname = fte.ifname();
	vifname = fte.vifname();
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_1_add_entry6(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	gw,
	const string&	ifname,
	const string&	vifname)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiAddEntry6(_xftm.fti(), dst, gw, ifname, vifname)
	);

    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_delete_entry6(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.
    FtiTransactionManager::Operation op(
	new FtiDeleteEntry6(_xftm.fti(), dst)
	);

    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_delete_all_entries6(
	// Input values,
	const uint32_t&	tid)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.

    FtiTransactionManager::Operation op(
	new FtiDeleteAllEntries6(_xftm.fti())
	);

    return _xftm.add(tid, op);
}

XrlCmdError
XrlFeaTarget::fti_0_1_lookup_route6(
	// Input values,
	const IPv6&	dst,
	// Output values,
	IPv6Net&	netmask,
	IPv6&		gateway,
	string&		ifname,
	string&		vifname)
{
    Fte6 fte;
    if (_xftm.fti().lookup_route6(dst, fte)) {
	netmask = fte.net();
	gateway = fte.gateway();
	ifname = fte.ifname();
	vifname = fte.vifname();
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_1_lookup_entry6(
	// Input values,
	const IPv6Net&	dst,
	// Output values,
	IPv6&		gateway,
	string&		ifname,
	string&		vifname)
{
    Fte6 fte;
    if (_xftm.fti().lookup_entry6(dst, fte)) {
	gateway = fte.gateway();
	ifname = fte.ifname();
	vifname = fte.vifname();
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_1_delete_all_entries(
	// Input values,
	const uint32_t&	tid)
{
    // FtiTransactionManager::Operation is a ref_ptr object, allocated
    // memory here is handed it to to manage.

    FtiTransactionManager::Operation op(
	new FtiDeleteAllEntries(_xftm.fti())
	);
    return _xftm.add(tid, op);
}

// ----------------------------------------------------------------------------
// Raw Socket related

static const char* XRL_RAW_SOCKET_NULL = "XrlRawSocket4Manager not present" ;

XrlCmdError
XrlFeaTarget::raw_packet_0_1_send4(
				   // Input values,
				   const IPv4&		  src_address,
				   const IPv4&		  dst_address,
				   const string&	  vifname,
				   const uint32_t&	  proto,
				   const uint32_t&	  ttl,
				   const uint32_t&	  tos,
				   const vector<uint8_t>& options,
				   const vector<uint8_t>& payload
				   )
{
    if (_xrsm == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET_NULL);
    }
    return _xrsm->send(src_address, dst_address, vifname,
		       proto, ttl, tos, options, payload);
}

XrlCmdError
XrlFeaTarget::raw_packet_0_1_send_raw4(
				       // Input values,
				       const string&		vifname,
				       const vector<uint8_t>&	packet
				       )
{
    if (_xrsm == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET_NULL);
    }
    return _xrsm->send(vifname, packet);
}

XrlCmdError
XrlFeaTarget::raw_packet_0_1_register_vif_receiver(
						   // Input values,
						   const string&   router_name,
						   const string&   ifname,
						   const string&   vifname,
						   const uint32_t& proto
						   )
{
    if (_xrsm == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET_NULL);
    }
    return _xrsm->register_vif_receiver(router_name, ifname, vifname, proto);
}

XrlCmdError
XrlFeaTarget::raw_packet_0_1_unregister_vif_receiver(
						     // Input values,
						     const string& router_name,
						     const string& ifname,
						     const string& vifname,
						     const uint32_t& proto
						     )
{
    if (_xrsm == 0) {
	return XrlCmdError::COMMAND_FAILED(XRL_RAW_SOCKET_NULL);
    }
    return _xrsm->unregister_vif_receiver(router_name, ifname, vifname, proto);
}
