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

#ident "$XORP: xorp/rtrmgr/tools/show_interfaces.cc,v 1.10 2003/10/24 20:48:52 pavlin Exp $"

#include "rtrmgr/rtrmgr_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "show_interfaces.hh"

InterfaceMonitor::InterfaceMonitor(XrlRouter& xrl_rtr,
				   EventLoop& eventloop)
    : _xrl_rtr(xrl_rtr), _eventloop(eventloop), _ifmgr_client(&xrl_rtr)
{
    _state = INITIALIZING;
    _register_retry_counter = 0;
    _interfaces_remaining = 0;
    _vifs_remaining = 0;
    _addrs_remaining = 0;
    _flags_remaining = 0;
}

InterfaceMonitor::~InterfaceMonitor()
{
    stop();

    map <string, Vif*>::iterator i;
    for (i = _vifs_by_name.begin(); i != _vifs_by_name.end(); i++) {
	delete i->second;
    }
    for (i = _vifs_by_name.begin(); i != _vifs_by_name.end(); i++) {
	_vifs_by_name.erase(i);
    }
}

void
InterfaceMonitor::start()
{
    register_interface_monitor();
}

void
InterfaceMonitor::stop()
{
    unregister_interface_monitor();
}

void
InterfaceMonitor::unregister_interface_monitor()
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &InterfaceMonitor::unregister_interface_monitor_done);
    if (_ifmgr_client.send_unregister_client("fea", _xrl_rtr.name(), cb)
	== false) {
	XLOG_ERROR("Failed to unregister interface monitor fea client");
	_state = FAILED;
    }
}

void
InterfaceMonitor::unregister_interface_monitor_done(const XrlError& e)
{
    UNUSED(e);

    //
    // We really don't care here if the request succeeded or failed,
    // because most likely we are exiting anyway.
    //
    _state = DONE;
}

void
InterfaceMonitor::register_interface_monitor()
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(this, &InterfaceMonitor::register_interface_monitor_done);
    if (_ifmgr_client.send_register_client("fea", _xrl_rtr.name(), cb)
	== false) {
	XLOG_ERROR("Failed to register interface monitor fea client");
	_state = FAILED;
    }
}

void
InterfaceMonitor::register_interface_monitor_done(const XrlError& e)
{
    if (e == XrlError::OKAY()) {
	// The registration was successful.  Now we need to query the
	// entries that are already there.  First, find out the set of
	// configured interfaces.
	XorpCallback2<void, const XrlError&, const XrlAtomList*>::RefPtr cb;
	cb = callback(this, &InterfaceMonitor::interface_names_done);
	if (_ifmgr_client.send_get_configured_interface_names("fea", cb)
	    == false) {
	    XLOG_ERROR("Failed to request interface names");
	    _state = FAILED;
	}
	return;
    }

    // if the resolve failed, it could be that we got going too quickly
    // for the FEA.  Retry every two seconds.  If after ten seconds we
    // still can't register, give up.  It's a higher level issue as to
    // whether failing to register is a fatal error.
    if (e == XrlError::RESOLVE_FAILED() && (_register_retry_counter < 5)) {
	XLOG_WARNING("Register Interface Monitor: RESOLVE_FAILED");
	_register_retry_counter++;
	OneoffTimerCallback cb;
	cb = callback(this, &InterfaceMonitor::register_interface_monitor);
	_register_retry_timer = _eventloop.new_oneoff_after_ms(2000, cb);
	return;
    }
    _state = FAILED;
    XLOG_ERROR("Register Interface Monitor: Permanent Error");
}

void
InterfaceMonitor::interface_names_done(const XrlError&	  e,
				       const XrlAtomList* alist)
{
    debug_msg("interface_names_done\n");
    if (e == XrlError::OKAY()) {
	debug_msg("OK\n");
	for (u_int i = 0; i < alist->size(); i++) {
	    // Spin through the list of interfaces, and fire off
	    // requests in parallel for all the Vifs on each interface
	    XrlAtom atom = alist->get(i);
	    string ifname = atom.text();
	    XorpCallback2<void, const XrlError&,
		const XrlAtomList*>::RefPtr cb;
	    debug_msg("got interface name: %s\n", ifname.c_str());
	    cb = callback(this, &InterfaceMonitor::vif_names_done, ifname);
	    if (_ifmgr_client.send_get_configured_vif_names("fea", ifname, cb)
		== false) {
		XLOG_ERROR("Failed to request vif names");
		_state = FAILED;
	    }
	    _interfaces_remaining++;
	}
	return;
    }
    _state = FAILED;
    XLOG_ERROR("Get Interface Names: Permanent Error");
}

