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

#ident "$XORP: xorp/pim/pim_proto_hello.cc,v 1.7 2003/04/01 00:56:24 pavlin Exp $"


//
// PIM PIM_HELLO messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "mrt/random.h"
#include "pim_node.hh"
#include "pim_vif.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//
static bool	pim_dr_is_better(PimNbr *pim_nbr1, PimNbr *pim_nbr2,
				 bool consider_dr_priority);


void
PimVif::pim_hello_start(void)
{
    // Generate a new Gen-ID
    genid().set(RANDOM(0xffffffffU));
    
    // On startup, I will become the PIM Designated Router
    pim_dr_elect();
    
    // Schedule the first PIM_HELLO message at random in the
    // interval [0, hello_triggered_delay)
    hello_timer_start_random(hello_triggered_delay().get(), 0);
}

void
PimVif::pim_hello_stop()
{
    uint16_t save_holdtime = hello_holdtime().get();
    
    hello_holdtime().set(0);		// XXX: timeout immediately
    pim_hello_send();
    hello_holdtime().set(save_holdtime);
}

void
PimVif::pim_hello_stop_dr()
{
    uint32_t save_dr_priority = dr_priority().get();
    
    dr_priority().set(PIM_HELLO_DR_ELECTION_PRIORITY_LOWEST);	// XXX: lowest
    pim_hello_send();
    dr_priority().set(save_dr_priority);
}

