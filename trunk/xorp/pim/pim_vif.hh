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

// $XORP: xorp/pim/pim_vif.hh,v 1.14 2003/05/31 07:03:33 pavlin Exp $


#ifndef __PIM_PIM_VIF_HH__
#define __PIM_PIM_VIF_HH__


//
// PIM virtual interface definition.
//


#include <list>

#include "libxorp/config_param.hh"
#include "libxorp/vif.hh"
#include "libxorp/timer.hh"
#include "libproto/proto_unit.hh"
#include "mrt/buffer.h"
#include "mrt/mifset.hh"
#include "mrt/multicast_defs.h"
#include "pim_nbr.hh"
#include "pim_proto_join_prune_message.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class AssertMetric;
class BsrZone;
class PimJpGroup;
class PimJpHeader;
class PimNbr;
class PimNode;


/**
 * @short A class for PIM-specific virtual interface.
 */
class PimVif : public ProtoUnit, public Vif {
public:
    /**
     * Constructor for a given PIM node and a generic virtual interface.
     * 
     * @param pim_node the @ref PimNode this interface belongs to.
     * @param vif the generic Vif interface that contains various information.
     */
    PimVif(PimNode& pim_node, const Vif& vif);
    
    /**
     * Destructor
     */
    virtual ~PimVif();

    /**
     * Set configuration to default values.
     */
    void	set_default_config();
    
    /**
     * Set the current protocol version.
     * 
     * The protocol version must be in the interval
     * [PIM_VERSION_MIN, PIM_VERSION_MAX].
     * 
     * @param proto_version the protocol version to set.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_proto_version(int proto_version);
    
    /**
     *  Start PIM on a single virtual interface.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Gracefully stop PIM on a single virtual interface.
     * 
     *The graceful stop will attempt to send Join/Prune, Assert, etc.
     * messages for all multicast routing entries to gracefully clean-up
     * state with neighbors.
     * After the multicast routing entries cleanup is completed,
     * PimVif::final_stop() is called to complete the job.
     * If this method is called one-after-another, the second one
     * will force calling immediately PimVif::final_stop() to quickly
     * finish the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Completely stop PIM on a single virtual interface.
     * 
     * This method should be called after @ref PimVif::stop() to complete
     * the job.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		final_stop();
    
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
    int		pim_recv(const IPvX& src, const IPvX& dst,
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
    string	flags_string(void) const;
    
    /**
     * Get the PIM node (@ref PimNode).
     * 
     * @return a reference to the PIM node (@ref PimNode).
     */
    PimNode&	pim_node() const	{ return (_pim_node);		}
    
    /**
     * Get the PIM Multicast Routing Table (@ref PimMrt).
     * 
     * @return a reference to the PIM Multicast Routing Table (@ref PimMrt).
     */
    PimMrt&	pim_mrt() const;
    
    /**
     * Get the PIM neighbor information (@ref PimNbr) about myself.
     * 
     * @return a reference to the PIM neighbor information (@ref PimNbr)
     * about myself.
     */
    PimNbr&	pim_nbr_me()		{ return (_pim_nbr_me);		}
    
    /**
     * Start the PIM Hello operation.
     */
    void	pim_hello_start();
    
    /**
     * Stop the PIM Hello operation.
     */
    void	pim_hello_stop();
    
    /**
     * Attempt to inform other PIM neighbors that this interface is not more
     * the Designated Router.
     */
    void	pim_hello_stop_dr();
    
    /**
     * Elect a Designated Router on this interface.
     */
    void	pim_dr_elect();
    
    /**
     * Compute if I may become the Designated Router on this interface
     * if one of the PIM neighbor addresses is not considered.
     * 
     * Compute if I may become the DR on this interface if @ref exclude_addr
     * is excluded.
     * 
     * @param exclude_addr the address to exclude in the computation.
     * @return true if I may become the DR on this interface, otherwise
     * false.
     */
    bool	i_may_become_dr(const IPvX& exclude_addr);
    
    /**
     * Get my primary address on this interface.
     * 
     * @return my primary address on this interface.
     */
    const IPvX&	addr() const		{ return (_pim_nbr_me.addr());	}
    
