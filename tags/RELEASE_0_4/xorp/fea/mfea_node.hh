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

// $XORP: xorp/fea/mfea_node.hh,v 1.11 2003/08/06 18:50:22 pavlin Exp $


#ifndef __FEA_MFEA_NODE_HH__
#define __FEA_MFEA_NODE_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) node definition.
//


#include <vector>

#include "libxorp/ipvx.hh"
#include "libxorp/config_param.hh"
#include "libproto/proto_node.hh"
#include "libproto/proto_register.hh"
#include "mrt/mrib_table.hh"
#include "mrt/mifset.hh"
#include "mfea_dataflow.hh"
#include "mfea_mrouter.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class MfeaVif;
class ProtoComm;
class SgCount;
class VifCount;
class FtiConfig;	// TODO: XXX: PAVPAVPAV: temporary here!!

// The MFEA node
/**
 * @short The MFEA node class.
 * 
 * There should be one node per MFEA instance. There should be
 * one instance per address family.
 */
class MfeaNode : public ProtoNode<MfeaVif> {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param module_id the module ID (@ref xorp_module_id). Should be
     * equal to XORP_MODULE_MFEA.
     * @param eventloop the event loop to use.
     * @param ftic the @ref FtiConfig entry to use to obtain the routing
     * table (NOTE: this parameter is only temporary here, and will
     * be removed in the near future).
     */
    MfeaNode(int family, xorp_module_id module_id, EventLoop& eventloop,
	     FtiConfig& ftic);
    
    /**
     * Destructor
     */
    virtual	~MfeaNode();

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
     * Install a new MFEA vif.
     * 
     * @param vif vif information about new MfeaVif to install.
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
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_config_vif(const Vif& vif, string& error_msg);
    
    /**
     * Add a configured vif.
     * 
     * @param vif_name the name of the vif to add.
     * @param vif_index the vif index of the vif to add.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_config_vif(const string& vif_name,
			       uint16_t vif_index,
			       string& error_msg);
    
    /**
     * Delete a configured vif.
     * 
     * @param vif_name the name of the vif to delete.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_config_vif(const string& vif_name,
				  string& error_msg);

    /**
     * Add an address to a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the address to add.
     * @param subnet the subnet address to add.
     * @param broadcast the broadcast address to add.
     * @param peer the peer address to add.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_config_vif_addr(const string& vif_name,
				    const IPvX& addr,
				    const IPvXNet& subnet,
				    const IPvX& broadcast,
				    const IPvX& peer,
				    string& error_msg);
    
    /**
     * Delete an address from a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the address to delete.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_config_vif_addr(const string& vif_name,
				       const IPvX& addr,
				       string& error_msg);
    
    /**
     * Set the pif_index to a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param pif_index the physical interface index.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_pif_index(const string& vif_name,
				     uint16_t pif_index,
				     string& error_msg);
    
    /**
     * Set the vif flags to a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_vif_flags(const string& vif_name,
				     bool is_pim_register,
				     bool is_p2p,
				     bool is_loopback,
				     bool is_multicast,
				     bool is_broadcast,
				     bool is_up,
				     string& error_msg);
    
    /**
     * Complete the set of vif configuration changes.
     * 
     * @param error_msg the error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_all_vifs_done(string& error_msg);
    
    /**
     * Send to a client to add a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif to add.
     * @param vif_index the vif index of the vif to add.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_add_config_vif(const string& dst_module_instance_name,
				    xorp_module_id dst_module_id,
				    const string& vif_name,
				    uint16_t vif_index) = 0;
    
    /**
     * Send to a client to delete a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif to delete.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_delete_config_vif(const string& dst_module_instance_name,
				       xorp_module_id dst_module_id,
				       const string& vif_name) = 0;
    
    /**
     * Send to a client to add an address to a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param addr the address to add.
     * @param subnet the subnet address to add.
     * @param broadcast the broadcast address to add.
     * @param peer the peer address to add.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_add_config_vif_addr(const string& dst_module_instance_name,
					 xorp_module_id dst_module_id,
					 const string& vif_name,
					 const IPvX& addr,
					 const IPvXNet& subnet,
					 const IPvX& broadcast,
					 const IPvX& peer) = 0;
    
    /**
     * Send to a client to delete an address from a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param addr the address to delete.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_delete_config_vif_addr(const string& dst_module_instance_name,
					    xorp_module_id dst_module_id,
					    const string& vif_name,
					    const IPvX& addr) = 0;
    
    /**
     * Send to a client to set the vif flags to a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_set_config_vif_flags(const string& dst_module_instance_name,
					  xorp_module_id dst_module_id,
					  const string& vif_name,
					  bool is_pim_register,
					  bool is_p2p,
					  bool is_loopback,
					  bool is_multicast,
					  bool is_broadcast,
					  bool is_up) = 0;
    
    /**
     * Send to a client to complete the set of vif configuration changes.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_set_config_all_vifs_done(const string& dst_module_instance_name,
					      xorp_module_id dst_module_id) = 0;
    
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
     * @return the number of virtual interfaces MFEA was started on,
     * or XORP_ERROR if error occured.
     */
    int		start_all_vifs();
    
