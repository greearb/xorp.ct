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

// $XORP: xorp/mld6igmp/mld6igmp_node.hh,v 1.6 2003/04/22 23:27:22 hodson Exp $

#ifndef __MLD6IGMP_MLD6IGMP_NODE_HH__
#define __MLD6IGMP_MLD6IGMP_NODE_HH__


//
// IGMP and MLD node definition.
//


#include <vector>

#include "libxorp/vif.hh"
#include "libproto/proto_node.hh"
#include "mrt/buffer.h"
#include "mrt/multicast_defs.h"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class IPvX;
class IPvXNet;
class Mld6igmpVif;

/**
 * @short The MLD/IGMP node class.
 * 
 * There should be one node per MLD or IGMP instance. There should be
 * one instance per address family.
 */
class Mld6igmpNode : public ProtoNode<Mld6igmpVif> {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param module_id the module ID (@ref xorp_module_id). Should be
     * equal to XORP_MODULE_MLD6IGMP.
     * @param eventloop the event loop to use.
     */
    Mld6igmpNode(int family, xorp_module_id module_id, EventLoop& eventloop);
    
    /**
     * Destructor
     */
    virtual ~Mld6igmpNode();
    
    /**
     * Start the node operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the node operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Install a new MLD/IGMP vif.
     * 
     * @param vif vif information about new Mld6igmpVif to install.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const Vif& vif, string& err);
    
    /**
     * Install a new MLD/IGMP vif.
     * 
     * @param vif_name the name of the new vif.
     * @param vif_index the vif index of the new vif.
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const string& vif_name, uint32_t vif_index,
			string& err);
    
    /**
     * Delete an existing MLD/IGMP vif.
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
     * @param is_multicast true if the vif is multicast-capable.
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
     * Add an address to a vif.
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
     * @param err the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif_addr(const string& vif_name,
				const IPvX& addr,
				string& err);
    
    /**
     * Start MLD/IGMP on all enabled interfaces.
     * 
     * @return the number of virtual interfaces MLD/IGMP was started on,
     * or XORP_ERROR if error occured.
     */
    int		start_all_vifs();
    
    /**
     * Stop MLD/IGMP on all interfaces it was running on.
     * 
     * @return he number of virtual interfaces MLD/IGMP was stopped on,
     * or XORP_ERROR if error occured.
     */
    int		stop_all_vifs();
    
    /**
     * Enable MLD/IGMP on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_all_vifs();
    
    /**
     * Disable MLD/IGMP on all interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_all_vifs();
    
    /**
     * Delete all MLD/IGMP vifs.
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
    int		mld6igmp_send(uint16_t vif_index,
			      const IPvX& src, const IPvX& dst,
			      int ip_ttl, int ip_tos,
			      bool router_alert_bool,
			      buffer_t *buffer);
    
    /**
     * Receive signal message: not used by MLD/IGMP.
     */
    int	signal_message_recv(const string&	, // src_module_instance_name,
			    xorp_module_id	, // src_module_id,
			    int			, // message_type,
			    uint16_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // rcvbuf,
			    size_t		  // rcvlen
	) { XLOG_ASSERT(false); return (XORP_ERROR); }
    
    /**
     * Send signal message: not used by MLD/IGMP.
     */
    int	signal_message_send(const string&	, // dst_module_instance_name,
			    xorp_module_id	, // dst_module_id,
			    int			, // message_type,
			    uint16_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // sndbuf,
			    size_t		  // sndlen
	) { XLOG_ASSERT(false); return (XORP_ERROR); }
    
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
     * Add a protocol that needs to be notified about multicast membership
     * changes.
     * 
     * Add a protocol to the list of entries that would be notified
     * if there is membership change on a particular interface.
     * 
     * @param module_instance_name the module instance name of the
     * protocol to add.
     * @param module_id the module ID (@ref xorp_module_id) of the
     * protocol to add.
     * @param vif_index the vif index of the interface to add the protocol to.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_protocol(const string& module_instance_name,
		     xorp_module_id module_id,
		     uint16_t vif_index);
    
    /**
     * Delete a protocol that needs to be notified about multicast membership
     * changes.
     * 
     * Delete a protocol from the list of entries that would be notified
     * if there is membership change on a particular interface.
     * 
     * @param module_instance_name the module instance name of the
     * protocol to delete.
     * @param module_id the module ID (@ref xorp_module_id) of the
     * protocol to delete.
     * @param vif_index the vif index of the interface to delete the
     * protocol from.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_protocol(const string& module_instance_name,
			xorp_module_id module_id,
			uint16_t vif_index);
    
    /**
     * Send "add membership" to a protocol that needs to be notified
     * about multicast membership changes.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * protocol to notify.
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * protocol to notify.
     * @param vif_index the vif index of the interface with membership change.
     * @param source the source address of the (S,G) or (*,G) entry that has
     * changed membership. In case of Any-Source Multicast, it is IPvX::ZERO().
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send_add_membership(const string& dst_module_instance_name,
				    xorp_module_id dst_module_id,
				    uint16_t vif_index,
				    const IPvX& source,
				    const IPvX& group) = 0;
    /**
     * Send "delete membership" to a protocol that needs to be notified
     * about multicast membership changes.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * protocol to notify.
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * protocol to notify.
     * @param vif_index the vif index of the interface with membership change.
     * @param source the source address of the (S,G) or (*,G) entry that has
     * changed membership. In case of Any-Source Multicast, it is IPvX::ZERO().
     * @param group the group address of the (S,G) or (*,G) entry that has
     * changed.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send_delete_membership(const string& dst_module_instance_name,
				       xorp_module_id dst_module_id,
				       uint16_t vif_index,
				       const IPvX& source,
				       const IPvX& group) = 0;
    
    /**
     * Notify a protocol about multicast membership change.
     * 
     * @param module_instance_name the module instance name of the
     * protocol to notify.
     * @param module_id the module ID (@ref xorp_module_id) of the
     * protocol to notify.
     * @param vif_index the vif index of the interface with membership change.
     * @param source the source address of the (S,G) or (*,G) entry that has
     * changed. In case of group-specific multicast, it is IPvX::ZERO().
     * @param group the group address of the (S,G) or (*,G) entry that has
     * changed.
     * @param action_jp the membership change type (@ref action_jp_t):
     * either ACTION_JOIN or ACTION_PRUNE.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int join_prune_notify_routing(const string& module_instance_name,
				  xorp_module_id module_id,
				  uint16_t vif_index,
				  const IPvX& source,
				  const IPvX& group,
				  action_jp_t action_jp);

    //
    // Configuration methods
    //
    
    /**
     * Set the protocol version on an interface.
     * 
     * @param vif_name the name of the vif to set the protocol version of.
     * @param proto_version the new protocol version.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_proto_version(const string& vif_name,
				      int proto_version);
    
    /**
     * Reset the protocol version on an interface to its default value.
     * 
     * @param vif_name the name of the vif to reset the protocol version of
     * to its default value.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_proto_version(const string& vif_name);
    
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
    
private:
    buffer_t	*_buffer_recv;		// Buffer for receiving messages
    
    //
    // Debug and test-related state
    //
    bool	_is_log_trace;		// If true, enable XLOG_TRACE()
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_NODE_HH__
