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

// $XORP: xorp/rip/xrl_target_common.hh,v 1.12 2004/05/31 04:06:40 hodson Exp $

#ifndef __RIP_XRL_TARGET_COMMON_HH__
#define __RIP_XRL_TARGET_COMMON_HH__

#include "libxorp/status_codes.h"

#include "peer.hh"

class XrlProcessSpy;
template <typename A> class XrlPortManager;
template <typename A> class System;
template <typename A> class XrlRedistManager;

/**
 * @short Common handler for Xrl Requests.
 *
 * This class implements Xrl Target code that is common to both RIP
 * and RIP NG.
 */
template <typename A>
class XrlRipCommonTarget {
public:
    XrlRipCommonTarget(XrlProcessSpy& 		xps,
		       XrlPortManager<A>&	xpm,
		       XrlRedistManager<A>&	xrm,
		       bool& 			should_exit);

    ~XrlRipCommonTarget();

    void set_status(ProcessStatus ps, const string& annotation = "");

    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);

    XrlCmdError common_0_1_shutdown();

    XrlCmdError
    finder_event_observer_0_1_xrl_target_birth(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    finder_event_observer_0_1_xrl_target_death(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    ripx_0_1_add_rip_address(const string&	ifname,
			     const string&	vifname,
			     const A&		addr);

    XrlCmdError
    ripx_0_1_remove_rip_address(const string&	ifname,
				const string&	vifname,
				const A&	addr);

    XrlCmdError
    ripx_0_1_set_rip_address_enabled(const string&	ifname,
				     const string&	vifname,
				     const A&		addr,
				     const bool&	enabled);

    XrlCmdError
    ripx_0_1_rip_address_enabled(const string&	ifname,
				 const string&	vifname,
				 const A&	addr,
				 bool&		enabled);

    XrlCmdError ripx_0_1_set_cost(const string&		ifname,
				  const string&		vifname,
				  const A&		addr,
				  const uint32_t&	cost);

    XrlCmdError ripx_0_1_cost(const string&	ifname,
			      const string&	vifname,
			      const A&		addr,
			      uint32_t&		cost);

    XrlCmdError ripx_0_1_set_horizon(const string&	ifname,
				     const string&	vifname,
				     const A&		addr,
				     const string&	horizon);

    XrlCmdError ripx_0_1_horizon(const string&	ifname,
				 const string&	vifname,
				 const A&	addr,
				 string&	horizon);

    XrlCmdError ripx_0_1_set_passive(const string&	ifname,
				     const string&	vifname,
				     const A&		addr,
				     const bool&	passive);

    XrlCmdError ripx_0_1_passive(const string&	ifname,
				 const string&	vifname,
				 const A&	addr,
				 bool&		passive);

    XrlCmdError
    ripx_0_1_set_accept_non_rip_requests(const string&	ifname,
					 const string&	vifname,
					 const A&	addr,
					 const bool&	accept);

    XrlCmdError ripx_0_1_accept_non_rip_requests(const string&	ifname,
						 const string&	vifname,
						 const A&	addr,
						 bool&		accept);

    XrlCmdError ripx_0_1_set_accept_default_route(const string&	ifname,
						  const string&	vifname,
						  const A&	addr,
						  const bool&	accept);

    XrlCmdError ripx_0_1_accept_default_route(const string&	ifname,
					      const string&	vifname,
					      const A&		addr,
					      bool&		accept);

    XrlCmdError
    ripx_0_1_set_advertise_default_route(const string&	ifname,
					 const string&	vifname,
					 const A&	addr,
					 const bool&	advertise);

    XrlCmdError ripx_0_1_advertise_default_route(const string&	ifname,
						 const string&	vifname,
						 const A&	addr,
						 bool&		advertise);

    XrlCmdError
    ripx_0_1_set_route_expiry_seconds(const string&	ifname,
				      const string&	vifname,
				      const A&		addr,
				      const uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_route_expiry_seconds(const string&	ifname,
				  const string&	vifname,
				  const A&	addr,
				  uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_set_route_deletion_seconds(const string&	ifname,
					const string&	vifname,
					const A&	addr,
					const uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_route_deletion_seconds(const string&	ifname,
				    const string&	vifname,
				    const A&		addr,
				    uint32_t&		t_secs);

    XrlCmdError
    ripx_0_1_set_table_request_seconds(const string&	ifname,
				       const string&	vifname,
				       const A&		addr,
				       const uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_table_request_seconds(const string&	ifname,
				   const string&	vifname,
				   const A&		addr,
				   uint32_t&		t_secs);

    XrlCmdError
    ripx_0_1_set_unsolicited_response_min_seconds(const string&	ifname,
						  const string&	vifname,
						  const A&	addr,
						  const uint32_t& t_secs);

    XrlCmdError
    ripx_0_1_unsolicited_response_min_seconds(const string&	ifname,
					      const string&	vifname,
					      const A&		addr,
					      uint32_t&		t_secs);

    XrlCmdError
    ripx_0_1_set_unsolicited_response_max_seconds(
					const string&	ifname,
					const string&	vifname,
					const A&	addr,
					const uint32_t&	t_secs
					);

    XrlCmdError
    ripx_0_1_unsolicited_response_max_seconds(const string&	ifname,
					      const string&	vifname,
					      const A&		addr,
					      uint32_t&		t_secs);

    XrlCmdError
    ripx_0_1_set_triggered_update_min_seconds(const string&	ifname,
					      const string&	vifname,
					      const A&		addr,
					      const uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_triggered_update_min_seconds(const string&	ifname,
					  const string&	vifname,
					  const A&	addr,
					  uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_set_triggered_update_max_seconds(const string&	ifname,
					      const string&	vifname,
					      const A&		addr,
					      const uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_triggered_update_max_seconds(const string&	ifname,
					  const string&	vifname,
					  const A&	addr,
					  uint32_t&	t_secs);

    XrlCmdError
    ripx_0_1_set_interpacket_delay_milliseconds(const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t_msecs);

    XrlCmdError
    ripx_0_1_interpacket_delay_milliseconds(const string&	ifname,
					    const string&	vifname,
					    const A&		addr,
					    uint32_t&		t_msecs);

    XrlCmdError ripx_0_1_rip_address_status(const string&	ifname,
					    const string&	vifname,
					    const A&		addr,
					    string&		status);

    XrlCmdError ripx_0_1_get_all_addresses(XrlAtomList&	ifnames,
					   XrlAtomList&	vifnames,
					   XrlAtomList&	addrs);

    XrlCmdError ripx_0_1_get_peers(const string& ifname,
				   const string& vifname,
				   const A&	 addr,
				   XrlAtomList&	 peers);

    XrlCmdError ripx_0_1_get_all_peers(XrlAtomList& peers,
				       XrlAtomList& ifnames,
				       XrlAtomList& vifnames,
				       XrlAtomList& addrs);

    XrlCmdError ripx_0_1_get_counters(const string&	ifname,
				      const string&	vifname,
				      const A&		addr,
				      XrlAtomList&	descriptions,
				      XrlAtomList&	values);

    XrlCmdError ripx_0_1_get_peer_counters(const string&	ifname,
					   const string&	vifname,
					   const A&		addr,
					   const A&		peer,
					   XrlAtomList&		descriptions,
					   XrlAtomList&		values,
					   uint32_t&		peer_last_pkt);

    XrlCmdError ripx_0_1_import_protocol_routes(const string&	protocol,
						const uint32_t& cost,
						const uint32_t& tag);

    XrlCmdError ripx_0_1_no_import_protocol_routes(const string& protocol);

    XrlCmdError redistx_0_1_add_route(const IPNet<A>&		net,
				      const A&			nexthop,
				      const string&		ifname,
				      const string&		vifname,
				      const uint32_t&		metric,
				      const uint32_t&		ad,
				      const string&		cookie);

    XrlCmdError redistx_0_1_delete_route(const IPNet<A>&	net,
					 const string&		cookie);

    XrlCmdError socketx_user_0_1_recv_event(const string&	sockid,
					    const A&		src_host,
					    const uint32_t&	src_port,
					    const vector<uint8_t>& pdata);

    XrlCmdError socketx_user_0_1_connect_event(const string&	sockid,
					       const A&		src_host,
					       const uint32_t&	src_port,
					       const string&	new_sockid,
					       bool&		accept);

    XrlCmdError socketx_user_0_1_error_event(const string&	sockid,
					     const string& 	reason,
					     const bool&	fatal);

    XrlCmdError socketx_user_0_1_close_event(const string&	sockid,
					     const string&	reason);

    /**
     * Find Port associated with ifname, vifname, addr.
     *
     * @return on success the first item in the pair will be a
     * non-null pointer to the port and the second item with be
     * XrlCmdError::OKAY().  On failyre the first item in the pair
     * will be null and the XrlCmdError will signify the reason for
     * the failure.
     */
    pair<Port<A>*,XrlCmdError> find_port(const string&	ifname,
					 const string&	vifname,
					 const A&	addr);

protected:
    XrlProcessSpy&		_xps;
    XrlPortManager<A>&		_xpm;
    XrlRedistManager<A>&	_xrm;

    bool&			_should_exit;

    ProcessStatus		_status;
    string			_status_note;
};


// ----------------------------------------------------------------------------
// Inline implementation of XrlRipCommonTarget

template <typename A>
XrlRipCommonTarget<A>::XrlRipCommonTarget(XrlProcessSpy&	xps,
					  XrlPortManager<A>& 	xpm,
					  XrlRedistManager<A>&	xrm,
					  bool&			should_exit)
    : _xps(xps), _xpm(xpm), _xrm(xrm),
      _should_exit(should_exit), _status(PROC_NULL), _status_note("")
{
}

template <typename A>
XrlRipCommonTarget<A>::~XrlRipCommonTarget()
{
}

template <typename A>
void
XrlRipCommonTarget<A>::set_status(ProcessStatus status, const string& note)
{
    debug_msg("Status Update %d -> %d: %s\n", _status, status, note.c_str());
    _status 	 = status;
    _status_note = note;
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::common_0_1_get_status(uint32_t& status,
					     string&   reason)
{
    status = _status;
    reason = _status_note;
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::common_0_1_shutdown()
{
    debug_msg("Shutdown requested.\n");
    _should_exit = true;
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::finder_event_observer_0_1_xrl_target_birth(
							const string& cname,
							const string& iname
							)
{
    _xps.birth_event(cname, iname);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::finder_event_observer_0_1_xrl_target_death(
							const string& cname,
							const string& iname
							)
{
    _xps.death_event(cname, iname);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_add_rip_address(const string&	ifname,
					       const string&	vifname,
					       const A&		addr)
{
    debug_msg("rip_x_1_add_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.add_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_remove_rip_address(const string& ifname,
					 const string&		 vifname,
					 const A&   		 addr)
{
    debug_msg("ripx_0_1_remove_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.remove_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// Utility methods

template <typename A>
pair<Port<A>*, XrlCmdError>
XrlRipCommonTarget<A>::find_port(const string&	ifn,
				 const string&	vifn,
				 const A&	addr)
{
    Port<A>* p = _xpm.find_port(ifn, vifn, addr);
    if (p == 0) {
	string e = c_format("RIP not running on %s/%s/%s",
			    ifn.c_str(), vifn.c_str(), addr.str().c_str());
	return make_pair(p, XrlCmdError::COMMAND_FAILED(e));
    }
    return make_pair(p, XrlCmdError::OKAY());
}


// ----------------------------------------------------------------------------
// Port configuration accessors and modifiers

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_rip_address_enabled(const string&	ifn,
							const string&	vifn,
							const A&	addr,
							const bool&	en)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_enabled(en);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_rip_address_enabled(const string& ifn,
						    const string& vifn,
						    const A&	  addr,
						    bool&	  en)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    en = p->enabled();

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_cost(const string&	ifname,
			       const string&		vifname,
			       const A&			addr,
			       const uint32_t&		cost)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;

    if (cost > RIP_INFINITY) {
	string e = c_format("Cost must be less that RIP infinity (%u)",
			    RIP_INFINITY);
	return XrlCmdError::COMMAND_FAILED(e);
    }

    p->set_cost(cost);

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_cost(const string&	ifname,
				     const string&	vifname,
				     const A&		addr,
				     uint32_t&		cost)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    cost = p->cost();

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_horizon(const string&	ifname,
					    const string&	vifname,
					    const A&		addr,
					    const string&	horizon)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;

    RipHorizon rh[3] = { NONE, SPLIT, SPLIT_POISON_REVERSE };
    for (uint32_t i = 0; i < 3; ++i) {
	if (string(rip_horizon_name(rh[i])) == horizon) {
	    p->set_horizon(rh[i]);
	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED(
			c_format("Horizon type \"%s\" not recognized.",
				 horizon.c_str()));
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_horizon(const string&	ifname,
					const string&	vifname,
					const A&	addr,
					string&		horizon)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    horizon = rip_horizon_name(p->horizon());
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_passive(const string&	ifname,
					    const string&	vifname,
					    const A&		addr,
					    const bool&		passive)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_passive(passive);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_passive(const string&	ifname,
					const string&	vifname,
					const A&	addr,
					bool&		passive)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    passive = p->passive();
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_accept_non_rip_requests(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const bool&	accept
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_accept_non_rip_requests(accept);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_accept_non_rip_requests(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						bool&		accept
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    accept = p->accept_non_rip_requests();
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_accept_default_route(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const bool&	accept
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_accept_default_route(accept);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_accept_default_route(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						bool&		accept
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    accept = p->accept_default_route();
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_advertise_default_route(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const bool&	advertise
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_advertise_default_route(advertise);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_advertise_default_route(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						bool&		advertise
						)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    advertise = p->advertise_default_route();
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// The following pair of macros are used in setting timer constants on
// RIP ports.

#define PORT_TIMER_SET_HANDLER(field, min_val, max_val)			\
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);	\
    if (pp.first == 0)							\
	return pp.second;						\
    Port<A>* p = pp.first;						\
									\
    if (t < min_val) 							\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
	    "value supplied less than permitted minimum (%u < %u)", 	\
	    t, min_val));						\
    if (t > max_val) 							\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
	    "value supplied greater than permitted maximum (%u > %u)", 	\
	    t, max_val));						\
    if (p->constants().set_##field (t) == false)			\
	return XrlCmdError::COMMAND_FAILED(				\
	    "Failed to set value.");					\
    return XrlCmdError::OKAY();

#define PORT_TIMER_GET_HANDLER(field)					\
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);	\
    if (pp.first == 0)							\
	return pp.second;						\
    Port<A>* p = pp.first;						\
    t = p->constants(). field ();					\
    return XrlCmdError::OKAY();


template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_route_expiry_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(expiry_secs, 10, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_route_expiry_seconds(
					     const string&	ifname,
					     const string&	vifname,
					     const A&		addr,
					     uint32_t&		t
					     )
{
    PORT_TIMER_GET_HANDLER(expiry_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_route_deletion_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(deletion_secs, 10, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_route_deletion_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(deletion_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_table_request_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(table_request_period_secs, 1, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_table_request_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(table_request_period_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_unsolicited_response_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t& t
						)
{
    PORT_TIMER_SET_HANDLER(unsolicited_response_min_secs, 1, 300);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_unsolicited_response_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(unsolicited_response_min_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_unsolicited_response_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t& t
						)
{
    PORT_TIMER_SET_HANDLER(unsolicited_response_max_secs, 1, 600);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_unsolicited_response_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(unsolicited_response_max_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_triggered_update_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(triggered_update_min_wait_secs, 1, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_triggered_update_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(triggered_update_min_wait_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_triggered_update_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(triggered_update_max_wait_secs, 1, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_triggered_update_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(triggered_update_max_wait_secs);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_interpacket_delay_milliseconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(interpacket_delay_ms, 10, 10000);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_interpacket_delay_milliseconds(
						const string&	ifname,
						const string&	vifname,
						const A&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(interpacket_delay_ms);
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_rip_address_status(const string&	ifn,
						  const string&		vifn,
						  const A&		addr,
						  string&		status)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    status = p->enabled() ? "enabled" : "disabled";

    if (p->enabled()
	&& _xpm.underlying_rip_address_up(ifn, vifn, addr) == false) {
	if (_xpm.underlying_rip_address_exists(ifn, vifn, addr)) {
	    status += " [gated by disabled FEA interface/vif/address]";
	} else {
	    status += " [gated by non-existant FEA interface/vif/address]";
	}
    }

    debug_msg("ripx_0_1_rip_address_status %s/%s/%s -> %s\n",
	      ifn.c_str(), vifn.c_str(), addr.str().c_str(),
	      status.c_str());

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_all_addresses(XrlAtomList&	ifnames,
						  XrlAtomList&	vifnames,
						  XrlAtomList&	addrs)
{
    const typename PortManagerBase<A>::PortList & ports = _xpm.const_ports();
    typename PortManagerBase<A>::PortList::const_iterator pci;

    for (pci = ports.begin(); pci != ports.end(); ++pci) {
	const Port<A>*	     	port = *pci;
	const PortIOBase<A>* 	pio  = port->io_handler();
	if (pio == 0) {
	    continue;
	}
	ifnames.append  ( XrlAtom(pio->ifname())   );
	vifnames.append ( XrlAtom(pio->vifname())  );
	addrs.append    ( XrlAtom(pio->address())  );
    }
    return XrlCmdError::OKAY();
}


template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_peers(const string&	ifn,
					  const string&	vifn,
					  const A&	addr,
					  XrlAtomList&	peers)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    typename Port<A>::PeerList::const_iterator pi = p->peers().begin();
    while (pi != p->peers().end()) {
	const Peer<A>* peer = *pi;
	peers.append(XrlAtom(peer->address()));
	++pi;
    }
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_all_peers(XrlAtomList&	peers,
					      XrlAtomList&	ifnames,
					      XrlAtomList&	vifnames,
					      XrlAtomList&	addrs)
{
    const typename PortManagerBase<A>::PortList & ports = _xpm.const_ports();
    typename PortManagerBase<A>::PortList::const_iterator pci;

    for (pci = ports.begin(); pci != ports.end(); ++pci) {
	const Port<A>* port = *pci;
	const PortIOBase<A>* pio = port->io_handler();
	if (pio == 0) {
	    continue;
	}

	typename Port<A>::PeerList::const_iterator pi;
	for (pi = port->peers().begin(); pi != port->peers().end(); ++pi) {
	    const Peer<A>* peer = *pi;
	    peers.append    ( XrlAtom(peer->address()) );
	    ifnames.append  ( XrlAtom(pio->ifname())   );
	    vifnames.append ( XrlAtom(pio->vifname())  );
	    addrs.append    ( XrlAtom(pio->address())  );
	}
    }
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_counters(const string&	ifn,
					     const string&	vifn,
					     const A&		addr,
					     XrlAtomList&	descriptions,
					     XrlAtomList&	values)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    const Port<A>* p = pp.first;
    descriptions.append(XrlAtom("", string("Requests Sent")));
    values.append(XrlAtom(p->counters().table_requests_sent()));

    descriptions.append(XrlAtom("", string("Updates Sent")));
    values.append(XrlAtom(p->counters().unsolicited_updates()));

    descriptions.append(XrlAtom("", string("Triggered Updates Sent")));
    values.append(XrlAtom(p->counters().triggered_updates()));

    descriptions.append(XrlAtom("", string("Non-RIP Updates Sent")));
    values.append(XrlAtom(p->counters().non_rip_updates_sent()));

    descriptions.append(XrlAtom("", string("Total Packets Received")));
    values.append(XrlAtom(p->counters().packets_recv()));

    descriptions.append(XrlAtom("", string("Request Packets Received")));
    values.append(XrlAtom(p->counters().table_requests_recv()));

    descriptions.append(XrlAtom("", string("Update Packets Received")));
    values.append(XrlAtom(p->counters().update_packets_recv()));

    descriptions.append(XrlAtom("", string("Bad Packets Received")));
    values.append(XrlAtom(p->counters().bad_packets()));

    if (A::ip_version() == 4) {
	descriptions.append(XrlAtom("", string("Authentication Failures")));
	values.append(XrlAtom(p->counters().bad_auth_packets()));
    }

    descriptions.append(XrlAtom("", string("Bad Routes Received")));
    values.append(XrlAtom(p->counters().bad_routes()));

    descriptions.append(XrlAtom("", string("Non-RIP Requests Received")));
    values.append(XrlAtom(p->counters().non_rip_requests_recv()));

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_peer_counters(
					const string&	ifn,
					const string&	vifn,
					const A&	addr,
					const A&	peer_addr,
					XrlAtomList&	descriptions,
					XrlAtomList&	values,
					uint32_t&	peer_last_active)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    const Port<A>* port = pp.first;

    typename Port<A>::PeerList::const_iterator pi;
    pi = find_if(port->peers().begin(), port->peers().end(),
		 peer_has_address<A>(peer_addr));

    if (pi == port->peers().end()) {
	return XrlCmdError::COMMAND_FAILED(
		c_format("Peer %s not found on %s/%s/%s",
			 peer_addr.str().c_str(), ifn.c_str(), vifn.c_str(),
			 addr.str().c_str())
		);
    }

    const Peer<A>* const peer = *pi;

    descriptions.append(  XrlAtom("", string("Total Packets Received")) );
    values.append( XrlAtom(peer->counters().packets_recv()) );

    descriptions.append(XrlAtom("", string("Request Packets Received")));
    values.append(XrlAtom(peer->counters().table_requests_recv()));

    descriptions.append(XrlAtom("", string("Update Packets Received")));
    values.append(XrlAtom(peer->counters().update_packets_recv()));

    descriptions.append(XrlAtom("", string("Bad Packets Received")));
    values.append(XrlAtom(peer->counters().bad_packets()));

    if (A::ip_version() == 4) {
	descriptions.append(XrlAtom("", string("Authentication Failures")));
	values.append(XrlAtom(peer->counters().bad_auth_packets()));
    }

    descriptions.append(XrlAtom("", string("Bad Routes Received")));
    values.append(XrlAtom(peer->counters().bad_routes()));

    descriptions.append(XrlAtom("", string("Routes Active")));
    values.append(XrlAtom(peer->route_count()));

    peer_last_active = peer->last_active().secs();

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_import_protocol_routes(const string&	protocol,
						       const uint32_t&	cost,
						       const uint32_t& 	tag)
{
    _xrm.request_redist_for(protocol, cost, tag);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_no_import_protocol_routes(const string& protocol)
{
    _xrm.request_no_redist_for(protocol);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::redistx_0_1_add_route(const IPNet<A>&	net,
					     const A&		nexthop,
					     const string&	/* ifname */,
					     const string&	/* vifname */,
					     const uint32_t&	/* metric */,
					     const uint32_t&	/* admin_d */,
					     const string&	cookie)
{
    // We use cookie of the protocol name to make find the relevant redist table simple.
    _xrm.add_route(cookie, net, nexthop);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::redistx_0_1_delete_route(const IPNet<A>&	net,
						const string&	cookie)
{
    _xrm.delete_route(cookie, net);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_recv_event(
					const string&		sockid,
					const A&		src_host,
					const uint32_t&		src_port,
					const vector<uint8_t>&	pdata
					)
{
    _xpm.deliver_packet(sockid, src_host, src_port, pdata);
    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_connect_event(
					const string&	sockid,
					const A&	src_host,
					const uint32_t&	src_port,
					const string&	new_sockid,
					bool&		accept
					)
{
    debug_msg("socketx_user_0_1_connect_event %s %s/%u %s\n",
	      sockid.c_str(), src_host.str().c_str(), src_port,
	      new_sockid.c_str());

    UNUSED(sockid);
    UNUSED(src_host);
    UNUSED(src_port);
    UNUSED(new_sockid);

    accept = false;
    return XrlCmdError::COMMAND_FAILED("Connect not requested.");
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_error_event(
					const string&	sockid,
					const string& 	reason,
					const bool&	fatal
					)
{
    debug_msg("socketx_user_0_1_error_event %s %s %s \n",
	      sockid.c_str(), reason.c_str(),
	      fatal ? "fatal" : "non-fatal");

    UNUSED(sockid);
    UNUSED(reason);
    UNUSED(fatal);

    return XrlCmdError::OKAY();
}

template <typename A>
XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_close_event(
					const string&	sockid,
					const string&	reason
					)
{
    debug_msg("socketx_user_0_1_close_event %s %s\n",
	      sockid.c_str(), reason.c_str());

    UNUSED(sockid);
    UNUSED(reason);

    return XrlCmdError::OKAY();
}

#endif // __RIPX_XRL_TARGET_COMMON_HH__
