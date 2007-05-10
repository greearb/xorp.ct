// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/mld6igmp/xrl_mld6igmp_node.hh,v 1.41 2007/05/08 19:23:17 pavlin Exp $

#ifndef __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
#define __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__


//
// MLD6IGMP XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/mfea_xif.hh"
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
		    const string&	mfea_target);
    virtual ~XrlMld6igmpNode();

    /**
     * Startup the node operation.
     *
     * @return true on success, false on failure.
     */
    bool	startup();

    /**
     * Shutdown the node operation.
     *
     * @return true on success, false on failure.
     */
    bool	shutdown();

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

    //
    // Methods used by the classed derived from XrlTaskBase, that need to
    // be public.
    //
    void send_mfea_add_delete_protocol();
    void send_start_stop_protocol_kernel_vif();
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
     *  Receive a protocol message from the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that sends a message.
     *  
     *  @param protocol_id the ID of the protocol that sends a message (both
     *  sides must agree on the particular values).
     *  
     *  @param vif_name the name of the vif the message was received on.
     *  
     *  @param vif_index the index of the vif the message was received on.
     *  
     *  @param source_address the address of the sender.
     *  
     *  @param dest_address the destination address.
     *  
     *  @param ip_ttl the TTL of the received IP packet. If it has a negative
     *  value, it should be ignored.
     *  
     *  @param ip_tos the TOS of the received IP packet. If it has a negative
     *  value, it should be ignored.
     *  
     *  @param is_router_alert if true, the IP Router Alert option in
     *  the IP packet was set (when applicable).
     *  
     *  @param ip_internet_control if true, then this is IP control traffic.
     *  
     *  @param protocol_message the protocol message.
     */
    XrlCmdError mfea_client_0_1_recv_protocol_message4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source_address, 
	const IPv4&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const bool&	ip_internet_control,
	const vector<uint8_t>&	protocol_message);

    XrlCmdError mfea_client_0_1_recv_protocol_message6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source_address, 
	const IPv6&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const bool&	ip_internet_control,
	const vector<uint8_t>&	protocol_message);
    
    /**
     *  Receive a kernel signal message from the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that sends a message.
     *  
     *  @param protocol_id the ID of the protocol that sends a message (both
     *  sides must agree on the particular values).
     *  
     *  @param message_type the type of the kernel signal message (TODO:
     *  integer for now: the particular types are well-known by both sides).
     *  
     *  @param vif_name the name of the vif the message was received on.
     *  
     *  @param vif_index the index of the vif the message was received on.
     *  
     *  @param source_address the address of the sender.
     *  
     *  @param dest_address the destination address.
     *  
     *  @param protocol_message the protocol message.
     */
    XrlCmdError mfea_client_0_1_recv_kernel_signal_message4(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const string&		, // protocol_name, 
	const uint32_t&		, // protocol_id, 
	const uint32_t&		, // message_type, 
	const string&		, // vif_name, 
	const uint32_t&		, // vif_index, 
	const IPv4&		, // source_address, 
	const IPv4&		, // dest_address, 
	const vector<uint8_t>&	  // protocol_message
	) {
	string msg  = "Unexpected kernel signal message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    XrlCmdError mfea_client_0_1_recv_kernel_signal_message6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const string&		, // protocol_name, 
	const uint32_t&		, // protocol_id, 
	const uint32_t&		, // message_type, 
	const string&		, // vif_name, 
	const uint32_t&		, // vif_index, 
	const IPv6&		, // source_address, 
	const IPv6&		, // dest_address, 
	const vector<uint8_t>&	  // protocol_message
	) {
	string msg = "Unexpected kernel signal message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }
    
    //
    // A signal that a dataflow-related pre-condition is true:
    // not used by MLD or IGMP.
    //
    XrlCmdError mfea_client_0_1_recv_dataflow_signal4(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv4&		, // source_address, 
	const IPv4&		, // group_address, 
	const uint32_t&		, // threshold_interval_sec, 
	const uint32_t&		, // threshold_interval_usec, 
	const uint32_t&		, // measured_interval_sec, 
	const uint32_t&		, // measured_interval_usec, 
	const uint32_t&		, // threshold_packets, 
	const uint32_t&		, // threshold_bytes, 
	const uint32_t&		, // measured_packets, 
	const uint32_t&		, // measured_bytes, 
	const bool&		, // is_threshold_in_packets,
	const bool&		, // is_threshold_in_bytes,
	const bool&		, // is_geq_upcall,
	const bool&		  // is_leq_upcall
	) {
	string msg = "Unexpected recv_dataflow_signal4() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

    XrlCmdError mfea_client_0_1_recv_dataflow_signal6(
	// Input values, 
	const string&		, // xrl_sender_name, 
	const IPv6&		, // source_address, 
	const IPv6&		, // group_address, 
	const uint32_t&		, // threshold_interval_sec, 
	const uint32_t&		, // threshold_interval_usec, 
	const uint32_t&		, // measured_interval_sec, 
	const uint32_t&		, // measured_interval_usec, 
	const uint32_t&		, // threshold_packets, 
	const uint32_t&		, // threshold_bytes, 
	const uint32_t&		, // measured_packets, 
	const uint32_t&		, // measured_bytes, 
	const bool&		, // is_threshold_in_packets,
	const bool&		, // is_threshold_in_bytes,
	const bool&		, // is_geq_upcall,
	const bool&		  // is_leq_upcall
	) {
	string msg = "Unexpected recv_dataflow_signal4() message";
	return XrlCmdError::COMMAND_FAILED(msg);
    }

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

    void mfea_register_startup();
    void finder_register_interest_mfea_cb(const XrlError& xrl_error);
    void mfea_register_shutdown();
    void finder_deregister_interest_mfea_cb(const XrlError& xrl_error);

    void mfea_client_send_add_delete_protocol_cb(const XrlError& xrl_error);

    //
    // Protocol node methods
    //
    int start_protocol_kernel_vif(uint32_t vif_index);
    int stop_protocol_kernel_vif(uint32_t vif_index);
    void mfea_client_send_start_stop_protocol_kernel_vif_cb(const XrlError& xrl_error);

    int join_multicast_group(uint32_t vif_index, const IPvX& multicast_group);
    int leave_multicast_group(uint32_t vif_index, const IPvX& multicast_group);
    void mfea_client_send_join_leave_multicast_group_cb(const XrlError& xrl_error);

    int	proto_send(const string& dst_module_instance_name,
		   xorp_module_id dst_module_id,
		   uint32_t vif_index,
		   const IPvX& src, const IPvX& dst,
		   int ip_ttl, int ip_tos, bool is_router_alert,
		   bool ip_internet_control,
		   const uint8_t* sndbuf, size_t sndlen, string& error_msg);
    void mfea_client_send_protocol_message_cb(const XrlError& xrl_error);
    
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

    const string& my_xrl_target_name() {
	return XrlMld6igmpTargetBase::name();
    }
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

    protected:
	XrlMld6igmpNode&	_xrl_mld6igmp_node;
    private:
    };

    /**
     * Class for handling the task of start/stop a protocol interface with
     * the MFEA
     */
    class StartStopProtocolKernelVif : public XrlTaskBase {
    public:
	StartStopProtocolKernelVif(XrlMld6igmpNode&	xrl_mld6igmp_node,
				   uint32_t		vif_index,
				   bool			is_start)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _vif_index(vif_index),
	      _is_start(is_start) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_start_stop_protocol_kernel_vif();
	}
	uint32_t	vif_index() const { return _vif_index; }
	bool		is_start() const { return _is_start; }

    private:
	uint32_t	_vif_index;
	bool		_is_start;
    };

    /**
     * Class for handling the task of adding/deleting a protocol with the MFEA
     */
    class MfeaAddDeleteProtocol : public XrlTaskBase {
    public:
	MfeaAddDeleteProtocol(XrlMld6igmpNode&	xrl_mld6igmp_node,
			      bool		is_add)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _is_add(is_add) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_mfea_add_delete_protocol();
	}
	bool		is_add() const { return _is_add; }

    private:
	bool		_is_add;
    };

    /**
     * Class for handling the task of join/leave multicast group requests
     */
    class JoinLeaveMulticastGroup : public XrlTaskBase {
    public:
	JoinLeaveMulticastGroup(XrlMld6igmpNode&	xrl_mld6igmp_node,
				uint32_t		vif_index,
				const IPvX&		multicast_group,
				bool			is_join)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _vif_index(vif_index),
	      _multicast_group(multicast_group),
	      _is_join(is_join) {}

	void		dispatch() {
	    _xrl_mld6igmp_node.send_join_leave_multicast_group();
	}
	uint32_t	vif_index() const { return _vif_index; }
	const IPvX&	multicast_group() const { return _multicast_group; }
	bool		is_join() const { return _is_join; }

    private:
	uint32_t	_vif_index;
	IPvX		_multicast_group;
	bool		_is_join;
    };

    /**
     * Class for handling the task of sending protocol messages
     */
    class SendProtocolMessage : public XrlTaskBase {
    public:
	SendProtocolMessage(XrlMld6igmpNode&	xrl_mld6igmp_node,
			    const string&	dst_module_instance_name,
			    xorp_module_id	dst_module_id,
			    uint32_t		vif_index,
			    const IPvX&		src,
			    const IPvX&		dst,
			    int			ip_ttl,
			    int			ip_tos,
			    bool		is_router_alert,
			    bool		ip_internet_control,
			    const uint8_t*	sndbuf,
			    size_t		sndlen)
	    : XrlTaskBase(xrl_mld6igmp_node),
	      _dst_module_instance_name(dst_module_instance_name),
	      _dst_module_id(dst_module_id),
	      _vif_index(vif_index),
	      _src(src),
	      _dst(dst),
	      _ip_ttl(ip_ttl),
	      _ip_tos(ip_tos),
	      _is_router_alert(is_router_alert),
	      _ip_internet_control(ip_internet_control) {
		  _message.resize(sndlen);
		  for (size_t i = 0; i < sndlen; i++)
		      _message[i] = sndbuf[i];
	      }

	void		dispatch() {
	    _xrl_mld6igmp_node.send_protocol_message();
	}
	const string&	dst_module_instance_name() const { return _dst_module_instance_name; }
	xorp_module_id	dst_module_id() const { return _dst_module_id; }
	uint32_t	vif_index() const { return _vif_index; }
	const IPvX&	src() const { return _src; }
	const IPvX&	dst() const { return _dst; }
	int		ip_ttl() const { return _ip_ttl; }
	int		ip_tos() const { return _ip_tos; }
	bool		is_router_alert() const { return _is_router_alert; }
	bool		ip_internet_control() const { return _ip_internet_control; }
	const vector<uint8_t>& message() const { return _message; }

    private:
	string		_dst_module_instance_name;
	xorp_module_id	_dst_module_id;
	uint32_t	_vif_index;
	IPvX		_src;
	IPvX		_dst;
	int		_ip_ttl;
	int		_ip_tos;
	bool		_is_router_alert;
	bool		_ip_internet_control;
	vector<uint8_t>	_message;
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
    const string		_class_name;
    const string		_instance_name;
    const string		_finder_target;
    const string		_mfea_target;

    IfMgrXrlMirror		_ifmgr;

    XrlMfeaV0p1Client		_xrl_mfea_client;
    XrlMld6igmpClientV0p1Client	_xrl_mld6igmp_client_client;
    XrlCliManagerV0p1Client	_xrl_cli_manager_client;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    static const TimeVal	RETRY_TIMEVAL;

    bool			_is_finder_alive;

    bool			_is_mfea_alive;
    bool			_is_mfea_registered;
    bool			_is_mfea_registering;
    bool			_is_mfea_deregistering;
    XorpTimer			_mfea_register_startup_timer;
    XorpTimer			_mfea_register_shutdown_timer;
    bool			_is_mfea_add_protocol_registered;
    XorpTimer			_mfea_add_protocol_timer;

    list<XrlTaskBase* >		_xrl_tasks_queue;
    XorpTimer			_xrl_tasks_queue_timer;
    list<SendAddDeleteMembership> _send_add_delete_membership_queue;
    XorpTimer			_send_add_delete_membership_queue_timer;
};

#endif // __MLD6IGMP_XRL_MLD6IGMP_NODE_HH__
