// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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
#include "libxorp/utils.hh"

#include "libxipc/xrl_error.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "rib_manager.hh"
#include "redist_xrl.hh"
#include "redist_policy.hh"
#include "profile_vars.hh"

RibManager::RibManager(EventLoop& eventloop, XrlStdRouter& xrl_std_router,
		       const string& fea_target)
    : _status_code(PROC_NOT_READY),
      _status_reason("Initializing"),
      _eventloop(eventloop),
      _xrl_router(xrl_std_router),
      _register_server(&_xrl_router),
      _urib4(UNICAST, *this, _eventloop),
      _mrib4(MULTICAST, *this, _eventloop),
#ifdef HAVE_IPV6
      _urib6(UNICAST, *this, _eventloop),
      _mrib6(MULTICAST, *this, _eventloop),
#endif
      _vif_manager(_xrl_router, _eventloop, this, fea_target),
      _xrl_rib_target(&_xrl_router, _urib4, _mrib4,
#ifdef HAVE_IPV6
		      _urib6, _mrib6,
#endif
		      _vif_manager, this),
      _fea_target(fea_target)
{
    _urib4.initialize(_register_server);
    _mrib4.initialize(_register_server);
#ifdef HAVE_IPV6
    _urib6.initialize(_register_server);
    _mrib6.initialize(_register_server);
#endif
    PeriodicTimerCallback cb = callback(this, &RibManager::status_updater);
    _status_update_timer = _eventloop.new_periodic_ms(1000, cb);
#ifndef XORP_DISABLE_PROFILE
    initialize_profiling_variables(_profile);
#endif
}

RibManager::~RibManager()
{
    stop();
}

int
RibManager::start()
{
    if (ProtoState::start() != XORP_OK)
	return (XORP_ERROR);

    _vif_manager.start();

    return (XORP_OK);
}

int
RibManager::stop()
{
    if (! is_up())
	return (XORP_ERROR);

    _vif_manager.stop();

    ProtoState::stop();

    _status_code = PROC_SHUTDOWN;
    _status_reason = "Shutting down";
    status_updater();
    return (XORP_OK);
}

bool
RibManager::status_updater()
{
    ProcessStatus s = PROC_READY;
    string reason = "Ready";
    bool ret = true;

    //
    // Check the VifManager's status
    //
    ServiceStatus vif_mgr_status = _vif_manager.status();
    switch (vif_mgr_status) {
    case SERVICE_READY:
	break;
    case SERVICE_STARTING:
	s = PROC_NOT_READY;
	reason = "VifManager starting";
	break;
    case SERVICE_RUNNING:
	break;
    case SERVICE_PAUSING:
	s = PROC_NOT_READY;
	reason = "VifManager pausing";
	break;
    case SERVICE_PAUSED:
	s = PROC_NOT_READY;
	reason = "VifManager paused";
	break;
    case SERVICE_RESUMING:
	s = PROC_NOT_READY;
	reason = "VifManager resuming";
	break;
    case SERVICE_SHUTTING_DOWN:
	s = PROC_SHUTDOWN;
	reason = "VifManager shutting down";
	break;
    case SERVICE_SHUTDOWN:
	s = PROC_DONE;
	reason = "VifManager Shutdown";
       break;
    case SERVICE_FAILED:
	// VifManager failed: set process state to failed.
	// TODO: XXX: Should we exit here, or wait to be restarted?
	s = PROC_FAILED;
	reason = "VifManager Failed";
	ret = false;
	break;
    case SERVICE_ALL:
	XLOG_UNREACHABLE();
	break;
    }

    //
    // PROC_SHUTDOWN and PROC_DOWN states are persistent unless
    // there's a failure.
    //
    _status_code = s;
    _status_reason = reason;

    return ret;
}


template <typename A>
int
RibManager::add_rib_vif(RIB<A>& rib, const string& vifname, const Vif& vif, string& err)
{
    int result = rib.new_vif(vifname, vif);
    if (result != XORP_OK) {
	if (err.size() == 0) {
	    err = c_format("Failed to add VIF \"%s\" to %s",
			   vifname.c_str(), rib.name().c_str());
	} else {
	    err = c_format(", and failed to add VIF \"%s\" to %s",
			   vifname.c_str(), rib.name().c_str());
	}
    }
    return result;
}

int
RibManager::new_vif(const string& vifname, const Vif& vif, string& err)
{
    err.resize(0);
    return (add_rib_vif(_urib4, vifname, vif, err)
	    | add_rib_vif(_mrib4, vifname, vif, err)
#ifdef HAVE_IPV6
	    | add_rib_vif(_urib6, vifname, vif, err)
	    | add_rib_vif(_mrib6, vifname, vif, err)
#endif
	);
}

