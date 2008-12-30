// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/mld6igmp/xrl_mld6igmp_node.hh,v 1.48 2008/07/23 05:11:04 pavlin Exp $

#ifndef __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
#define __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__


//
// MLD6IGMP XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/fea_rawpkt4_xif.hh"
#include "xrl/interfaces/fea_rawpkt6_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "xrl/interfaces/mld6igmp_client_xif.hh"
#include "xrl/targets/mld6igmp_base.hh"

#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlMld6igmpNode : public Mld6igmpNode,
			public XrlStdRouter,
			public XrlMld6igmpTargetBase,
			public Mld6igmpNodeCli {
public:
    XrlMld6igmpNode(int			family,
		    xorp_module_id	module_id, 
		    EventLoop&		eventloop,
		    const string&	class_name,
		    const string&	finder_hostname,
		    uint16_t		finder_port,
		    const string&	finder_target,
		    const string&	fea_target,
		    const string&	mfea_target);
    virtual ~XrlMld6igmpNode();

    /**
     * Startup the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

    /**
     * Get a const reference to the XrlRouter instance.
     *
     * @return a const reference to the XrlRouter (@ref XrlRouter) instance.
     */
    const XrlRouter& xrl_router() const { return *this; }

    //
    // XrlMld6igmpNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    int enable_mld6igmp();
    int disable_mld6igmp();
    int start_mld6igmp();
    int stop_mld6igmp();

    // XrlTask relatedMethods that need to be public
    void send_register_unregister_interest();
    void send_register_unregister_receiver();
    void send_join_leave_multicast_group();
    void send_protocol_message();

protected:
    //
    // XRL target methods
    //
    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string&	reason);

    /**
     * Shutdown cleanly
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Announce target birth to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Process a CLI command.
     *  
     *  @param processor_name the processor name for this command.
     *  
     *  @param cli_term_name the terminal name the command was entered from.
     *  
     *  @param cli_session_id the CLI session ID the command was entered from.
     *  
     *  @param command_name the command name to process.
     *  
     *  @param command_args the command arguments to process.
     *  
     *  @param ret_processor_name the processor name to return back to the CLI.
     *  
     *  @param ret_cli_term_name the terminal name to return back.
     *  
     *  @param ret_cli_session_id the CLI session ID to return back.
     *  
     *  @param ret_command_output the command output to return back.
     */
    XrlCmdError cli_processor_0_1_process_command(
	// Input values, 
	const string&	processor_name, 
	const string&	cli_term_name, 
	const uint32_t&	cli_session_id,
	const string&	command_name, 
	const string&	command_args, 
	// Output values, 
	string&	ret_processor_name, 
	string&	ret_cli_term_name, 
	uint32_t& ret_cli_session_id,
	string&	ret_command_output);

    /**
     *  Receive an IPv4 packet from a raw socket.
     *
     *  @param if_name the interface name the packet arrived on.
     *
     *  @param vif_name the vif name the packet arrived on.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     *  the received value is unknown.
     *
     *  @param ip_tos the Type of Service (Diffserv/ECN bits for IPv4). If it
     *  has a negative value, then the received value is unknown.
     *
     *  @param ip_router_alert if true, the IP Router Alert option was included
     *  in the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     */
    XrlCmdError raw_packet4_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const vector<uint8_t>&	payload);

    /**
     *  Receive an IPv6 packet from a raw socket.
     *
     *  @param if_name the interface name the packet arrived on.
     *
     *  @param vif_name the vif name the packet arrived on.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     *  the received value is unknown.
     *
     *  @param ip_tos the Type Of Service (IP traffic class for IPv4). If it
     *  has a negative value, then the received value is unknown.
     *
     *  @param ip_router_alert if true, the IP Router Alert option was included
     *  in the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     *
     *  @param ext_headers_type a list of u32 integers with the types of the
     *  optional extention headers.
     *
     *  @param ext_headers_payload a list of payload data, one for each
     *  optional extention header. The number of entries must match
     *  ext_headers_type.
     */
    XrlCmdError raw_packet6_client_0_1_recv(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv6&	src_address,
	const IPv6&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const XrlAtomList&	ext_headers_type,
	const XrlAtomList&	ext_headers_payload,
	const vector<uint8_t>&	payload);
    
    /**
     *  Enable/disable/start/stop a MLD6IGMP vif interface.
     *
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     *
     *  @param enable if true, then enable the vif, otherwise disable it.
     */
    XrlCmdError mld6igmp_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable);

    XrlCmdError mld6igmp_0_1_start_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mld6igmp_0_1_stop_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all MLD6IGMP vif interfaces.
     *
     *  @param enable if true, then enable the vifs, otherwise disable them.
     */
    XrlCmdError mld6igmp_0_1_enable_all_vifs(
	// Input values,
	const bool&	enable);

    XrlCmdError mld6igmp_0_1_start_all_vifs();

    XrlCmdError mld6igmp_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop the MLD6IGMP protocol.
     *
     *  @param enable if true, then enable the MLD6IGMP protocol, otherwise
     *  disable it.
     */
    XrlCmdError mld6igmp_0_1_enable_mld6igmp(
	// Input values,
	const bool&	enable);

    XrlCmdError mld6igmp_0_1_start_mld6igmp();

    XrlCmdError mld6igmp_0_1_stop_mld6igmp();

    /**
     *  Enable/disable/start/stop the MLD6IGMP CLI access.
     *
     *  @param enable if true, then enable the MLD6IGMP CLI access, otherwise
     *  disable it.
     */
    XrlCmdError mld6igmp_0_1_enable_cli(
	// Input values,
	const bool&	enable);

    XrlCmdError mld6igmp_0_1_start_cli();

    XrlCmdError mld6igmp_0_1_stop_cli();

    /**
     *  Get the configured protocol version per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param proto_version the protocol version.
     */
    XrlCmdError mld6igmp_0_1_get_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	// Output values, 
	uint32_t&	proto_version);

    /**
     *  Set the protocol version per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param proto_version the protocol version.
     */
    XrlCmdError mld6igmp_0_1_set_vif_proto_version(
	// Input values, 
	const string&	vif_name, 
	const uint32_t&	proto_version);

    /**
     *  Reset the protocol version per interface to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_proto_version(
	// Input values, 
	const string&	vif_name);

    /**
     *  Get the IP Router Alert option check per interface for received
     *  packets.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param enabled if true, then the IP Router Alert option check was
     *  enabled, otherwise it was disabled.
     */
    XrlCmdError mld6igmp_0_1_get_vif_ip_router_alert_option_check(
	// Input values,
	const string&	vif_name,
	// Output values,
	bool&	enabled);

    /**
     *  Set the IP Router Alert option check per interface for received
     *  packets.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param enable if true, then enable the IP Router Alert option check,
     *  otherwise disable it.
     */
    XrlCmdError mld6igmp_0_1_set_vif_ip_router_alert_option_check(
	// Input values,
	const string&	vif_name,
	const bool&	enable);

    /**
     *  Reset the IP Router Alert option check per interface for received
     *  packets to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_ip_router_alert_option_check(
	// Input values,
	const string&	vif_name);

    /**
     *  Get the Query Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_get_vif_query_interval(
	// Input values,
	const string&	vif_name,
	// Output values,
	uint32_t&	interval_sec,
	uint32_t&	interval_usec);

    /**
     *  Set the Query Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_set_vif_query_interval(
	// Input values,
	const string&	vif_name,
	const uint32_t&	interval_sec,
	const uint32_t&	interval_usec);

    /**
     *  Reset the Query Interval per interface to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_query_interval(
	// Input values,
	const string&	vif_name);

    /**
     *  Get the Last Member Query Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_get_vif_query_last_member_interval(
	// Input values,
	const string&	vif_name,
	// Output values,
	uint32_t&	interval_sec,
	uint32_t&	interval_usec);

    /**
     *  Set the Last Member Query Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_set_vif_query_last_member_interval(
	// Input values,
	const string&	vif_name,
	const uint32_t&	interval_sec,
	const uint32_t&	interval_usec);

    /**
     *  Reset the Last Member Query Interval per interface to its default
     *  value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_query_last_member_interval(
	// Input values,
	const string&	vif_name);

    /**
     *  Get the Query Response Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_get_vif_query_response_interval(
	// Input values,
	const string&	vif_name,
	// Output values,
	uint32_t&	interval_sec,
	uint32_t&	interval_usec);

    /**
     *  Set the Query Response Interval per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param interval_sec the number of seconds in the interval.
     *
     *  @param interval_usec the number of microseconds (in addition to
     *  interval_sec) in the interval.
     */
    XrlCmdError mld6igmp_0_1_set_vif_query_response_interval(
	// Input values,
	const string&	vif_name,
	const uint32_t&	interval_sec,
	const uint32_t&	interval_usec);

    /**
     *  Reset the Query Response Interval per interface to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_query_response_interval(
	// Input values,
	const string&	vif_name);

    /**
     *  Get the Robustness Variable count per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param robust_count the count value.
     */
    XrlCmdError mld6igmp_0_1_get_vif_robust_count(
	// Input values,
	const string&	vif_name,
	// Output values,
	uint32_t&	robust_count);

    /**
     *  Set the Robustness Variable count per interface.
     *
     *  @param vif_name the name of the vif to apply to.
     *
     *  @param robust_count the count value.
     */
    XrlCmdError mld6igmp_0_1_set_vif_robust_count(
	// Input values,
	const string&	vif_name,
	const uint32_t&	robust_count);

    /**
     *  Reset the Robustness Variable count per interface to its default value.
     *
     *  @param vif_name the name of the vif to apply to.
     */
    XrlCmdError mld6igmp_0_1_reset_vif_robust_count(
	// Input values,
	const string&	vif_name);

    /**
     *  Enable/disable the MLD6IGMP trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError mld6igmp_0_1_log_trace_all(
	// Input values,
	const bool&	enable);

    /**
     *  Add/delete a client protocol in the MLD/IGMP protocol.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol to add/delete.
     *  
     *  @param protocol_id the ID of the protocol to add/delete (both sides
     *  must agree on the particular values).
     *  
     *  @param vif_name the name of the vif the protocol add/delete to apply
     *  to.
     *  
     *  @param vif_index the index of the vif the protocol add/delete to apply
     *  to. The added protocol will receive Join/Leave membership information
     *  about same-LAN members for the particular vif.
     */
    XrlCmdError mld6igmp_0_1_add_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_add_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_delete_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mld6igmp_0_1_delete_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);
    
