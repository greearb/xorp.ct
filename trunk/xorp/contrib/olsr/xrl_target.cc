// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/xrl_target.cc,v 1.1 2008/04/24 15:19:57 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxipc/xrl_std_router.hh"

#include "olsr.hh"
#include "xrl_io.hh"
#include "xrl_target.hh"

XrlOlsr4Target::XrlOlsr4Target(XrlRouter* r, Olsr& olsr, XrlIO& xrl_io)
 : XrlOlsr4TargetBase(r),
   _olsr(olsr),
   _xrl_io(xrl_io)
{
}

/*
 * common/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::common_0_1_get_target_name(string& name)
{
    name = "ospf4";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::common_0_1_get_version(string& version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::common_0_1_get_status(uint32_t& status, string& reason)
{
    status = _olsr.status(reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::common_0_1_shutdown()
{
    debug_msg("common_0_1_shutdown\n");

    _olsr.shutdown();

    return XrlCmdError::OKAY();
}


/*
 * socket4_user/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::finder_event_observer_0_1_xrl_target_birth(
    const string& target_class,
    const string& target_instance)
{
    debug_msg("finder_event_observer_0_1_xrl_target_birth %s %s\n",
	      target_class.c_str(), target_instance.c_str());

    return XrlCmdError::OKAY();
    UNUSED(target_class);
    UNUSED(target_instance);
}

XrlCmdError
XrlOlsr4Target::finder_event_observer_0_1_xrl_target_death(
    const string& target_class,
    const string& target_instance)
{
    debug_msg("finder_event_observer_0_1_xrl_target_death %s %s\n",
	      target_class.c_str(), target_instance.c_str());

    return XrlCmdError::OKAY();
    UNUSED(target_class);
    UNUSED(target_instance);
}


/*
 * socket4_user/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::socket4_user_0_1_recv_event(
    const string&	    sockid,
    const string&	    if_name,
    const string&	    vif_name,
    const IPv4&		    src_host,
    const uint32_t&	    src_port,
    const vector<uint8_t>&  data)
{
    debug_msg("socket4_user_0_1_recv_event %s %s/%s %s/%u %u\n",
	      sockid.c_str(), if_name.c_str(), vif_name.c_str(),
	      cstring(src_host), XORP_UINT_CAST(src_port),
	      XORP_UINT_CAST(data.size()));

    if (if_name == "" || vif_name == "") {
	// Note: This can occur if an interface was added to the FEA interface { }
	// block at runtime and a binding later added without restarting the FEA.
	XLOG_FATAL("No FEA platform support for determining interface name, "
		   "bailing. Please report this to the XORP/OLSR maintainer.");
    }

    //
    // Note: The socket4_user interface does not tell us the
    // destination address of the IPv4 datagram.
    //
    _xrl_io.receive(sockid,
		    if_name,
		    vif_name,
		    src_host,
		    src_port,
		    data);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::socket4_user_0_1_inbound_connect_event(
    const string&	sockid,
    const IPv4&		src_host,
    const uint32_t&	src_port,
    const string&	new_sockid,
    bool&		accept)
{
    debug_msg("socket4_user_0_1_inbound_connect_event %s %s/%u %s\n",
	      sockid.c_str(),
	      cstring(src_host), XORP_UINT_CAST(src_port),
	      new_sockid.c_str());

    accept = false;

    return XrlCmdError::COMMAND_FAILED("Inbound connect not requested.");
    UNUSED(sockid);
    UNUSED(src_host);
    UNUSED(src_port);
    UNUSED(new_sockid);
}

XrlCmdError
XrlOlsr4Target::socket4_user_0_1_outgoing_connect_event(
	const string& sockid)
{
    debug_msg("socket4_user_0_1_outgoing_connect_event %s\n",
	      sockid.c_str());

    return XrlCmdError::COMMAND_FAILED("Outgoing connect not requested.");
    UNUSED(sockid);
}

XrlCmdError
XrlOlsr4Target::socket4_user_0_1_error_event(
    const string&	sockid,
    const string&	error,
    const bool&		fatal)
{
    debug_msg("socket4_user_0_1_error_event %s %s %s\n",
	      sockid.c_str(),
	      error.c_str(),
	      fatal ? "fatal" : "non-fatal");

    return XrlCmdError::OKAY();
    UNUSED(sockid);
    UNUSED(error);
    UNUSED(fatal);
}

XrlCmdError
XrlOlsr4Target::socket4_user_0_1_disconnect_event(const string& sockid)
{
    debug_msg("socket4_user_0_1_disconnect_event %s\n", sockid.c_str());

    return XrlCmdError::OKAY();
    UNUSED(sockid);
}


/*
 * policy_backend/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::policy_backend_0_1_configure(const uint32_t& filter,
					     const string& conf)
{
    debug_msg("policy_backend_0_1_configure %u %s\n",
	      XORP_UINT_CAST(filter), conf.c_str());

    try {
#ifdef notyet
	XLOG_TRACE(_olsr.profile().enabled(trace_policy_configure),
		   "policy filter: %d conf: %s\n", filter, conf.c_str());
#else
	debug_msg("policy filter: %d conf: %s\n", filter, conf.c_str());
#endif
	_olsr.configure_filter(filter, conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::policy_backend_0_1_reset(const uint32_t& filter)
{
    debug_msg("policy_backend_0_1_reset %u\n", XORP_UINT_CAST(filter));

    try {
#ifdef notyet
	XLOG_TRACE(_olsr.profile().enabled(trace_policy_configure),
		   "policy filter: %d\n", filter);
#else
	debug_msg("policy filter: %d\n", filter);
#endif
	_olsr.reset_filter(filter);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::policy_backend_0_1_push_routes()
{
    debug_msg("policy_backend_0_1_push_routes\n");

    _olsr.push_routes();

    return XrlCmdError::OKAY();
}


/*
 * policy_redist/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::policy_redist4_0_1_add_route4(
    const IPv4Net&	network,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4&		nexthop,
    const uint32_t& 	metric,
    const XrlAtomList&	policytags)
{
    debug_msg("policy_redist4_0_1_add_route4 %s %s %s %s %u %u\n",
	      cstring(network),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      cstring(nexthop),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(policytags.size()));

    // OLSR does not [yet] support multicast routes.
    if (! unicast)
	return XrlCmdError::OKAY();

    if (! _olsr.originate_external_route(network, nexthop, metric,
					 policytags)) {
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());
    }

    return XrlCmdError::OKAY();
    UNUSED(multicast);
}

XrlCmdError
XrlOlsr4Target::policy_redist4_0_1_delete_route4(
    const IPv4Net&	network,
    const bool&		unicast,
    const bool&		multicast)
{
    debug_msg("policy_redist4_0_1_delete_route4 %s %s %s\n",
	      cstring(network),
	      bool_c_str(unicast),
	      bool_c_str(multicast));

    if (! unicast)
	return XrlCmdError::OKAY();

    if (! _olsr.withdraw_external_route(network))
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());

    return XrlCmdError::OKAY();
    UNUSED(multicast);
}


/*
 * profile/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::profile_0_1_enable(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError
XrlOlsr4Target::profile_0_1_disable(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError
XrlOlsr4Target::profile_0_1_get_entries(const string& pname,
					const string& instance_name)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
    UNUSED(instance_name);
}

XrlCmdError
XrlOlsr4Target::profile_0_1_clear(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError
XrlOlsr4Target::profile_0_1_list(string& info)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(info);
}


/*
 * olsr4/0.1 target interface.
 */

