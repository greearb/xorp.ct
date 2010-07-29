// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// #define DEBUG_LOGGING

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "auth.hh"
#include "system.hh"
#include "xrl_process_spy.hh"
#include "xrl_port_manager.hh"
#include "xrl_redist_manager.hh"
#include "xrl_target_rip.hh"
#include "xrl_target_common.hh"


static int
decode_time_string(EventLoop& eventloop, const string& time_string,
		   TimeVal& timeval)
{
    const char* s;
    const char* format = "%Y-%m-%d.%H:%M";
    struct tm tm;
    time_t result;

    if (time_string.empty()) {
	timeval = TimeVal::ZERO();
	return (XORP_OK);
    }

    //
    // Initialize the parsed result with the current time, because
    // strptime(3) would not set/modify the unspecified members of the
    // time format (e.g, the timezone and the summer time flag).
    //
    TimeVal now;
    eventloop.current_time(now);
    time_t local_time = now.sec();
    const struct tm* local_tm = localtime(&local_time);
    memcpy(&tm, local_tm, sizeof(tm));

    s = xorp_strptime(time_string.c_str(), format, &tm);
    if ((s == NULL) || (*s != '\0')) {
	return (XORP_ERROR);
    }

    result = mktime(&tm);
    if (result == -1)
	return (XORP_ERROR);

    timeval = TimeVal(result, 0);
    return (XORP_OK);
}

XrlRipTarget::XrlRipTarget(EventLoop&			el,
			   XrlRouter&			xr,
			   XrlProcessSpy&		xps,
			   XrlPortManager<IPv4>& 	xpm,
			   XrlRedistManager<IPv4>&	xrm,
			   System<IPv4>&		rip_system)
    : XrlRipTargetBase(&xr), _e(el)
{
    _ct = new XrlRipCommonTarget<IPv4>(xps, xpm, xrm, rip_system);
}

XrlRipTarget::~XrlRipTarget()
{
    delete _ct;
}

