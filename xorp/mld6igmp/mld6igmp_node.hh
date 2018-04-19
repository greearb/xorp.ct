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


#ifndef __MLD6IGMP_MLD6IGMP_NODE_HH__
#define __MLD6IGMP_MLD6IGMP_NODE_HH__


//
// IGMP and MLD node definition.
//

#include "libxorp/vif.hh"
#include "libproto/proto_node.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "mrt/buffer.h"
#include "mrt/multicast_defs.h"


class EventLoop;
class IPvX;
class IPvXNet;
class Mld6igmpVif;

/**
 * @short The MLD/IGMP node class.
 * 
 * There should be one node per MLD or IGMP instance. There should be
 * one instance per address family.
 */
class Mld6igmpNode : public ProtoNode<Mld6igmpVif>,
		     public IfMgrHintObserver,
		     public ServiceChangeObserverBase {
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
     * Start the MLD or IGMP protocol.
     * After the startup operations are completed,
     * @ref Mld6igmpNode::final_start() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the node operation.
     * 
     * Gracefully stop the MLD or IGMP protocol.
     * After the shutdown operations are completed,
     * @ref Mld6igmpNode::final_stop() is called internally
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();

    /**
     * Completely start the node operation.
     * 
     * This method should be called internally after @ref Mld6igmpNode::start()
     * to complete the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_start();

    /**
     * Completely stop the node operation.
     * 
     * This method should be called internally after @ref Mld6igmpNode::stop()
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
     * Get the IP protocol number.
     *
     * @return the IP protocol number.
     */
    uint8_t	ip_protocol_number() const;

    /**  Will create dummy VIF if needed (and if possible) */
    Mld6igmpVif* find_or_create_vif(const string& vif_name, string& error_msg);

    /**
     * Install a new MLD/IGMP vif.
     * 
     * @param vif vif information about the new Mld6igmpVif to install.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const Vif& vif, string& error_msg);
    
    /**
     * Install a new MLD/IGMP vif.
     * 
     * @param vif_name the name of the new vif.
     * @param vif_index the vif index of the new vif.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif(const string& vif_name, uint32_t vif_index,
			string& error_msg);
    
    /**
     * Delete an existing MLD/IGMP vif.
     * 
     * @param vif_name the name of the vif to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif(const string& vif_name, string& error_msg);
    
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
     * @param mtu the MTU of the vif.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_flags(const string& vif_name,
			      bool is_pim_register, bool is_p2p,
			      bool is_loopback, bool is_multicast,
			      bool is_broadcast, bool is_up, uint32_t mtu,
			      string& error_msg);
    
    /**
     * Add an address to a vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the unicast address to add.
     * @param subnet_addr the subnet address to add.
     * @param broadcast_addr the broadcast address (when applicable).
     * @param peer_addr the peer address (when applicable).
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_vif_addr(const string& vif_name,
			     const IPvX& addr,
			     const IPvXNet& subnet_addr,
			     const IPvX& broadcast_addr,
			     const IPvX& peer_addr,
			     string& error_msg);
    
    /**
     * Delete an address from a vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the unicast address to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_vif_addr(const string& vif_name,
				const IPvX& addr,
				string& error_msg);
    
    /**
     * Enable an existing MLD6IGMP vif.
     * 
     * @param vif_name the name of the vif to enable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_vif(const string& vif_name, string& error_msg);

    /**
     * Disable an existing MLD6IGMP vif.
     * 
     * @param vif_name the name of the vif to disable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		disable_vif(const string& vif_name, string& error_msg);

    /**
     * Start an existing MLD6IGMP vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_vif(const string& vif_name, string& error_msg);
    
    /**
     * Stop an existing MLD6IGMP vif.
     * 
     * @param vif_name the name of the vif to start.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_vif(const string& vif_name, string& error_msg);
    
    /**
     * Start MLD/IGMP on all enabled interfaces.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_all_vifs();
    
    /**
     * Stop MLD/IGMP on all interfaces it was running on.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
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
     * A method called when a vif has completed its shutdown.
     * 
     * @param vif_name the name of the vif that has completed its shutdown.
     */
    void	vif_shutdown_completed(const string& vif_name);

    /**
     * Receive a protocol packet.
     *
     * @param if_name the interface name the packet arrived on.
     * @param vif_name the vif name the packet arrived on.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_protocol the IP protocol number.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value, then
     * the received value is unknown.
     * @param ip_tos the Type of Service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, then the received value is unknown.
     * @param ip_router_alert if true, the IP Router Alert option was included
     * in the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_recv(const string& if_name,
			   const string& vif_name,
			   const IPvX& src_address,
			   const IPvX& dst_address,
			   uint8_t ip_protocol,
			   int32_t ip_ttl,
			   int32_t ip_tos,
			   bool ip_router_alert,
			   bool ip_internet_control,
			   const vector<uint8_t>& payload,
			   string& error_msg);
    
    /**
     * Send a protocol packet.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_protocol the IP protocol number. It must be between 1 and
     * 255.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     * TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, the TOS will be set internally before
     * transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param buffer the data buffer with the packet to send.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		mld6igmp_send(const string& if_name,
			      const string& vif_name,
			      const IPvX& src_address,
			      const IPvX& dst_address,
			      uint8_t ip_protocol,
			      int32_t ip_ttl,
			      int32_t ip_tos,
			      bool ip_router_alert,
			      bool ip_internet_control,
			      buffer_t *buffer,
			      string& error_msg);
    
    /**
     * Receive signal message: not used by MLD/IGMP.
     */
    int	signal_message_recv(const string&	, // src_module_instance_name,
			    int			, // message_type,
			    uint32_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // rcvbuf,
			    size_t		  // rcvlen
	) { XLOG_UNREACHABLE(); return (XORP_ERROR); }
    
    /**
     * Send signal message: not used by MLD/IGMP.
     */
    int	signal_message_send(const string&	, // dst_module_instance_name,
			    int			, // message_type,
			    uint32_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // sndbuf,
			    size_t		  // sndlen
	) { XLOG_UNREACHABLE(); return (XORP_ERROR); }

    /**
     * Register as a receiver to receive packets.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param enable_multicast_loopback if true then enable delivering
     * of multicast datagrams back to this host (assuming the host is
     * a member of the same multicast group).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int register_receiver(const string& if_name,
				  const string& vif_name,
				  uint8_t ip_protocol,
				  bool enable_multicast_loopback) = 0;

    /**
     * Unregister as a receiver to receive packets.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP Protocol number that the receiver is
     * not interested in anymore. It must be between 0 and 255. A protocol
     * number of 0 is used to specify all protocols.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int unregister_receiver(const string& if_name,
				    const string& vif_name,
				    uint8_t ip_protocol) = 0;

    /**
     * Join a multicast group on an interface.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * TODO: add a source address as well!!
     * 
     * @param if_name the interface name to join.
     * @param vif_name the vif name to join.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in.
     * @param group_address the multicast group address to join.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int join_multicast_group(const string& if_name,
				     const string& vif_name,
				     uint8_t ip_protocol,
				     const IPvX& group_address) = 0;
    /**
     * Leave a multicast group on an interface.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * TODO: add a source address as well!!
     * 
     * @param if_name the interface name to leave.
     * @param vif_name the vif name to leave.
     * @param ip_protocol the IP protocol number that the receiver is
     * not interested in anymore.
     * @param group_address the multicast group address to leave.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int leave_multicast_group(const string& if_name,
				      const string& vif_name,
				      uint8_t ip_protocol,
				      const IPvX& group_address) = 0;

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
		     uint32_t vif_index);
    
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
			uint32_t vif_index, string& error_msg);
    
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
				    uint32_t vif_index,
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
				       uint32_t vif_index,
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
				  uint32_t vif_index,
				  const IPvX& source,
				  const IPvX& group,
				  action_jp_t action_jp);

    /**
     * Test if an address is directly connected to a specified virtual
     * interface.
     * 
     * Note that the virtual interface the address is directly connected to
     * must be UP.
     * 
     * @param mld6igmp_vif the virtual interface to test against.
     * @param ipaddr_test the address to test.
     * @return true if @ref ipaddr_test is directly connected to @ref vif,
     * otherwise false.
     */
    bool is_directly_connected(const Mld6igmpVif& mld6igmp_vif,
			       const IPvX& ipaddr_test) const;

    //
    // Configuration methods
    //

    /**
     * Complete the set of vif configuration changes.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_all_vifs_done(string& error_msg);

    /**
     * Get the protocol version on an interface.
     * 
     * @param vif_name the name of the vif to get the protocol version of.
     * @param proto_version the return-by-reference protocol version.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_proto_version(const string& vif_name,
				      int& proto_version,
				      string& error_msg);
    
    /**
     * Set the protocol version on an interface.
     * 
     * @param vif_name the name of the vif to set the protocol version of.
     * @param proto_version the new protocol version.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_proto_version(const string& vif_name,
				      int proto_version,
				      string& error_msg);
    
    /**
     * Reset the protocol version on an interface to its default value.
     * 
     * @param vif_name the name of the vif to reset the protocol version of
     * to its default value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_proto_version(const string& vif_name,
					string& error_msg);

    /**
     * Get the value of the flag that enables/disables the IP Router Alert
     * option check per interface for received packets.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param enabled the return-by-reference flag value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_ip_router_alert_option_check(const string& vif_name,
						     bool& enabled,
						     string& error_msg);
    
    /**
     * Enable/disable the IP Router Alert option check per interface for
     * received packets.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param enable if true, then enable the IP Router Alert option check,
     * otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_ip_router_alert_option_check(const string& vif_name,
						     bool enable,
						     string& error_msg);
    
    /**
     * Reset the value of the flag that enables/disables the IP Router Alert
     * option check per interface for received packets to its default value.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_ip_router_alert_option_check(const string& vif_name,
						       string& error_msg);

    /**
     * Get the Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the return-by-reference interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_query_interval(const string& vif_name,
				       TimeVal& interval,
				       string& error_msg);
    
    /**
     * Set the Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_query_interval(const string& vif_name,
				       const TimeVal& interval,
				       string& error_msg);
    
    /**
     * Reset the Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_query_interval(const string& vif_name,
					 string& error_msg);

    /**
     * Get the Last Member Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the return-by-reference interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_query_last_member_interval(const string& vif_name,
						   TimeVal& interval,
						   string& error_msg);
    
    /**
     * Set the Last Member Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_query_last_member_interval(const string& vif_name,
						   const TimeVal& interval,
						   string& error_msg);
    
    /**
     * Reset the Last Member Query Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_query_last_member_interval(const string& vif_name,
						     string& error_msg);

    /**
     * Get the Query Response Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the return-by-reference interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_query_response_interval(const string& vif_name,
						TimeVal& interval,
						string& error_msg);
    
    /**
     * Set the Query Response Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param interval the interval.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_query_response_interval(const string& vif_name,
						const TimeVal& interval,
						string& error_msg);
    
    /**
     * Reset the Query Response Interval per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_query_response_interval(const string& vif_name,
						  string& error_msg);

    /**
     * Get the Robustness Variable count per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param robust_count the return-by-reference count value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_robust_count(const string& vif_name,
				     uint32_t& robust_count,
				     string& error_msg);
    
    /**
     * Set the Robustness Variable count per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param robust_count the count value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_vif_robust_count(const string& vif_name,
				     uint32_t robust_count,
				     string& error_msg);
    
    /**
     * Reset the  Robustness Variable count per interface.
     * 
     * @param vif_name the name of the vif to apply to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		reset_vif_robust_count(const string& vif_name,
				       string& error_msg);
    
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
    bool is_log_trace() const { return (_is_log_trace); }
    
    /**
     * Enable/disable trace log.
     * 
     * This method is used to enable/disable trace log debug messages output.
     * 
     * @param is_enabled if true, trace log is enabled, otherwise is disabled.
     */
    void	set_log_trace(bool is_enabled) { _is_log_trace = is_enabled; }

protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

private:
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

    /**
     * Get a reference to the service base of the interface manager.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @return a reference to the service base of the interface manager.
     */
    virtual const ServiceBase* ifmgr_mirror_service_base() const = 0;

    /**
     * Get a reference to the interface manager tree.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @return a reference to the interface manager tree.
     */
    virtual const IfMgrIfTree&	ifmgr_iftree() const = 0;

    /**
     * Initiate registration with the FEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void fea_register_startup() = 0;

    /**
     * Initiate registration with the MFEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void mfea_register_startup() = 0;

    /**
     * Initiate de-registration with the FEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void fea_register_shutdown() = 0;

    /**
     * Initiate de-registration with the MFEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void mfea_register_shutdown() = 0;

    buffer_t	*_buffer_recv;		// Buffer for receiving messages
    
    //
    // Status-related state
    //
    size_t	_waiting_for_mfea_startup_events;

    //
    // A local copy with the interface state information
    //
    IfMgrIfTree	_iftree;
    
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
