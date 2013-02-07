// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
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
// ===========================================================================

#include "wrapper_module.h"

#include "xorp_wrapper4.hh"
#include "xorp_io.hh"
#include "wrapper.hh"

XrlWrapper4Target::XrlWrapper4Target(XrlRouter* r, Wrapper& wrapper, XrlIO& xio)
    : XrlWrapper4TargetBase(r),
      _wrapper(wrapper),
      _xio(xio)
{
}

XrlCmdError XrlWrapper4Target::common_0_1_get_target_name(string& name)
{
    name = "wrapper4";
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::common_0_1_get_version(string& version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::common_0_1_get_status (uint32_t& status, string& reason)
{
    status = _wrapper.status(reason);
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::common_0_1_shutdown()
{
    _wrapper.quiting();
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::finder_event_observer_0_1_xrl_target_birth(
    const string& target_class,
    const string& target_instance)
{
    debug_msg("finder_event_observer_0_1_xrl_target_birth %s %s\n",
              target_class.c_str(), target_instance.c_str());

    return XrlCmdError::OKAY();
    UNUSED(target_class);
    UNUSED(target_instance);
}

XrlCmdError XrlWrapper4Target::finder_event_observer_0_1_xrl_target_death(
    const string& target_class,
    const string& target_instance)
{
    debug_msg("finder_event_observer_0_1_xrl_target_death %s %s\n",
              target_class.c_str(), target_instance.c_str());

    return XrlCmdError::OKAY();
    UNUSED(target_class);
    UNUSED(target_instance);
}

XrlCmdError XrlWrapper4Target::socket4_user_0_1_recv_event(
    const string&           sockid,
    const string&           if_name,
    const string&           vif_name,
    const IPv4&             src_host,
    const uint32_t&         src_port,
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
    _xio.receive(sockid, if_name, vif_name, src_host, src_port,  data);

    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::socket4_user_0_1_inbound_connect_event(
    const string&       sockid,
    const IPv4&         src_host,
    const uint32_t&     src_port,
    const string&       new_sockid,
    bool&               accept)
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

XrlCmdError XrlWrapper4Target::socket4_user_0_1_outgoing_connect_event(const string& sockid)
{
    debug_msg("socket4_user_0_1_outgoing_connect_event %s\n",
              sockid.c_str());

    return XrlCmdError::COMMAND_FAILED("Outgoing connect not requested.");
    UNUSED(sockid);
}

XrlCmdError XrlWrapper4Target::socket4_user_0_1_error_event(
    const string&       sockid,
    const string&       error,
    const bool&         fatal)
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

XrlCmdError XrlWrapper4Target::socket4_user_0_1_disconnect_event(const string& sockid)
{
    debug_msg("socket4_user_0_1_disconnect_event %s\n", sockid.c_str());

    return XrlCmdError::OKAY();
    UNUSED(sockid);
}

XrlCmdError XrlWrapper4Target::policy_backend_0_1_configure(const uint32_t& filter,
        const string& conf)
{
    debug_msg("policy_backend_0_1_configure %u %s\n",
              XORP_UINT_CAST(filter), conf.c_str());

    try {
        _wrapper.configure_filter(filter, conf);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
                                           e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::policy_backend_0_1_reset(const uint32_t& filter)
{
    debug_msg("policy_backend_0_1_reset %u\n", XORP_UINT_CAST(filter));
    try {
        _wrapper.reset_filter(filter);
    } catch(const PolicyException& e) {
        return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
                                           e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::policy_backend_0_1_push_routes()
{
    debug_msg("policy_backend_0_1_push_routes\n");
    _xio.push_routes();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::policy_redist4_0_1_add_route4(
    const IPv4Net&      network,
    const bool&         unicast,
    const bool&         multicast,
    const IPv4&         nexthop,
    const uint32_t&     metric,
    const XrlAtomList&  policytags)
{
    debug_msg("policy_redist4_0_1_add_route4 %s %s %s %s %u %u\n",
              cstring(network),
              bool_c_str(unicast),
              bool_c_str(multicast),
              cstring(nexthop),
              XORP_UINT_CAST(metric),
              XORP_UINT_CAST(policytags.size()));
    if (unicast) {
        _xio.fromXorp(POLICY_ADD_ROUTE,network.str(),unicast,multicast,nexthop.str(),metric);
    }
    return XrlCmdError::OKAY();
    UNUSED(multicast);
}

XrlCmdError XrlWrapper4Target::policy_redist4_0_1_delete_route4(
    const IPv4Net&      network,
    const bool&         unicast,
    const bool&         multicast)
{
    debug_msg("policy_redist4_0_1_delete_route4 %s %s %s\n",
              cstring(network),
              bool_c_str(unicast),
              bool_c_str(multicast));
    if (unicast) {
        _xio.fromXorp(POLICY_DEL_ROUTE,network.str(),unicast,multicast,string(""),0);
    }
    return XrlCmdError::OKAY();
    UNUSED(multicast);
}

XrlCmdError XrlWrapper4Target::profile_0_1_enable(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError XrlWrapper4Target::profile_0_1_disable(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError XrlWrapper4Target::profile_0_1_get_entries(const string& pname,
        const string& instance_name)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
    UNUSED(instance_name);
}

XrlCmdError XrlWrapper4Target::profile_0_1_clear(const string& pname)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(pname);
}

XrlCmdError XrlWrapper4Target::profile_0_1_list(string& info)
{
    return XrlCmdError::COMMAND_FAILED("Profiling not yet implemented");
    UNUSED(info);
}

XrlCmdError XrlWrapper4Target::wrapper4_0_1_set_admin_distance(const uint32_t& admin)
{
    _wrapper.set_admin_dist(admin);
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::wrapper4_0_1_get_admin_distance(uint32_t& admin)
{
    admin = _wrapper.get_admin_dist();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlWrapper4Target::wrapper4_0_1_set_main_address(const IPv4& addr)
{
    _wrapper.set_main_addr(addr);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlWrapper4Target::wrapper4_0_1_get_main_address(IPv4& addr)
{
    addr = _wrapper.get_main_addr();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlWrapper4Target::wrapper4_0_1_restart()
{
    _wrapper.restart();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlWrapper4Target::wrapper4_0_1_wrapper_application(
    const string& app,
    const string& para)
{
    _wrapper.runClient(app,para);
    return XrlCmdError::OKAY();
}

XrlCmdError XrlWrapper4Target::wrapper4_0_1_get_interface_list(XrlAtomList& interfaces)
{
    UNUSED(interfaces);
    return XrlCmdError::COMMAND_FAILED("Unable to obtain interface list");
}

XrlCmdError XrlWrapper4Target::wrapper4_0_1_get_interface_info(
    const uint32_t&     faceid,
    string&             ifname,
    string&             vifname,
    IPv4&               local_addr,
    uint32_t&           local_port,
    IPv4&               all_nodes_addr,
    uint32_t&           all_nodes_port)
{
    UNUSED(faceid);
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(all_nodes_addr);
    UNUSED(all_nodes_port);
    return XrlCmdError::COMMAND_FAILED("Unable to get interface entry");
}