private:
    class XrlTaskBase;

    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

    /**
     * Called when Finder connection is established.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_disconnect_event();

    //
    // Methods to handle the XRL tasks
    //
    void add_task(XrlTaskBase* xrl_task);
    void send_xrl_task();
    void pop_xrl_task();
    void retry_xrl_task();

    void fea_register_startup();
    void mfea_register_startup();
    void fea_register_shutdown();
    void mfea_register_shutdown();
    void finder_send_register_unregister_interest_cb(const XrlError& xrl_error);

    //
    // Protocol node methods
    //
    int register_receiver(const string& if_name, const string& vif_name,
			  uint8_t ip_protocol,
			  bool enable_multicast_loopback);
    int unregister_receiver(const string& if_name, const string& vif_name,
			    uint8_t ip_protocol);
    void fea_client_send_register_unregister_receiver_cb(const XrlError& xrl_error);

    int join_multicast_group(const string& if_name, const string& vif_name,
			     uint8_t ip_protocol, const IPvX& group_address);
    int leave_multicast_group(const string& if_name, const string& vif_name,
			      uint8_t ip_protocol, const IPvX& group_address);
    void fea_client_send_join_leave_multicast_group_cb(const XrlError& xrl_error);

    int	proto_send(const string& if_name,
		   const string& vif_name,
		   const IPvX& src_address,
		   const IPvX& dst_address,
		   uint8_t ip_protocol,
		   int32_t ip_ttl,
		   int32_t ip_tos,
		   bool ip_router_alert,
		   bool ip_internet_control,
		   const uint8_t* sndbuf,
		   size_t sndlen,
		   string& error_msg);
    void fea_client_send_protocol_message_cb(const XrlError& xrl_error);
    
    int send_add_membership(const string& dst_module_instance_name,
			    xorp_module_id dst_module_id,
			    uint32_t vif_index,
			    const IPvX& source,
			    const IPvX& group);
    int send_delete_membership(const string& dst_module_instance_name,
			       xorp_module_id dst_module_id,
			       uint32_t vif_index,
			       const IPvX& source,
			       const IPvX& group);
    void send_add_delete_membership();
    void mld6igmp_client_send_add_delete_membership_cb(const XrlError& xrl_error);
    
    //
    // Protocol node CLI methods
    //
    int add_cli_command_to_cli_manager(const char *command_name,
				       const char *command_help,
				       bool is_command_cd,
				       const char *command_cd_prompt,
				       bool is_command_processor);
    void cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error);
    int delete_cli_command_from_cli_manager(const char *command_name);
    void cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error);

    int family() const { return (Mld6igmpNode::family()); }


    /**
     * A base class for handling tasks for sending XRL requests.
     */
    class XrlTaskBase {
    public:
	XrlTaskBase(XrlMld6igmpNode& xrl_mld6igmp_node)
	    : _xrl_mld6igmp_node(xrl_mld6igmp_node) {}
	virtual ~XrlTaskBase() {}

	virtual void dispatch() = 0;
	virtual const char* operation_name() const = 0;

    protected:
	XrlMld6igmpNode&	_xrl_mld6igmp_node;
    private:
    };

    /**
     * Class for handling the task to register/unregister interest
     * in the FEA or MFEA with the Finder.
     */
    class RegisterUnregisterInterest : public XrlTaskBase {
    public:
	RegisterUnregisterInterest(XrlMld6igmpNode&	xrl_mld6igmp_node,
				   const string&	target_name,
				   bool			is_register)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _target_name(target_name),
	      _is_register(is_register) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_register_unregister_interest();
	}
	const char*	operation_name() const {
	    return ((_is_register)? "register" : "unregister");
	}
	const string&	target_name() const { return _target_name; }
	bool		is_register() const { return _is_register; }

    private:
	string		_target_name;
	bool		_is_register;
    };

    /**
     * Class for handling the task to register/unregister with the FEA
     * as a receiver on an interface.
     */
    class RegisterUnregisterReceiver : public XrlTaskBase {
    public:
	RegisterUnregisterReceiver(XrlMld6igmpNode&	xrl_mld6igmp_node,
				   const string&	if_name,
				   const string&	vif_name,
				   uint8_t		ip_protocol,
				   bool			enable_multicast_loopback,
				   bool			is_register)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ip_protocol(ip_protocol),
	      _enable_multicast_loopback(enable_multicast_loopback),
	      _is_register(is_register) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_register_unregister_receiver();
	}
	const char*	operation_name() const {
	    return ((_is_register)? "register" : "unregister");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	bool		enable_multicast_loopback() const {
	    return _enable_multicast_loopback;
	}
	bool		is_register() const { return _is_register; }

    private:
	string		_if_name;
	string		_vif_name;
	uint8_t		_ip_protocol;
	bool		_enable_multicast_loopback;
	bool		_is_register;
    };

    /**
     * Class for handling the task of join/leave multicast group requests
     */
    class JoinLeaveMulticastGroup : public XrlTaskBase {
    public:
	JoinLeaveMulticastGroup(XrlMld6igmpNode&	xrl_mld6igmp_node,
				const string&		if_name,
				const string&		vif_name,
				uint8_t			ip_protocol,
				const IPvX&		group_address,
				bool			is_join)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _ip_protocol(ip_protocol),
	      _group_address(group_address),
	      _is_join(is_join) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_join_leave_multicast_group();
	}
	const char*	operation_name() const {
	    return ((_is_join)? "join" : "leave");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	const IPvX&	group_address() const { return _group_address; }
	bool		is_join() const { return _is_join; }

    private:
	string		_if_name;
	string		_vif_name;
	uint8_t		_ip_protocol;
	IPvX		_group_address;
	bool		_is_join;
    };

    /**
     * Class for handling the task of sending protocol messages
     */
    class SendProtocolMessage : public XrlTaskBase {
    public:
	SendProtocolMessage(XrlMld6igmpNode&	xrl_mld6igmp_node,
			    const string&	if_name,
			    const string&	vif_name,
			    const IPvX&		src_address,
			    const IPvX&		dst_address,
			    uint8_t		ip_protocol,
			    int32_t		ip_ttl,
			    int32_t		ip_tos,
			    bool		ip_router_alert,
			    bool		ip_internet_control,
			    const uint8_t*	sndbuf,
			    size_t		sndlen)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _if_name(if_name),
	      _vif_name(vif_name),
	      _src_address(src_address),
	      _dst_address(dst_address),
	      _ip_protocol(ip_protocol),
	      _ip_ttl(ip_ttl),
	      _ip_tos(ip_tos),
	      _ip_router_alert(ip_router_alert),
	      _ip_internet_control(ip_internet_control) {
	    _payload.resize(sndlen);
	    for (size_t i = 0; i < sndlen; i++)
		_payload[i] = sndbuf[i];
	}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_protocol_message();
	}
	const char*	operation_name() const {
	    return ("send");
	}
	const string&	if_name() const { return _if_name; }
	const string&	vif_name() const { return _vif_name; }
	const IPvX&	src_address() const { return _src_address; }
	const IPvX&	dst_address() const { return _dst_address; }
	uint8_t		ip_protocol() const { return _ip_protocol; }
	int32_t		ip_ttl() const { return _ip_ttl; }
	int32_t		ip_tos() const { return _ip_tos; }
	bool		ip_router_alert() const { return _ip_router_alert; }
	bool		ip_internet_control() const { return _ip_internet_control; }
	const vector<uint8_t>& payload() const { return _payload; }

    private:
	string		_if_name;
	string		_vif_name;
	IPvX		_src_address;
	IPvX		_dst_address;
	uint8_t		_ip_protocol;
	int32_t		_ip_ttl;
	int32_t		_ip_tos;
	bool		_ip_router_alert;
	bool		_ip_internet_control;
	vector<uint8_t>	_payload;
    };

    /**
     * Class for handling the queue of sending Add/Delete membership requests
     */
    class SendAddDeleteMembership {
    public:
	SendAddDeleteMembership(const string& dst_module_instance_name,
				xorp_module_id dst_module_id,
				uint32_t vif_index,
				const IPvX& source,
				const IPvX& group,
				bool is_add)
	    : _dst_module_instance_name(dst_module_instance_name),
	      _dst_module_id(dst_module_id),
	      _vif_index(vif_index),
	      _source(source),
	      _group(group),
	      _is_add(is_add) {}
	const char*	operation_name() const {
	    return ((_is_add)? "add membership" : "delete membership");
	}
	const string& dst_module_instance_name() const { return _dst_module_instance_name; }
	xorp_module_id dst_module_id() const { return _dst_module_id; }
	uint32_t vif_index() const { return _vif_index; }
	const IPvX& source() const { return _source; }
	const IPvX& group() const { return _group; }
	bool is_add() const { return _is_add; }

    private:
	string		_dst_module_instance_name;
	xorp_module_id	_dst_module_id;
	uint32_t	_vif_index;
	IPvX		_source;
	IPvX		_group;
	bool		_is_add;
    };

    EventLoop&			_eventloop;
    const string		_finder_target;
    const string		_fea_target;
    const string		_mfea_target;

    IfMgrXrlMirror		_ifmgr;

    XrlRawPacket4V0p1Client	_xrl_fea_client4;
    XrlRawPacket6V0p1Client	_xrl_fea_client6;
    XrlMld6igmpClientV0p1Client	_xrl_mld6igmp_client_client;
    XrlCliManagerV0p1Client	_xrl_cli_manager_client;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    static const TimeVal	RETRY_TIMEVAL;

    bool			_is_finder_alive;

    bool			_is_fea_alive;
    bool			_is_fea_registered;

    bool			_is_mfea_alive;
    bool			_is_mfea_registered;

    list<XrlTaskBase* >		_xrl_tasks_queue;
    XorpTimer			_xrl_tasks_queue_timer;
    list<SendAddDeleteMembership> _send_add_delete_membership_queue;
    XorpTimer			_send_add_delete_membership_queue_timer;
};

#endif // __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