template <typename A>
int
RibManager::delete_rib_vif(RIB<A>& rib, const string& vifname, string& err)
{
    int result = rib.delete_vif(vifname);
    if (result != XORP_OK) {
	if (err.empty()) {
	    err = c_format("Failed to delete VIF \"%s\" from %s",
			   vifname.c_str(), rib.name().c_str());
	} else {
	    err += c_format(", and failed to delete VIF \"%s\" from %s",
			    vifname.c_str(), rib.name().c_str());
	}
    }
    return result;
}

int
RibManager::delete_vif(const string& vifname, string& err)
{
    err.resize(0);
    return (delete_rib_vif(_urib4, vifname, err)
	    | delete_rib_vif(_mrib4, vifname, err)
#ifdef HAVE_IPV6
	    | delete_rib_vif(_urib6, vifname, err)
	    | delete_rib_vif(_mrib6, vifname, err)
#endif
	);
}


template <typename A>
int
RibManager::set_rib_vif_flags(RIB<A>& rib, const string& vifname, bool is_p2p,
		  bool is_loopback, bool is_multicast, bool is_broadcast,
		  bool is_up, uint32_t mtu, string& err)
{
    int result = rib.set_vif_flags(vifname, is_p2p, is_loopback, is_multicast,
				   is_broadcast, is_up, mtu);
    if (result != XORP_OK) {
	err = c_format("Failed to add flags for VIF \"%s\" to %s",
			vifname.c_str(), rib.name().c_str());
    }
    return result;
}

int
RibManager::set_vif_flags(const string& vifname,
			  bool is_p2p,
			  bool is_loopback,
			  bool is_multicast,
			  bool is_broadcast,
			  bool is_up,
			  uint32_t mtu,
			  string& err)
{
    if (set_rib_vif_flags(_urib4, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, mtu, err) != XORP_OK ||
	set_rib_vif_flags(_mrib4, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, mtu, err) != XORP_OK
#ifdef HAVE_IPV6
	|| set_rib_vif_flags(_urib6, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, mtu, err) != XORP_OK ||
	set_rib_vif_flags(_mrib6, vifname, is_up, is_loopback, is_multicast,
			  is_broadcast, is_up, mtu, err) != XORP_OK
#endif
	) {
	return XORP_ERROR;
    }
    return XORP_OK;
}


template <typename A>
int
RibManager::add_vif_address_to_ribs(RIB<A>& 	urib,
			RIB<A>& 	mrib,
			const string&	vifn,
			const A& 	addr,
			const IPNet<A>& subnet,
			const A&	broadcast_addr,
			const A&	peer_addr,
			string& 	err)
{
    RIB<A>* ribs[2] = { &urib, &mrib };
    for (uint32_t i = 0; i < sizeof(ribs)/sizeof(ribs[0]); i++) {
	if (ribs[i]->add_vif_address(vifn, addr, subnet, broadcast_addr,
				     peer_addr) != XORP_OK) {
	    err = c_format("Failed to add VIF address %s to %s\n",
			   addr.str().c_str(), ribs[i]->name().c_str());
	    return XORP_ERROR;
	}
    }
    return XORP_OK;
}

template <typename A>
int
RibManager::delete_vif_address_from_ribs(RIB<A>& 		urib,
			     RIB<A>& 		mrib,
			     const string&	vifn,
			     const A& 		addr,
			     string& 		err)
{
    RIB<A>* ribs[2] = { &urib, &mrib };
    for (uint32_t i = 0; i < sizeof(ribs)/sizeof(ribs[0]); i++) {
	if (ribs[i]->delete_vif_address(vifn, addr) != XORP_OK) {
	    err = c_format("Failed to delete VIF address %s from %s\n",
			   addr.str().c_str(), ribs[i]->name().c_str());
	    return XORP_ERROR;
	}
    }
    return XORP_OK;
}

int
RibManager::add_vif_address(const string& 	vifn,
			    const IPv4& 	addr,
			    const IPv4Net& 	subnet,
			    const IPv4&		broadcast_addr,
			    const IPv4&		peer_addr,
			    string& 		err)
{
    return add_vif_address_to_ribs(_urib4, _mrib4, vifn, addr, subnet,
				   broadcast_addr, peer_addr, err);
}

int
RibManager::delete_vif_address(const string& 	vifn,
			       const IPv4& 	addr,
			       string& 		err)
{
    return delete_vif_address_from_ribs(_urib4, _mrib4, vifn, addr, err);
}


void
RibManager::make_errors_fatal()
{
    _urib4.set_errors_are_fatal();
    _mrib4.set_errors_are_fatal();
#ifdef HAVE_IPV6
    _urib6.set_errors_are_fatal();
    _mrib6.set_errors_are_fatal();
#endif
}

