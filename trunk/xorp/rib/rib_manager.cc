// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rib/rib_manager.cc,v 1.36 2004/06/10 13:49:27 hodson Exp $"

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

template <typename A>
void
initialize_rib(RIB<A>& 			rib,
	       list<RibClient*>& 	rib_clients_list,
	       RegisterServer& 		register_server)
{
    if (rib.initialize_export(&rib_clients_list) != XORP_OK) {
	XLOG_FATAL("Could not initialize export table for %s",
		   rib.name().c_str());
    }
    if (rib.initialize_register(&register_server) != XORP_OK) {
	XLOG_FATAL("Could not initialize register table for %s",
		   rib.name().c_str());
    }
    if (rib.initialize_redist_all("all") != XORP_OK) {
	XLOG_FATAL("Could not initialize all-protocol redistribution "
		   "table for %s",
		   rib.name().c_str());
    }
    if (rib.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_FATAL("Could not add igp table \"connected\" for %s",
		   rib.name().c_str());
    }
    if (rib.add_igp_table("static", "", "") != XORP_OK) {
	XLOG_FATAL("Could not add igp table \"connected\" for %s",
		   rib.name().c_str());
    }
}

RibManager::RibManager(EventLoop& eventloop, XrlStdRouter& xrl_std_router,
		       const string& fea_target)
    : _status_code(PROC_NOT_READY),
      _status_reason("Initializing"),
      _eventloop(eventloop),
      _xrl_router(xrl_std_router),
      _register_server(&_xrl_router),
      _urib4(UNICAST, *this, _eventloop),
      _mrib4(MULTICAST, *this, _eventloop),
      _urib6(UNICAST, *this, _eventloop),
      _mrib6(MULTICAST, *this, _eventloop),
      _vif_manager(_xrl_router, _eventloop, this, fea_target),
      _xrl_rib_target(&_xrl_router, _urib4, _mrib4, _urib6, _mrib6,
		      _vif_manager, this),
      _fea_target(fea_target)
{
    initialize_rib(_urib4, _urib4_clients_list, _register_server);
    initialize_rib(_mrib4, _mrib4_clients_list, _register_server);
    initialize_rib(_urib6, _urib6_clients_list, _register_server);
    initialize_rib(_mrib6, _mrib6_clients_list, _register_server);
    PeriodicTimerCallback cb = callback(this, &RibManager::status_updater);
    _status_update_timer = _eventloop.new_periodic(1000, cb);
}

RibManager::~RibManager()
{
    stop();

    delete_pointers_list(_urib4_clients_list);
    delete_pointers_list(_mrib4_clients_list);
    delete_pointers_list(_urib6_clients_list);
    delete_pointers_list(_mrib6_clients_list);
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
    string reason;

    //
    // Check the VifManager's status
    //
    ServiceStatus vif_mgr_status = _vif_manager.status();
    switch (vif_mgr_status) {
    case STARTING:
	s = PROC_NOT_READY;
	reason = "VifManager starting";
	break;
    case RUNNING:
	break;
    case FAILED:
	// VifManager failed: set process state to failed.
	// TODO: XXX: Should we exit here, or wait to be restarted?
	_status_code = PROC_FAILED;
	_status_reason = "VifManager Failed";
	return false;
    default:
	// TODO: XXX: PAVPAVPAV: more cases may need to be added
	break;
    }

    //
    // Check the unicast RIBs can still talk to the FEA
    //
    RibClient* fea_client;
    fea_client = find_rib_client(_fea_target, AF_INET, true, false);
    if (fea_client != NULL) {
	if (fea_client->failed()) {
	    _status_code = PROC_FAILED;
	    _status_reason = "Fatal error talking to FEA (IPv4, unicast)";
	    return false;
	}
    }
    fea_client = find_rib_client(_fea_target, AF_INET6, true, false);
    if (fea_client != NULL) {
	if (fea_client->failed()) {
	    _status_code = PROC_FAILED;
	    _status_reason = "Fatal error talking to FEA (IPv6, unicast)";
	    return false;
	}
    }

    //
    // SHUTDOWN state is persistent unless there's a failure.
    //
    if (_status_code != PROC_SHUTDOWN) {
	_status_code = s;
	_status_reason = "Ready";
    }
    return true;
}