void
InterfaceMonitor::vif_names_done(const XrlError&    e,
				 const XrlAtomList* alist,
				 string		    ifname)
{
    debug_msg("vif_names_done\n");
    UNUSED(ifname);
    if (e == XrlError::OKAY()) {
	for (u_int i = 0; i < alist->size(); i++) {
	    // Spin through all the Vifs on this interface, and fire
	    // off requests in parallel for all the addresses on each
	    // Vif.
	    XrlAtom atom = alist->get(i);
	    string vifname = atom.text();
	    vif_created(ifname, vifname);
	    XorpCallback2<void, const XrlError&,
		const XrlAtomList*>::RefPtr cb;
	    cb = callback(this, &InterfaceMonitor::get_vifaddr4_done,
			  ifname, vifname);
	    if (_ifmgr_client.send_get_configured_vif_addresses4("fea",
								 ifname,
								 vifname,
								 cb)
		== false) {
		XLOG_ERROR("Failed to request IPv4 addresses");
		_state = FAILED;
	    }
	    _vifs_remaining++;
	}
	_interfaces_remaining--;
    } else if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the interface went away.
	_interfaces_remaining--;
    } else {
	XLOG_ERROR("Get VIF Names: Permanent Error");
	_state = FAILED;
	return;
    }
    if (_interfaces_remaining == 0 && _vifs_remaining == 0 &&
	_addrs_remaining == 0 && _flags_remaining == 0) {
	_state = READY;
    }
}

void
InterfaceMonitor::get_vifaddr4_done(const XrlError&	e,
				    const XrlAtomList*	alist,
				    string	 	ifname,
				    string		vifname)

{
    if (e == XrlError::OKAY()) {
	for (u_int i = 0; i < alist->size(); i++) {
	    XrlAtom atom = alist->get(i);
	    IPv4 addr = atom.ipv4();
	    vifaddr4_created(ifname, vifname, addr);
	    XorpCallback6<void, const XrlError&, const bool*,
		const bool*, const bool*, const bool*, const bool*>::RefPtr cb;
	    cb = callback(this, &InterfaceMonitor::get_flags4_done,
			  ifname, vifname, addr);
	    if (_ifmgr_client.send_get_configured_address_flags4("fea",
								 ifname,
								 vifname,
								 addr,
								 cb)
		== false) {
		XLOG_ERROR("Failed to request interface flags");
		_state = FAILED;
		return;
	    }
	    _flags_remaining++;
	}
	_vifs_remaining--;
    } else if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away?
	_vifs_remaining--;
    } else {
	XLOG_ERROR("Get VIF Names: Permanent Error");
	_state = FAILED;
	return;
    }

    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0
	&& _flags_remaining == 0) {
	_state = READY;
    }
}

void
InterfaceMonitor::get_flags4_done(const XrlError& e,
				  const bool*	  enabled,
				  const bool*	  broadcast,
				  const bool*	  loopback,
				  const bool*	  point_to_point,
				  const bool*	  multicast,
				  string	  ifname,
				  string	  vifname,
				  IPv4		  addr)
{
    UNUSED(addr);
    UNUSED(ifname);
    if (e == XrlError::OKAY()) {
	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	// XXX this should be per-addr, not per VIF!
	vif->set_multicast_capable(*multicast);
	vif->set_p2p(*point_to_point);
	vif->set_broadcast_capable(*broadcast);
	vif->set_underlying_vif_up(*enabled);
	vif->set_loopback(*loopback);
	_flags_remaining--;
    } else if (e == XrlError::COMMAND_FAILED()) {
	// perhaps the vif went away?
	_flags_remaining--;
    } else {
	_state = FAILED;
	XLOG_ERROR("Get VIF Flags: Permanent Error");
	return;
    }

    if (_interfaces_remaining == 0
	&& _vifs_remaining == 0
	&& _addrs_remaining == 0
	&& _flags_remaining == 0) {
	_state = READY;
    }
}

void
InterfaceMonitor::interface_update(const string& ifname,
				   const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	// doesn't directly affect vifs - we'll get a vif_update for
	// any new vifs
	break;
    case IF_EVENT_DELETED:
	interface_deleted(ifname);
	break;
    case IF_EVENT_CHANGED:
	// doesn't directly affect vifs
	break;
    }
}

void
InterfaceMonitor::vif_update(const string& ifname,
			     const string& vifname, const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vif_created(ifname, vifname);
	break;
    case IF_EVENT_DELETED:
	vif_deleted(ifname, vifname);
	break;
    case IF_EVENT_CHANGED:
	// doesn't directly affect us
	break;
    }
}

void
InterfaceMonitor::vifaddr4_update(const string& ifname,
				  const string& vifname,
				  const IPv4& addr,
				  const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vifaddr4_created(ifname, vifname, addr);
	break;
    case IF_EVENT_DELETED:
	vifaddr4_deleted(ifname, vifname, addr);
	break;
    case IF_EVENT_CHANGED:
	// shouldn't happen
	abort();
	break;
    }
}