void
RibManager::register_interest_in_target(const string& target_class)
{
    if (_targets_of_interest.find(target_class)
	== _targets_of_interest.end()) {
	_targets_of_interest.insert(target_class);
	XrlFinderEventNotifierV0p1Client finder(&_xrl_router);
	XrlFinderEventNotifierV0p1Client::RegisterClassEventInterestCB cb =
	    callback(this, &RibManager::register_interest_in_target_done);
	finder.send_register_class_event_interest("finder",
						  _xrl_router.instance_name(),
						  target_class, cb);
    }
}

void
RibManager::register_interest_in_target_done(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to register interest in an XRL target\n");
    }
}

void
RibManager::deregister_interest_in_target(const string& target_class)
{
    if (_targets_of_interest.find(target_class)
	!= _targets_of_interest.end()) {
	_targets_of_interest.erase(target_class);
	XrlFinderEventNotifierV0p1Client finder(&_xrl_router);
	XrlFinderEventNotifierV0p1Client::RegisterClassEventInterestCB cb =
	    callback(this, &RibManager::deregister_interest_in_target_done);
	finder.send_deregister_class_event_interest("finder",
						    _xrl_router.instance_name(),
						    target_class, cb);
    }
}

void
RibManager::deregister_interest_in_target_done(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to deregister interest in an XRL target\n");
    }
}

void
RibManager::target_death(const string& target_class,
			 const string& target_instance)
{
    if (target_class == "fea") {
	// No cleanup - we just exit.
	XLOG_ERROR("FEA died, so RIB is exiting too\n");
	exit(0);
    }

    // Deregister interest in the target
    deregister_interest_in_target(target_class);

    // Inform the RIBs in case this was a routing protocol that died.
    _urib4.target_death(target_class, target_instance);
    _mrib4.target_death(target_class, target_instance);
#ifdef HAVE_IPV6
    _urib6.target_death(target_class, target_instance);
    _mrib6.target_death(target_class, target_instance);
#endif
}

string
RibManager::make_redist_name(const string& xrl_target, const string& cookie,
		 bool is_xrl_transaction_output)
{
    string redist_name = xrl_target + ":" + cookie;

    if (is_xrl_transaction_output)
	redist_name += " (transaction)";
    else
	redist_name += " (no transaction)";

    return redist_name;
}

template <typename A>
int
RibManager::redist_enable_xrl_output(EventLoop&	eventloop,
			 XrlRouter&	rtr,
			 Profile&	profile,
			 RIB<A>&	rib,
			 const string&	to_xrl_target,
			 const string&	proto,
			 const IPNet<A>& network_prefix,
			 const string&	cookie,
			 bool		is_xrl_transaction_output)
{
    string protocol(proto);

    RedistPolicy<A>* redist_policy = 0;
    if (protocol.find("all-") == 0) {
	// XXX Voodoo magic, user requests a name like all-<protocol>
	// then we attach xrl output to the redist output table (known
	// internally by the name "all") and set the redist_policy filter
	// to correspond with the protocol
	protocol = "all";

	string sub = proto.substr(4, string::npos);
	if (sub != "all") {
	    Protocol* p = rib.find_protocol(sub);
	    if (p != 0) {
		redist_policy = new IsOfProtocol<A>(*p);
	    } else {
		return XORP_ERROR;
	    }
	}
    }

    RedistTable<A>* rt = rib.protocol_redist_table(protocol);
    if (rt == 0) {
	delete redist_policy;
	return XORP_ERROR;
    }

    string redist_name = make_redist_name(to_xrl_target, cookie,
					  is_xrl_transaction_output);
    if (rt->redistributor(redist_name) != 0) {
	delete redist_policy;
	return XORP_ERROR;
    }

    Redistributor<A>* redist = new Redistributor<A>(eventloop,
						    redist_name);
    redist->set_redist_table(rt);
    if (is_xrl_transaction_output) {
	redist->set_output(
		    new RedistTransactionXrlOutput<A>(redist, rtr,
						      profile,
						      protocol,
						      to_xrl_target,
						      network_prefix,
						      cookie)
		    );
    } else {
	redist->set_output(new RedistXrlOutput<A>(redist, rtr,
						  profile,
						  protocol,
						  to_xrl_target,
						  network_prefix, cookie));
    }

    redist->set_policy(redist_policy);

    return XORP_OK;
}