    /**
     * Stop MFEA on all interfaces it was running on.
     * 
     * @return the number of virtual interfaces MFEA was stopped on,
     * or XORP_ERROR if error occured.
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
     * Start operation for a given protocol.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to start.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_protocol(xorp_module_id module_id);
    
    /**
     * Stop operation for a given protocol.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to stop.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_protocol(xorp_module_id module_id);
    
    /**
     * A method used by a protocol instance to register with this MfeaNode.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to add.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_protocol(const string& module_instance_name,
			     xorp_module_id module_id);
    
    /**
     * A method used by a protocol instance to deregister with this MfeaNode.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to delete.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_protocol(const string& module_instance_name,
				xorp_module_id module_id);
    
    /**
     * Start a protocol on an interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to start on the interface.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to start on the interface.
     * @param vif_index the vif index of the interface to start the
     * protocol on.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_protocol_vif(const string& module_instance_name,
				   xorp_module_id module_id,
				   uint16_t vif_index);

    /**
     * Stop a protocol on an interface.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to stop on the interface.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to stop on the interface.
     * @param vif_index the vif index of the interface to stop the
     * protocol on.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_protocol_vif(const string& module_instance_name,
				  xorp_module_id module_id,
				  uint16_t vif_index);
    
    /**
     * Add a protocol to receive kernel signal messages.
     * 
     * Add a protocol to the set of protocols that are interested in
     * receiving kernel signal messages.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to add.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_allow_kernel_signal_messages(const string& module_instance_name,
						 xorp_module_id module_id);
    
    /**
     * Delete a protocol from receiving kernel signal messages.
     * 
     * Delete a protocol from the set of protocols that are interested in
     * receiving kernel signal messages.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to delete.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_allow_kernel_signal_messages(const string& module_instance_name,
						    xorp_module_id module_id);
    
    /**
     * Add a protocol to receive MRIB messages.
     * 
     * Add a protocol to the set of protocols that are interested in
     * receiving MRIB (see @ref Mrib) messages.
     * 
     * The MRIB (see @ref Mrib) messages contain (unicast) routing
     * information that can be used for obtaining the RPF (Reverse-Path
     * Forwarding) information.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to add.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_allow_mrib_messages(const string& module_instance_name,
					xorp_module_id module_id);
    
    /**
     * Delete a protocol from receiving MRIB messages.
     * 
     * Delete a protocol to the set of protocols that are interested in
     * receiving MRIB (see @ref Mrib) messages.
     * 
     * The MRIB (see @ref Mrib) messages contain (unicast) routing
     * information that can be used for obtaining the RPF (Reverse-Path
     * Forwarding) information.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to delete.
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_allow_mrib_messages(const string& module_instance_name,
					   xorp_module_id module_id);
    
    /**
     * Add a @ref Mrib entry to user-level protocols.
     * 
     * Add a @ref Mrib entry to all user-level protocols that are
     * interested in receiving MRIB messages.
     * 
     * @param mrib the @ref Mrib entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_add_mrib(const Mrib& mrib);
    
    /**
     * Delete a @ref Mrib entry from user-level protocols.
     * 
     * Delete a @ref Mrib entry from all user-level protocols that are
     * interested in receiving MRIB messages.
     * 
     * @param mrib the @ref Mrib entry to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_delete_mrib(const Mrib& mrib);
    
    /**
     * Complete a transaction of add/delete @ref Mrib entries.
     * 
     * Complete a transaction of add/delete @ref Mrib entries with all
     * user-level protocols that are interested in receiving MRIB messages.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_set_mrib_done();
    
    /**
     * Add a @ref Mrib entry to an user-level protocol.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @param mrib the @ref Mrib entry to add.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_add_mrib(const string& dst_module_instance_name,
			      xorp_module_id dst_module_id,
			      const Mrib& mrib) = 0;

    /**
     * Delete a @ref Mrib entry from an user-level protocol.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @param mrib the @ref Mrib entry to delete.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_delete_mrib(const string& dst_module_instance_name,
				 xorp_module_id dst_module_id,
				 const Mrib& mrib) = 0;

    /**
     * Signal completion of a transaction of add/delete @ref Mrib entries
     * to an user-level protocol.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	send_set_mrib_done(const string& dst_module_instance_name,
				   xorp_module_id dst_module_id) = 0;
    
    /**
     * Receive a protocol message from an user-level protocol.
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
     * the TTL will be set by the lower layers (including the MFEA).
     * 
     * @param ip_tos the IP TOS of the message. If it has a negative value,
     * the TOS will be set by the lower layers (including the MFEA).
     * 
     * @param router_alert_bool if true, set the Router Alert IP option for
     * the IP packet of the outgoung message.
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
     * Process an incoming message from the kernel.
     * 
     * This method sends the protocol message to an user-level protocol module.
     * Note: it uses the pure virtual ProtoNode::proto_send() method
     * that is implemented somewhere else (at a class that inherits this one).
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @param vif_index the vif index of the interface used to receive this
     * message.
     * 
     * @param src the source address of the message.
     * 
     * @param dst the destination address of the message.
     * 
     * @param ip_ttl the IP TTL (Time To Live) of the message. If it has
     * a negative value, it should be ignored.
     * 
     * @param ip_tos the IP TOS (Type of Service) of the message. If it has
     * a negative value, it should be ignored.
     * 
     * @param router_alert_bool if true, the Router Alert IP option for the IP
     * packet of the incoming message was set.
     * 
     * @param rcvbuf the data buffer with the received message.
     * 
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_comm_recv(xorp_module_id dst_module_id,
				uint16_t vif_index,
				const IPvX& src, const IPvX& dst,
				int ip_ttl, int ip_tos, bool router_alert_bool,
				const uint8_t *rcvbuf, size_t rcvlen);
    
    /**
     * Process NOCACHE, WRONGVIF/WRONGMIF, WHOLEPKT signals from the
     * kernel.
     * 
     * The signal is sent to all user-level protocols that expect it.
     * 
     * @param src_module_instance_name unused.
     * @param src_module_id the @ref xorp_module_id module ID of the
     * associated @ref ProtoComm entry. Note: in the future it may become
     * irrelevant.
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
				    xorp_module_id src_module_id,
				    int message_type,
				    uint16_t vif_index,
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
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
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
				     xorp_module_id dst_module_id,
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
     * Join a multicast group.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to join the multicast group.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to join the multicast group.
     * 
     * @param vif_index the vif index of the interface to join.
     * 
     * @param group the multicast group to join.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(const string& module_instance_name,
				     xorp_module_id module_id,
				     uint16_t vif_index,
				     const IPvX& group);
    
    /**
     * Leave a multicast group.
     * 
     * @param module_instance_name the module instance name of the protocol
     * to leave the multicast group.
     * 
     * @param module_id the module ID (@ref xorp_module_id) of the protocol
     * to leave the multicast group.
     * 
     * @param vif_index the vif index of the interface to leave.
     * 
     * @param group the multicast group to leave.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(const string& module_instance_name,
				      xorp_module_id module_id,
				      uint16_t vif_index,
				      const IPvX& group);
    
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
    int		add_mfc(const string& module_instance_name,
			const IPvX& source, const IPvX& group,
			uint16_t iif_vif_index, const Mifset& oiflist,
			const Mifset& oiflist_disable_wrongvif,
			uint32_t max_vifs_oiflist,
			const IPvX& rp_addr);
    
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
    int		delete_mfc(const string& module_instance_name,
			   const IPvX& source, const IPvX& group);
    
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
				     bool is_leq_upcall);
    
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
					bool is_leq_upcall);
    
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_all_dataflow_monitor(const string& module_instance_name,
					    const IPvX& source,
					    const IPvX& group);
    
    /**
     * Add a multicast vif to the kernel.
     * 
     * @param vif_index the vif index of the interface to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_multicast_vif(uint16_t vif_index);
    
    /**
     * Delete a multicast vif from the kernel.
     * 
     * @param vif_index the vif index of the interface to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_multicast_vif(uint16_t vif_index);
    
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
    int		get_vif_count(uint16_t vif_index, VifCount& vif_count);
    
    /**
     * Get a reference to the local copy of the MRIB table (@ref MribTable).
     * 
     * @return a reference to the local copy of the MRIB table
     * (@ref MribTable).
     */
    MribTable&	mrib_table() { return (_mrib_table); }
    
