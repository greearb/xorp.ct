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

// $XORP: xorp/pim/pim_node.hh,v 1.23 2003/07/12 01:06:56 pavlin Exp $


#ifndef __PIM_PIM_NODE_HH__
#define __PIM_PIM_NODE_HH__


//
// PIM node definition.
//


#include <vector>

#include "libxorp/vif.hh"
#include "libproto/proto_node.hh"
#include "mrt/buffer.h"
#include "mrt/mifset.hh"
#include "pim_bsr.hh"
#include "pim_mrib_table.hh"
#include "pim_mrt.hh"
#include "pim_rp.hh"
#include "pim_scope_zone_table.hh"
#include "pim_vif.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class PimVif;
class PimMrt;
class PimNbr;

/**
 * @short The PIM node class.
 * 
 * There should be one node per PIM instance. There should be
 * one instance per address family.
 */
class PimNode : public ProtoNode<PimVif> {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param module_id the module ID (@ref xorp_module_id). Should be
     * XORP_MODULE_PIMSM Note: if/after PIM-DM is implemented,
     * XORP_MODULE_PIMDM would be allowed as well.
     * @param eventloop the event loop to use.
     */
    PimNode(int family, xorp_module_id module_id, EventLoop& eventloop);
    
    /**
     * Destructor
     */
    virtual	~PimNode();
    
    /**
     * Start the node operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the node operation.
     * 
     * Gracefully stop the PIM protocol.
     * The graceful stop will attempt to send Join/Prune, Assert, etc.
     * messages for all multicast routing entries to gracefully clean-up
     * state with neighbors.
     * After the multicast routing entries cleanup is completed,
     * @ref PimNode::final_stop() is called to complete the job.
     * If this method is called one-after-another, the second one
     * will force calling immediately @ref PimNode::final_stop() to quickly
     * finish the job.
     * This function, unlike start(), will stop the protocol
     * operation on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Completely stop the node operation.
     * 
     * This method should be called after @ref PimNode::stop() to complete
     * the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_stop();

    /**
     * Test if waiting to complete registration with the MFEA.
     * 
     * @return true if waiting to complete registration with the MFEA,
     * otherwise false.
     */
    bool is_waiting_for_mfea_startup() const;
    
    /**
     * Test if waiting to complete registration with the MLD6IGMP.
     * 
     * @return true if waiting to complete registration with the MLD6IGMP,
     * otherwise false.
     */
    bool is_waiting_for_mld6igmp_startup() const;
    
    /**
     * Test if there is an unit that is in PENDING_DOWN state.
     * 
     * @param reason return-by-reference string that contains human-readable
     * information about the unit that is in PENDING_DOWN state (if any).
     * @return true if there is an unit that is in PENDING_DOWN state,
     * otherwise false.
     */
    bool	has_pending_down_units(string& reason);

    /**
     * Get the node status (see @ref ProcessStatus).
     * 
     * @param reason return-by-reference string that contains human-readable
     * information about the status.
     * @return the node status (see @ref ProcessStatus).
     */
    ProcessStatus	node_status(string& reason);
    
