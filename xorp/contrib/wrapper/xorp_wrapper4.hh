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

#ifndef __WRAPPER_XRL_TARGET_HH__
#define __WRAPPER_XRL_TARGET_HH__

#include "xrl/targets/wrapper4_base.hh"
#include "libxipc/xrl_std_router.hh"

#include "xorp_io.hh"

class XrlWrapper4Target : public XrlWrapper4TargetBase {
public:
    /**
     * Constructor.
     */
    XrlWrapper4Target(XrlRouter* r, Wrapper& wrapper, XrlIO& xio);

    /**
     * Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
        // Output values,
        string& name);

    /**
     * Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
        // Output values,
        string& version);

    /**
     * Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
        // Output values,
        uint32_t&       status,
        string&         reason);

    /**
     * Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    XrlCmdError common_0_1_startup() {
        return XrlCmdError::OKAY();
    }

    /**
     * Announce target birth to observer.
     *
     * @param target_class the target class name.
     * @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
        // Input values,
        const string&   target_class,
        const string&   target_instance);

    /**
     * Announce target death to observer.
     *
     * @param target_class the target class name.
     * @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
        // Input values,
        const string&   target_class,
        const string&   target_instance);

    /**
     * Method invoked by target implementing socket4/0.1 when a packet arrives
     * from an IPv4 source.
     *
     * @param sockid the identifier associated with socket where the event
     * occurred.
     * @param if_name the interface name the packet arrived on, if known. If
     * unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known. If
     * unknown, then it is an empty string.
     * @param src_host the originating host.
     * @param src_port the originating IP port.
     * @param data the data received.
     */
    XrlCmdError socket4_user_0_1_recv_event(
        // Input values,
        const string&   sockid,
        const string&   if_name,
        const string&   vif_name,
        const IPv4&     src_host,
        const uint32_t& src_port,
        const vector<uint8_t>&  data);

    /**
     * Method invoked by target implementing socket4/0.1 when a connection
     * request is received from an IPv4 source. It applies only to TCP
     * sockets.
     *
     * @param sockid the identifier associated with socket where the event
     * occurred.
     * @param src_host the connecting host.
     * @param src_port the connecting IP port.
     * @param new_sockid the identifier associated with the new socket that
     * has been created to handle the new connection.
     * @param accept if true, the connection request has been accepted,
     * otherwise it has been rejected.
     */
    XrlCmdError socket4_user_0_1_inbound_connect_event(
        // Input values,
        const string&   sockid,
        const IPv4&     src_host,
        const uint32_t& src_port,
        const string&   new_sockid,
        // Output values,
        bool&   accept);

    /**
     * Method invoked by target implementing socket4/0.1 when an outgoing
     * connection request originated by the local host is completed. It
     * applies only to TCP sockets. Note that if the connection failed, the
     * error_event will be dispatched instead.
     *
     * @param sockid the identifier associated with socket where the event
     * occurred.
     */
    XrlCmdError socket4_user_0_1_outgoing_connect_event(
        // Input values,
        const string&   sockid);

    /**
     * Method invoked by target implementing socket4/0.1 when an error occurs.
     *
     * @param sockid the identifier associated with socket where the event
     * occurred.
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of error.
     */
    XrlCmdError socket4_user_0_1_error_event(
        // Input values,
        const string&   sockid,
        const string&   error,
        const bool&     fatal);

    /**
     * Method invoked by target implementing socket4/0.1 when the peer has
     * closed the connection. It applies only to TCP sockets. Note that the
     * socket itself is left open and must be explicitly closed.
     *
     * @param sockid the identifier associated with socket where the event
     * occurred.
     */
    XrlCmdError socket4_user_0_1_disconnect_event(
        // Input values,
        const string&   sockid);

    /**
     * Configure a policy filter.
     *
     * @param filter the identifier of the filter to configure.
     * @param conf the configuration of the filter.
     */
    XrlCmdError policy_backend_0_1_configure(
        // Input values,
        const uint32_t& filter,
        const string&   conf);

    /**
     * Reset a policy filter.
     *
     * @param filter the identifier of the filter to reset.
     */
    XrlCmdError policy_backend_0_1_reset(
        // Input values,
        const uint32_t& filter);

