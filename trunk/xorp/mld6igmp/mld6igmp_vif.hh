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

// $XORP: xorp/mld6igmp/mld6igmp_vif.hh,v 1.50 2008/07/23 05:11:04 pavlin Exp $

#ifndef __MLD6IGMP_MLD6IGMP_VIF_HH__
#define __MLD6IGMP_MLD6IGMP_VIF_HH__


//
// IGMP and MLD virtual interface definition.
//


#include <utility>

#include "libxorp/config_param.hh"
#include "libxorp/timer.hh"
#include "libxorp/vif.hh"
#include "libproto/proto_unit.hh"
#include "mrt/buffer.h"
#include "mrt/multicast_defs.h"
#include "igmp_proto.h"
#include "mld6_proto.h"
#include "mld6igmp_node.hh"
#include "mld6igmp_group_record.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

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
    virtual	~Mld6igmpVif();

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
     * Enable MLD/IGMP on a single virtual interface.
     * 
     * If an unit is not enabled, it cannot be start, or pending-start.
     */
    void	enable();
    
    /**
     * Disable MLD/IGMP on a single virtual interface.
     * 
     * If an unit is disabled, it cannot be start or pending-start.
     * If the unit was runnning, it will be stop first.
     */
    void	disable();

    /**
     * Receive a protocol message.
     * 
     * @param src the source address of the message.
     * @param dst the destination address of the message.
     * @param ip_ttl the IP TTL of the message. If it has a negative value
     * it should be ignored.
     * @param ip_ttl the IP TOS of the message. If it has a negative value,
     * it should be ignored.
     * @param ip_router_alert if true, the IP Router Alert option in the IP
     * packet was set (when applicable).
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param buffer the data buffer with the received message.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		mld6igmp_recv(const IPvX& src, const IPvX& dst,
			      int ip_ttl, int ip_tos, bool ip_router_alert,
			      bool ip_internet_control,
			      buffer_t *buffer, string& error_msg);
    
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
     * Get the set with the multicast group records information
     * (@ref Mld6igmpGroupSet).
     * 
     * @return the set with the multicast group records information
     * (@ref Mld6igmpGroupSet).
     */
    Mld6igmpGroupSet& group_records() { return (_group_records); }

    /**
     * Get the const set with the multicast group records information
     * (@ref Mld6igmpGroupSet).
     * 
     * @return the const set with the multicast group records information
     * (@ref Mld6igmpGroupSet).
     */
    const Mld6igmpGroupSet& group_records() const { return (_group_records); }

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

    /**
     * Optain a reference to the "IP Router Alert option check" flag.
     *
     * @return a reference to the "IP Router Alert option check" flag.
     */
    ConfigParam<bool>& ip_router_alert_option_check() { return (_ip_router_alert_option_check); }
    
    /**
     * Optain a reference to the configured Query Interval.
     *
     * @return a reference to the configured Query Interval.
     */
    ConfigParam<TimeVal>& configured_query_interval() { return (_configured_query_interval); }

    /**
     * Get the effective Query Interval value.
     *
     * Note that this value may be modified by reconfiguring the router,
     * or by the Query message from the current Querier.
     *
     * @return the value of the effective Query Interval.
     */
    const TimeVal& effective_query_interval() const { return (_effective_query_interval); }

    /**
     * Set the effective Query Interval.
     *
     * Note that this value may be modified by reconfiguring the router,
     * or by the Query message from the current Querier.
     *
     * @param v the value of the effective Query Interval.
     */
    void	set_effective_query_interval(const TimeVal& v);

    /**
     * Optain a reference to the Last Member Query Interval.
     *
     * @return a reference to the Last Member Query Interval.
     */
    ConfigParam<TimeVal>& query_last_member_interval() { return (_query_last_member_interval); }

    /**
     * Optain a reference to the Query Response Interval.
     *
     * @return a reference to the Query Response Interval.
     */
    ConfigParam<TimeVal>& query_response_interval() { return (_query_response_interval); }

    /**
     * Optain a reference to the configured Robustness Variable count.
     *
     * @return a reference to the configured Robustness Variable count.
     */
    ConfigParam<uint32_t>& configured_robust_count() { return (_configured_robust_count); }

    /**
     * Get the effective Robustness Variable value.
     *
     * Note that this value may be modified by reconfiguring the router,
     * or by the Query messages from the current Querier.
     *
     * @return the value of the effective Robustness Variable.
     */
    uint32_t	effective_robustness_variable() const { return (_effective_robustness_variable); }

    /**
     * Set the effective Robustness Variable.
     *
     * Note that this value may be modified by reconfiguring the router,
     * or by the Query messages from the current Querier.
     *
     * @param v the value of the effective Robustness Variable.
     */
    void	set_effective_robustness_variable(uint32_t v);

    /**
     * Get the Last Member Query Count value.
     *
     * Note: According to the IGMP/MLD spec, the default value for the
     * Last Member Query Count is the Robustness Variable.
     * Hence, the Last Member Query Count itself should be configurable.
     * For simplicity (and for consistency with other router vendors), it
     * is always same as the Robustness Variable.
     *
     * @return the value of the Last Member Query Count.
     */
    uint32_t	last_member_query_count() const { return (_last_member_query_count); }

    /**
     * Obtain a reference to the Group Membership Interval.
     *
     * Note that it is not directly configurable, but may be tuned by
     * changing the values of the parameters it depends on.
     *
     * @return a reference to the Group Membership Interval.
     */
    const TimeVal& group_membership_interval() const { return (_group_membership_interval); }

    /**
     * Obtain a reference to the Last Member Query Time.
     *
     * Note that it is not directly configurable, but may be tuned by
     * changing the values of the parameters it depends on.
     *
     * @return a reference to the Last Member Query Time.
     */
    const TimeVal& last_member_query_time() const { return (_last_member_query_time); }

    /**
     * Obtain a reference to the Older Version Host Present Interval.
     *
     * Note that it is not directly configurable, but may be tuned by
     * changing the values of the parameters it depends on.
     *
     * @return a reference to the Older Version Host Present Interval.
     */
    const TimeVal& older_version_host_present_interval() const { return (_older_version_host_present_interval); }

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
    int		add_protocol(xorp_module_id module_id,
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
    int		delete_protocol(xorp_module_id module_id,
				const string& module_instance_name);

    /**
     * Notify the interested parties that there is membership change among
     * the local members.
     *
     * @param source the source address of the (S,G) entry that has changed.
     * In case of group-specific membership, it could be IPvX::ZERO().
     * @param group the group address of the (S,G) entry that has changed.
     * @param action_jp the membership change: @ref ACTION_JOIN
     * or @ref ACTION_PRUNE.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_prune_notify_routing(const IPvX& source,
					  const IPvX& group,
					  action_jp_t action_jp) const;
    //
    // Functions for sending protocol messages
    //

    /**
     * Send MLD or IGMP message.
     *
     * @param src the message source address.
     * @param dst the message destination address.
     * @param message_type the MLD or IGMP type of the message.
     * @param max_resp_code the "Maximum Response Code" or "Max Resp Code"
     * field in the MLD or IGMP headers respectively (in the particular
     * protocol resolution).
     * @param group_address the "Multicast Address" or "Group Address" field
     * in the MLD or IGMP headers respectively.
     * @param buffer the buffer with the rest of the message.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     **/
    int		mld6igmp_send(const IPvX& src, const IPvX& dst,
			      uint8_t message_type, uint16_t max_resp_code,
			      const IPvX& group_address, buffer_t *buffer,
			      string& error_msg);

    /**
     * Send Group-Specific Query message.
     *
     * @param group_address the "Multicast Address" or "Group Address" field
     * in the MLD or IGMP headers respectively.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     **/
    int mld6igmp_group_query_send(const IPvX& group_address,
				  string& error_msg);

    /**
     * Send MLDv2 or IGMPv3 Group-and-Source-Specific Query message.
     *
     * @param group_address the "Multicast Address" or "Group Address" field
     * in the MLD or IGMP headers respectively.
     * @param sources the set of source addresses.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     **/
    int		mld6igmp_group_source_query_send(const IPvX& group_address,
						 const set<IPvX>& sources,
						 string& error_msg);

    /**
     * Send MLD or IGMP Query message.
     *
     * @param src the message source address.
     * @param dst the message destination address.
     * @param max_resp_time the maximum response time.
     * @param group_address the "Multicast Address" or "Group Address" field
     * in the MLD or IGMP headers respectively.
     * @param sources the set of source addresses (for IGMPv3 or MLDv2 only).
     * @param s_flag the "Suppress Router-Side Processing" bit (for IGMPv3
     * or MLDv2 only; in all other cases it should be set to false).
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     **/
    int		mld6igmp_query_send(const IPvX& src, const IPvX& dst,
				    const TimeVal& max_resp_time,
				    const IPvX& group_address,
				    const set<IPvX>& sources,
				    bool s_flag,
				    string& error_msg);

    /**
     * Test if the interface is running in IGMPv1 mode.
     *
     * @return true if the interface is running in IGMPv1 mode,
     * otherwise false.
     */
    bool	is_igmpv1_mode() const;

    /**
     * Test if the interface is running in IGMPv2 mode.
     *
     * @return true if the interface is running in IGMPv2 mode,
     * otherwise false.
     */
    bool	is_igmpv2_mode() const;

    /**
     * Test if the interface is running in IGMPv3 mode.
     *
     * @return true if the interface is running in IGMPv3 mode,
     * otherwise false.
     */
    bool	is_igmpv3_mode() const;

    /**
     * Test if the interface is running in MLDv1 mode.
     *
     * @return true if the interface is running in MLDv1 mode,
     * otherwise false.
     */
    bool	is_mldv1_mode() const;

    /**
     * Test if the interface is running in MLDv2 mode.
     *
     * @return true if the interface is running in MLDv2 mode,
     * otherwise false.
     */
    bool	is_mldv2_mode() const;

    /**
     * Test if a group is running in IGMPv1 mode.
     *
     * Note that if @ref group_record is NULL, then we test whether the
     * interface itself is running in IGMPv1 mode.
     * @param group_record the group record to test.
     * @return true if the group is running in IGMPv1 mode,
     * otherwise false.
     */
    bool	is_igmpv1_mode(const Mld6igmpGroupRecord* group_record) const;

    /**
     * Test if a group is running in IGMPv2 mode.
     *
     * Note that if @ref group_record is NULL, then we test whether the
     * interface itself is running in IGMPv2 mode.
     * @param group_record the group record to test.
     * @return true if the group is running in IGMPv2 mode,
     * otherwise false.
     */
    bool	is_igmpv2_mode(const Mld6igmpGroupRecord* group_record) const;

    /**
     * Test if a group is running in IGMPv3 mode.
     *
     * Note that if @ref group_record is NULL, then we test whether the
     * interface itself is running in IGMPv3 mode.
     * @param group_record the group record to test.
     * @return true if the group is running in IGMPv3 mode,
     * otherwise false.
     */
    bool	is_igmpv3_mode(const Mld6igmpGroupRecord* group_record) const;

    /**
     * Test if a group is running in MLDv1 mode.
     *
     * Note that if @ref group_record is NULL, then we test whether the
     * interface itself is running in MLDv1 mode.
     * @param group_record the group record to test.
     * @return true if the group is running in MLDv1 mode,
     * otherwise false.
     */
    bool	is_mldv1_mode(const Mld6igmpGroupRecord* group_record) const;

    /**
     * Test if a group is running in MLDv2 mode.
     *
     * Note that if @ref group_record is NULL, then we test whether the
     * interface itself is running in MLDv2 mode.
     * @param group_record the group record to test.
     * @return true if the group is running in MLDv2 mode,
     * otherwise false.
     */
    bool	is_mldv2_mode(const Mld6igmpGroupRecord* group_record) const;

private:
    //
    // Private functions
    //

    /**
     * Return the ASCII text description of the protocol message.
     *
     * @param message_type the protocol message type.
     * @return the ASCII text descrpition of the protocol message.
     */
    const char	*proto_message_type2ascii(uint8_t message_type) const;

    /**
     * Reset and prepare the buffer for sending data.
     *
     * @return the prepared buffer.
     */
    buffer_t	*buffer_send_prepare();

    /**
     * Calculate the checksum of an IPv6 "pseudo-header" as described
     * in RFC 2460.
     * 
     * @param src the source address of the pseudo-header.
     * @param dst the destination address of the pseudo-header.
     * @param len the upper-layer packet length of the pseudo-header
     * (in host-order).
     * @param protocol the upper-layer protocol number.
     * @return the checksum of the IPv6 "pseudo-header".
     */
    uint16_t	calculate_ipv6_pseudo_header_checksum(const IPvX& src,
						      const IPvX& dst,
						      size_t len,
						      uint8_t protocol);

    /**
     * Test whether I am the querier for this vif.
     *
     * @return true if I am the querier for this vif, otherwise false.
     */
    bool	i_am_querier() const;

    /**
     * Set the state whether I am the querier for this vif.
     *
     * @param v if true, then I am the querier for this vif.
     */
    void	set_i_am_querier(bool v);

    //
    // Callbacks for configuration and non-configurable parameters
    //
    void	set_configured_query_interval_cb(TimeVal v);
    void	set_query_last_member_interval_cb(TimeVal v);
    void	set_query_response_interval_cb(TimeVal v);
    void	set_configured_robust_count_cb(uint32_t v);
    void	recalculate_effective_query_interval();
    void	recalculate_effective_robustness_variable();
    void	recalculate_last_member_query_count();
    void	recalculate_group_membership_interval();
    void	recalculate_last_member_query_time();
    void	recalculate_older_version_host_present_interval();
    void	restore_effective_variables();

    //
    // Private state
    //
    Mld6igmpNode& _mld6igmp_node;	// The MLD6IGMP node I belong to
    buffer_t	*_buffer_send;		// Buffer for sending messages
    enum {
	MLD6IGMP_VIF_QUERIER	= 1 << 0 // I am the querier
    };
    uint32_t	_proto_flags;		// Various flags (MLD6IGMP_VIF_*)
    IPvX	_primary_addr;		// The primary address on this vif
    IPvX	_querier_addr;		// IP address of the current querier
    XorpTimer	_other_querier_timer;	// To timeout the (other) 'querier'
    XorpTimer	_query_timer;		// Timer to send queries
    uint8_t	_startup_query_count;	// Number of queries to send quickly
					// during startup
    Mld6igmpGroupSet _group_records;	// The group records

    //
    // Misc configuration parameters
    //
    ConfigParam<bool> _ip_router_alert_option_check; // The IP Router Alert option check flag
    ConfigParam<TimeVal> _configured_query_interval;	// The configured Query Interval
    TimeVal	_effective_query_interval;	// The effective Query Interval
    ConfigParam<TimeVal> _query_last_member_interval;	// The Last Member Query Interval
    ConfigParam<TimeVal> _query_response_interval;	// The Query Response Interval
    ConfigParam<uint32_t> _configured_robust_count;	// The configured Robustness Variable count
    uint32_t	_effective_robustness_variable;	// The effective Robustness Variable

    //
    // Other parameters that are not directly configurable
    //
    uint32_t	_last_member_query_count;	// The Last Member Query Count
    TimeVal	_group_membership_interval;	// The Group Membership Interval
    TimeVal	_last_member_query_time;	// The Last Member Query Time
    TimeVal	_older_version_host_present_interval;	// The Older Version Host Present Interval

    //
    // Misc. other state
    //
    // Registered protocols to notify for membership change.
    vector<pair<xorp_module_id, string> > _notify_routing_protocols;
    bool _dummy_flag;			// Dummy flag
    
    //
    // Not-so handy private functions that should go somewhere else
    //
    // MLD/IGMP control messages recv functions
    int		mld6igmp_membership_query_recv(const IPvX& src,
					       const IPvX& dst,
					       uint8_t message_type,
					       uint16_t max_resp_code,
					       const IPvX& group_address,
					       buffer_t *buffer);
    int		mld6igmp_ssm_membership_query_recv(const IPvX& src,
						   const IPvX& dst,
						   uint8_t message_type,
						   uint16_t max_resp_code,
						   const IPvX& group_address,
						   buffer_t *buffer);
    int		mld6igmp_membership_report_recv(const IPvX& src,
						const IPvX& dst,
						uint8_t message_type,
						uint16_t max_resp_code,
						const IPvX& group_address,
						buffer_t *buffer);
    int		mld6igmp_leave_group_recv(const IPvX& src,
					  const IPvX& dst,
					  uint8_t message_type,
					  uint16_t max_resp_code,
					  const IPvX& group_address,
					  buffer_t *buffer);
    int		mld6igmp_ssm_membership_report_recv(const IPvX& src,
						    const IPvX& dst,
						    uint8_t message_type,
						    buffer_t *buffer);
    int		mld6igmp_query_version_consistency_check(const IPvX& src,
							 const IPvX& dst,
							 uint8_t message_type,
							 int message_version);

    // MLD/IGMP control messages process functions
    int		mld6igmp_process(const IPvX& src,
				 const IPvX& dst,
				 int ip_ttl,
				 int ip_tos,
				 bool ip_router_alert,
				 bool ip_internet_control,
				 buffer_t *buffer,
				 string& error_msg);

    // MLD/IGMP uniform interface for protocol-related constants
    size_t	mld6igmp_constant_minlen() const;
    uint32_t	mld6igmp_constant_timer_scale() const;
    uint8_t	mld6igmp_constant_membership_query() const;

    void	other_querier_timer_timeout();
    void	query_timer_timeout();

    void	decode_exp_time_code8(uint8_t code, TimeVal& timeval,
				      uint32_t timer_scale);
    void	decode_exp_time_code16(uint16_t code, TimeVal& timeval,
				       uint32_t timer_scale);
    void	encode_exp_time_code8(const TimeVal& timeval,
				      uint8_t& code,
				      uint32_t timer_scale);
    void	encode_exp_time_code16(const TimeVal& timeval,
				       uint16_t& code,
				       uint32_t timer_scale);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_VIF_HH__