/**
 * PimVif::pim_hello_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * @nbr_proto_version: The protocol version from this heighbor.
 * 
 * Receive PIM_HELLO message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_hello_recv(PimNbr *pim_nbr,
		       const IPvX& src,
		       const IPvX& dst,
		       buffer_t *buffer,
		       int nbr_proto_version)
{
    bool	holdtime_rcvd = false;
    bool	lan_prune_delay_rcvd = false;
    bool	dr_election_priority_rcvd = false;
    bool	genid_rcvd = false;
    bool	is_genid_changed = false;
    uint16_t	option_type, option_length, option_length_spec;
    uint16_t	holdtime;
    uint16_t	lan_delay;
    uint16_t	lan_prune_delay_tbit;
    uint16_t	override_interval;
    uint32_t	dr_priority;
    uint32_t	genid;
    bool	new_nbr_flag = false;
    
    //
    // Parse the message
    //
    while (BUFFER_DATA_SIZE(buffer) > 0) {
	BUFFER_GET_HOST_16(option_type, buffer);
	BUFFER_GET_HOST_16(option_length, buffer);
	if (BUFFER_DATA_SIZE(buffer) < option_length)
	    goto rcvlen_error;
	switch (option_type) {
	    
	case PIM_HELLO_HOLDTIME_OPTION:
	    // Holdtime option
	    option_length_spec = PIM_HELLO_HOLDTIME_LENGTH;
	    if (option_length < option_length_spec) {
		BUFFER_GET_SKIP(option_length, buffer);
		continue;
	    }
	    BUFFER_GET_HOST_16(holdtime, buffer);
	    BUFFER_GET_SKIP(option_length - option_length_spec, buffer);
	    holdtime_rcvd = true;
	    break;
	    
	case PIM_HELLO_LAN_PRUNE_DELAY_OPTION:
	    // LAN Prune Delay option
	    option_length_spec = PIM_HELLO_LAN_PRUNE_DELAY_LENGTH;
	    if (option_length < option_length_spec) {
		BUFFER_GET_SKIP(option_length, buffer);
		continue;
	    }
	    BUFFER_GET_HOST_16(lan_delay, buffer);
	    BUFFER_GET_HOST_16(override_interval, buffer);
	    lan_prune_delay_tbit
		= (lan_delay & PIM_HELLO_LAN_PRUNE_DELAY_TBIT) ?
		true : false;
	    lan_delay &= ~PIM_HELLO_LAN_PRUNE_DELAY_TBIT;
	    BUFFER_GET_SKIP(option_length - option_length_spec, buffer);
	    lan_prune_delay_rcvd = true;
	    break;
	    
	case PIM_HELLO_DR_ELECTION_PRIORITY_OPTION:
	    // DR priority option
	    option_length_spec = PIM_HELLO_DR_ELECTION_PRIORITY_LENGTH;
	    if (option_length < option_length_spec) {
		BUFFER_GET_SKIP(option_length, buffer);
		continue;
	    }
	    BUFFER_GET_HOST_32(dr_priority, buffer);
	    BUFFER_GET_SKIP(option_length - option_length_spec, buffer);
	    dr_election_priority_rcvd = true;
	    break;
	    
	case PIM_HELLO_GENID_OPTION:
	    // Gen ID option
	    option_length_spec = PIM_HELLO_GENID_LENGTH;
	    if (option_length < option_length_spec) {
		BUFFER_GET_SKIP(option_length, buffer);
		continue;
	    }
	    BUFFER_GET_HOST_32(genid, buffer);
	    BUFFER_GET_SKIP(option_length - option_length_spec, buffer);
	    genid_rcvd = true;
	    break;
	default:
	    // XXX: skip unrecognized options
	    BUFFER_GET_SKIP(option_length, buffer);
	    break;
	}
    }
    
    if ((pim_nbr != NULL) && genid_rcvd) {
	if ( (! pim_nbr->is_genid_present())
	     || (pim_nbr->is_genid_present() && genid != pim_nbr->genid())) {
	    //
	    // This neighbor has just restarted (or something similar).
	    //
	    is_genid_changed = true;
	    
	    //
	    // Reset any old Hello information about this neighbor.
	    //
	    pim_nbr->reset_received_options();
	}
    }
    
    if (pim_nbr == NULL) {
	// A new neighbor
	pim_nbr = new PimNbr(*this, src, nbr_proto_version);
	add_pim_nbr(pim_nbr);
	new_nbr_flag = true;
	XLOG_TRACE(pim_node().is_log_trace(),
		   "Added new neighbor %s on vif %s",
		   cstring(pim_nbr->addr()), name().c_str());
    }
    
    // Set the protocol version for this neighbor
    pim_nbr->set_proto_version(nbr_proto_version);
    
    //
    // XXX: pim_hello_holdtime_process() is called with default value
    // even if no Holdtime option is received.
    //
    if (! holdtime_rcvd)
	holdtime = PIM_HELLO_HELLO_HOLDTIME_DEFAULT;
    pim_nbr->pim_hello_holdtime_process(holdtime);
    
    if (lan_prune_delay_rcvd)
	pim_nbr->pim_hello_lan_prune_delay_process(lan_prune_delay_tbit,
						   lan_delay,
						   override_interval);
    
    if (dr_election_priority_rcvd)
	pim_nbr->pim_hello_dr_election_priority_process(dr_priority);
    
    if (genid_rcvd)
	pim_nbr->pim_hello_genid_process(genid);
    
    if (new_nbr_flag || is_genid_changed) {
	if (i_am_dr() || (is_genid_changed && i_may_become_dr(src))) {
	    // Schedule to send an unicast Bootstrap message
	    add_send_unicast_bootstrap_nbr(src);
	}
	
	//
	// Set the flag that we must send first a Hello message before
	// any other control messages.
	//
	set_should_send_pim_hello(true);
	
	// Schedule a PIM_HELLO message at random in the
	// interval [0, hello_triggered_delay)
	// XXX: this message should not affect the periodic `hello_timer'.
	TimeVal tv(hello_triggered_delay().get(), 0);
	tv = random_uniform(tv);
	_hello_once_timer =
	    pim_node().event_loop().new_oneoff_after(
		tv,
		callback(this, &PimVif::hello_once_timer_timeout));
	
	//
	// Add the task that will process all PimMre entries that have no
	// neighbor.
	//
	if (new_nbr_flag) {
	    pim_node().pim_mrt().add_task_pim_nbr_changed(
		Vif::VIF_INDEX_INVALID,
		IPvX::ZERO(family()));
	}
	
	//
	// Add the task that will process all PimMre entries and take
	// action because the GenID of this neighbor changed.
	//
	if (is_genid_changed) {
	    pim_node().pim_mrt().add_task_pim_nbr_gen_id_changed(
		vif_index(),
		pim_nbr->addr());
	}
    }
    
    // (Re)elect the DR
    pim_dr_elect();
    
    return (XORP_OK);
    
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_HELLO),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
    
    // UNUSED(dst);
}

/**
 * PimNbr::pim_hello_holdtime_process:
 * @holdtime: The holdtime for the neighbor (in seconds).
 * 
 * Process PIM_HELLO Holdtime option.
 * XXX: if no Holdtime option is received, this function is still
 * called with the default holdtime value of %PIM_HELLO_HELLO_HOLDTIME_DEFAULT.
 **/
