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

#include "config.h"
#include "libxorp/xorp.h"

#define DEBUG_LOGGING

#include "libxorp/debug.h"
#include "libxorp/status_codes.h"
#include "libxipc/xrl_router.hh"

#include "system.hh"
#include "xrl_target4.hh"
#include "xrl_process_spy.hh"
#include "xrl_port_manager.hh"

XrlRip4Target::XrlRip4Target(XrlRouter&			xr,
			     XrlProcessSpy&		xps,
			     XrlPortManager<IPv4>& 	xpm,
			     bool&			should_exit)
    : XrlRip4TargetBase(&xr), _xps(xps), _xpm(xpm), _system(xpm.system()),
      _should_exit(should_exit),
      _status(PROC_NULL), _status_note("")
{
}

XrlRip4Target::~XrlRip4Target()
{
}

void
XrlRip4Target::set_status(ProcessStatus status, const string& note)
{
    debug_msg("Status Update %d -> %d: %s\n", _status, status, note.c_str());
    _status 	 = status;
    _status_note = note;
}

XrlCmdError
XrlRip4Target::common_0_1_get_target_name(string& n)
{
    n = name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_get_version(string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_get_status(uint32_t& status,
				     string&   reason)
{
    status = _status;
    reason = _status_note;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_shutdown()
{
    debug_msg("Shutdown requested.\n");
    _should_exit = true;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_birth(const string& cname,
							  const string& iname)
{
    _xps.birth_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_death(const string& cname,
							  const string& iname)
{
    _xps.death_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_add_rip_address(const string& ifname,
					const string& vifname,
					const IPv4&   addr)
{
    debug_msg("rip4_0_1_add_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.add_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_remove_rip_address(const string& ifname,
					   const string& vifname,
					   const IPv4&   addr)
{
    debug_msg("rip4_0_1_remove_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.remove_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

static bool s_enabled = false;

XrlCmdError
XrlRip4Target::rip4_0_1_set_rip_address_enabled(const string& ifname,
						const string& vifname,
						const IPv4&   addr,
						const bool&   enabled)
{
    debug_msg("rip4_0_1_set_rip_address_enabled %s/%s/%s %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      enabled ? "true" : "false");
    s_enabled = enabled;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_rip_address_enabled(const string& ifname,
					    const string& vifname,
					    const IPv4&   addr,
					    bool&	  enabled)
{
    enabled = s_enabled;

    debug_msg("rip4_0_1_set_rip_address_enabled %s/%s/%s -> %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      enabled ? "true" : "false");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_cost(const string&		ifname,
				 const string&		vifname,
				 const IPv4&		addr,
				 const uint32_t&	cost)
{
    Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);
    if (p == 0) {
	return XrlCmdError::COMMAND_FAILED(c_format(
		"RIP not running on %s/%s/%s",
		ifname.c_str(), vifname.c_str(), addr.str().c_str()));
    }
    if (cost > RIP_INFINITY) {
	return XrlCmdError::COMMAND_FAILED(c_format(
		"Cost must be less that RIP infinity (%u)",
		RIP_INFINITY));
    }
    p->set_cost(cost);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_cost(const string&	ifname,
			     const string&	vifname,
			     const IPv4&	addr,
			     uint32_t&		cost)
{
    const Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);
    if (p == 0) {
	return XrlCmdError::COMMAND_FAILED(c_format(
		"RIP not running on %s/%s/%s",
		ifname.c_str(), vifname.c_str(), addr.str().c_str()));
    }
    cost = p->cost();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_horizon(const string&	ifname,
				    const string&	vifname,
				    const IPv4&		addr,
				    const string&	horizon)
{
    Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);
    if (p == 0) {
	return XrlCmdError::COMMAND_FAILED(c_format(
		"RIP not running on %s/%s/%s",
		ifname.c_str(), vifname.c_str(), addr.str().c_str()));
    }
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

XrlCmdError
XrlRip4Target::rip4_0_1_horizon(const string&	ifname,
				const string&	vifname,
				const IPv4&	addr,
				string&		horizon)
{
    const Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);
    if (p == 0) {
	return XrlCmdError::COMMAND_FAILED(c_format(
		"RIP not running on %s/%s/%s",
		ifname.c_str(), vifname.c_str(), addr.str().c_str()));
    }
    horizon = rip_horizon_name(p->horizon());
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// The following pair of macros are used in setting timer constants on
// RIP ports.

#define PORT_TIMER_SET_HANDLER(field, min_val, max_val)			\
    Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);		\
    if (p == 0)								\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
	    "RIP not running on %s/%s/%s",				\
	    ifname.c_str(), vifname.c_str(), addr.str().c_str()));	\
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
    Port<IPv4>* p = _xpm.find_port(ifname, vifname, addr);		\
    if (p == 0)								\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
	    "RIP not running on %s/%s/%s",				\
	    ifname.c_str(), vifname.c_str(), addr.str().c_str()));	\
    t = p->constants().##field ();					\
    return XrlCmdError::OKAY();


XrlCmdError
XrlRip4Target::rip4_0_1_set_route_expiry_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(expiry_secs, 10, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_route_expiry_seconds(
					     const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     uint32_t&		t
					     )
{
    PORT_TIMER_GET_HANDLER(expiry_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_route_deletion_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(deletion_secs, 10, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_route_deletion_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(deletion_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_table_request_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(table_request_period_secs, 1, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_table_request_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(table_request_period_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_unsolicited_response_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t& t
						)
{
    PORT_TIMER_SET_HANDLER(unsolicited_response_min_secs, 1, 300);
}

XrlCmdError
XrlRip4Target::rip4_0_1_unsolicited_response_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(unsolicited_response_min_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_unsolicited_response_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t& t
						)
{
    PORT_TIMER_SET_HANDLER(unsolicited_response_max_secs, 1, 600);
}

XrlCmdError
XrlRip4Target::rip4_0_1_unsolicited_response_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(unsolicited_response_max_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_triggered_update_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(triggered_update_min_wait_secs, 1, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_triggered_update_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(triggered_update_min_wait_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_triggered_update_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(triggered_update_max_wait_secs, 1, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_triggered_update_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(triggered_update_max_wait_secs);
}

XrlCmdError
XrlRip4Target::rip4_0_1_set_interpacket_delay_milliseconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						const uint32_t&	t
						)
{
    PORT_TIMER_SET_HANDLER(interpacket_delay_ms, 10, 10000);
}

XrlCmdError
XrlRip4Target::rip4_0_1_interpacket_delay_milliseconds(
						const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						uint32_t&	t
						)
{
    PORT_TIMER_GET_HANDLER(interpacket_delay_ms);
}

XrlCmdError
XrlRip4Target::rip4_0_1_rip_address_status(const string& ifname,
					   const string& vifname,
					   const IPv4&   addr,
					   string&	 status)
{
    status = (s_enabled) ? "running" : "not running";

    debug_msg("rip4_0_1_rip_address_status %s/%s/%s -> %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      status.c_str());

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_add_static_route(const IPv4Net& 	network,
					 const IPv4& 		nexthop,
					 const uint32_t& 	cost)
{
    if (cost > RIP_INFINITY) {
	return XrlCmdError::COMMAND_FAILED(c_format("Bad cost %u", cost));
    }

    _system.add_route_redistributor("static", 0);
    if (_system.redistributor_add_route("static", network, nexthop, cost)
	== false) {
	return XrlCmdError::COMMAND_FAILED("Route exists already.");
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_delete_static_route(const IPv4Net& 	network)
{
    if (_system.redistributor_remove_route("static", network) == false) {
	return XrlCmdError::COMMAND_FAILED("No route to delete.");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_recv_event(const string&	sockid,
					   const IPv4&		src_host,
					   const uint32_t&	src_port,
					   const vector<uint8_t>& pdata)
{
    _xpm.deliver_packet(sockid, src_host, src_port, pdata);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_connect_event(const string&	sockid,
					      const IPv4&	src_host,
					      const uint32_t&	src_port,
					      const string&	new_sockid,
					      bool&		accept)
{
    debug_msg("socket4_user_0_1_connect_event %s %s/%u %s\n",
	      sockid.c_str(), src_host.str().c_str(), src_port,
	      new_sockid.c_str());
    accept = false;
    return XrlCmdError::COMMAND_FAILED("Connect not requested.");
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_error_event(const string&	sockid,
					    const string& 	reason,
					    const bool&		fatal)
{
    debug_msg("socket4_user_0_1_error_event %s %s %s \n",
	      sockid.c_str(), reason.c_str(),
	      fatal ? "fatal" : "non-fatal");
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_close_event(const string&	sockid,
					    const string&	reason)
{
    debug_msg("socket4_user_0_1_close_event %s %s\n",
	      sockid.c_str(), reason.c_str());
    return XrlCmdError::OKAY();
}