    /**
     * Get the address of the Designated Router on this interface.
     * 
     * @return the address of the Designated Router on this interface.
     */
    const IPvX&	dr_addr() const		{ return (_dr_addr);		}
    
    //
    // Hello-related configuration parameters
    //
    ConfigParam<uint16_t>& hello_triggered_delay() { return (_hello_triggered_delay); }
    ConfigParam<uint16_t>& hello_period() { return (_hello_period); }
    ConfigParam<uint16_t>& hello_holdtime() { return (_hello_holdtime); }
    ConfigParam<uint32_t>& genid() { return (_genid); }
    ConfigParam<uint32_t>& dr_priority() { return (_dr_priority); }
    ConfigParam<uint16_t>& lan_delay() { return (_lan_delay); }
    ConfigParam<uint16_t>& override_interval() { return (_override_interval); }
    ConfigParam<bool>& is_tracking_support_disabled() { return (_is_tracking_support_disabled); }
    ConfigParam<bool>& accept_nohello_neighbors() { return (_accept_nohello_neighbors); }
    
    //
    // Join/Prune-related configuration parameters
    //
    ConfigParam<uint16_t>& join_prune_period() { return (_join_prune_period); }
    ConfigParam<uint16_t>& join_prune_holdtime() { return (_join_prune_holdtime); }
    
    //
    // Assert-related configuration parameters
    //
    ConfigParam<uint32_t>& assert_time() { return (_assert_time); }
    ConfigParam<uint32_t>& assert_override_interval() { return (_assert_override_interval); }
    
    //
    // Functions for sending protocol messages.
    //
    int		pim_hello_send();
    int		pim_hello_first_send();
    int		pim_join_prune_send(PimNbr *pim_nbr, PimJpHeader *jp_header);
    int		pim_assert_mre_send(PimMre *pim_mre,
				    const IPvX& assert_source_addr);
    int		pim_assert_cancel_send(PimMre *pim_mre);
    int		pim_assert_send(const IPvX& assert_source_addr,
				const IPvX& assert_group_addr,
				bool rpt_bit,
				uint32_t metric_preference,
				uint32_t metric);
    int		pim_register_send(const IPvX& rp_addr,
				  const IPvX& source_addr,
				  const IPvX& group_addr,
				  const uint8_t *rcvbuf,
				  size_t rcvlen);
    int		pim_register_null_send(const IPvX& rp_addr,
				       const IPvX& source_addr,
				       const IPvX& group_addr);
    int		pim_register_stop_send(const IPvX& dr_addr,
				       const IPvX& source_addr,
				       const IPvX& group_addr);
    int		pim_bootstrap_send(const IPvX& dst_addr,
				   const BsrZone& bsr_zone);
    buffer_t	*pim_bootstrap_send_prepare(const IPvX& dst_addr,
					    const BsrZone& bsr_zone,
					    bool is_first_fragment);
    int		pim_cand_rp_adv_send(const IPvX& bsr_addr,
				     const BsrZone& bsr_zone);
    
    void	hello_timer_start(uint32_t sec, uint32_t usec);
    void	hello_timer_start_random(uint32_t sec, uint32_t usec);
    
    bool	is_lan_delay_enabled() const;
    // Link-related time intervals
    const TimeVal& vif_propagation_delay() const;
    const TimeVal& vif_override_interval() const;
    bool	is_lan_suppression_state_enabled() const;
    const TimeVal& upstream_join_timer_t_suppressed() const;
    const TimeVal& upstream_join_timer_t_override() const;
    
    // Misc. functions
    const TimeVal& jp_override_interval() const;
    list<PimNbr *>& pim_nbrs() { return (_pim_nbrs); }
    int		pim_nbrs_number() const { return (_pim_nbrs.size()); }
    bool	i_am_dr() const;
    void	set_i_am_dr(bool v);
    PimNbr	*pim_nbr_find(const IPvX& nbr_addr);
    void	add_pim_nbr(PimNbr *pim_nbr);
    int		delete_pim_nbr(PimNbr *pim_nbr);
    void	delete_pim_nbr_from_nbr_list(PimNbr *pim_nbr);
    