XrlCmdError
XrlOlsr4Target::olsr4_0_1_trace(const string& tvar, const bool& enable)
{
    debug_msg("olsr4_0_1_trace %s %s\n", tvar.c_str(), bool_c_str(enable));

    // TODO: Enable profiling points here.

    if (tvar == "all") {
	_olsr.trace().all(enable);
    } else {
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Unknown variable %s", tvar.c_str()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_clear_database()
{
    debug_msg("olsr4_0_1_clear_database\n");

    if (! _olsr.clear_database())
	return XrlCmdError::COMMAND_FAILED("Unable to clear database");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_willingness(const uint32_t& willingness)
{
    debug_msg("olsr4_0_1_set_willingness %u\n", XORP_UINT_CAST(willingness));

    if (! _olsr.set_willingness(OlsrTypes::WillType(willingness)))
	return XrlCmdError::COMMAND_FAILED("Unable to set willingness");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_willingness(uint32_t& willingness)
{
    debug_msg("olsr4_0_1_get_willingness\n");

    willingness = _olsr.get_willingness();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_mpr_coverage(const uint32_t& coverage)
{
    debug_msg("olsr4_0_1_set_mpr_coverage %u\n", XORP_UINT_CAST(coverage));

    if (! _olsr.set_mpr_coverage(coverage))
	return XrlCmdError::COMMAND_FAILED("Unable to set MPR_COVERAGE");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_mpr_coverage(uint32_t& coverage)
{
    debug_msg("olsr4_0_1_get_mpr_coverage\n");

    coverage = _olsr.get_mpr_coverage();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_tc_redundancy(const string& redundancy)
{
    debug_msg("olsr4_0_1_set_tc_redundancy %s\n", redundancy.c_str());

    OlsrTypes::TcRedundancyType type;
    do {
	if (strcasecmp(redundancy.c_str(), "mprs") == 0) {
	    type = OlsrTypes::TCR_MPRS_IN;
	    break;
	}
	if (strcasecmp(redundancy.c_str(), "mprs-and-selectors") == 0) {
	    type = OlsrTypes::TCR_MPRS_INOUT;
	    break;
	}
	if (strcasecmp(redundancy.c_str(), "all") == 0) {
	    type = OlsrTypes::TCR_ALL;
	    break;
	}
	return XrlCmdError::BAD_ARGS("Unknown TC_REDUNDANCY mode" +
				     redundancy);
    } while (false);

    if (! _olsr.set_tc_redundancy(type))
	return XrlCmdError::COMMAND_FAILED("Unable to set TC_REDUNDANCY");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_tc_redundancy(string& redundancy)
{
    debug_msg("olsr4_0_1_get_tc_redundancy\n");

    redundancy = _olsr.get_tc_redundancy();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_tc_fisheye(const bool& enabled)
{
    debug_msg("olsr4_0_1_set_tc_fisheye %s\n", bool_c_str(enabled));

    return XrlCmdError::COMMAND_FAILED("Unable to set TC fisheye; "
				       "not yet implemented");
    UNUSED(enabled);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_tc_fisheye(bool& enabled)
{
    debug_msg("olsr4_0_1_get_tc_fisheye\n");

    return XrlCmdError::COMMAND_FAILED("Unable to get TC fisheye; "
				       "not yet implemented");
    UNUSED(enabled);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_hna_base_cost(const uint32_t& metric)
{
    debug_msg("olsr4_0_1_set_hna_base_cost %u\n", XORP_UINT_CAST(metric));

    return XrlCmdError::COMMAND_FAILED("Unable to set HNA base cost; "
				       "not yet implemented");
    UNUSED(metric);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_hna_base_cost(uint32_t& metric)
{
    debug_msg("olsr4_0_1_get_hna_base_cost\n");

    return XrlCmdError::COMMAND_FAILED("Unable to get HNA base cost; "
				       "not yet implemented");
    UNUSED(metric);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_hello_interval(const uint32_t& interval)
{
    debug_msg("olsr4_0_1_set_hello_interval %u\n",
	      XORP_UINT_CAST(interval));

    if (! _olsr.set_hello_interval(TimeVal(interval, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set HELLO_INTERVAL");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_hello_interval(uint32_t& interval)
{
    debug_msg("olsr4_0_1_get_hello_interval\n");

    interval = _olsr.get_hello_interval().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_refresh_interval(const uint32_t& interval)
{
    debug_msg("olsr4_0_1_set_refresh_interval %u\n",
	      XORP_UINT_CAST(interval));

    if (! _olsr.set_refresh_interval(TimeVal(interval, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set REFRESH_INTERVAL");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_refresh_interval(uint32_t& interval)
{
    debug_msg("olsr4_0_1_get_refresh_interval\n");

    interval = _olsr.get_refresh_interval().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_tc_interval(const uint32_t& interval)
{
    debug_msg("olsr4_0_1_set_tc_interval %u\n",
	      XORP_UINT_CAST(interval));

    if (! _olsr.set_tc_interval(TimeVal(interval, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set TC_INTERVAL");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_tc_interval(uint32_t& interval)
{
    debug_msg("olsr4_0_1_get_tc_interval\n");

    interval = _olsr.get_tc_interval().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_mid_interval(const uint32_t& interval)
{
    debug_msg("olsr4_0_1_set_mid_interval %u\n",
	      XORP_UINT_CAST(interval));

    if (! _olsr.set_mid_interval(TimeVal(interval, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set MID_INTERVAL");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_mid_interval(uint32_t& interval)
{
    debug_msg("olsr4_0_1_get_mid_interval\n");

    interval = _olsr.get_mid_interval().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_hna_interval(const uint32_t& interval)
{
    debug_msg("olsr4_0_1_set_hna_interval %u\n",
	      XORP_UINT_CAST(interval));

    if (! _olsr.set_hna_interval(TimeVal(interval, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set HNA_INTERVAL");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_hna_interval(uint32_t& interval)
{
    debug_msg("olsr4_0_1_get_hna_interval\n");

    interval = _olsr.get_hna_interval().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_dup_hold_time(const uint32_t& dup_hold_time)
{
    debug_msg("olsr4_0_1_set_dup_hold_time %u\n",
	      XORP_UINT_CAST(dup_hold_time));

    if (! _olsr.set_dup_hold_time(TimeVal(dup_hold_time, 0)))
	return XrlCmdError::COMMAND_FAILED("Unable to set DUP_HOLD_TIME");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_dup_hold_time(uint32_t& dup_hold_time)
{
    debug_msg("olsr4_0_1_set_dup_hold_time\n");

    dup_hold_time = _olsr.get_dup_hold_time().secs();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_main_address(const IPv4& addr)
{
    debug_msg("olsr4_0_1_set_main_address %s\n", cstring(addr));

    if (! _olsr.set_main_addr(addr))
	return XrlCmdError::COMMAND_FAILED("Unable to set main address");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_main_address(IPv4& addr)
{
    debug_msg("olsr4_0_1_set_main_address\n");

    addr = _olsr.get_main_addr();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_bind_address(
    const string&	ifname,
    const string&	vifname,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		all_nodes_addr,
    const uint32_t&	all_nodes_port)
{
    debug_msg("olsr4_0_1_bind_address %s/%s %s/%u %s/%u\n",
	      ifname.c_str(), vifname.c_str(),
	      cstring(local_addr), XORP_UINT_CAST(local_port),
	      cstring(all_nodes_addr), XORP_UINT_CAST(all_nodes_port));

    if (! _olsr.bind_address(ifname, vifname,
			     local_addr, local_port,
			     all_nodes_addr, all_nodes_port)) {
	return XrlCmdError::COMMAND_FAILED(
	    c_format("Unable to bind to %s/%s %s/%u %s/%u\n",
	      ifname.c_str(), vifname.c_str(),
	      cstring(local_addr), XORP_UINT_CAST(local_port),
	      cstring(all_nodes_addr), XORP_UINT_CAST(all_nodes_port)));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_unbind_address(
    const string&	ifname,
    const string&	vifname)
{
    debug_msg("olsr4_0_1_unbind_address %s/%s\n",
	      ifname.c_str(), vifname.c_str());

    if (! _olsr.unbind_address(ifname, vifname)) {
	return XrlCmdError::COMMAND_FAILED(
	    c_format("Unable to unbind from %s/%s",
		     ifname.c_str(), vifname.c_str()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_binding_enabled(
    const string&	ifname,
    const string&	vifname,
    const bool&		enabled)
{
    debug_msg("olsr4_0_1_set_binding_enabled %s/%s %s\n",
	      ifname.c_str(), vifname.c_str(), bool_c_str(enabled));

    if (! _olsr.set_interface_enabled(ifname, vifname, enabled)) {
	return XrlCmdError::COMMAND_FAILED(
	    c_format("Unable to enable/disable binding on %s/%s",
		     ifname.c_str(), vifname.c_str()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_binding_enabled(
    const string&	ifname,
    const string&	vifname,
    bool&		enabled)
{
    debug_msg("olsr4_0_1_get_binding_enabled %s/%s\n",
	      ifname.c_str(), vifname.c_str());

    // XXX

    return XrlCmdError::OKAY();
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(enabled);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_change_local_addr_port(
    const string&	ifname,
    const string&	vifname,
    const IPv4&		local_addr,
    const uint32_t&	local_port)
{
    debug_msg("olsr4_0_1_change_local_addr_port %s/%s %s/%u\n",
	      ifname.c_str(), vifname.c_str(),
	      cstring(local_addr), XORP_UINT_CAST(local_port));

    // XXX
    XLOG_WARNING("OLSR does not yet support changing local address and "
		 "port at runtime.");

    return XrlCmdError::OKAY();
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(local_addr);
    UNUSED(local_port);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_change_all_nodes_addr_port(
    const string&	ifname,
    const string&	vifname,
    const IPv4&		all_nodes_addr,
    const uint32_t&	all_nodes_port)
{
    debug_msg("olsr4_0_1_change_all_nodes_addr_port %s/%s %s/%u\n",
	      ifname.c_str(), vifname.c_str(),
	      cstring(all_nodes_addr), XORP_UINT_CAST(all_nodes_port));

    // XXX
    XLOG_WARNING("OLSR does not yet support changing remote address and "
		 "port at runtime.");

    return XrlCmdError::OKAY();
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(all_nodes_addr);
    UNUSED(all_nodes_port);
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_interface_list(XrlAtomList& interfaces)
{
    debug_msg("olsr4_0_1_get_interface_list\n");

    try {
	list<OlsrTypes::FaceID> face_list;
	_olsr.face_manager().get_face_list(face_list);

	list<OlsrTypes::FaceID>::const_iterator ii;
	for (ii = face_list.begin(); ii != face_list.end(); ii++)
	    interfaces.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain interface list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_interface_info(
    const uint32_t&	faceid,
    string&		ifname,
    string&		vifname,
    IPv4&		local_addr,
    uint32_t&		local_port,
    IPv4&		all_nodes_addr,
    uint32_t&		all_nodes_port)
{
    debug_msg("olsr4_0_1_get_interface_info %u\n", XORP_UINT_CAST(faceid));

    try {
	const Face* face =
	    _olsr.face_manager().get_face_by_id(faceid);

	ifname = face->interface();
	vifname = face->vif();
	local_addr = face->local_addr();
	local_port = face->local_port();
	all_nodes_addr = face->all_nodes_addr();
	all_nodes_port = face->all_nodes_port();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get interface entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_set_interface_cost(
    const string& ifname,
    const string& vifname,
    const uint32_t& cost)
{
    debug_msg("olsr4_0_1_set_interface_cost %s/%s %u\n",
	      ifname.c_str(), vifname.c_str(),
	      XORP_UINT_CAST(cost));

    if (! _olsr.set_interface_cost(ifname, vifname, cost))
	return XrlCmdError::COMMAND_FAILED("Unable to set interface cost");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_interface_stats(
    const string&	ifname,
    const string&	vifname,
    uint32_t&		bad_packets,
    uint32_t&		bad_messages,
    uint32_t&		messages_from_self,
    uint32_t&		unknown_messages,
    uint32_t&		duplicates,
    uint32_t&		forwarded)
{
    debug_msg("olsr4_0_1_get_interface_stats %s/%s\n",
	      ifname.c_str(), vifname.c_str());

    FaceCounters stats;
    if (! _olsr.get_interface_stats(ifname, vifname, stats)) {
	return XrlCmdError::COMMAND_FAILED(
	    "Unable to get interface statistics");
    }
#define copy_stat(var)		var = stats. var ()
    copy_stat(bad_packets);
    copy_stat(bad_messages);
    copy_stat(messages_from_self);
    copy_stat(unknown_messages);
    copy_stat(duplicates);
    copy_stat(forwarded);
#undef copy_stat

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_link_list(XrlAtomList& links)
{
    debug_msg("olsr4_0_1_get_link_list\n");

    try {
	list<OlsrTypes::LogicalLinkID> l1_list;
	_olsr.neighborhood().get_logical_link_list(l1_list);

	list<OlsrTypes::LogicalLinkID>::const_iterator ii;
	for (ii = l1_list.begin(); ii != l1_list.end(); ii++)
	    links.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain link entry list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_link_info(
    const uint32_t& linkid,
    IPv4&	    local_addr,
    IPv4&	    remote_addr,
    IPv4&	    main_addr,
    uint32_t&	    link_type,
    uint32_t&	    sym_time,
    uint32_t&	    asym_time,
    uint32_t&	    hold_time)
{
    debug_msg("olsr4_0_1_get_link_info %u\n", XORP_UINT_CAST(linkid));

    try {
	const LogicalLink* l1 =
	    _olsr.neighborhood().get_logical_link(linkid);

	local_addr = l1->local_addr();
	remote_addr = l1->remote_addr();
	main_addr = l1->destination()->main_addr();
	link_type = l1->link_type();
	sym_time = l1->sym_time_remaining().secs();
	asym_time = l1->asym_time_remaining().secs();
	hold_time = l1->time_remaining().secs();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get link entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_neighbor_list(XrlAtomList& neighbors)
{
    debug_msg("olsr4_0_1_get_neighbor_list\n");

    try {
	list<OlsrTypes::NeighborID> n1_list;
	_olsr.neighborhood().get_neighbor_list(n1_list);

	list<OlsrTypes::NeighborID>::const_iterator ii;
	for (ii = n1_list.begin(); ii != n1_list.end(); ii++)
	    neighbors.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain neighbor entry list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_neighbor_info(
    const uint32_t&	nid,
    IPv4&		main_addr,
    uint32_t&		willingness,
    uint32_t&		degree,
    uint32_t&		link_count,
    uint32_t&		twohop_link_count,
    bool&		is_advertised,
    bool&		is_sym,
    bool&		is_mpr,
    bool&		is_mpr_selector)
{
    debug_msg("olsr4_0_1_get_neighbor_info %u\n", XORP_UINT_CAST(nid));

    try {
	const Neighbor* n1 =
	    _olsr.neighborhood().get_neighbor(nid);

	main_addr = n1->main_addr();
	willingness = n1->willingness();

	degree = n1->degree();
	link_count = n1->links().size();
	twohop_link_count = n1->twohop_links().size();

	is_advertised = n1->is_advertised();
	is_sym = n1->is_sym();
	is_mpr = n1->is_mpr();
	is_mpr_selector = n1->is_mpr_selector();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get neighbor entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_twohop_link_list(XrlAtomList& twohop_links)
{
    debug_msg("olsr4_0_1_get_twohop_link_list\n");

    try {
	list<OlsrTypes::TwoHopLinkID> l2_list;
	_olsr.neighborhood().get_twohop_link_list(l2_list);

	list<OlsrTypes::TwoHopLinkID>::const_iterator ii;
	for (ii = l2_list.begin(); ii != l2_list.end(); ii++)
	    twohop_links.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain two-hop link list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_twohop_link_info(
    const uint32_t& tlid,
    uint32_t& last_face_id,
    IPv4& nexthop_addr,
    IPv4& dest_addr,
    uint32_t& hold_time)
{
    debug_msg("olsr4_0_1_get_twohop_link_info %u\n", XORP_UINT_CAST(tlid));

    try {
	const TwoHopLink* l2 =
	    _olsr.neighborhood().get_twohop_link(tlid);

	// We don't convert the face ID to a name in the
	// XRL, perhaps we should, this might change.
	last_face_id = l2->face_id();
	nexthop_addr = l2->nexthop()->main_addr();
	dest_addr = l2->destination()->main_addr();
	hold_time = l2->time_remaining().secs();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get two-hop link entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_twohop_neighbor_list(
    XrlAtomList& twohop_neighbors)
{
    debug_msg("olsr4_0_1_get_twohop_neighbor_list\n");

    try {
	list<OlsrTypes::TwoHopNodeID> n2_list;
	_olsr.neighborhood().get_twohop_link_list(n2_list);

	list<OlsrTypes::TwoHopNodeID>::const_iterator ii;
	for (ii = n2_list.begin(); ii != n2_list.end(); ii++)
	    twohop_neighbors.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::
	COMMAND_FAILED("Unable to obtain two-hop neighbor list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_twohop_neighbor_info(
    const uint32_t&	tnid,
    IPv4&		main_addr,
    bool&		is_strict,
    uint32_t&		link_count,
    uint32_t&		reachability,
    uint32_t&		coverage)
{
    debug_msg("olsr4_0_1_get_twohop_neighbor_info %u\n",
	      XORP_UINT_CAST(tnid));

    try {
	const TwoHopNeighbor* n2 =
	    _olsr.neighborhood().get_twohop_neighbor(tnid);

	main_addr = n2->main_addr();
	is_strict = n2->is_strict();
	link_count = n2->twohop_links().size();
	reachability = n2->reachability();
	coverage = n2->coverage();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get two-hop neighbor entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_mid_entry_list(XrlAtomList& mid_entries)
{
    debug_msg("olsr4_0_1_get_mid_entry_list\n");

    try {
	list<OlsrTypes::MidEntryID> midlist;
	_olsr.topology_manager().get_mid_list(midlist);

	list<OlsrTypes::MidEntryID>::const_iterator ii;
	for (ii = midlist.begin(); ii != midlist.end(); ii++)
	    mid_entries.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain MID entry list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_mid_entry(
    const uint32_t&	midid,
    IPv4&		main_addr,
    IPv4&		iface_addr,
    uint32_t&		distance,
    uint32_t&		hold_time)
{
    debug_msg("olsr4_0_1_get_mid_entry %u\n", XORP_UINT_CAST(midid));

    try {
	const MidEntry* mid =
	    _olsr.topology_manager().get_mid_entry_by_id(midid);

	main_addr = mid->main_addr();
	iface_addr = mid->iface_addr();
	distance = mid->distance();
	hold_time = mid->time_remaining().secs();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get MID entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_tc_entry_list(XrlAtomList& tc_entries)
{
    debug_msg("olsr4_0_1_get_tc_entry_list\n");

    try {
	list<OlsrTypes::TopologyID> tclist;
	_olsr.topology_manager().get_topology_list(tclist);

	list<OlsrTypes::TopologyID>::const_iterator ii;
	for (ii = tclist.begin(); ii != tclist.end(); ii++)
	    tc_entries.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain TC entry list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_tc_entry(
    const uint32_t&	tcid,
    IPv4&		destination,
    IPv4&		lasthop,
    uint32_t&		distance,
    uint32_t&		seqno,
    uint32_t&		hold_time)
{
    debug_msg("olsr4_0_1_get_tc_entry %u\n", XORP_UINT_CAST(tcid));

    try {
	const TopologyEntry* tc =
	    _olsr.topology_manager().get_topology_entry_by_id(tcid);

	destination = tc->destination();
	lasthop = tc->lasthop();
	distance = tc->distance();
	seqno = tc->seqno();
	hold_time = tc->time_remaining().secs();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get TC entry");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_hna_entry_list(XrlAtomList& hna_entries)
{
    debug_msg("olsr4_0_1_get_hna_entry_list\n");

    try {
	list<OlsrTypes::ExternalID> hnalist;
	_olsr.external_routes().get_hna_route_in_list(hnalist);

	list<OlsrTypes::ExternalID>::const_iterator ii;
	for (ii = hnalist.begin(); ii != hnalist.end(); ii++)
	    hna_entries.append(XrlAtom(*ii));

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to obtain HNA entry list");
}

XrlCmdError
XrlOlsr4Target::olsr4_0_1_get_hna_entry(
    const uint32_t&	hnaid,
    IPv4Net&		destination,
    IPv4&		lasthop,
    uint32_t&		distance,
    uint32_t&		hold_time)
{
    debug_msg("olsr4_0_1_get_hna_entry %u\n", XORP_UINT_CAST(hnaid));

    try {
	const ExternalRoute* er =
	    _olsr.external_routes().get_hna_route_in_by_id(hnaid);

	destination = er->dest();
	lasthop = er->lasthop();
	distance = er->distance();
	hold_time = er->time_remaining().secs();

	return XrlCmdError::OKAY();
    } catch (...) {}

    return XrlCmdError::COMMAND_FAILED("Unable to get HNA entry");
}