    /**
     * Install a new PIM vif.
     * 
     * @param vif vif information about new PimVif to install.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const Vif& vif, string& err);
    
    /**
     * Install a new PIM vif.
     * 
     * @param vif_name the name of the new vif.
     * @param vif_index the vif index of the new vif.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const string& vif_name, uint16_t vif_index,
			string& err);
    
    /**
     * Delete an existing PIM vif.
     * 
     * @param vif_name the name of the vif to delete.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif(const string& vif_name, string& err);
    
    /**
     * Set flags to a vif.
     * 
     * @param vif_name the name of the vif.
     * @param is_pim_register true if this is a PIM Register vif.
     * @param is_p2p true if this is a point-to-point vif.
     * @param is_loopback true if this is a loopback interface.
     * @param is_multicast rue if the vif is multicast-capable.
     * @param is_broadcast true if the vif is broadcast-capable.
     * @param is_up true if the vif is UP and running.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_flags(const string& vif_name,
			      bool is_pim_register, bool is_p2p,
			      bool is_loopback, bool is_multicast,
			      bool is_broadcast, bool is_up,
			      string& err);
    
    /**
     * Add a new address to a vif, or update an existing address.
     * 
     * @param vif_name the name of the vif.
     * @param addr the unicast address to add.
     * @param subnet_addr the subnet address to add.
     * @param broadcast_addr the broadcast address (when applicable).
     * @param peer_addr the peer address (when applicable).
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif_addr(const string& vif_name,
			     const IPvX& addr,
			     const IPvXNet& subnet_addr,
			     const IPvX& broadcast_addr,
			     const IPvX& peer_addr,
			     string& err);
    
    /**
     * Delete an address from a vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the unicast address to delete.
     * @param vif_name the name of the vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif_addr(const string& vif_name,
				const IPvX& addr,
				string& err);
    
    /**
     * Enable an existing PIM vif.
     * 
     * @param vif_name the name of the vif to enable.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_vif(const string& vif_name, string& err);

    /**
     * Disable an existing PIM vif.
     * 
     * @param vif_name the name of the vif to disable.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_vif(const string& vif_name, string& err);

    /**
     * Start an existing PIM vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_vif(const string& vif_name, string& err);
    
    /**
     * Stop an existing PIM vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_vif(const string& vif_name, string& err);
    
    /**
     * Start PIM on all enabled interfaces.
     * 
     * @return the number of virtual interfaces PIM was started on,
     * or XORP_ERROR if error occured.
     */
    int		start_all_vifs();
    
    /**
     * Stop PIM on all interfaces it was running on.
     * 
     * @return he number of virtual interfaces PIM was stopped on,
     * or XORP_ERROR if error occured.
     */
    int		stop_all_vifs();
    
    /**
     * Enable PIM on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_all_vifs();
    
    /**
     * Disable PIM on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_all_vifs();
    
    /**
     * Delete all PIM vifs.
     */
    void	delete_all_vifs();
    
    /**
     * Receive a protocol message.
     * 
     * @param src_module_instance_name the module instance name of the
     * module-origin of the message.
     * 
     * @param src_module_id the module ID (@ref xorp_module_id) of the
     * module-origin of the message.
     * 
     * @param vif_index the vif index of the interface used to receive this
     * message.
     * 
     * @param src the source address of the message.
     * 
     * @param dst the destination address of the message.
     * 
     * @param ip_ttl the IP TTL of the message. If it has a negative value,
     * it should be ignored.
     * 
     * @param ip_tos the IP TOS of the message. If it has a negative value,
     * it should be ignored.
     * 
     * @param router_alert_bool if true, the IP Router Alert option in
     * the IP packet was set (when applicable).
     * 
     * @param rcvbuf the data buffer with the received message.
     * 
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_recv(const string& src_module_instance_name,
			   xorp_module_id src_module_id,
			   uint16_t vif_index,
			   const IPvX& src, const IPvX& dst,
			   int ip_ttl, int ip_tos, bool router_alert_bool,
			   const uint8_t *rcvbuf, size_t rcvlen);
    
    /**
     * Send a protocol message.
     * 
     * Note: this method uses the pure virtual ProtoNode::proto_send() method
     * that is implemented somewhere else (in a class that inherits this one).
     * 
     * @param vif_index the vif index of the vif to send the message.
     * @param src the source address of the message.
     * @param dst the destination address of the message.
     * @param ip_ttl the TTL of the IP packet to send. If it has a
     * negative value, the TTL will be set by the lower layers.
     * @param ip_tos the TOS of the IP packet to send. If it has a
     * negative value, the TOS will be set by the lower layers.
     * @param router_alert_bool if true, set the IP Router Alert option in
     * the IP packet to send (when applicable).
     * @param buffer the data buffer with the message to send.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		pim_send(uint16_t vif_index,
			 const IPvX& src, const IPvX& dst,
			 int ip_ttl, int ip_tos, bool router_alert_bool,
			 buffer_t *buffer);
    
    /**
     * Receive a signal message from the kernel.
     * 
     * @param src_module_instance_name the module instance name of the
     * module-origin of the message.
     * 
     * @param src_module_id the module ID (@ref xorp_module_id) of the
     * module-origin of the message.
     * 
     * @param message_type the message type.
     * Currently, the type of messages received from the kernel are:
     * 
<pre>
#define MFEA_KERNEL_MESSAGE_NOCACHE        1
#define MFEA_KERNEL_MESSAGE_WRONGVIF       2
#define MFEA_KERNEL_MESSAGE_WHOLEPKT       3
</pre>
     * 
     * 
     * @param vif_index the vif index of the related interface
     * (message-specific relation).
     * 
     * @param src the source address in the message.
     * 
     * @param dst the destination address in the message.
     * 
     * @param rcvbuf the data buffer with the additional information in
     * the message.
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		signal_message_recv(const string& src_module_instance_name,
				    xorp_module_id src_module_id,
				    int message_type,
				    uint16_t vif_index,
				    const IPvX& src,
				    const IPvX& dst,
				    const uint8_t *rcvbuf,
				    size_t rcvlen);
    /**
     * Send signal message: not used by PIM.
     */
    int		signal_message_send(const string&, // dst_module_instance_name,
				    xorp_module_id, // dst_module_id,
				    int		, // message_type,
				    uint16_t	, // vif_index,
				    const IPvX&	, // src,
				    const IPvX&	, // dst,
				    const uint8_t * , // sndbuf,
				    size_t	  // sndlen
	) { XLOG_UNREACHABLE(); return (XORP_ERROR); }