    // Usage-related functions
    size_t	usage_by_pim_mre_task() const { return (_usage_by_pim_mre_task); }
    void	incr_usage_by_pim_mre_task();
    void	decr_usage_by_pim_mre_task();
    
private:
    // Private functions
    void	hello_timer_timeout();
    void	hello_once_timer_timeout();
    
    //
    // Callbacks for configuration parameters
    //
    void	set_hello_period_callback(uint16_t v) {
	uint16_t old_hello_holdtime_divided
	    = (uint16_t) (_hello_holdtime.get() / PIM_HELLO_HELLO_HOLDTIME_PERIOD_RATIO);
	if (v != old_hello_holdtime_divided)
	    _hello_holdtime.set(
		(uint16_t)(v * PIM_HELLO_HELLO_HOLDTIME_PERIOD_RATIO));
	_pim_nbr_me.set_hello_holdtime(_hello_holdtime.get());
    }
    void	set_hello_holdtime_callback(uint16_t v) {
	uint16_t new_hello_period
	    = (uint16_t)(v / PIM_HELLO_HELLO_HOLDTIME_PERIOD_RATIO);
	if (_hello_period.get() != new_hello_period)
	    _hello_period.set(new_hello_period);
	_pim_nbr_me.set_hello_holdtime(_hello_holdtime.get());
    }
    void	set_genid_callback(uint32_t v) {
	_pim_nbr_me.set_genid(v);
	_pim_nbr_me.set_is_genid_present(true);
    }
    void	set_dr_priority_callback(uint32_t v) {
	_pim_nbr_me.set_dr_priority(v);
	_pim_nbr_me.set_is_dr_priority_present(true);
    }
    void	set_lan_delay_callback(uint16_t v) {
	_pim_nbr_me.set_lan_delay(v);
	_pim_nbr_me.set_is_lan_prune_delay_present(true);
    }
    void	set_override_interval_callback(uint16_t v) {
	_pim_nbr_me.set_override_interval(v);
	_pim_nbr_me.set_is_lan_prune_delay_present(true);
    }
    void	set_is_tracking_support_disabled_callback(bool v) {
	_pim_nbr_me.set_is_tracking_support_disabled(v);
    }
    void	set_join_prune_period_callback(uint16_t v) {
	_join_prune_holdtime.set(
	    (uint16_t)(v * PIM_JOIN_PRUNE_HOLDTIME_PERIOD_RATIO));
    }
    
    
    int jp_entry_add(const IPvX& source_addr, const IPvX& group_addr,
		     mrt_entry_type_t mrt_entry_type, action_jp_t action_jp,
		     uint16_t holdtime);
    int jp_entry_flush();
    
    bool is_send_unicast_bootstrap() const {
	return (! _send_unicast_bootstrap_nbr_list.empty());
    }
    void add_send_unicast_bootstrap_nbr(const IPvX& nbr_addr) {
	_send_unicast_bootstrap_nbr_list.push_back(nbr_addr);
    }
    const list<IPvX>& send_unicast_bootstrap_nbr_list() const {
	return (_send_unicast_bootstrap_nbr_list);
    }
    void delete_send_unicast_bootstrap_nbr_list() {
	_send_unicast_bootstrap_nbr_list.clear();
    }
    
    bool should_send_pim_hello() const { return (_should_send_pim_hello); }
    void set_should_send_pim_hello(bool v) { _should_send_pim_hello = v; }
    
    // Private state
    PimNode&	_pim_node;		// The PIM node I belong to
    buffer_t	*_buffer_send;		// Buffer for sending messages
    buffer_t	*_buffer_send_hello;	// Buffer for sending Hello messages
    buffer_t	*_buffer_send_bootstrap;// Buffer for sending Bootstrap msgs
    enum {
	PIM_VIF_DR	= 1 << 0	// I am the Designated Router
    };
    uint32_t	_proto_flags;		// Various flags (PIM_VIF_*)
    IPvX	_dr_addr;		// IP address of the current DR
    XorpTimer	_hello_timer;		// Timer to send a HELLO message
    XorpTimer	_hello_once_timer;	// Timer to send once a HELLO message
    list<PimNbr *> _pim_nbrs;		// List of all PIM neighbors
    PimNbr	_pim_nbr_me;		// Myself (for misc. purpose)
    list<IPvX>	_send_unicast_bootstrap_nbr_list; // List of new nbrs to
						  // unicast to them the
						  // Bootstrap message.
    
