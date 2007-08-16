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

// $XORP: xorp/fea/xrl_mfea_node.hh,v 1.35 2007/05/19 01:52:42 pavlin Exp $

#ifndef __FEA_XRL_MFEA_NODE_HH__
#define __FEA_XRL_MFEA_NODE_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/mfea_client_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "xrl/targets/mfea_base.hh"

#include "libfeaclient_bridge.hh"
#include "mfea_node.hh"
#include "mfea_node_cli.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlMfeaNode : public MfeaNode,
		    public XrlStdRouter,
		    public XrlMfeaTargetBase,
		    public MfeaNodeCli {
public:
    XrlMfeaNode(FeaNode&	fea_node,
		int		family,
		xorp_module_id	module_id,
		EventLoop&	eventloop,
		const string&	class_name,
		const string&	finder_hostname,
		uint16_t	finder_port,
		const string&	finder_target);
    virtual ~XrlMfeaNode();

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
    // XrlMfeaNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    int enable_mfea();
    int disable_mfea();
    int start_mfea();
    int stop_mfea();
    
    
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
    XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    /**
     *  Shutdown cleanly
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
	const uint32_t& cli_session_id,
	const string&	command_name, 
	const string&	command_args, 
	// Output values, 
	string&	ret_processor_name, 
	string&	ret_cli_term_name, 
	uint32_t& ret_cli_session_id,
	string&	ret_command_output);

    /**
     *  Register remote mirror of interface state.
     */
    XrlCmdError ifmgr_replicator_0_1_register_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    /**
     *  Register remote mirror of interface state.
     */
    XrlCmdError ifmgr_replicator_0_1_unregister_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    /**
     *  Test if the underlying system supports IPv4 multicast routing.
     *  
     *  @param result true if the underlying system supports IPv4 multicast
     *  routing, otherwise false.
     */
    XrlCmdError mfea_0_1_have_multicast_routing4(
	// Output values, 
	bool&	result);

    /**
     *  Test if the underlying system supports IPv6 multicast routing.
     *  
     *  @param result true if the underlying system supports IPv6 multicast
     *  routing, otherwise false.
     */
    XrlCmdError mfea_0_1_have_multicast_routing6(
	// Output values, 
	bool&	result);

    /**
     *  Register a protocol on an interface in the Multicast FEA. There could
     *  be only one registered protocol per interface/vif.
     *
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *
     *  @param if_name the name of the interface to register for the particular
     *  protocol.
     *
     *  @param vif_name the name of the vif to register for the particular
     *  protocol.
     *
     *  @param ip_protocol the IP protocol number. It must be between 1 and
     *  255.
     */
    XrlCmdError mfea_0_1_register_protocol4(
	// Input values,
	const string&	xrl_sender_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    XrlCmdError mfea_0_1_register_protocol6(
	// Input values,
	const string&	xrl_sender_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    /**
     *  Unregister a protocol on an interface in the Multicast FEA. There could
     *  be only one registered protocol per interface/vif.
     *
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *
     *  @param if_name the name of the interface to unregister for the
     *  particular protocol.
     *
     *  @param vif_name the name of the vif to unregister for the particular
     *  protocol.
     */
    XrlCmdError mfea_0_1_unregister_protocol4(
	// Input values,
	const string&	xrl_sender_name,
	const string&	if_name,
	const string&	vif_name);

    XrlCmdError mfea_0_1_unregister_protocol6(
	// Input values,
	const string&	xrl_sender_name,
	const string&	if_name,
	const string&	vif_name);

    /**
     *  Add/delete a Multicast Forwarding Cache with the kernel.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param source_address the source address of the MFC to add/delete.
     *  
     *  @param group_address the group address of the MFC to add/delete.
     *  
     *  @param iif_vif_index the index of the vif that is the incoming
     *  interface.
     *  
     *  @param oiflist the bit-vector with the set of outgoing interfaces.
     *
     *  @param oiflist_disable_wrongvif the bit-vector with the set of outgoing
     *  interfaces to disable WRONGVIF kernel signal.
     *  
     *  @param max_vifs_oiflist the number of vifs covered by oiflist or
     *  oiflist_disable_wrongvif .
     *  
     *  @param rp_address the RP address of the MFC to add.
     */
    XrlCmdError mfea_0_1_add_mfc4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	iif_vif_index, 
	const vector<uint8_t>&	oiflist, 
	const vector<uint8_t>&	oiflist_disable_wrongvif, 
	const uint32_t&	max_vifs_oiflist, 
	const IPv4&	rp_address);

    XrlCmdError mfea_0_1_add_mfc6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	iif_vif_index, 
	const vector<uint8_t>&	oiflist, 
	const vector<uint8_t>&	oiflist_disable_wrongvif, 
	const uint32_t&	max_vifs_oiflist, 
	const IPv6&	rp_address);

    XrlCmdError mfea_0_1_delete_mfc4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_delete_mfc6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address);

    /**
     *  Add/delete a dataflow monitor with the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param source_address the source address of the dataflow to start/stop
     *  monitoring.
     *  
     *  @param group_address the group address of the dataflow to start/stop
     *  monitoring.
     *  
     *  @param threshold_interval_sec the number of seconds in the interval to
     *  measure.
     *  
     *  @param threshold_interval_usec the number of microseconds in the
     *  interval to measure.
     *  
     *  @param threshold_packets the threshold (in number of packets) to
     *  compare against.
     *  
     *  @param threshold_bytes the threshold (in number of bytes) to compare
     *  against.
     *  
     *  @param is_threshold_in_packets if true, threshold_packets is valid.
     *  
     *  @param is_threshold_in_bytes if true, threshold_bytes is valid.
     *  
     *  @param is_geq_upcall if true, the operation for comparison is ">=".
     *  
     *  @param is_leq_upcall if true, the operation for comparison is "<=".
     */
    XrlCmdError mfea_0_1_add_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_add_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_all_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_delete_all_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address);

    /**
     *  Enable/disable/start/stop a MFEA vif interface.
     *
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     *
     *  @param enable if true, then enable the vif, otherwise disable it.
     */
    XrlCmdError mfea_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mfea_0_1_stop_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all MFEA vif interfaces.
     *
     *  @param enable if true, then enable the vifs, otherwise disable them.
     */
    XrlCmdError mfea_0_1_enable_all_vifs(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_all_vifs();

    XrlCmdError mfea_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop the MFEA.
     *
     *  @param enable if true, then enable the MFEA, otherwise disable it.
     */
    XrlCmdError mfea_0_1_enable_mfea(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_mfea();

    XrlCmdError mfea_0_1_stop_mfea();

    /**
     *  Enable/disable/start/stop the MFEA CLI access.
     *
     *  @param enable if true, then enable the MFEA CLI access, otherwise
     *  disable it.
     */
    XrlCmdError mfea_0_1_enable_cli(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_cli();

    XrlCmdError mfea_0_1_stop_cli();

    /**
     *  Enable/disable the MFEA trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError mfea_0_1_log_trace_all(
	// Input values,
	const bool&	enable);

private:
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
    // Protocol node methods
    //
    int signal_message_send(const string& dst_module_instance_name,
			    int message_type,
			    uint32_t vif_index,
			    const IPvX& src, const IPvX& dst,
			    const uint8_t *rcvbuf, size_t rcvlen);
    //
    // XXX: mfea_client_client_send_recv_kernel_signal_message_cb() in fact
    // is the callback when signal_message_send() calls
    // send_recv_kernel_signal_message()
    // so the destination protocol will receive a kernel message.
    // Sigh, the 'recv' part in the name is rather confusing, but that
    // is in the name of consistency between the XRL calling function
    // and the return result callback...
    //
    void mfea_client_client_send_recv_kernel_signal_message_cb(const XrlError& xrl_error);

    int dataflow_signal_send(const string& dst_module_instance_name,
			     const IPvX& source_addr,
			     const IPvX& group_addr,
			     uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t measured_interval_sec,
			     uint32_t measured_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     uint32_t measured_packets,
			     uint32_t measured_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall);
    
    void mfea_client_client_send_recv_dataflow_signal_cb(const XrlError& xrl_error);

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
    
    int family() const { return (MfeaNode::family()); }

    EventLoop&			_eventloop;
    const string		_finder_target;

    XrlMfeaClientV0p1Client	_xrl_mfea_client_client;
    XrlCliManagerV0p1Client	_xrl_cli_manager_client;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    LibFeaClientBridge		_lib_mfea_client_bridge;    // The MFEA clients

    bool			_is_finder_alive;
};

#endif // __FEA_XRL_MFEA_NODE_HH__