template <typename A>
static int
add_rib_vif(RIB<A>& rib, const string& vifname, const Vif& vif, string& err)
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
	    | add_rib_vif(_urib6, vifname, vif, err)
	    | add_rib_vif(_mrib6, vifname, vif, err));
}

template <typename A>
static int
delete_rib_vif(RIB<A>& rib, const string& vifname, string& err)
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
	    | delete_rib_vif(_urib6, vifname, err)
	    | delete_rib_vif(_mrib6, vifname, err));
}


template <typename A>
static int
set_rib_vif_flags(RIB<A>& rib, const string& vifname, bool is_p2p,
		  bool is_loopback, bool is_multicast, bool is_broadcast,
		  bool is_up, string& err)
{
    int result = rib.set_vif_flags(vifname, is_p2p, is_loopback, is_multicast,
				   is_broadcast, is_up);
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
			  string& err)
{
    if (set_rib_vif_flags(_urib4, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, err) != XORP_OK ||
	set_rib_vif_flags(_mrib4, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, err) != XORP_OK ||
	set_rib_vif_flags(_urib6, vifname, is_p2p, is_loopback, is_multicast,
			  is_broadcast, is_up, err) != XORP_OK ||
	set_rib_vif_flags(_mrib6, vifname, is_up, is_loopback, is_multicast,
			  is_broadcast, is_up, err) != XORP_OK) {
	return XORP_ERROR;
    }
    return XORP_OK;
}


template <typename A>
int
add_vif_address_to_ribs(RIB<A>& 	urib,
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
delete_vif_address_from_ribs(RIB<A>& 		urib,
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


//
// Select the appropriate list of RIB clients
//
list<RibClient* >*
RibManager::select_rib_clients_list(int family, bool unicast, bool multicast)
{
    if (! (unicast ^ multicast))
	return (NULL);	// Only one of the flags must be set

    //
    // Select the appropriate list
    //
    switch (family) {
    case AF_INET:
	if (unicast)
	    return (&_urib4_clients_list);
	if (multicast)
	    return (&_mrib4_clients_list);
	break;
    case AF_INET6:
	if (unicast)
	    return (&_urib6_clients_list);
	if (multicast)
	    return (&_mrib6_clients_list);
	break;
    default:
	XLOG_FATAL("Invalid address family: %d", family);
	break;
    }

    //
    // Nothing found
    //
    return (NULL);
}

//
// Find a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
RibClient*
RibManager::find_rib_client(const string& target_name, int family,
			    bool unicast, bool multicast)
{
    list<RibClient* >* rib_clients_list;

    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (NULL);

    //
    // Search for a RIB client with the same target name
    //
    list<RibClient* >::iterator iter;
    for (iter = rib_clients_list->begin();
	 iter != rib_clients_list->end();
	 ++iter) {
	RibClient* rib_client = *iter;
	if (rib_client->target_name() == target_name)
	    return (rib_client);
    }

    //
    // Nothing found
    //
    return (NULL);
}

//
// Add a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::add_rib_client(const string& target_name, int family,
			   bool unicast, bool multicast)
{
    list<RibClient* >* rib_clients_list;

    //
    // Check if this RIB client has been added already
    //
    if (find_rib_client(target_name, family, unicast, multicast) != NULL)
	return (XORP_OK);

    //
    // Find the list of RIB clients to add the new client to.
    //
    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (XORP_ERROR);

    //
    // Create a new RibClient, and add it to the list
    //
    RibClient* rib_client = new RibClient(_xrl_router, target_name);
    rib_clients_list->push_back(rib_client);

    return (XORP_OK);
}

//
// Delete a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::delete_rib_client(const string& target_name, int family,
			      bool unicast, bool multicast)
{
    RibClient* rib_client;
    list<RibClient* >* rib_clients_list;

    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);

    //
    // Find the list of RIB clients
    //
    rib_clients_list = select_rib_clients_list(family, unicast, multicast);
    if (rib_clients_list == NULL)
	return (XORP_ERROR);

    list<RibClient* >::iterator iter;
    iter = find(rib_clients_list->begin(),
		rib_clients_list->end(),
		rib_client);
    XLOG_ASSERT(iter != rib_clients_list->end());
    rib_clients_list->erase(iter);
    delete rib_client;

    return (XORP_OK);
}

//
// Enable a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::enable_rib_client(const string& target_name, int family,
			      bool unicast, bool multicast)
{
    RibClient* rib_client;

    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);

    rib_client->set_enabled(true);

    return (XORP_OK);
}

