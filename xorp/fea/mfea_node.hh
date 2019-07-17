// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __FEA_MFEA_NODE_HH__
#define __FEA_MFEA_NODE_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) node definition.
//

#include "libxorp/ipvx.hh"
#include "libxorp/config_param.hh"
#include "libproto/proto_node.hh"
#include "mrt/mifset.hh"
#include "ifconfig_reporter.hh"
#include "iftree.hh"
#include "mfea_dataflow.hh"
#include "mfea_mrouter.hh"


//
// Structures/classes, typedefs and macros
//

class EventLoop;
class FeaNode;
class MfeaVif;
class SgCount;
class VifCount;


class MfeaRouteStorage {
public:
    uint32_t distance; // lower is higer priority.
    bool is_binary;

    string module_instance_name;
    IPvX source;
    IPvX group;

    // String based route
    string iif_name;
    string oif_names;

    // Binary based route
    uint32_t iif_vif_index;
    Mifset oiflist;
    Mifset oiflist_disable_wrongvif;
    uint32_t max_vifs_oiflist;
    IPvX rp_addr;

    MfeaRouteStorage(uint32_t d, const string module_name, const IPvX& _source,
		     const IPvX& _group, const string& _iif_name, const string& _oif_names);

    MfeaRouteStorage(uint32_t d, const string module_name, const IPvX& _source,
		     const IPvX& _group, uint32_t iif_vif_idx,
		     const Mifset& _oiflist, const Mifset& _oif_disable_wrongvif,
		     uint32_t max_vifs, const IPvX& _rp_addr);

    MfeaRouteStorage(const MfeaRouteStorage& o);
    MfeaRouteStorage();

    MfeaRouteStorage& operator=(const MfeaRouteStorage& o);

    bool isBinary() const { return is_binary; }

    string getHash() { return source.str() + ":" + group.str(); }
};


/**
 * @short The MFEA node class.
 * 
 * There should be one node per MFEA instance. There should be
 * one instance per address family.
 */