    //
    // Hello-related configuration parameters
    //
    ConfigParam<uint16_t> _hello_triggered_delay; // The Triggered_Hello_Delay
    ConfigParam<uint16_t> _hello_period;	// The Hello_Period
    ConfigParam<uint16_t> _hello_holdtime;	// The Hello_Holdtime
    ConfigParam<uint32_t> _genid;		// The Generation ID
    ConfigParam<uint32_t> _dr_priority;		// The DR Priority
    ConfigParam<uint16_t> _lan_delay;		// The LAN Delay
    ConfigParam<uint16_t> _override_interval;	// The Override_Interval
    ConfigParam<bool>	  _is_tracking_support_disabled; // The T-bit
    ConfigParam<bool>	  _accept_nohello_neighbors; // If true, accept
						// neighbors that didn't send
						// a Hello message first
    
    //
    // Join/Prune-related configuration parameters
    //
    ConfigParam<uint16_t> _join_prune_period;	// The period between J/P msgs
    ConfigParam<uint16_t> _join_prune_holdtime;	// The holdtime in J/P msgs
    
    //
    // Assert-related configuration parameters
    //
    ConfigParam<uint32_t> _assert_time;		// The Assert_Time
    ConfigParam<uint32_t> _assert_override_interval; // The Assert_Override_Interval
    
    bool	_should_send_pim_hello;	// True if PIM_HELLO should be sent
					// before any other control messages
    
    size_t	_usage_by_pim_mre_task;	// Counter for usage by PimMreTask
    
    // Not-so handy private functions that should go somewhere else
    // PIM control messages send functions
    int		pim_send(const IPvX& dst, uint8_t message_type,
			 buffer_t *buffer);
    // PIM control messages recv functions
    int		pim_hello_recv(PimNbr *pim_nbr, const IPvX& src,
			       const IPvX& dst, buffer_t *buffer,
			       int nbr_proto_version);
    int		pim_register_recv(PimNbr *pim_nbr, const IPvX& src,
				  const IPvX& dst, buffer_t *buffer);
    int		pim_register_stop_recv(PimNbr *pim_nbr, const IPvX& src,
				       const IPvX& dst, buffer_t *buffer);
    int		pim_join_prune_recv(PimNbr *pim_nbr, const IPvX& src,
				    const IPvX& dst, buffer_t *buffer,
				    uint8_t message_type);
    int		pim_bootstrap_recv(PimNbr *pim_nbr, const IPvX& src,
				   const IPvX& dst, buffer_t *buffer);
    int		pim_assert_recv(PimNbr *pim_nbr, const IPvX& src,
				const IPvX& dst, buffer_t *buffer);
    int		pim_graft_recv(PimNbr *pim_nbr, const IPvX& src,
			       const IPvX& dst, buffer_t *buffer);
    int		pim_graft_ack_recv(PimNbr *pim_nbr, const IPvX& src,
				   const IPvX& dst, buffer_t *buffer);
    int		pim_cand_rp_adv_recv(PimNbr *pim_nbr, const IPvX& src,
				     const IPvX& dst, buffer_t *buffer);
    
    // PIM control messages process functions
    int		pim_process(const IPvX& src, const IPvX& dst,
			    buffer_t *buffer);
    int		pim_assert_process(PimNbr *pim_nbr,
				   const IPvX& src,
				   const IPvX& dst,
				   const IPvX& assert_source_addr,
				   const IPvX& assert_group_addr,
				   uint8_t assert_group_masklen,
				   AssertMetric *assert_metric);
    int		pim_register_stop_process(const IPvX& rp_addr,
					  const IPvX& source_addr,
					  const IPvX& group_addr,
					  uint8_t group_masklen);
    
    buffer_t	*buffer_send_prepare();
    buffer_t	*buffer_send_prepare(buffer_t *buffer);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_VIF_HH__