void
InterfaceMonitor::vifaddr6_update(const string& ifname,
				  const string& vifname,
				  const IPv6& addr,
				  const uint32_t& event)
{
    switch (event) {
    case IF_EVENT_CREATED:
	vifaddr6_created(ifname, vifname, addr);
	break;
    case IF_EVENT_DELETED:
	vifaddr6_deleted(ifname, vifname, addr);
	break;
    case IF_EVENT_CHANGED:
	// shouldn't happen
	abort();
	break;
    }
}

void
InterfaceMonitor::interface_deleted(const string& ifname)
{
    multimap<string, Vif*>::iterator i;
    i = _vifs_by_interface.find(ifname);
    if (i != _vifs_by_interface.end()) {
	vif_deleted(ifname, i->second->name());
    }
}

void
InterfaceMonitor::vif_deleted(const string& ifname, const string& vifname)
{
    Vif *vif;
    debug_msg("vif_deleted %s %s\n", ifname.c_str(), vifname.c_str());
    map<string, Vif*>::iterator vi = _vifs_by_name.find(vifname);
    if (vi != _vifs_by_name.end()) {
	vif = vi->second;
	_vifs_by_name.erase(vi);
	multimap<string, Vif*>::iterator ii = _vifs_by_interface.find(ifname);
	assert(ii != _vifs_by_interface.end());
	while (ii != _vifs_by_interface.end() &&
	       ii->first == ifname) {
	    if (ii->second == vif) {
		_vifs_by_interface.erase(ii);
		break;
	    }
	    ii++;
	    assert(ii != _vifs_by_interface.end());
	}

	// now process the deletion...
	// XXX
    } else {
	XLOG_ERROR("vif_deleted: vif %s not found.", vifname.c_str());
    }
}

void
InterfaceMonitor::vif_created(const string& ifname, const string& vifname)
{
    debug_msg("vif_created: %s\n", vifname.c_str());
    if (_vifs_by_name.find(vifname) != _vifs_by_name.end()) {
	XLOG_ERROR("vif_created: vif %s already exists.", vifname.c_str());
	return;
    }
    Vif *vif = new Vif(vifname, ifname);
    _vifs_by_name[vifname] = vif;
    _vifs_by_interface.insert(pair<string, Vif*>(ifname, vif));

    // now process the addition
    // XXX
}

void
InterfaceMonitor::vifaddr4_created(const string& ifn,
				   const string& vifn,
				   const IPv4&   addr)
{
    if (_vifs_by_name.find(vifn) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_created on unknown vif: %s", vifn.c_str());
	return;
    }
    XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
    cb = callback(this, &InterfaceMonitor::vifaddr4_done, ifn, vifn, addr);
    if (_ifmgr_client.send_get_configured_prefix4("fea", ifn, vifn, addr, cb)
	== false) {
	XLOG_ERROR("Failed to send address prefix request");
	_state = FAILED;
    }
    _addrs_remaining++;
}

void
InterfaceMonitor::vifaddr4_done(const XrlError& e, const uint32_t* prefix_len,
				string ifname, string vifname,
				IPv4 addr)
{
    UNUSED(ifname);

    if (e == XrlError::OKAY()) {

	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s prefix_len %d to vif %s\n",
		  addr.str().c_str(), *prefix_len, vifname.c_str());
	vif->add_address(addr, IPvXNet(addr, *prefix_len),
			 IPvX("0.0.0.0"), IPvX("0.0.0.0"));
	_addrs_remaining--;
    } else if (e != XrlError::COMMAND_FAILED()) {
	_addrs_remaining--;
    } else {
	XLOG_ERROR("Failed to get prefix_len for address %s.",
		   addr.str().c_str());
	_addrs_remaining--;
    }

    if (_interfaces_remaining == 0 && _vifs_remaining == 0 &&
	_addrs_remaining == 0 && _flags_remaining == 0 &&
	_state == INITIALIZING) {
	_state = READY;
    }
}

void
InterfaceMonitor::vifaddr4_deleted(const string& ifname,
				   const string& vifname,
				   const IPv4&	 addr)
{
    UNUSED(ifname);
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr4_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
    Vif* vif = _vifs_by_name[vifname];
    vif->delete_address(addr);
}

void
InterfaceMonitor::vifaddr6_created(const string& ifn,
				   const string& vifn,
				   const IPv6&   addr)
{
    if (_vifs_by_name.find(vifn) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_created on unknown vif: %s",  vifn.c_str());
	return;
    }
    XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb;
    cb = callback(this, &InterfaceMonitor::vifaddr6_done, ifn, vifn, addr);
    if (_ifmgr_client.send_get_configured_prefix6("fea", ifn, vifn, addr, cb)
	== false) {
	XLOG_ERROR("Failed to send address prefix request");
	_state = FAILED;
    }
    _addrs_remaining++;
}