class MfeaNode : public ProtoNode<MfeaVif>,
		 public IfConfigUpdateReporterBase,
		 public ServiceChangeObserverBase {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param fea_node the corresponding FeaNode (@ref FeaNode).
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param module_id the module ID (@ref xorp_module_id). Should be
     * equal to XORP_MODULE_MFEA.
     * @param eventloop the event loop to use.
     */
    MfeaNode(FeaNode& fea_node, int family, xorp_module_id module_id,
	     EventLoop& eventloop);
    
    /**
     * Destructor
     */
    virtual	~MfeaNode();

    /**
     * Get the FEA node instance.
     *
     * @return reference to the FEA node instance.
     */
    FeaNode&	fea_node() { return (_fea_node); }

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool	is_dummy() const;

    /**
     * Start the node operation.
     * 
     * After the startup operations are completed,
     * @ref MfeaNode::final_start() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();

    /**
     * Stop the node operation.
     * 
     * After the shutdown operations are completed,
     * @ref MfeaNode::final_stop() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();

    /**
     * Completely start the node operation.
     * 
     * This method should be called internally after @ref MfeaNode::start()
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_start();

    /**
     * Completely stop the node operation.
     * 
     * This method should be called internally after @ref MfeaNode::stop()
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_stop();

    /**
     * Enable node operation.
     * 
     * If an unit is not enabled, it cannot be start, or pending-start.
     */
    void	enable();
    
    /**
     * Disable node operation.
     * 
     * If an unit is disabled, it cannot be start or pending-start.
     * If the unit was runnning, it will be stop first.
     */
    void	disable();

    /**
     * Test if the underlying system supports IPv4 multicast routing.
     * 
     * @return true if the underlying system supports IPv4 multicast routing,
     * otherwise false.
     */
    bool have_multicast_routing4() const {
	return (_mfea_mrouter.have_multicast_routing4());
    }
    
#ifdef HAVE_IPV6
    /**
     * Test if the underlying system supports IPv6 multicast routing.
     * 
     * @return true if the underlying system supports IPv6 multicast routing,
     * otherwise false.
     */
    bool have_multicast_routing6() const {
	return (_mfea_mrouter.have_multicast_routing6());
    }
#endif
    
    /**
     * Install a new MFEA vif.
     * 
     * @param vif vif information about the new MfeaVif to install.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const Vif& vif, string& error_msg);
    
    /**
     *  Delete an existing MFEA vif.
     * 
     * @param vif_name the name of the vif to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif(const string& vif_name, string& error_msg);
    
    /**
     * Add a configured vif.
     *
     * @param vif the vif with the information to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_config_vif(const Vif& vif, string& error_msg);

    /**
     * Set the vif flags of a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @param mtu the MTU of the vif.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_vif_flags(const string& vif_name,
				     bool is_pim_register,
				     bool is_p2p,
				     bool is_loopback,
				     bool is_multicast,
				     bool is_broadcast,
				     bool is_up,
				     uint32_t mtu,
				     string& error_msg);
    
    /**
     * Complete the set of vif configuration changes.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_all_vifs_done(string& error_msg);
    
    /**
     * Enable an existing MFEA vif.
     * 
     * @param vif_name the name of the vif to enable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_vif(const string& vif_name, string& error_msg);

    /**
     * Disable an existing MFEA vif.
     * 
     * @param vif_name the name of the vif to disable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_vif(const string& vif_name, string& error_msg);

    /**
     * Start an existing MFEA vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_vif(const string& vif_name, string& error_msg);
    
    /**
     * Stop an existing MFEA vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_vif(const string& vif_name, string& error_msg);
    
    /**
     * Start MFEA on all enabled interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_all_vifs();
    
    /**
     * Stop MFEA on all interfaces it was running on.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_all_vifs();
    
    /**
     * Enable MFEA on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_all_vifs();
    
    /**
     * Disable MFEA on all interfaces.
     * 
     * All running interfaces are stopped first.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_all_vifs();
    
    /**
     * Delete all MFEA vifs.
     */
    void	delete_all_vifs();

    /**
     * A method called when a vif has completed its shutdown.
     * 
     * @param vif_name the name of the vif that has completed its shutdown.
     */
    void	vif_shutdown_completed(const string& vif_name);

    /**
     * Register a protocol on an interface in the Multicast FEA.
     *
     * There could be only one registered protocol per interface/vif.
     *
     * @param module_instance_name the module instance name of the protocol
     * to register.
     * @param if_name the name of the interface to register for the particular
     * protocol.
     * @param vif_name the name of the vif to register for the particular
     * protocol.
     * @param ip_protocol the IP protocol number. It must be between 1 and
     * 255.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_protocol(const string&		module_instance_name,
			  const string&		if_name,
			  const string&		vif_name,
			  uint8_t		ip_protocol,
			  string&		error_msg);


    /** Helper method to unregister all protocols on all vifs for this
     * interface.  This needs to be called before we delete an interface
     * to ensure the cleanup can happen properly.
     */
    void unregister_protocols_for_iface(const string& ifname);

    /** Helper method to unregister all protocols on this vif.
     * This needs to be called before we delete an interface
     * to ensure the cleanup can happen properly.
     */
    void unregister_protocols_for_vif(const string& ifname, const string& vifname);

    /**
     * Unregister a protocol on an interface in the Multicast FEA.
     *
     * There could be only one registered protocol per interface/vif.
     *
     * @param module_instance_name the module instance name of the protocol
     * to unregister.
     * @param if_name the name of the interface to unregister for the
     * particular protocol.
     * @param vif_name the name of the vif to unregister for the particular
     * protocol.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_protocol(const string&	module_instance_name,
			    const string&	if_name,
			    const string&	vif_name,
			    string&		error_msg);

    /**
     * UNUSED
     */
    int proto_recv(const string&	, // if_name,
		   const string&	, // vif_name,
		   const IPvX&		, // src_address,
		   const IPvX&		, // dst_address,
		   uint8_t		, // ip_protocol,
		   int32_t		, // ip_ttl,
		   int32_t		, // ip_tos,
		   bool			, // ip_router_alert,
		   bool			, // ip_internet_control,
		   const vector<uint8_t>& , // payload,
		   string&		// error_msg
	) { assert (false); return (XORP_ERROR); }

    /**
     * UNUSED
     */
    int	proto_send(const string&	, // if_name,
		   const string&	, // vif_name,
		   const IPvX&		, // src_address,
		   const IPvX&		, // dst_address,
		   uint8_t		, // ip_protocol,
		   int32_t		, // ip_ttl,
		   int32_t		, // ip_tos,
		   bool			, // ip_router_alert,
		   bool			, // ip_internet_control,
		   const uint8_t*	, // sndbuf,
		   size_t		, // sndlen,
		   string&		  // error_msg
	) { assert (false); return (XORP_ERROR); }

    /**
     * Process NOCACHE, WRONGVIF/WRONGMIF, WHOLEPKT signals from the
     * kernel.
     * 
     * The signal is sent to all user-level protocols that expect it.
     * 
     * @param src_module_instance_name unused.
     * @param message_type the message type of the kernel signal (for IPv4
     * and IPv6 respectively):
     * 
<pre>
#define IGMPMSG_NOCACHE         1
#define IGMPMSG_WRONGVIF        2
#define IGMPMSG_WHOLEPKT        3

#define MRT6MSG_NOCACHE         1
#define MRT6MSG_WRONGMIF        2
#define MRT6MSG_WHOLEPKT        3
</pre>
     * 
     * @param vif_index the vif index of the related interface
     * (message-specific).
     * 
     * @param src the source address in the message.
     * @param dst the destination address in the message.
     * @param rcvbuf the data buffer with the additional information in
     * the message.
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		signal_message_recv(const string& src_module_instance_name,
				    int message_type,
				    uint32_t vif_index,
				    const IPvX& src,
				    const IPvX& dst,
				    const uint8_t *rcvbuf,
				    size_t rcvlen);
    
    /**
     * Process a dataflow upcall from the kernel or from the MFEA internal
     * mechanism.
     * 
     * The MFEA internal bandwidth-estimation mechanism is based on 
     * periodic reading of the kernel multicast forwarding statistics.
     * The signal is sent to all user-level protocols that expect it.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @param threshold_interval the dataflow threshold interval.
     * 
     * @param measured_interval the dataflow measured interval.
     * 
     * @param threshold_packets the threshold (in number of packets)
     * to compare against.
     * 
     * @param threshold_bytes the threshold (in number of bytes)
     * to compare against.
     * 
     * @param measured_packets the number of packets measured within
     * the @ref measured_interval.
     * 
     * @param measured_bytes the number of bytes measured within
     * the @ref measured_interval.
     * 
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * 
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * 
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * 
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int signal_dataflow_message_recv(const IPvX& source,
				     const IPvX& group,
				     const TimeVal& threshold_interval,
				     const TimeVal& measured_interval,
				     uint32_t threshold_packets,
				     uint32_t threshold_bytes,
				     uint32_t measured_packets,
				     uint32_t measured_bytes,
				     bool is_threshold_in_packets,
				     bool is_threshold_in_bytes,
				     bool is_geq_upcall,
				     bool is_leq_upcall);
    
    /**
     * Send a signal that a dataflow-related pre-condition is true.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param source_addr the source address of the dataflow.
     * 
     * @param group_addr the group address of the dataflow.
     * 
     * @param threshold_interval_sec the number of seconds in the
     * interval requested for measurement.
     * 
     * @param threshold_interval_usec the number of microseconds in the
     * interval requested for measurement.
     * 
     * @param measured_interval_sec the number of seconds in the
     * last measured interval that has triggered the signal.
     * 
     * @param measured_interval_usec the number of microseconds in the
     * last measured interval that has triggered the signal.
     * 
     * @param threshold_packets the threshold value to trigger a signal
     * (in number of packets).
     * 
     * @param threshold_bytes the threshold value to trigger a signal
     * (in bytes).
     * 
     * @param measured_packets the number of packets measured within
     * the @ref measured_interval.
     * 
     * @param measured_bytes the number of bytes measured within
     * the @ref measured_interval.
     * 
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * 
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * 
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * 
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int dataflow_signal_send(const string& dst_module_instance_name,
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
				     bool is_leq_upcall) = 0;
    
    /**
     * Add Multicast Forwarding Cache (MFC) to the kernel.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that adds the MFC.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @param iif_vif_index the vif index of the incoming interface.
     * 
     * @param oiflist the bitset with the outgoing interfaces.
     * 
     * @param oiflist_disable_wrongvif the bitset with the outgoing interfaces
     * to disable the WRONGVIF signal.
     * 
     * @param max_vifs_oiflist the number of vifs covered by @ref oiflist
     * or @ref oiflist_disable_wrongvif.
     * 
     * @param rp_addr the RP address.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	add_mfc(const string& module_instance_name,
		const IPvX& source, const IPvX& group,
		uint32_t iif_vif_index, const Mifset& oiflist,
		const Mifset& oiflist_disable_wrongvif,
		uint32_t max_vifs_oiflist,
		const IPvX& rp_addr, uint32_t distance,
		string& error_msg,
		bool check_stored_routes);

    int	add_mfc_str(const string& module_instance_name,
		    const IPvX& source,
		    const IPvX& group,
		    const string& iif_name,
		    const string& oif_names,
		    uint32_t distance,
		    string& error_msg, bool check_stored_routes);
    
    /**
     * Delete Multicast Forwarding Cache (MFC) from the kernel.
     * 
     * Note: all corresponding dataflow entries are also removed.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that deletes the MFC.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	delete_mfc(const string& module_instance_name,
		   const IPvX& source, const IPvX& group,
		   string& error_msg, bool check_stored_routes);
    
    /**
     * Add a dataflow monitor entry.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that adds the dataflow monitor entry.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @param threshold_interval the dataflow threshold interval.
     * 
     * @param threshold_packets the threshold (in number of packets) to
     * compare against.
     * 
     * @param threshold_bytes the threshold (in number of bytes) to
     * compare against.
     * 
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * 
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * 
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * 
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * 
     * @param error_msg the error message (if error).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_dataflow_monitor(const string& module_instance_name,
				     const IPvX& source, const IPvX& group,
				     const TimeVal& threshold_interval,
				     uint32_t threshold_packets,
				     uint32_t threshold_bytes,
				     bool is_threshold_in_packets,
				     bool is_threshold_in_bytes,
				     bool is_geq_upcall,
				     bool is_leq_upcall,
				     string& error_msg);
    
    /**
     * Delete a dataflow monitor entry.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that deletes the dataflow monitor entry.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @param threshold_interval the dataflow threshold interval.
     * 
     * @param threshold_packets the threshold (in number of packets) to
     * compare against.
     * 
     * @param threshold_bytes the threshold (in number of bytes) to
     * compare against.
     * 
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * 
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * 
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * 
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * 
     * @param error_msg the error message (if error).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_dataflow_monitor(const string& module_instance_name,
					const IPvX& source, const IPvX& group,
					const TimeVal& threshold_interval,
					uint32_t threshold_packets,
					uint32_t threshold_bytes,
					bool is_threshold_in_packets,
					bool is_threshold_in_bytes,
					bool is_geq_upcall,
					bool is_leq_upcall,
					string& error_msg);
    
    /**
     * Delete all dataflow monitor entries for a given source and group
     * address.
     * 
     * @param module_instance_name the module instance name of the protocol
     * that deletes the dataflow monitor entry.
     * 
     * @param source the source address.
     * 
     * @param group the group address.
     * 
     * @param error_msg the error message (if error).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_all_dataflow_monitor(const string& module_instance_name,
					    const IPvX& source,
					    const IPvX& group,
					    string& error_msg);
    
    /**
     * Add a multicast vif to the kernel.
     * 
     * @param vif_index the vif index of the interface to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_multicast_vif(uint32_t vif_index, string& error_msg);
    
    /**
     * Delete a multicast vif from the kernel.
     * 
     * @param vif_index the vif index of the interface to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_multicast_vif(uint32_t vif_index);
    
    /**
     * Get MFC multicast forwarding statistics from the kernel.
     * 
     * Get the number of packets and bytes forwarded by a particular
     * Multicast Forwarding Cache (MFC) entry in the kernel, and the number
     * of packets arrived on wrong interface for that entry.
     * 
     * @param source the MFC source address.
     * @param group the MFC group address.
     * @param sg_count a reference to a @ref SgCount class to place
     * the result: the number of packets and bytes forwarded by the particular
     * MFC entry, and the number of packets arrived on a wrong interface.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_sg_count(const IPvX& source, const IPvX& group,
			     SgCount& sg_count);
    
    /**
     * Get interface multicast forwarding statistics from the kernel.
     * 
     * Get the number of packets and bytes received on, or forwarded on
     * a particular multicast interface.
     * 
     * @param vif_index the vif index of the virtual multicast interface whose
     * statistics we need.
     * @param vif_count a reference to a @ref VifCount class to store
     * the result.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_count(uint32_t vif_index, VifCount& vif_count);
    
    /**
     * Get a reference to the mrouter (@ref MfeaMrouter).
     * 
     * @return a reference to the mrouter (@ref MfeaMrouter).
     */
    MfeaMrouter& mfea_mrouter() { return (_mfea_mrouter); }
    
    /**
     * Get a reference to the dataflow table (@ref MfeaDft).
     * 
     * @return a reference to the dataflow table (@ref MfeaDft).
     */
    MfeaDft&	mfea_dft() { return (_mfea_dft); }
    
    /**
     * Test if trace log is enabled.
     * 
     * This method is used to test whether to output trace log debug messges.
     * 
     * @return true if trace log is enabled, otherwise false.
     */
    bool	is_log_trace() const { return (_is_log_trace); }

    /**
     * Enable/disable trace log.
     * 
     * This method is used to enable/disable trace log debug messages output.
     * 
     * @param is_enabled if true, trace log is enabled, otherwise is disabled.
     */
    void	set_log_trace(bool is_enabled) { _is_log_trace = is_enabled; }

protected:
    IfConfigUpdateReplicator&	mfea_iftree_update_replicator() {
	return (_mfea_iftree_update_replicator);
    }

private:
    void interface_update(const string& ifname,
			  const Update& update);

    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& update);

    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& update);