    /**
     * Start the protocol with the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start_protocol_kernel() = 0;

    /**
     * Stop the protocol with the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop_protocol_kernel() = 0;
    
    /**
     * Start a protocol vif with the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param vif_index the vif index of the interface to start.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start_protocol_kernel_vif(uint16_t vif_index) = 0;
    
    /**
     * Stop a protocol vif with the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param vif_index the vif index of the interface to stop.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop_protocol_kernel_vif(uint16_t vif_index) = 0;
    
    /**
     * Join a multicast group on an interface.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * TODO: add a source address as well!!
     * 
     * @param vif_index the vif index of the interface to join.
     * @param multicast_group the multicast group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int join_multicast_group(uint16_t vif_index,
				     const IPvX& multicast_group) = 0;
    
    /**
     * Leave a multicast group on an interface.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * TODO: add a source address as well!!
     * 
     * @param vif_index the vif index of the interface to leave.
     * @param multicast_group the multicast group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int leave_multicast_group(uint16_t vif_index,
				      const IPvX& multicast_group) = 0;
    
    /**
     * Add a Multicast Forwarding Cache to the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param pim_mfc the @ref PimMfc entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_mfc_to_kernel(const PimMfc& pim_mfc) = 0;
    
    /**
     * Delete a Multicast Forwarding Cache to the kernel.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param pim_mfc the @ref PimMfc entry to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_mfc_from_kernel(const PimMfc& pim_mfc) = 0;
    
    /**
     * Add a dataflow monitor to the MFEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
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
     * @param threshold_interval_sec the dataflow threshold interval
     * (seconds).
     * 
     * @param threshold_interval_usec the dataflow threshold interval
     * (microseconds).
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_dataflow_monitor(const IPvX& source_addr,
				     const IPvX& group_addr,
				     uint32_t threshold_interval_sec,
				     uint32_t threshold_interval_usec,
				     uint32_t threshold_packets,
				     uint32_t threshold_bytes,
				     bool is_threshold_in_packets,
				     bool is_threshold_in_bytes,
				     bool is_geq_upcall,
				     bool is_leq_upcall) = 0;
    
    /**
     * Delete a dataflow monitor from the MFEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
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
     * @param threshold_interval_sec the dataflow threshold interval
     * (seconds).
     * 
     * @param threshold_interval_usec the dataflow threshold interval
     * (microseconds).
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_dataflow_monitor(const IPvX& source_addr,
					const IPvX& group_addr,
					uint32_t threshold_interval_sec,
					uint32_t threshold_interval_usec,
					uint32_t threshold_packets,
					uint32_t threshold_bytes,
					bool is_threshold_in_packets,
					bool is_threshold_in_bytes,
					bool is_geq_upcall,
					bool is_leq_upcall) = 0;
    
    /**
     * Delete all dataflow monitors for a given source and group address from
     * the MFEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param source_addr the source address.
     * @param group_addr the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_all_dataflow_monitor(const IPvX& source_addr,
					    const IPvX& group_addr) = 0;
    
    /**
     * Register this protocol with the MLD/IGMP module.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * By registering this protocol with the MLD/IGMP module, it will
     * be notified about multicast group membership events.
     * 
     * @param vif_index the vif index of the interface to register with
     * the MLD/IGMP module.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_protocol_mld6igmp(uint16_t vif_index) = 0;
    
    /**
     * Deregister this protocol with the MLD/IGMP module.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param vif_index the vif index of the interface to deregister with
     * the MLD/IGMP module.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_protocol_mld6igmp(uint16_t vif_index) = 0;
    
    /**
     * Receive "add membership" from the MLD/IGMP module.
     * 
     * @param vif_index the vif index of the interface with membership change.
     * @param source the source address of the (S,G) or (*,G) entry that has
     * changed membership. In case of Any-Source Multicast, it is IPvX::ZERO().
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_membership(uint16_t vif_index, const IPvX& source,
			       const IPvX& group);
    
    /**
     * Receive "delete membership" from the MLD/IGMP module.
     * 
     * @param vif_index the vif index of the interface with membership change.
     * @param source the source address of the (S,G) or (*,G) entry that has
     * changed membership. In case of Any-Source Multicast, it is IPvX::ZERO().
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_membership(uint16_t vif_index, const IPvX& source,
				  const IPvX& group);
    
    /**
     * Add an entry to the MRIB table (@ref MribTable).
     * 
     * @param mrib the @ref Mrib entry to add.
     */
    void add_mrib_entry(const Mrib& mrib) { _pim_mrib_table.insert(mrib); }
    
