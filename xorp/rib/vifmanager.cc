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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "rib_manager.hh"
#include "vifmanager.hh"


VifManager::VifManager(XrlRouter& xrl_router,
		       EventLoop& eventloop,
		       RibManager* rib_manager,
		       const string& fea_target)
    : _xrl_router(xrl_router),
      _eventloop(eventloop),
      _rib_manager(rib_manager),
      _ifmgr(eventloop, fea_target.c_str(), xrl_router.finder_address(),
	     xrl_router.finder_port()),
      _startup_requests_n(0),
      _shutdown_requests_n(0)
{
    enable();	// XXX: by default the VifManager is always enabled

    //
    // Set myself as an observer when the node status changes
    //
    set_observer(this);

    _ifmgr.set_observer(this);
    _ifmgr.attach_hint_observer(this);
}

VifManager::~VifManager()
{
    //
    // Unset myself as an observer when the node status changes
    //
    unset_observer(this);

    stop();

    _ifmgr.detach_hint_observer(this);
    _ifmgr.unset_observer(this);
}

/**
 * Start operation.
 *
 * Start the process of registering with the FEA, etc.
 * After the startup operations are completed,
 * @ref VifManager::final_start() is called internally
 * to complete the job.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::start()
{
    if (is_up() || is_pending_up())
	return (XORP_OK);

    enable();	// XXX: by default the VifManager is always enabled

    if (ProtoState::pending_start() != XORP_OK)
	return (XORP_ERROR);

    //
    // Startup the interface manager
    //
    if (ifmgr_startup() != XORP_OK) {
	ServiceBase::set_status(SERVICE_FAILED);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Completely start the node operation.
 *
 * This method should be called internally after @ref VifManager::start()
 * to complete the job.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::final_start()
{
#if 0	// TODO: XXX: PAVPAVPAV
    if (! is_pending_up())
	return (XORP_ERROR);
#endif

    if (ProtoState::start() != XORP_OK) {
	ProtoState::stop();
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Stop operation.
 *
 * Gracefully stop operation.
 * After the shutdown operations are completed,
 * @ref VifManager::final_stop() is called internally
 * to complete the job.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::stop()
{
    if (is_down())
	return (XORP_OK);

    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);

    if (ProtoState::pending_stop() != XORP_OK)
	return (XORP_ERROR);

    //
    // Shutdown the interface manager
    //
    if (ifmgr_shutdown() != XORP_OK) {
	ServiceBase::set_status(SERVICE_FAILED);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Completely stop the node operation.
 *
 * This method should be called internally after @ref VifManager::stop()
 * to complete the job.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
VifManager::final_stop()
{
#if	0
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
#endif
    if (ProtoState::stop() != XORP_OK)
	return (XORP_ERROR);

    // Clear the old state
    _iftree.clear();
    _old_iftree.clear();

    return (XORP_OK);
}

void
VifManager::status_change(ServiceBase*  service,
			  ServiceStatus old_status,
			  ServiceStatus new_status)
{
    if (service == this) {
	// My own status has changed
	if ((old_status == SERVICE_STARTING)
	    && (new_status == SERVICE_RUNNING)) {
	    // The startup process has completed
	    if (final_start() != XORP_OK) {
		XLOG_ERROR("Cannot complete the startup process; "
			   "current state is %s",
			   ProtoState::state_str().c_str());
		return;
	    }
	    return;
	}

	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    // The shutdown process has completed
	    final_stop();
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    decr_shutdown_requests_n();
	}
    }
}

void
VifManager::update_status()
{
    //
    // Test if the startup process has completed
    //
    if (ServiceBase::status() == SERVICE_STARTING) {
	if (_startup_requests_n > 0)
	    return;

	// The startup process has completed
	ServiceBase::set_status(SERVICE_RUNNING);
	return;
    }

    //
    // Test if the shutdown process has completed
    //
    if (ServiceBase::status() == SERVICE_SHUTTING_DOWN) {
	if (_shutdown_requests_n > 0)
	    return;

	// The shutdown process has completed
	ServiceBase::set_status(SERVICE_SHUTDOWN);
	return;
    }

    //
    // Test if we have failed
    //
    if (ServiceBase::status() == SERVICE_FAILED) {
	return;
    }
}

int
VifManager::ifmgr_startup()
{
    int ret_value;

    // TODO: XXX: we should startup the ifmgr only if it hasn't started yet
    incr_startup_requests_n();

    ret_value = _ifmgr.startup();

    //
    // XXX: when the startup is completed, IfMgrHintObserver::tree_complete()
    // will be called.
    //

    return (ret_value);
}

int
VifManager::ifmgr_shutdown()
{
    int ret_value;

    incr_shutdown_requests_n();

    ret_value = _ifmgr.shutdown();

    //
    // XXX: when the shutdown is completed, VifManager::status_change()
    // will be called.
    //

    return (ret_value);
}

void
VifManager::tree_complete()
{
    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();

    decr_startup_requests_n();
}

void
VifManager::updates_made()
{
    string error_msg;
    IfMgrIfTree::IfMap::const_iterator ifmgr_iface_iter;
    IfMgrIfAtom::VifMap::const_iterator ifmgr_vif_iter;
    IfMgrVifAtom::IPv4Map::const_iterator a4_iter;
    IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

    //
    // Update the local copy of the interface tree
    //
    _old_iftree = _iftree;
    _iftree = ifmgr_iftree();

    //
    // Remove vifs that don't exist anymore
    //
    for (ifmgr_iface_iter = _old_iftree.interfaces().begin();
	 ifmgr_iface_iter != _old_iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();
	if (_iftree.find_vif(ifmgr_iface_name, ifmgr_iface_name) == NULL) {
	    if (_rib_manager->delete_vif(ifmgr_iface_name, error_msg)
		!= XORP_OK) {
		XLOG_ERROR("Cannot delete vif %s from the set of configured "
			   "vifs: %s",
			   ifmgr_iface_name.c_str(), error_msg.c_str());
	    }
	}
    }

    //
    // Add new vifs, update existing ones and remove old addresses
    //
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();

	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();

	    const IfMgrVifAtom* old_ifmgr_vif_ptr;
	    old_ifmgr_vif_ptr = _old_iftree.find_vif(ifmgr_iface_name,
						     ifmgr_vif_name);

	    // The vif to add (eventually)
	    Vif vif(ifmgr_vif_name);

	    //
	    // Set the pif_index
	    //
	    if (old_ifmgr_vif_ptr == NULL)
		vif.set_pif_index(ifmgr_vif.pif_index());

	    //
	    // Update or set the vif flags
	    // XXX: Other properties (discard, unreachable, management)
	    // need to be inherited from the parent interface, once it has
	    // been marshaled.
	    //
	    bool is_up = ifmgr_iface.enabled() && ifmgr_vif.enabled();
	    is_up &= (! ifmgr_iface.no_carrier());	// XXX: the link state

	    if (old_ifmgr_vif_ptr != NULL) {
		if (_rib_manager->set_vif_flags(ifmgr_vif_name,
						ifmgr_vif.p2p_capable(),
						ifmgr_vif.loopback(),
						ifmgr_vif.multicast_capable(),
						ifmgr_vif.broadcast_capable(),
						is_up,
						ifmgr_iface.mtu(),
						error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot update the flags for vif %s: %s",
			       ifmgr_vif_name.c_str(), error_msg.c_str());
		}
	    } else {
		vif.set_ifname(ifmgr_iface_name);
		vif.set_p2p(ifmgr_vif.p2p_capable());
		vif.set_loopback(ifmgr_vif.loopback());
		vif.set_multicast_capable(ifmgr_vif.multicast_capable());
		vif.set_broadcast_capable(ifmgr_vif.broadcast_capable());
		vif.set_underlying_vif_up(is_up);
		vif.set_mtu(ifmgr_iface.mtu());
	    }

	    //
	    // Delete vif addresses that don't exist anymore
	    //
	    if (old_ifmgr_vif_ptr != NULL) {
		for (a4_iter = old_ifmgr_vif_ptr->ipv4addrs().begin();
		     a4_iter != old_ifmgr_vif_ptr->ipv4addrs().end();
		     ++a4_iter) {
		    const IfMgrIPv4Atom& a4 = a4_iter->second;
		    const IPv4& addr = a4.addr();
		    if (_iftree.find_addr(ifmgr_iface_name,
					  ifmgr_vif_name,
					  addr)
			== NULL) {
			if (_rib_manager->delete_vif_address(ifmgr_vif_name,
							     addr,
							     error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot delete address %s "
				       "for vif %s: %s",
				       addr.str().c_str(),
				       ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
		    }
		}

#ifdef HAVE_IPV6
		for (a6_iter = old_ifmgr_vif_ptr->ipv6addrs().begin();
		     a6_iter != old_ifmgr_vif_ptr->ipv6addrs().end();
		     ++a6_iter) {
		    const IfMgrIPv6Atom& a6 = a6_iter->second;
		    const IPv6& addr = a6.addr();
		    if (_iftree.find_addr(ifmgr_iface_name,
					  ifmgr_vif_name,
					  addr)
			== NULL) {
			if (_rib_manager->delete_vif_address(ifmgr_vif_name,
							     addr,
							     error_msg)
			    != XORP_OK) {
			    XLOG_ERROR("Cannot delete address %s "
				       "for vif %s: %s",
				       addr.str().c_str(),
				       ifmgr_vif_name.c_str(),
				       error_msg.c_str());
			}
		    }
		}
#endif
	    }

	    //
	    // Add a new vif
	    //
	    if (old_ifmgr_vif_ptr == NULL) {
		if (_rib_manager->new_vif(ifmgr_vif_name, vif, error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot add vif %s to the set of configured "
			       "vifs: %s",
			       ifmgr_vif_name.c_str(), error_msg.c_str());
		}
	    }
	}
    }

    //
    // Add new vif addresses, and update existing ones
    //
    for (ifmgr_iface_iter = _iftree.interfaces().begin();
	 ifmgr_iface_iter != _iftree.interfaces().end();
	 ++ifmgr_iface_iter) {
	const IfMgrIfAtom& ifmgr_iface = ifmgr_iface_iter->second;
	const string& ifmgr_iface_name = ifmgr_iface.name();

	for (ifmgr_vif_iter = ifmgr_iface.vifs().begin();
	     ifmgr_vif_iter != ifmgr_iface.vifs().end();
	     ++ifmgr_vif_iter) {
	    const IfMgrVifAtom& ifmgr_vif = ifmgr_vif_iter->second;
	    const string& ifmgr_vif_name = ifmgr_vif.name();

	    const IfMgrVifAtom* old_ifmgr_vif_ptr;
	    old_ifmgr_vif_ptr = _old_iftree.find_vif(ifmgr_iface_name,
						     ifmgr_vif_name);

	    for (a4_iter = ifmgr_vif.ipv4addrs().begin();
		 a4_iter != ifmgr_vif.ipv4addrs().end();
		 ++a4_iter) {
		const IfMgrIPv4Atom& a4 = a4_iter->second;
		const IPv4& addr = a4.addr();

		const IfMgrIPv4Atom* old_a4_ptr = NULL;
		if (old_ifmgr_vif_ptr != NULL)
		    old_a4_ptr = old_ifmgr_vif_ptr->find_addr(addr);
		if ((old_a4_ptr != NULL) && (*old_a4_ptr == a4))
		    continue;		// Nothing changed

		IPv4Net subnet_addr(addr, a4.prefix_len());
		IPv4 broadcast_addr(IPv4::ZERO());
		IPv4 peer_addr(IPv4::ZERO());
		if (a4.has_broadcast())
		    broadcast_addr = a4.broadcast_addr();
		if (a4.has_endpoint())
		    peer_addr = a4.endpoint_addr();

		if (old_a4_ptr != NULL) {
		    // Delete the old address so it can be replaced
		    if (_rib_manager->delete_vif_address(ifmgr_vif_name,
							 addr,
							 error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s "
				   "for vif %s: %s",
				   addr.str().c_str(),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}

		if (_rib_manager->add_vif_address(ifmgr_vif_name,
						  addr,
						  subnet_addr,
						  broadcast_addr,
						  peer_addr,
						  error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot add address %s to vif %s from "
			       "the set of configured vifs: %s",
			       cstring(addr), ifmgr_vif_name.c_str(),
			       error_msg.c_str());
		}
	    }

#ifdef HAVE_IPV6
	    for (a6_iter = ifmgr_vif.ipv6addrs().begin();
		 a6_iter != ifmgr_vif.ipv6addrs().end();
		 ++a6_iter) {
		const IfMgrIPv6Atom& a6 = a6_iter->second;
		const IPv6& addr = a6.addr();

		const IfMgrIPv6Atom* old_a6_ptr = NULL;
		if (old_ifmgr_vif_ptr != NULL)
		    old_a6_ptr = old_ifmgr_vif_ptr->find_addr(addr);
		if ((old_a6_ptr != NULL) && (*old_a6_ptr == a6))
		    continue;		// Nothing changed

		IPv6Net subnet_addr(addr, a6.prefix_len());
		IPv6 peer_addr(IPv6::ZERO());
		if (a6.has_endpoint())
		    peer_addr = a6.endpoint_addr();

		if (old_a6_ptr != NULL) {
		    // Delete the old address so it can be replaced
		    if (_rib_manager->delete_vif_address(ifmgr_vif_name,
							 addr,
							 error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot delete address %s "
				   "for vif %s: %s",
				   addr.str().c_str(),
				   ifmgr_vif_name.c_str(),
				   error_msg.c_str());
		    }
		}

		if (_rib_manager->add_vif_address(ifmgr_vif_name,
						  addr,
						  subnet_addr,
						  peer_addr,
						  error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot add address %s to vif %s from "
			       "the set of configured vifs: %s",
			       cstring(addr), ifmgr_vif_name.c_str(),
			       error_msg.c_str());
		}
	    }
#endif
	}
    }
}

void
VifManager::incr_startup_requests_n()
{
    _startup_requests_n++;
    XLOG_ASSERT(_startup_requests_n > 0);
}

void
VifManager::decr_startup_requests_n()
{
    XLOG_ASSERT(_startup_requests_n > 0);
    _startup_requests_n--;

    update_status();
}

void
VifManager::incr_shutdown_requests_n()
{
    _shutdown_requests_n++;
    XLOG_ASSERT(_shutdown_requests_n > 0);
}

void
VifManager::decr_shutdown_requests_n()
{
    XLOG_ASSERT(_shutdown_requests_n > 0);
    _shutdown_requests_n--;

    update_status();
}