    /**
     * Get a reference to the default metric preference for the MRIB table.
     * 
     * Note that this is a configurable parameter.
     * 
     * @return a reference to the default metric preference for the MRIB table.
     */
    ConfigParam<uint32_t>& mrib_table_default_metric_preference() {
	return (_mrib_table_default_metric_preference);
    }
    
    /**
     * Set the default metric preference for the MRIB table.
     * 
     * @param metric_preference the new value for the default metric
     * preference.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_mrib_table_default_metric_preference(uint32_t metric_preference);
    
    /**
     * Reset the default metric preference for the MRIB table to its original
     * value.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int reset_mrib_table_default_metric_preference();
    
    /**
     * Get a reference to the default metric for the MRIB table.
     * 
     * Note that this is a configurable parameter.
     * 
     * @return a reference to the default metric for the MRIB table.
     */
    ConfigParam<uint32_t>& mrib_table_default_metric() {
	return (_mrib_table_default_metric);
    }

    /**
     * Set the default metric for the MRIB table.
     * 
     * @param metric the new value for the default metric.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_mrib_table_default_metric(uint32_t metric);
    
    /**
     * Reset the default metric for the MRIB table to its original value.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int reset_mrib_table_default_metric();
    
    /**
     * Get a copy of the kernel MRIB (@ref Mrib) information.
     * 
     * @param mrib_table a pointer to the routing table array composed
     * of @ref Mrib elements.
     * @return The number of entries in @ref mrib_table, or XORP_ERROR
     * if there was an error.
     */
    int		get_mrib_table(Mrib **mrib_table);
    
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
     * Get a reference to the vector-array of installed @ref ProtoComm entries.
     * 
     * @return a reference to the vector-array of installed @ref ProtoComm
     * entries.
     */
    vector<ProtoComm *>& proto_comms() { return (_proto_comms); }
    