    /**
     * Delete an entry from the MRIB table (@ref MribTable).
     * 
     * @param mrib the @ref Mrib entry to delete.
     */
    void delete_mrib_entry(const Mrib& mrib) { _pim_mrib_table.remove(mrib); }
    
    /**
     * Test if an address is directly connected to one of my virtual
     * interfaces.
     * 
     * Note that the MRIB-based RPF info must point toward the same interface.
     * 
     * @param ipaddr_test the address to test.
     * @return true if @ref ipaddr_test is directly connected to one of
     * my virtual interfaces, otherwise false.
     */
    bool is_directly_connected(const IPvX& ipaddr_test) const;
    
    /**
     * Test if an address is directly connected to a specified virtual
     * interface.
     * 
     * Note that the MRIB-based RPF info must point toward the same interface.
     * 
     * @param pim_vif the virtual interface to test against.
     * @param ipaddr_test the address to test.
     * @return true if @ref ipaddr_test is directly connected to @ref vif,
     * otherwise false.
     */
    bool is_directly_connected(const PimVif& pim_vif,
			       const IPvX& ipaddr_test) const;
    
    /**
     * Get the PIM-Register virtual interface.
     * 
     * @return the PIM-Register virtual interface if exists, otherwise NULL.
     */
    PimVif	*vif_find_pim_register() const;
    
    /**
     * Get the vif index of the PIM-Register virtual interface.
     * 
     * @return the vif index of the PIM-Register virtual interface if exists,
     * otherwise @ref Vif::VIF_INDEX_INVALID.
     */
    uint16_t	pim_register_vif_index() const { return (_pim_register_vif_index); }
    
    /**
     * Get the PIM Multicast Routing Table.
     * 
     * @return a reference to the PIM Multicast Routing Table (@ref PimMrt).
     */
    PimMrt&	pim_mrt()		{ return (_pim_mrt);		}
    
    /**
     * Get the table with the Multicast Routing Information Base used by PIM.
     * 
     * @return a reference to the table with the Multicast Routing Information
     * Base used by PIM (@ref PimMribTable).
     */
    PimMribTable& pim_mrib_table()	{ return (_pim_mrib_table);	}
    
    /**
     * Get the PIM Bootstrap entity.
     * 
     * @return a reference to the PIM Bootstrap entity (@ref PimBsr).
     */
    PimBsr&	pim_bsr()		{ return (_pim_bsr);		}
    
    /**
     * Get the PIM RP table.
     * 
     * @return a reference to the PIM RP table (@ref RpTable).
     */
    RpTable&	rp_table()		{ return (_rp_table);		}
    