    /**
     * Push all available routes through all filters for re-filtering.
     */
    XrlCmdError policy_backend_0_1_push_routes();

    /**
     * Start route redistribution for an IPv4 route.
     *
     * @param network the route to advertise.
     * @param unicast whether the route is unicast.
     * @param multicast whether the route is multicast.
     * @param nexthop the nexthop of the route.
     * @param metric the metric of the route.
     * @param policytags the set of policy-tags associated with the route.
     */
    XrlCmdError policy_redist4_0_1_add_route4(
        // Input values,
        const IPv4Net&  network,
        const bool&     unicast,
        const bool&     multicast,
        const IPv4&     nexthop,
        const uint32_t& metric,
        const XrlAtomList&      policytags);

    /**
     * Terminate route redistribution for an IPv4 route.
     *
     * @param network the route for which advertisements should cease.
     * @param unicast whether the route is unicast.
     * @param multicast whether the route is multicast.
     */
    XrlCmdError policy_redist4_0_1_delete_route4(
        // Input values,
        const IPv4Net&  network,
        const bool&     unicast,
        const bool&     multicast);

    /**
     * Enable profiling.
     *
     * @param pname profile variable
     */
    XrlCmdError profile_0_1_enable(
        // Input values,
        const string&   pname);

    /**
     * Disable profiling.
     *
     * @param pname profile variable
     */
    XrlCmdError profile_0_1_disable(
        // Input values,
        const string&   pname);

    /**
     * Get log entries.
     *
     * @param pname profile variable
     * @param instance_name to send the profiling info to.
     */
    XrlCmdError profile_0_1_get_entries(
        // Input values,
        const string&   pname,
        const string&   instance_name);

    /**
     * Clear the profiling entries.
     *
     * @param pname profile variable
     */
    XrlCmdError profile_0_1_clear(
        // Input values,
        const string&   pname);

    /**
     *  List all the profiling variables registered with this target.
     */
    XrlCmdError profile_0_1_list(
        // Output values,
        string& info);

    /**
     */
    XrlCmdError wrapper4_0_1_set_admin_distance(
        // input value
        const uint32_t& admin
    );

    XrlCmdError wrapper4_0_1_get_admin_distance(
        // output value
        uint32_t& admin
    );

    XrlCmdError wrapper4_0_1_set_main_address(
        // input value
        const IPv4& addr);

    XrlCmdError wrapper4_0_1_get_main_address(
        // output value
        IPv4& addr);

    XrlCmdError wrapper4_0_1_restart(
    );

    XrlCmdError wrapper4_0_1_wrapper_application(
        // input value
        const string& app,
        const string& para);

    /**
     * Get the state of an IPv4 address binding for Wrapper.
     *
     * @param ifname the interface to query.
     * @param vifname the vif to qurery
     * @param enabled true if OLSR is configured administratively up on the
     * given interface.
     */
    XrlCmdError wrapper4_0_1_get_binding_enabled(
        // Input values,
        const string&   ifname,
        const string&   vifname,
        // Output values,
        bool&           enabled);


    /**
     * Get the list of interfaces currently configured for OLSR. Return a list
     * of u32 type values. Each value is an internal ID that can be used with
     * the get_interface_info XRL.
     */
    XrlCmdError wrapper4_0_1_get_interface_list(
        // Output values,
        XrlAtomList&    interfaces);

    /**
     * Get the per-interface information for the given interface.
     *
     * @param faceid interface ID returned by get_interface_list.
     * @param ifname the name of the interface.
     * @param vifname the name of the vif.
     * @param local_addr the IPv4 address where Wrapper is listening.
     * @param local_port the UDP port where Wrapper is listening.
     * @param all_nodes_addr the IPv4 address where Wrapper sends packets.
     * @param all_nodes_port the UDP port where Wrapper sends packets.
     */
    XrlCmdError wrapper4_0_1_get_interface_info(
        // Input values,
        const uint32_t& faceid,
        // Output values,
        string&         ifname,
        string&         vifname,
        IPv4&           local_addr,
        uint32_t&       local_port,
        IPv4&           all_nodes_addr,
        uint32_t&       all_nodes_port);




private:
    Wrapper&    _wrapper;
    XrlIO&      _xio;
};



#endif // __WRAPPER_XRL_TARGET_HH__

