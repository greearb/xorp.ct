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

// $XORP: xorp/mld6igmp/mld6igmp_vif.hh,v 1.13 2004/02/25 02:38:40 pavlin Exp $

#ifndef __MLD6IGMP_MLD6IGMP_VIF_HH__
#define __MLD6IGMP_MLD6IGMP_VIF_HH__


//
// IGMP and MLD virtual interface definition.
//


#include <list>
#include <utility>

#include "libxorp/timer.hh"
#include "libxorp/vif.hh"
#include "libproto/proto_unit.hh"
#include "mrt/multicast_defs.h"
#include "igmp_proto.h"
#include "mld6_proto.h"
#include "mld6igmp_node.hh"
#include "mld6igmp_member_query.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//
class	MemberQuery;

/**
 * @short A class for MLD/IGMP-specific virtual interface.
 */
class Mld6igmpVif : public ProtoUnit, public Vif {
public:
    /**
     * Constructor for a given MLD/IGMP node and a generic virtual interface.
     * 
     * @param mld6igmp_node the @ref Mld6igmpNode this interface belongs to.
     * @param vif the generic Vif interface that contains various information.
     */
    Mld6igmpVif(Mld6igmpNode& mld6igmp_node, const Vif& vif);
    
    /**
     * Destructor
     */
    virtual ~Mld6igmpVif();

    /**
     * Set the current protocol version.
     * 
     * The protocol version must be in the interval
     * [IGMP_VERSION_MIN, IGMP_VERSION_MAX]
     * or [MLD_VERSION_MIN, MLD_VERSION_MAX]
     * 
     * @param proto_version the protocol version to set.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_proto_version(int proto_version);
    
    /**
     *  Start MLD/IGMP on a single virtual interface.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start(string& error_msg);
    
    /**
     *  Stop MLD/IGMP on a single virtual interface.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop(string& error_msg);

    /**
     * Receive a protocol message.
     * 
     * @param src the source address of the message.
     * @param dst the destination address of the message.
     * @param ip_ttl the IP TTL of the message. If it has a negative value
     * it should be ignored.
     * @param ip_ttl the IP TOS of the message. If it has a negative value,
     * it should be ignored.
     * @param router_alert_bool if true, the IP Router Alert option in
     * the IP packet was set (when applicable).
     * @param buffer the data buffer with the received message.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		mld6igmp_recv(const IPvX& src, const IPvX& dst,
			      int ip_ttl, int ip_tos, bool router_alert_bool,
			      buffer_t *buffer);
    
    /**
     * Get the string with the flags about the vif status.
     * 
     * TODO: temporary here. Should go to the Vif class after the Vif
     * class starts using the Proto class.
     * 
     * @return the C++ style string with the flags about the vif status
     * (e.g., UP/DOWN/DISABLED, etc).
     */
    string	flags_string() const;
    
    /**
     * Get the MLD6IGMP node (@ref Mld6igmpNode).
     * 
     * @return a reference to the MLD6IGMP node (@ref Mld6igmpNode).
     */
    Mld6igmpNode& mld6igmp_node() const { return (_mld6igmp_node); }

    /**
     * Get my primary address on this interface.
     * 
     * @return my primary address on this interface.
     */
    const IPvX&	primary_addr() const	{ return (_primary_addr); }

    /**
     * Set my primary address on this interface.
     * 
     * @param v the value of the primary address.
     */
    void	set_primary_addr(const IPvX& v) { _primary_addr = v; }

    /**
     * Update the primary address.
     * 
     * The primary address should be a link-local unicast address, and
     * is used for transmitting the multicast control packets on the LAN.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		update_primary_address(string& error_msg);
    
    /**
     * Get the MLD/IGMP querier address.
     * 
     * @return the MLD/IGMP querier address.
     */
    const IPvX&	querier_addr()	const		{ return (_querier_addr); }
    
    /**
     * Set the MLD6/IGMP querier address.
     * 
     * @param v the value of the MLD/IGMP querier address.
     */
    void	set_querier_addr(const IPvX& v) { _querier_addr = v;	}
    
    /**
     * Get the list with the multicast membership
     * information (@ref MemberQuery).
     * 
     * @return the list with the multicast membership
     * information (@ref MemberQuery).
     */
    const list<MemberQuery *>& members() const { return (_members); }
    
    /**
     * Test if the protocol is Source-Specific Multicast (e.g., IGMPv3
     * or MLDv2).
     * 
     * @return true if the protocol is Source-Specific Multicast (e.g., IGMPv3
     * or MLDv2).
     */
    bool	proto_is_ssm() const;
    
    /**
     * Get the timer to timeout the (other) MLD/IGMP querier.
     * 
     * @return a reference to the timer to timeout the (other)
     * MLD/IGMP querier.
     *
     */
    const XorpTimer& const_other_querier_timer() const { return (_other_querier_timer); }
    
    //
    // Add/delete routing protocols that need to be notified for membership
    // changes.
    //

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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_protocol(xorp_module_id module_id,
		     const string& module_instance_name);
    
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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_protocol(xorp_module_id module_id,
			const string& module_instance_name);
    
private:
    friend class MemberQuery;
    