    /**
     * Get the PIM Scope-Zone table.
     * 
     * @return a reference to the PIM Scope-Zone table.
     */
    PimScopeZoneTable& pim_scope_zone_table() { return (_pim_scope_zone_table); }
    
    /**
     * Get the set of vifs for which this PIM note is a Designated Router.
     * 
     * @return the @ref Mifset indicating the vifs for which this PIM node is
     * a Designated Router.
     */
    Mifset&	pim_vifs_dr()		{ return (_pim_vifs_dr);	}
    
    /**
     * Set/reset a virtual interface as a Designated Router.
     * 
     * @param vif_index the vif index of the virtual interface to set/reset as
     * a Designated Router.
     * @param v if true, set the virtual interface as a Designated Router,
     * otherwise reset it.
     */
    void	set_pim_vifs_dr(uint16_t vif_index, bool v);
    
    /**
     * Find the RPF PIM neighbor for a given destination address.
     * 
     * @param dst_addr the destination address to lookup.
     * @return the RPF PIM neighbor (@ref PimNbr) toward @ref dst_addr
     * if found, otherwise NULL.
     */
    PimNbr	*pim_nbr_rpf_find(const IPvX& dst_addr);
    
    /**
     * Find the RPF PIM neighbor for a given destination address, and
     * already known @ref Mrib entry.
     * 
     * @param dst_addr the destination address to lookup.
     * @param mrib the already known @ref Mrib entry.
     * @return the RPF PIM neighbor (@ref PimNbr) toward @ref dst_addr
     * if found, otherwise NULL.
     */
    PimNbr	*pim_nbr_rpf_find(const IPvX& dst_addr, const Mrib *mrib);
    
    /**
     * Find a PIM neighbor.
     * 
     * @param nbr_addr the address of the PIM neighbor.
     * @return the PIM neighbor (@ref PimNbr) if found, otherwise NULL.
     */
    PimNbr	*pim_nbr_find(const IPvX& nbr_addr);
    
    /**
     * Enable the PIM Bootstrap mechanism.
     */
    void	enable_bsr() { pim_bsr().enable(); }
    
    /**
     * Disable the PIM Bootstrap mechanism.

     */
    void	disable_bsr() { pim_bsr().disable(); }
    