    /**
     * Find an @ref ProtoComm entry for a given module ID
     * (@ref xorp_module_id).
     * 
     * @param module_id the module ID (@ref xorp_module_id) to search for.
     * @return the corresponding @ref ProtoComm entry if found, otherwise NULL.
     */
    ProtoComm	*proto_comm_find_by_module_id(xorp_module_id module_id) const;

    /**
     * Find an @ref ProtoComm entry for a given IP protocol number.
     * 
     * @param ipproto the IP protocol number ot search for.
     * @return the corresponding @ref ProtoComm entry if found, otherwise NULL.
     */
    ProtoComm	*proto_comm_find_by_ipproto(int ipproto) const;
    
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
    // Private methods
    void mrib_table_read_timer_timeout();
    int add_pim_register_vif();
    
    // Private state
    MfeaMrouter		_mfea_mrouter;	// The mrouter state
    vector<ProtoComm *>	_proto_comms;	// The set of active ProtoComm entries
    
    MribTable	_mrib_table;		// The MRIB table (XXX: optional)
#define MRIB_TABLE_DEFAULT_METRIC_PREFERENCE	100
#define MRIB_TABLE_DEFAULT_METRIC		100
    ConfigParam<uint32_t> _mrib_table_default_metric_preference;
    ConfigParam<uint32_t> _mrib_table_default_metric;
#define MRIB_TABLE_READ_PERIOD_SEC 10
#define MRIB_TABLE_READ_PERIOD_USEC 0
    XorpTimer	_mrib_table_read_timer;	// Timer to (re)read the MRIB table
    MfeaDft	_mfea_dft;		// The dataflow monitoring table
    
    //
    // The state to register:
    //  - protocol instances
    //  - protocol instances interested in receiving kernel signal messages
    //  - protocol instances interested in receiving MRIB messages
    ProtoRegister	_proto_register;
    ProtoRegister	_kernel_signal_messages_register;
    ProtoRegister	_mrib_messages_register;
    
    //
    // Debug and test-related state
    //
    bool	_is_log_trace;		// If true, enable XLOG_TRACE()
    
    //
    FtiConfig&	_ftic;			// TODO: XXX: PAVPAVPAV: temp. here!!
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __FEA_MFEA_NODE_HH__