    //
    // Private functions
    //
    bool	is_igmpv1_mode() const;	// XXX: applies only to IGMP
    const char	*proto_message_type2ascii(uint8_t message_type) const;
    buffer_t	*buffer_send_prepare();
    int		join_prune_notify_routing(const IPvX& source,
					  const IPvX& group,
					  action_jp_t action_jp) const;
    
    //
    // Private state
    //
    Mld6igmpNode& _mld6igmp_node;	// The MLD6IGMP node I belong to
    buffer_t	*_buffer_send;		// Buffer for sending messages
#define MLD6IGMP_VIF_QUERIER  0x00000001U // I am the querier
    uint32_t	_proto_flags;		// Various flags (MLD6IGMP_VIF_*)
    IPvX	_primary_addr;		// The primary address on this vif
    IPvX	_querier_addr;		// IP address of the current querier
    XorpTimer	_other_querier_timer;	// To timeout the (other) 'querier'
    XorpTimer	_query_timer;		// Timer to send queries
    XorpTimer	_igmpv1_router_present_timer;	// IPGPv1 router present timer
						// XXX: does not apply to MLD
    uint8_t	_startup_query_count;	// Number of queries to send quickly
					// during startup
    list<MemberQuery *> _members;	// List of all groups with members
    
    //
    // Misc. other state
    //
    // Registered protocols to notify for membership change.
    vector<pair<xorp_module_id, string> > _notify_routing_protocols;
    bool _dummy_flag;			// Dummy flag
    
    //
    // Not-so handy private functions that should go somewhere else
    //
    int		mld6igmp_send(const IPvX& src, const IPvX& dst,
			      uint8_t message_type, int max_resp_time,
			      const IPvX& group_address);
    int		igmp_process(const IPvX& src, const IPvX& dst,
			     int ip_ttl, int ip_tos, bool router_alert_bool,
			     buffer_t *buffer);
    int		mld6_process(const IPvX& src, const IPvX& dst,
			     int ip_ttl, int ip_tos, bool router_alert_bool,
			     buffer_t *buffer);
    
    int		igmp_membership_query_recv(const IPvX& src,
					   const IPvX& dst,
					   uint8_t message_type,
					   int igmp_max_resp_time,
					   const IPvX& group_address,
					   buffer_t *buffer);
    int		igmp_membership_report_recv(const IPvX& src,
					    const IPvX& dst,
					    uint8_t message_type,
					    int igmp_max_resp_time,
					    const IPvX& group_address,
					    buffer_t *buffer);
    int		igmp_leave_group_recv(const IPvX& src,
				      const IPvX& dst,
				      uint8_t message_type,
				      int igmp_max_resp_time,
				      const IPvX& group_address,
				      buffer_t *buffer);
    int		igmp_dvmrp_ask_neighbors_recv(const IPvX& src,
					      const IPvX& dst,
					      uint8_t message_type,
					      int igmp_max_resp_time,
					      const IPvX& group_address,
					      buffer_t *buffer);
    int		igmp_dvmrp_ask_neighbors2_recv(const IPvX& src,
					       const IPvX& dst,
					       uint8_t message_type,
					       int igmp_max_resp_time,
					       const IPvX& group_address,
					       buffer_t *buffer);
    int		igmp_dvmrp_info_request_recv(const IPvX& src,
					     const IPvX& dst,
					     uint8_t message_type,
					     int igmp_max_resp_time,
					     const IPvX& group_address,
					     buffer_t *buffer);
    int		igmp_mtrace_recv(const IPvX& src,
				 const IPvX& dst,
				 uint8_t message_type,
				 int igmp_max_resp_time,
				 const IPvX& group_address,
				 buffer_t *buffer);
    int		igmp_v1_config_consistency_check(const IPvX& src,
						 const IPvX& dst,
						 uint8_t message_type,
						 int igmp_message_version);
    
    int		mld6_listener_query_recv(const IPvX& src,
					 const IPvX& dst,
					 uint8_t mld6_message_type,
					 int mld6_max_resp_time,
					 const IPvX& group_address,
					 buffer_t *buffer);
    int		mld6_listener_report_recv(const IPvX& src,
					  const IPvX& dst,
					  uint8_t mld6_message_type,
					  int mld6_max_resp_time,
					  const IPvX& group_address,
					  buffer_t *buffer);
    int		mld6_listener_done_recv(const IPvX& src,
					const IPvX& dst,
					uint8_t mld6_message_type,
					int mld6_max_resp_time,
					const IPvX& group_address,
					buffer_t *buffer);
    int		mld6_mtrace_recv(const IPvX& src,
				 const IPvX& dst,
				 uint8_t mld6_message_type,
				 int mld6_max_resp_time,
				 const IPvX& group_address,
				 buffer_t *buffer);
    
    void	other_querier_timer_timeout();
    
    void	query_timer_timeout();
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_VIF_HH__