#ifdef HAVE_IPV6
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& update);
#endif

    void updates_completed();

    /**
     * A method invoked when the status of a service changes.
     * 
     * @param service the service whose status has changed.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status);

    int add_pim_register_vif();
    
    // Private state
    FeaNode&		_fea_node;

    MfeaMrouter		_mfea_mrouter;	// The mrouter state
    MfeaDft		_mfea_dft;	// The dataflow monitoring table
    
    //
    // The state to register:
    //  - Protocol module instance names
    //  - IP protocol numbers
    set<string>		_registered_module_instance_names;
    set<uint8_t>	_registered_ip_protocols;

    //
    // Interface tree propagation
    //
    IfTree			_mfea_iftree;
    IfConfigUpdateReplicator	_mfea_iftree_update_replicator;

    // Store desired routes.  This is a cheap way of doing some of the
    // RIB logic, but for mcast routes.
    // Lower distance means higher priority.
    // key is:  source:group, ie:  192.168.1.1:226.0.0.1
#define MAX_MFEA_DISTANCE 8
    map<string, MfeaRouteStorage> routes[MAX_MFEA_DISTANCE];

    //
    // Debug and test-related state
    //
    bool	_is_log_trace;		// If true, enable XLOG_TRACE()
};

#endif // __FEA_MFEA_NODE_HH__