//
// Disable a RIB client for a given target name, address family, and
// unicast/multicast flags.
//
// Return: XORP_OK on success, otherwise XORP_ERROR.
//
int
RibManager::disable_rib_client(const string& target_name, int family,
			       bool unicast, bool multicast)
{
    RibClient* rib_client;

    //
    // Find the RIB client
    //
    rib_client = find_rib_client(target_name, family, unicast, multicast);
    if (rib_client == NULL)
	return (XORP_ERROR);

    rib_client->set_enabled(false);

    return (XORP_OK);
}

void
RibManager::make_errors_fatal()
{
    _urib4.set_errors_are_fatal();
    _urib6.set_errors_are_fatal();
    _mrib4.set_errors_are_fatal();
    _mrib6.set_errors_are_fatal();
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
RibManager::target_death(const string& target_class,
			 const string& target_instance)
{
    if (target_class == "fea") {
	// No cleanup - we just exit.
	XLOG_ERROR("FEA died, so RIB is exiting too\n");
	exit(0);
    }

    // Inform the RIBs in case this was a routing protocol that died.
    _urib4.target_death(target_class, target_instance);
    _urib6.target_death(target_class, target_instance);
    _mrib4.target_death(target_class, target_instance);
    _mrib6.target_death(target_class, target_instance);

    //
    // Remove any RibClients that died.
    //
    list<RibClient* >::iterator iter;
    for (iter = _urib4_clients_list.begin();
	 iter != _urib4_clients_list.end(); ++iter) {
	if (((*iter)->target_name() == target_instance)
	    || ((*iter)->target_name() == target_class)) {
	    delete (*iter);
	    _urib4_clients_list.erase(iter);
	    break;
	}
    }

    for (iter = _mrib4_clients_list.begin();
	 iter != _mrib4_clients_list.end();
	 ++iter) {
	if (((*iter)->target_name() == target_instance)
	    || ((*iter)->target_name() == target_class)) {
	    delete (*iter);
	    _mrib4_clients_list.erase(iter);
	    break;
	}
    }

    for (iter = _urib6_clients_list.begin();
	 iter != _urib6_clients_list.end();
	 ++iter) {
	if (((*iter)->target_name() == target_instance)
	    || ((*iter)->target_name() == target_class)) {
	    delete (*iter);
	    _urib6_clients_list.erase(iter);
	    break;
	}
    }

    for (iter = _mrib6_clients_list.begin();
	 iter != _mrib6_clients_list.end();
	 ++iter) {
	if (((*iter)->target_name() == target_instance)
	    || ((*iter)->target_name() == target_class)) {
	    delete (*iter);
	    _mrib6_clients_list.erase(iter);
	    break;
	}
    }
}

static inline
string
make_redist_name(const string& xrl_target, const string& cookie,
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
static int
redist_enable_xrl_output(EventLoop&	eventloop,
			 XrlRouter&	rtr,
			 RIB<A>&	rib,
			 const string&	to_xrl_target,
			 const string&	proto,
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
		cerr << "Could not find protocol " << sub << endl;
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
						      protocol, to_xrl_target,
						      cookie)
		    );
    } else {
	redist->set_output(new RedistXrlOutput<A>(redist, rtr, protocol,
						  to_xrl_target, cookie));
    }

    redist->set_policy(redist_policy);

    return XORP_OK;
}

template <typename A>
static int
redist_disable_xrl_output(RIB<A>& rib,
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
				   const string&	cookie,
				   bool			is_xrl_transaction_output)
{
    if (unicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _urib4,
					 to_xrl_target, from_protocol, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK) {
	    return e;
	}
    }
    if (multicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _mrib4,
					 to_xrl_target, from_protocol, cookie,
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
RibManager::add_redist_xrl_output6(const string&	to_xrl_target,
				   const string&	from_protocol,
				   bool			unicast,
				   bool			multicast,
				   const string&	cookie,
				   bool			is_xrl_transaction_output)
{
    if (unicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _urib6,
					 to_xrl_target, from_protocol, cookie,
					 is_xrl_transaction_output);
	if (e != XORP_OK) {
	    return e;
	}
    }
    if (multicast) {
	int e = redist_enable_xrl_output(_eventloop, _xrl_router, _mrib6,
					 to_xrl_target, from_protocol, cookie,
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