XrlCmdError
XrlRipTarget::common_0_1_get_target_name(string& n)
{
    n = get_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRipTarget::common_0_1_get_version(string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

void
XrlRipTarget::set_status(ProcessStatus status, const string& note)
{
    _ct->set_status(status, note);
}

XrlCmdError
XrlRipTarget::common_0_1_get_status(uint32_t& status,
				    string&   reason)
{
    return _ct->common_0_1_get_status(status, reason);
}

XrlCmdError
XrlRipTarget::common_0_1_shutdown()
{
    return _ct->common_0_1_shutdown();
}

XrlCmdError
XrlRipTarget::common_0_1_startup()
{
    return _ct->common_0_1_startup();
}

XrlCmdError
XrlRipTarget::finder_event_observer_0_1_xrl_target_birth(const string& cname,
							 const string& iname)
{
    return _ct->finder_event_observer_0_1_xrl_target_birth(cname, iname);
}

XrlCmdError
XrlRipTarget::finder_event_observer_0_1_xrl_target_death(const string& cname,
							 const string& iname)
{
    return _ct->finder_event_observer_0_1_xrl_target_death(cname, iname);
}

XrlCmdError
XrlRipTarget::rip_0_1_add_rip_address(const string& ifn,
				      const string& vifn,
				      const IPv4&   addr)
{
    return _ct->ripx_0_1_add_rip_address(ifn, vifn, addr);
}

XrlCmdError
XrlRipTarget::rip_0_1_remove_rip_address(const string& ifn,
					 const string& vifn,
					 const IPv4&   addr)
{
    return _ct->ripx_0_1_remove_rip_address(ifn, vifn, addr);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_rip_address_enabled(const string& ifn,
					      const string& vifn,
					      const IPv4&   a,
					      const bool&   en)
{
    return _ct->ripx_0_1_set_rip_address_enabled(ifn, vifn, a, en);
}

XrlCmdError
XrlRipTarget::rip_0_1_rip_address_enabled(const string& ifn,
					  const string& vifn,
					  const IPv4&   a,
					  bool&		en)
{
    return _ct->ripx_0_1_rip_address_enabled(ifn, vifn, a, en);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_cost(const string&	ifn,
			       const string&	vifn,
			       const IPv4&	a,
			       const uint32_t&	cost)
{
    return _ct->ripx_0_1_set_cost(ifn, vifn, a, cost);
}

XrlCmdError
XrlRipTarget::rip_0_1_cost(const string&	ifn,
			   const string&	vifn,
			   const IPv4&		a,
			   uint32_t&		cost)
{
    return _ct->ripx_0_1_cost(ifn, vifn, a, cost);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_horizon(const string&	ifn,
				  const string&	vifn,
				  const IPv4&	a,
				  const string&	horizon)
{
    return _ct->ripx_0_1_set_horizon(ifn, vifn, a, horizon);
}

XrlCmdError
XrlRipTarget::rip_0_1_horizon(const string&	ifn,
			      const string&	vifn,
			      const IPv4&	a,
			      string&		horizon)
{
    return _ct->ripx_0_1_horizon(ifn, vifn, a, horizon);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_passive(const string&	ifn,
				  const string&	vifn,
				  const IPv4&	a,
				  const bool&	passive)
{
    return _ct->ripx_0_1_set_passive(ifn, vifn, a, passive);
}

XrlCmdError
XrlRipTarget::rip_0_1_passive(const string&	ifn,
			      const string&	vifn,
			      const IPv4&	a,
			      bool&		passive)
{
    return _ct->ripx_0_1_passive(ifn, vifn, a, passive);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_accept_non_rip_requests(const string&	ifn,
						  const string&	vifn,
						  const IPv4&	addr,
						  const bool&	accept)
{
    return _ct->ripx_0_1_set_accept_non_rip_requests(ifn, vifn, addr, accept);
}

XrlCmdError
XrlRipTarget::rip_0_1_accept_non_rip_requests(const string&	ifn,
					      const string&	vifn,
					      const IPv4&	addr,
					      bool&		accept)
{
    return _ct->ripx_0_1_accept_non_rip_requests(ifn, vifn, addr, accept);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_accept_default_route(const string&	ifn,
						  const string&	vifn,
						  const IPv4&	addr,
						  const bool&	accept)
{
    return _ct->ripx_0_1_set_accept_default_route(ifn, vifn, addr, accept);
}

XrlCmdError
XrlRipTarget::rip_0_1_accept_default_route(const string&	ifn,
					   const string&	vifn,
					   const IPv4&		addr,
					   bool&		accept)
{
    return _ct->ripx_0_1_accept_default_route(ifn, vifn, addr, accept);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_advertise_default_route(const string&	ifn,
						  const string&	vifn,
						  const IPv4&	addr,
						  const bool&	adv)
{
    return _ct->ripx_0_1_set_advertise_default_route(ifn, vifn, addr, adv);
}

XrlCmdError
XrlRipTarget::rip_0_1_advertise_default_route(const string&	ifn,
					      const string&	vifn,
					      const IPv4&	addr,
					      bool&		adv)
{
    return _ct->ripx_0_1_advertise_default_route(ifn, vifn, addr, adv);
}


XrlCmdError
XrlRipTarget::rip_0_1_set_route_timeout(const string&	ifn,
					const string&	vifn,
					const IPv4&	a,
					const uint32_t&	t)
{
    return _ct->ripx_0_1_set_route_timeout(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_route_timeout(const string&	ifn,
				    const string&	vifn,
				    const IPv4&		a,
				    uint32_t&		t)
{
    return _ct->ripx_0_1_route_timeout(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_deletion_delay(const string&		ifn,
					 const string&		vifn,
					 const IPv4&		a,
					 const uint32_t&	t)
{
    return _ct->ripx_0_1_set_deletion_delay(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_deletion_delay(const string&	ifn,
				     const string&	vifn,
				     const IPv4&	a,
				     uint32_t&		t)
{
    return _ct->ripx_0_1_deletion_delay(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_request_interval(const string&	ifn,
					   const string&	vifn,
					   const IPv4&		a,
					   const uint32_t&	t)
{
    return _ct->ripx_0_1_set_request_interval(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_request_interval(const string&	ifn,
				       const string&	vifn,
				       const IPv4&	a,
				       uint32_t&	t)
{
    return _ct->ripx_0_1_request_interval(ifn, vifn, a, t);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_update_interval(const string&	ifn,
					  const string&	vifn,
					  const IPv4&	a,
					  const uint32_t& t_secs)
{
    return _ct->ripx_0_1_set_update_interval(ifn, vifn, a, t_secs);
}

XrlCmdError
XrlRipTarget::rip_0_1_update_interval(const string&	ifn,
				      const string&	vifn,
				      const IPv4&	a,
				      uint32_t&		t_secs)
{
    return _ct->ripx_0_1_update_interval(ifn, vifn, a, t_secs);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_update_jitter(const string&	ifn,
					const string&	vifn,
					const IPv4&	a,
					const uint32_t& t_jitter)
{
    return _ct->ripx_0_1_set_update_jitter(ifn, vifn, a, t_jitter);
}

XrlCmdError
XrlRipTarget::rip_0_1_update_jitter(const string&	ifn,
				    const string&	vifn,
				    const IPv4&		a,
				    uint32_t&		t_jitter)
{
    return _ct->ripx_0_1_update_jitter(ifn, vifn, a, t_jitter);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_triggered_update_delay(const string&	ifn,
						 const string&	vifn,
						 const IPv4&	a,
						 const uint32_t& t_secs)
{
    return _ct->ripx_0_1_set_triggered_update_delay(ifn, vifn, a, t_secs);
}

XrlCmdError
XrlRipTarget::rip_0_1_triggered_update_delay(const string&	ifn,
					     const string&	vifn,
					     const IPv4&	a,
					     uint32_t&		t_secs)
{
    return _ct->ripx_0_1_triggered_update_delay(ifn, vifn, a, t_secs);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_triggered_update_jitter(const string&	ifn,
						  const string&	vifn,
						  const IPv4&	a,
						  const uint32_t& t_jitter)
{
    return _ct->ripx_0_1_set_triggered_update_jitter(ifn, vifn, a, t_jitter);
}

XrlCmdError
XrlRipTarget::rip_0_1_triggered_update_jitter(const string&	ifn,
					      const string&	vifn,
					      const IPv4&	a,
					      uint32_t&		t_jitter)
{
    return _ct->ripx_0_1_triggered_update_jitter(ifn, vifn, a, t_jitter);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_interpacket_delay(const string&	ifn,
					    const string&	vifn,
					    const IPv4&		a,
					    const uint32_t&	t_msecs)
{
    return _ct->ripx_0_1_set_interpacket_delay(ifn, vifn, a, t_msecs);
}

XrlCmdError
XrlRipTarget::rip_0_1_interpacket_delay(const string&	ifn,
					const string&	vifn,
					const IPv4&	a,
					uint32_t&	t_msecs)
{
    return _ct->ripx_0_1_interpacket_delay(ifn, vifn, a, t_msecs);
}

XrlCmdError
XrlRipTarget::rip_0_1_set_simple_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		addr,
    const string&	password)
{
    AuthHandlerBase* current_ah = NULL;
    PlaintextAuthHandler* plaintext_ah = NULL;

    pair<Port<IPv4>*, XrlCmdError> pp = _ct->find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;
    Port<IPv4>* p = pp.first;
    PortAFSpecState<IPv4>& pss = p->af_state();

    //
    // Modify an existing simple authentication handler or create a new one
    //
    current_ah = pss.auth_handler();
    XLOG_ASSERT(current_ah != NULL);
    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(current_ah);
    if (plaintext_ah != NULL) {
	plaintext_ah->set_key(password);
    } else {
	plaintext_ah = new PlaintextAuthHandler();
	plaintext_ah->set_key(password);
	pss.set_auth_handler(plaintext_ah);
	delete current_ah;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRipTarget::rip_0_1_delete_simple_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		addr)
{
    AuthHandlerBase* current_ah = NULL;
    PlaintextAuthHandler* plaintext_ah = NULL;
    string error_msg;

    pair<Port<IPv4>*, XrlCmdError> pp = _ct->find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;
    Port<IPv4>* p = pp.first;
    PortAFSpecState<IPv4>& pss = p->af_state();

    //
    // Test whether the simple password handler is really configured
    //
    current_ah = pss.auth_handler();
    XLOG_ASSERT(current_ah != NULL);
    plaintext_ah = dynamic_cast<PlaintextAuthHandler*>(current_ah);
    if (plaintext_ah != NULL) {
	//
	// XXX: Here we should return a mismatch error.
	// However, if we are adding both a simple password and MD5 handlers,
	// then the rtrmgr configuration won't match the protocol state.
	// Ideally, the rtrmgr and xorpsh shouldn't allow the user to add
	// both handlers. For the time being we need to live with this
	// limitation and therefore we shouldn't return an error here.
	//
	return XrlCmdError::OKAY();
#if 0
	error_msg = c_format("Cannot delete simple password authentication: "
			     "no such is configured");
	return XrlCmdError::COMMAND_FAILED(error_msg);
#endif
    }

    //
    // Install an empty handler and delete the simple authentication handler
    //
    NullAuthHandler* null_handler = new NullAuthHandler();
    pss.set_auth_handler(null_handler);
    delete current_ah;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRipTarget::rip_0_1_set_md5_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		addr,
    const uint32_t&	key_id,
    const string&	password,
    const string&	start_time,
    const string&	end_time)
{
    TimeVal start_timeval = TimeVal::ZERO();
    TimeVal end_timeval = TimeVal::MAXIMUM();
    AuthHandlerBase* current_ah = NULL;
    MD5AuthHandler* md5_ah = NULL;
    string error_msg;

    pair<Port<IPv4>*, XrlCmdError> pp = _ct->find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;
    Port<IPv4>* p = pp.first;
    PortAFSpecState<IPv4>& pss = p->af_state();

    //
    // Check the key ID
    //
    if (key_id > 255) {
	error_msg = c_format("Invalid key ID %u (valid range is [0, 255])",
			     XORP_UINT_CAST(key_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Decode the start and end time
    //
    if (! start_time.empty()) {
	if (decode_time_string(_e, start_time, start_timeval) != XORP_OK) {
	    error_msg = c_format("Invalid start time: %s", start_time.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    }
    if (! end_time.empty()) {
	if (decode_time_string(_e, end_time, end_timeval) != XORP_OK) {
	    error_msg = c_format("Invalid end time: %s", end_time.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    }

    //
    // Modify an existing MD5 authentication handler or create a new one
    //
    current_ah = pss.auth_handler();
    XLOG_ASSERT(current_ah != NULL);
    md5_ah = dynamic_cast<MD5AuthHandler*>(current_ah);
    if (md5_ah != NULL) {
	if (md5_ah->add_key(key_id, password, start_timeval, end_timeval,
			    error_msg)
	    != true) {
	    error_msg = c_format("MD5 key add failed: %s", error_msg.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
    } else {
	md5_ah = new MD5AuthHandler(_e);
	if (md5_ah->add_key(key_id, password, start_timeval, end_timeval,
			    error_msg)
	    != true) {
	    delete md5_ah;
	    error_msg = c_format("MD5 key add failed: %s", error_msg.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
	pss.set_auth_handler(md5_ah);
	delete current_ah;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRipTarget::rip_0_1_delete_md5_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		addr,
    const uint32_t&	key_id)
{
    AuthHandlerBase* current_ah = NULL;
    MD5AuthHandler* md5_ah = NULL;
    string error_msg;

    pair<Port<IPv4>*, XrlCmdError> pp = _ct->find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;
    Port<IPv4>* p = pp.first;
    PortAFSpecState<IPv4>& pss = p->af_state();

    //
    // Test whether the MD5 password handler is really configured
    //
    current_ah = pss.auth_handler();
    XLOG_ASSERT(current_ah != NULL);
    md5_ah = dynamic_cast<MD5AuthHandler*>(current_ah);
    if (md5_ah != NULL) {
	//
	// XXX: Here we should return a mismatch error.
	// However, if we are adding both a simple password and MD5 handlers,
	// then the rtrmgr configuration won't match the protocol state.
	// Ideally, the rtrmgr and xorpsh shouldn't allow the user to add
	// both handlers. For the time being we need to live with this
	// limitation and therefore we shouldn't return an error here.
	//
	return XrlCmdError::OKAY();
#if 0
	error_msg = c_format("Cannot delete MD5 password authentication: "
			     "no such is configured");
	return XrlCmdError::COMMAND_FAILED(error_msg);
#endif
    }

    XLOG_ASSERT(md5_ah != NULL);

    //
    // Check the key ID
    //
    if (key_id > 255) {
	error_msg = c_format("Invalid key ID %u (valid range is [0, 255])",
			     XORP_UINT_CAST(key_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Remove the key
    //
    if (md5_ah->remove_key(key_id, error_msg) != true) {
	error_msg = c_format("Invalid MD5 key ID %u: %s",
			     XORP_UINT_CAST(key_id), error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // If the last key, then install an empty handler and delete the MD5
    // authentication handler.
    //
    if (md5_ah->empty()) {
	NullAuthHandler* null_handler = new NullAuthHandler();
	pss.set_auth_handler(null_handler);
	delete current_ah;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRipTarget::rip_0_1_rip_address_status(const string&	ifn,
					 const string&	vifn,
					 const IPv4&	a,
					 string&	status)
{
    return _ct->ripx_0_1_rip_address_status(ifn, vifn, a, status);
}

XrlCmdError
XrlRipTarget::rip_0_1_get_all_addresses(XrlAtomList&	ifnames,
					XrlAtomList&	vifnames,
					XrlAtomList&	addrs)
{
    return _ct->ripx_0_1_get_all_addresses(ifnames, vifnames, addrs);
}

XrlCmdError
XrlRipTarget::rip_0_1_get_peers(const string&	ifn,
				const string&	vifn,
				const IPv4&	a,
				XrlAtomList&	peers)
{
    return _ct->ripx_0_1_get_peers(ifn, vifn, a, peers);
}

XrlCmdError
XrlRipTarget::rip_0_1_get_all_peers(XrlAtomList&	peers,
				    XrlAtomList&	ifnames,
				    XrlAtomList&	vifnames,
				    XrlAtomList&	addrs)
{
    return _ct->ripx_0_1_get_all_peers(peers, ifnames, vifnames, addrs);
}

XrlCmdError
XrlRipTarget::rip_0_1_get_counters(const string&	ifname,
				   const string&	vifname,
				   const IPv4&		addr,
				   XrlAtomList&		descs,
				   XrlAtomList&		values)
{
    return _ct->ripx_0_1_get_counters(ifname, vifname, addr, descs, values);
}

XrlCmdError
XrlRipTarget::rip_0_1_get_peer_counters(const string&	ifn,
					const string&	vifn,
					const IPv4&	addr,
					const IPv4&	peer,
					XrlAtomList&	descs,
					XrlAtomList&	vals,
					uint32_t&	peer_last_active)
{
    return _ct->ripx_0_1_get_peer_counters(ifn, vifn, addr, peer,
					   descs, vals, peer_last_active);
}

XrlCmdError
XrlRipTarget::rip_0_1_trace(const string& tvar, const bool& enable)
{
    debug_msg("trace variable %s enable %s\n", tvar.c_str(),
	      bool_c_str(enable));

    if (tvar == "all") {
	_ct->trace(enable);
    } else {
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Unknown variable %s", tvar.c_str()));
    }

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRipTarget::socket4_user_0_1_recv_event(
					const string&		sockid,
					const string&		if_name,
					const string&		vif_name,
					const IPv4&		src_host,
					const uint32_t&		src_port,
					const vector<uint8_t>&	pdata
					)
{
    return _ct->socketx_user_0_1_recv_event(sockid, if_name, vif_name,
					    src_host, src_port, pdata);
}

XrlCmdError
XrlRipTarget::socket4_user_0_1_inbound_connect_event(
    const string&	sockid,
    const IPv4&		src_host,
    const uint32_t&	src_port,
    const string&	new_sockid,
    bool&		accept)
{
    return _ct->socketx_user_0_1_inbound_connect_event(sockid,
						       src_host,
						       src_port,
						       new_sockid,
						       accept);
}

XrlCmdError
XrlRipTarget::socket4_user_0_1_outgoing_connect_event(
    const string&	sockid)
{
    return _ct->socketx_user_0_1_outgoing_connect_event(sockid);
}

XrlCmdError
XrlRipTarget::socket4_user_0_1_error_event(const string&	sockid,
					   const string& 	reason,
					   const bool&		fatal)
{
    return _ct->socketx_user_0_1_error_event(sockid, reason, fatal);
}

XrlCmdError
XrlRipTarget::socket4_user_0_1_disconnect_event(const string&	sockid)
{
    return _ct->socketx_user_0_1_disconnect_event(sockid);
}

XrlCmdError
XrlRipTarget::policy_backend_0_1_configure(const uint32_t& filter,
					   const string& conf) 
{
    return _ct->policy_backend_0_1_configure(filter, conf);
}					   

XrlCmdError
XrlRipTarget::policy_backend_0_1_reset(const uint32_t& filter) 
{
    return _ct->policy_backend_0_1_reset(filter);
}					  

XrlCmdError
XrlRipTarget::policy_backend_0_1_push_routes() 
{
    return _ct->policy_backend_0_1_push_routes();
}

XrlCmdError 
XrlRipTarget::policy_redist4_0_1_add_route4(const IPv4Net&	network,
					    const bool&		unicast,
				            const bool&		multicast,
				            const IPv4&		nexthop,
				            const uint32_t&	metric,
    				            const XrlAtomList&  policytags) 
{
    return _ct->policy_redistx_0_1_add_routex(network, unicast, multicast,
					      nexthop, metric, policytags);
}

XrlCmdError 
XrlRipTarget::policy_redist4_0_1_delete_route4(const IPv4Net&  network,
					       const bool&     unicast,
					       const bool&     multicast)
{					       
    return _ct->policy_redistx_0_1_delete_routex(network, unicast, multicast);
}