    /**
     * Start the Bootstrap mechanism.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_bsr() { return (pim_bsr().start()); }
    
    /**
     * Stop the Bootstrap mechanism.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_bsr() { return (pim_bsr().stop()); }
    
    //
    // Configuration methods
    //
    
    /**
     * Complete the set of vif configuration changes.
     * 
     * @param reason return-by-reference string that contains human-readable
     * string with information about the reason for failure (if any).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_all_vifs_done(string& reason);
    
    int		set_vif_proto_version(const string& vif_name,
				      int proto_version,
				      string& reason);
    int		reset_vif_proto_version(const string& vif_name,
					string& reason);
    int		set_vif_hello_triggered_delay(const string& vif_name,
					      uint16_t hello_triggered_delay,
					      string& reason);
    int		reset_vif_hello_triggered_delay(const string& vif_name,
						string& reason);
    int		set_vif_hello_period(const string& vif_name,
				     uint16_t hello_period,
				     string& reason);
    int		reset_vif_hello_period(const string& vif_name, string& reason);
    int		set_vif_hello_holdtime(const string& vif_name,
				       uint16_t	hello_holdtime,
				       string& reason);
    int		reset_vif_hello_holdtime(const string& vif_name,
					 string& reason);
    int		set_vif_dr_priority(const string& vif_name,
				    uint32_t dr_priority,
				    string& reason);
    int		reset_vif_dr_priority(const string& vif_name, string& reason);
    int		set_vif_lan_delay(const string&	vif_name,
				  uint16_t lan_delay,
				  string& reason);
    int		reset_vif_lan_delay(const string& vif_name, string& reason);
    int		set_vif_override_interval(const string&	vif_name,
					  uint16_t override_interval,
					  string& reason);
    int		reset_vif_override_interval(const string& vif_name,
					    string& reason);
    int		set_vif_is_tracking_support_disabled(const string& vif_name,
						     bool is_tracking_support_disabled,
						     string& reason);
    int		reset_vif_is_tracking_support_disabled(const string& vif_name,
						       string& reason);
    int		set_vif_accept_nohello_neighbors(const string& vif_name,
						 bool accept_nohello_neighbors,
						 string& reason);
    int		reset_vif_accept_nohello_neighbors(const string& vif_name,
						   string& reason);
    int		set_vif_join_prune_period(const string&	vif_name,
					  uint16_t join_prune_period,
					  string& reason);
    int		reset_vif_join_prune_period(const string& vif_name,
					    string& reason);
    int		set_switch_to_spt_threshold(bool is_enabled,
					    uint32_t interval_sec,
					    uint32_t bytes,
					    string& reason);
    int		reset_switch_to_spt_threshold(string& reason);
    //
    int		add_config_scope_zone_by_vif_name(const IPvXNet& scope_zone_id,
						  const string& vif_name,
						  string& reason);
    int		add_config_scope_zone_by_vif_addr(const IPvXNet& scope_zone_id,
						  const IPvX& vif_addr,
						  string& reason);
    int		delete_config_scope_zone_by_vif_name(const IPvXNet& scope_zone_id,
						     const string& vif_name,
						     string& reason);
    int		delete_config_scope_zone_by_vif_addr(const IPvXNet& scope_zone_id,
						     const IPvX& vif_addr,
						     string& reason);
    //
    int		add_config_cand_bsr_by_vif_name(const IPvXNet& scope_zone_id,
						bool is_scope_zone,
						const string& vif_name,
						uint8_t bsr_priority,
						uint8_t hash_masklen,
						string& reason);
    int		add_config_cand_bsr_by_addr(const IPvXNet& scope_zone_id,
					    bool is_scope_zone,
					    const IPvX& my_cand_bsr_addr,
					    uint8_t bsr_priority,
					    uint8_t hash_masklen,
					    string& reason);
    int		delete_config_cand_bsr(const IPvXNet& scope_zone_id,
				       bool is_scope_zone,
				       string& reason);
    int		add_config_cand_rp_by_vif_name(const IPvXNet& group_prefix,
					       bool is_scope_zone,
					       const string& vif_name,
					       uint8_t rp_priority,
					       uint16_t rp_holdtime,
					       string& reason);
    int		add_config_cand_rp_by_addr(const IPvXNet& group_prefix,
					   bool is_scope_zone,
					   const IPvX& my_cand_rp_addr,
					   uint8_t rp_priority,
					   uint16_t rp_holdtime,
					   string& reason);
    int		delete_config_cand_rp_by_vif_name(const IPvXNet& group_prefix,
						  bool is_scope_zone,
						  const string& vif_name,
						  string& reason);
    int		delete_config_cand_rp_by_addr(const IPvXNet& group_prefix,
					      bool is_scope_zone,
					      const IPvX& my_cand_rp_addr,
					      string& reason);
    int		add_config_static_rp(const IPvXNet& group_prefix,
				     const IPvX& rp_addr,
				     uint8_t rp_priority,
				     uint8_t hash_masklen,
				     string& reason);
    int		delete_config_static_rp(const IPvXNet& group_prefix,
					const IPvX& rp_addr,
					string& reason);
    int		config_static_rp_done(string& reason);
    
    //
    // Debug-related methods
    //
    
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
    
    //
    // Test-related methods
    //
    // Join/Prune test-related methods
    int		add_test_jp_entry(const IPvX& source_addr,
				  const IPvX& group_addr,
				  uint8_t group_masklen,
				  mrt_entry_type_t mrt_entry_type,
				  action_jp_t action_jp, uint16_t holdtime,
				  bool new_group_bool);
    int		send_test_jp_entry(const IPvX& nbr_addr);
    // Assert test-related methods
    int		send_test_assert(const string& vif_name,
				 const IPvX& source_addr,
				 const IPvX& group_addr,
				 bool rpt_bit,
				 uint32_t metric_preference,
				 uint32_t metric);
    // Bootstrap test-related methods
    int		add_test_bsr_zone(const PimScopeZoneId& zone_id,
				  const IPvX& bsr_addr,
				  uint8_t bsr_priority,
				  uint8_t hash_masklen,
				  uint16_t fragment_tag);
    int		add_test_bsr_group_prefix(const PimScopeZoneId& zone_id,
					  const IPvXNet& group_prefix,
					  bool is_scope_zone,
					  uint8_t expected_rp_count);
    int		add_test_bsr_rp(const PimScopeZoneId& zone_id,
				const IPvXNet& group_prefix,
				const IPvX& rp_addr,
				uint8_t rp_priority,
				uint16_t rp_holdtime);
    int		send_test_bootstrap(const string& vif_name);
    int		send_test_bootstrap_by_dest(const string& vif_name,
					    const IPvX& dest_addr);
    int		send_test_cand_rp_adv();
    
    
    //
    // PimNbr-related methods
    //
    void	add_pim_mre_no_pim_nbr(PimMre *pim_mre);
    void	delete_pim_mre_no_pim_nbr(PimMre *pim_mre);
    list<PimNbr *>& processing_pim_nbr_list() {
	return (_processing_pim_nbr_list);
    }
    void	init_processing_pim_mre_rp(uint16_t vif_index,
					   const IPvX& pim_nbr_addr);
    void	init_processing_pim_mre_wc(uint16_t vif_index,
					   const IPvX& pim_nbr_addr);
    void	init_processing_pim_mre_sg(uint16_t vif_index,
					   const IPvX& pim_nbr_addr);
    void	init_processing_pim_mre_sg_rpt(uint16_t vif_index,
					       const IPvX& pim_nbr_addr);
    PimNbr	*find_processing_pim_mre_rp(uint16_t vif_index,
					    const IPvX& pim_nbr_addr);
    PimNbr	*find_processing_pim_mre_wc(uint16_t vif_index,
					    const IPvX& pim_nbr_addr);
    PimNbr	*find_processing_pim_mre_sg(uint16_t vif_index,
					    const IPvX& pim_nbr_addr);
    PimNbr	*find_processing_pim_mre_sg_rpt(uint16_t vif_index,
						const IPvX& pim_nbr_addr);
    
    //
    // Configuration parameters
    //
    ConfigParam<bool>& is_switch_to_spt_enabled() {
	return (_is_switch_to_spt_enabled);
    }
    ConfigParam<uint32_t>& switch_to_spt_threshold_interval_sec() {
	return (_switch_to_spt_threshold_interval_sec);
    }
    ConfigParam<uint32_t>& switch_to_spt_threshold_bytes() {
	return (_switch_to_spt_threshold_bytes);
    }

    //
    // Status-related methods
    //
    void incr_waiting_for_mfea_startup_events();
    void decr_waiting_for_mfea_startup_events();
    void incr_waiting_for_mld6igmp_startup_events();
    void decr_waiting_for_mld6igmp_startup_events();
    
private:
    
    PimMrt	_pim_mrt;		// PIM Multicast Routing Table
    PimMribTable _pim_mrib_table;	// PIM Multicast Routing Information Base table
    PimBsr	_pim_bsr;		// The BSR state
    RpTable	_rp_table;		// The RP table
    PimScopeZoneTable _pim_scope_zone_table; // The scope zone table
    
    uint16_t	_pim_register_vif_index;// The PIM Register vif index
    Mifset	_pim_vifs_dr;		// The vifs I am the DR
    
    buffer_t	*_buffer_recv;		// Buffer for receiving messages
    
    list<PimNbr *> _processing_pim_nbr_list; // The list of deleted PimNbr with
					// PimMre entries to process.
    
    //
    // Configuration parameters
    //
    ConfigParam<bool>		_is_switch_to_spt_enabled;
    ConfigParam<uint32_t>	_switch_to_spt_threshold_interval_sec;
    ConfigParam<uint32_t>	_switch_to_spt_threshold_bytes;
    
    //
    // Status-related state
    //
    size_t	_waiting_for_mfea_startup_events;
    size_t	_waiting_for_mld6igmp_startup_events;
    
    //
    // Debug and test-related state
    //
    bool	_is_log_trace;		// If true, enable XLOG_TRACE()
    PimJpHeader	_test_jp_header;	// J/P header to send test J/P messages
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __PIM_PIM_NODE_HH__