void
PimNbr::pim_hello_holdtime_process(uint16_t holdtime)
{
    _hello_holdtime = holdtime;
    
    switch (holdtime) {
    case PIM_HELLO_HOLDTIME_FOREVER:
	// Never expire this neighbor
	_neighbor_liveness_timer.unschedule();
	break;
    default:
	// Start the Neighbor Liveness Timer
	_neighbor_liveness_timer =
	    pim_node().event_loop().new_oneoff_after(
		TimeVal(holdtime, 0),
		callback(this, &PimNbr::neighbor_liveness_timer_timeout));
	break;
    }
}

void
PimNbr::pim_hello_lan_prune_delay_process(bool lan_prune_delay_tbit,
					  uint16_t lan_delay,
					  uint16_t override_interval)
{
    _is_lan_prune_delay_present = true;
    _is_tracking_support_disabled = lan_prune_delay_tbit;
    _lan_delay = lan_delay;
    _override_interval = override_interval;
}

void
PimNbr::pim_hello_dr_election_priority_process(uint32_t dr_priority)
{
    _is_dr_priority_present = true;
    _dr_priority = dr_priority;
}

void
PimNbr::pim_hello_genid_process(uint32_t genid)
{
    _is_genid_present = true;
    _genid = genid;
}

void
PimVif::pim_dr_elect()
{
    PimNbr *dr = &pim_nbr_me();
    list<PimNbr *>::iterator iter;
    bool consider_dr_priority = pim_nbr_me().is_dr_priority_present();
    
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (! pim_nbr->is_dr_priority_present()) {
	    consider_dr_priority = false;
	    break;
	}
    }
    
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (! pim_dr_is_better(dr, pim_nbr, consider_dr_priority))
	    dr = pim_nbr;
    }
    
    if (dr == NULL) {
	XLOG_FATAL("Cannot elect a DR on interface %s", name().c_str());
	return;
    }
    _dr_addr = dr->addr();
    
    // Set a flag if I am the DR
    if (dr_addr() == addr()) {
	if (! i_am_dr()) {
	    set_i_am_dr(true);
	    // TODO: take the appropriate action
	}
    } else {
	set_i_am_dr(false);
    }
}

/**
 * PimVif::i_may_become_dr:
 * @exclude_addr: The address to exclude in the computation.
 * 
 * Compute if I may become the DR on this interface if @exclude_addr
 * is excluded.
 * 
 * Return value: True if I may become the DR on this interface, otherwise
 * false.
 **/
bool
PimVif::i_may_become_dr(const IPvX& exclude_addr)
{
    PimNbr *dr = &pim_nbr_me();
    list<PimNbr *>::iterator iter;
    bool consider_dr_priority = pim_nbr_me().is_dr_priority_present();
    
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (! pim_nbr->is_dr_priority_present()) {
	    consider_dr_priority = false;
	    break;
	}
    }
    
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->addr() == exclude_addr)
	    continue;
	if (! pim_dr_is_better(dr, pim_nbr, consider_dr_priority))
	    dr = pim_nbr;
    }
    
    if ((dr != NULL) && (dr->addr() == addr()))
	return (true);
    
    return (false);
}

/**
 * pim_dr_is_better:
 * @pim_nbr1: The first neighbor to compare.
 * @pim_nbr2: The second neighbor to compare.
 * @consider_dr_priority: If true, in the comparison we consider
 * the DR priority, otherwise the DR priority is ignored.
 * 
 * Compare two PIM neighbors which of them is a better candidate to be a
 * DR (Designated Router).
 * 
 * Return value: true if @pim_nbr1 is a better candidate to be a DR,
 * otherwise %false.
 **/
static bool
pim_dr_is_better(PimNbr *pim_nbr1, PimNbr *pim_nbr2, bool consider_dr_priority)
{
    if (pim_nbr2 == NULL)
	return (true);
    if (pim_nbr1 == NULL)
	return (false);
    
    if (consider_dr_priority) {
	if (pim_nbr1->dr_priority() > pim_nbr2->dr_priority())
	    return (true);
	if (pim_nbr1->dr_priority() < pim_nbr2->dr_priority())
	    return (false);
    }
    
    // Either the DR priority is same, or we have to ignore it
    if (pim_nbr1->addr() > pim_nbr2->addr())
	return (true);
    
    return (false);
}