void
InterfaceMonitor::vifaddr6_done(const XrlError& e, const uint32_t* prefix_len,
				string ifname, string vifname,
				IPv6 addr)
{
    UNUSED(ifname);

    if (e == XrlError::OKAY()) {

	if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	    // silently ignore - the vif could have been deleted while we
	    // were waiting for the answer.
	    return;
	}
	Vif* vif = _vifs_by_name[vifname];
	debug_msg("adding address %s prefix_len %d to vif %s\n",
		  addr.str().c_str(), *prefix_len, vifname.c_str());
	vif->add_address(addr, IPvXNet(addr, *prefix_len),
			 IPvX("0.0.0.0"), IPvX("0.0.0.0"));
	_addrs_remaining--;
    } else if (e != XrlError::COMMAND_FAILED()) {
	_addrs_remaining--;
    } else {
	XLOG_ERROR("Failed to get prefix_len for address %s.",
		   addr.str().c_str());
	_addrs_remaining--;
    }

    if (_interfaces_remaining == 0 && _vifs_remaining == 0 &&
	_addrs_remaining == 0 && _flags_remaining == 0
	&& _state == INITIALIZING) {
	_state = READY;
    }
}

void
InterfaceMonitor::vifaddr6_deleted(const string& ifname,
				   const string& vifname,
				   const IPv6&   addr)
{
    UNUSED(ifname);
    if (_vifs_by_name.find(vifname) == _vifs_by_name.end()) {
	XLOG_ERROR("vifaddr6_deleted on unknown vif: %s", vifname.c_str());
	return;
    }
    Vif* vif = _vifs_by_name[vifname];
    vif->delete_address(addr);

}

void
InterfaceMonitor::print_results() const
{
    map<string, Vif*>::const_iterator i;
    for (i = _vifs_by_name.begin(); i != _vifs_by_name.end(); i++) {
	Vif *vif = i->second;
	printf("%s/%s: Flags:<", vif->ifname().c_str(), vif->name().c_str());
	bool prev = false;
	if (vif->is_underlying_vif_up()) {
	    if (prev) printf(",");
	    printf("ENABLED");
	    prev = true;
	}
	if (vif->is_broadcast_capable()) {
	    if (prev) printf(",");
	    printf("BROADCAST");
	    prev = true;
	}
	if (vif->is_multicast_capable()) {
	    if (prev) printf(",");
	    printf("MULTICAST");
	    prev = true;
	}
	if (vif->is_loopback()) {
	    if (prev) printf(",");
	    printf("LOOPBACK");
	    prev = true;
	}
	if (vif->is_p2p()) {
	    if (prev) printf(",");
	    printf("POINTTOPOINT");
	    prev = true;
	}
	printf(">\n");
	list<VifAddr>::const_iterator ai;
	for (ai = vif->addr_list().begin();
	     ai != vif->addr_list().end();
	     ai++) {
	    IPvX addr = ai->addr();
	    if (addr.is_ipv4()) {
		printf("    inet %s ", addr.str().c_str());
		printf("netmask %s ",
		       IPv4::make_prefix(ai->subnet_addr().
					 prefix_len()).str().c_str());
		printf("broadcast %s\n",
		       ai->broadcast_addr().str().c_str());
	    }
	}
	for (ai = vif->addr_list().begin();
	     ai != vif->addr_list().end();
	     ai++) {
	    IPvX addr = ai->addr();
	    if (addr.is_ipv6()) {
		printf("inet6 %s ", addr.str().c_str());
		printf("netmask %s ",
		       IPv6::make_prefix(ai->subnet_addr().
					 prefix_len()).str().c_str());
	    }
	}
    }
}

int
main(int argc, char* const argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    UNUSED(argc);
    UNUSED(argv);
    EventLoop eventloop;
    string process;
    process = c_format("interface_monitor%d", getpid());
    try {
	XrlRouter xrl_rtr(eventloop, process.c_str());

	InterfaceMonitor ifmon(xrl_rtr, eventloop);

	bool timed_out = false;
	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	xrl_rtr.finalize();
	while (xrl_rtr.ready() == false) {
	    eventloop.run();
	}

	ifmon.start();
	while (ifmon.state() == InterfaceMonitor::INITIALIZING) {
	    eventloop.run();
	}
	if (ifmon.state() == InterfaceMonitor::READY)
	    ifmon.print_results();
	
	ifmon.stop();
	while (ifmon.state() != InterfaceMonitor::DONE) {
	    eventloop.run();
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