template <typename A>
int
RibManager::redist_disable_xrl_output(RIB<A>& rib,
			  const string& to_xrl_target,
			  const string& proto,
			  const string& cookie,
			  bool is_xrl_transaction_output)
{
    string protocol(proto);
    if (protocol.find("ribout-") == 0) {
	// XXX Voodoo magic, user requests a name like ribout-<protocol>
	// then we attach xrl output to the redist output table (known
	// internally by the name "all"
	protocol = "all";
    }

    RedistTable<A>* rt = rib.protocol_redist_table(protocol);
    if (rt == 0)
	return XORP_ERROR;

    string redist_name = make_redist_name(to_xrl_target, cookie,
					  is_xrl_transaction_output);
    Redistributor<A>* redist = rt->redistributor(redist_name);
    if (redist == 0)
	return XORP_ERROR;

    rt->remove_redistributor(redist);
    delete redist;
    return XORP_OK;
}

int
RibManager::add_redist_xrl_output4(const string&	to_xrl_target,
				   const string&	from_protocol,
				   bool			unicast,
				   bool			multicast,
				   const IPv4Net&	network_prefix,
				   const string&	cookie,
				   bool			is_xrl_transaction_output)
{
    if (unicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _profile,
					 _urib4,
					 to_xrl_target, from_protocol,
					 network_prefix, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK) {
	    return e;
	}
    }
    if (multicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _profile,
					 _mrib4,
					 to_xrl_target, from_protocol,
					 network_prefix, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK && unicast) {
	    redist_disable_xrl_output(_urib4,
				      to_xrl_target, from_protocol, cookie,
				      is_xrl_transaction_output);
	}
	return e;
    }
    return XORP_OK;
}

int
RibManager::delete_redist_xrl_output4(const string&	to_xrl_target,
				      const string&	from_protocol,
				      bool	   	unicast,
				      bool		multicast,
				      const string&	cookie,
				      bool		is_xrl_transaction_output)
{
    if (unicast)
	redist_disable_xrl_output(_urib4, to_xrl_target, from_protocol, cookie,
				  is_xrl_transaction_output);
    if (multicast)
	redist_disable_xrl_output(_mrib4, to_xrl_target, from_protocol, cookie,
				  is_xrl_transaction_output);
    return XORP_OK;
}

void
RibManager::push_routes()
{
    _urib4.push_routes();
    _mrib4.push_routes();
#ifdef HAVE_IPV6
    _urib6.push_routes();
    _mrib6.push_routes();
#endif
}

void
RibManager::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter, conf);
}

void
RibManager::reset_filter(const uint32_t& filter)
{
    _policy_filters.reset(filter);
}

void
RibManager::remove_policy_redist_tags(const string& protocol)
{
    _policy_redist_map.remove(protocol);
}

void
RibManager::insert_policy_redist_tags(const string& protocol,
				      const PolicyTags& tags)
{
    _policy_redist_map.insert(protocol, tags);
}

void
RibManager::reset_policy_redist_tags()
{
    _policy_redist_map.reset();
}


#ifdef HAVE_IPV6
/** IPv6 stuff */


int
RibManager::add_vif_address(const string&	vifn,
			    const IPv6&		addr,
			    const IPv6Net&	subnet,
			    const IPv6&		peer_addr,
			    string&		err)
{
    int r = add_vif_address_to_ribs(_urib6, _mrib6, vifn, addr, subnet,
				    IPv6::ZERO(), peer_addr, err);
    return r;
}

int
RibManager::delete_vif_address(const string& 	vifn,
			       const IPv6& 	addr,
			       string& 		err)
{
    return delete_vif_address_from_ribs(_urib6, _mrib6, vifn, addr, err);
}

int
RibManager::add_redist_xrl_output6(const string&	to_xrl_target,
				   const string&	from_protocol,
				   bool			unicast,
				   bool			multicast,
				   const IPv6Net&	network_prefix,
				   const string&	cookie,
				   bool			is_xrl_transaction_output)
{
    if (unicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _profile,
					 _urib6,
					 to_xrl_target, from_protocol,
					 network_prefix, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK) {
	    return e;
	}
    }
    if (multicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _profile,
					 _mrib6,
					 to_xrl_target, from_protocol,
					 network_prefix, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK && unicast) {
	    redist_disable_xrl_output(_urib6,
				      to_xrl_target, from_protocol, cookie,
				      is_xrl_transaction_output);
	}
	return e;
    }
    return XORP_OK;
}

int
RibManager::delete_redist_xrl_output6(const string&	to_xrl_target,
				      const string&	from_protocol,
				      bool	   	unicast,
				      bool		multicast,
				      const string&	cookie,
				      bool		is_xrl_transaction_output)
{
    if (unicast)
	redist_disable_xrl_output(_urib6, to_xrl_target, from_protocol, cookie,
				  is_xrl_transaction_output);
    if (multicast)
	redist_disable_xrl_output(_mrib6, to_xrl_target, from_protocol, cookie,
				  is_xrl_transaction_output);
    return XORP_OK;
}

#endif //ipv6