void
PimVif::hello_timer_start(uint32_t sec, uint32_t usec)
{
    _hello_timer =
	pim_node().event_loop().new_oneoff_after(
	    TimeVal(sec, usec),
	    callback(this, &PimVif::hello_timer_timeout));
}

// Schedule a PIM_HELLO message at random in the
// interval [0, (sec, usec))
void
PimVif::hello_timer_start_random(uint32_t sec, uint32_t usec)
{
    TimeVal tv(sec, usec);
    
    tv = random_uniform(tv);
    
    _hello_timer =
	pim_node().event_loop().new_oneoff_after(
	    tv,
	    callback(this, &PimVif::hello_timer_timeout));
}

void
PimVif::hello_timer_timeout()
{
    pim_hello_send();
    hello_timer_start(hello_period().get(), 0);
}

//
// XXX: the presumption is that this timeout function is called only
// when we have added a new neighbor. If this is not true, the
// add_task_foo() task scheduling is not needed.
//
void
PimVif::hello_once_timer_timeout()
{
    pim_hello_first_send();
}

int
PimVif::pim_hello_first_send()
{
    pim_hello_send();
    
    //
    // Unicast the Bootstrap message if needed
    //
    if (is_send_unicast_bootstrap()) {
	list<IPvX>::const_iterator nbr_iter;
	for (nbr_iter = send_unicast_bootstrap_nbr_list().begin();
	     nbr_iter != send_unicast_bootstrap_nbr_list().end();
	     ++nbr_iter) {
	    const IPvX& nbr_addr = *nbr_iter;
	    
	    // Unicast the Bootstrap messages
	    pim_node().pim_bsr().unicast_pim_bootstrap(this, nbr_addr);
	}
	
	delete_send_unicast_bootstrap_nbr_list();
    }
    
    _hello_once_timer.unschedule();
    
    return (XORP_OK);
}

int
PimVif::pim_hello_send()
{
    // XXX: note that for PIM Hello messages only we use a separate
    // buffer for sending the data, because the sending of a Hello message
    // can be triggered during the sending of another control message.
    buffer_t *buffer = buffer_send_prepare(_buffer_send_hello);
    uint16_t lan_delay_tbit;
    
#if 0
    // XXX: enable if for any reason sending Hello messages is not desirable
    set_should_send_pim_hello(false);    
    return (XORP_OK);
#endif
    
    // Holdtime option
    BUFFER_PUT_HOST_16(PIM_HELLO_HOLDTIME_OPTION, buffer);
    BUFFER_PUT_HOST_16(PIM_HELLO_HOLDTIME_LENGTH, buffer);
    BUFFER_PUT_HOST_16(hello_holdtime().get(), buffer);
    
    // LAN Prune Delay option    
    BUFFER_PUT_HOST_16(PIM_HELLO_LAN_PRUNE_DELAY_OPTION, buffer);
    BUFFER_PUT_HOST_16(PIM_HELLO_LAN_PRUNE_DELAY_LENGTH, buffer);
    lan_delay_tbit = lan_delay().get();
    if (is_tracking_support_disabled().get())
	lan_delay_tbit |= PIM_HELLO_LAN_PRUNE_DELAY_TBIT;
    BUFFER_PUT_HOST_16(lan_delay_tbit, buffer);
    BUFFER_PUT_HOST_16(override_interval().get(), buffer);
    
    // DR priority option    
    BUFFER_PUT_HOST_16(PIM_HELLO_DR_ELECTION_PRIORITY_OPTION, buffer);
    BUFFER_PUT_HOST_16(PIM_HELLO_DR_ELECTION_PRIORITY_LENGTH, buffer);
    BUFFER_PUT_HOST_32(dr_priority().get(), buffer);
    
    // GenID option
    BUFFER_PUT_HOST_16(PIM_HELLO_GENID_OPTION, buffer);
    BUFFER_PUT_HOST_16(PIM_HELLO_GENID_LENGTH, buffer);
    BUFFER_PUT_HOST_32(genid().get(), buffer);
    
    return (pim_send(IPvX::PIM_ROUTERS(family()), PIM_HELLO, buffer));
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_HELLO),
	       cstring(addr()), cstring(IPvX::PIM_ROUTERS(family())));
    return (XORP_ERROR);
}
