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

#ident "$XORP: xorp/pim/pim_mre_register.cc,v 1.7 2003/04/01 00:56:21 pavlin Exp $"

//
// PIM Multicast Routing Entry Register handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_proto.h"
#include "pim_mfc.hh"
#include "pim_mre.hh"
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


// Note: applies for (S,G)
bool
PimMre::compute_is_could_register_sg() const
{
    uint16_t vif_index;
    Mifset mifs;
    
    if (! is_sg())
	return (false);
    
    vif_index = rpf_interface_s();
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    mifs = i_am_dr();
    
    return (mifs.test(vif_index)
	    && is_keepalive_timer_running()
	    && is_directly_connected_s()
	    && (! i_am_rp())); // TODO: XXX: PAVPAVPAV: not in the spec (yet)
}

//
// PIM Register state (for the pim_register_vif_index() interface)
//
// Note: applies for (S,G)
void
PimMre::set_register_noinfo_state()
{
    if (! is_sg())
	return;
    
    if (is_register_noinfo_state())
	return;			// Nothing changed
    
    _flags &= ~(PIM_MRE_REGISTER_JOIN_STATE
		| PIM_MRE_REGISTER_PRUNE_STATE
		| PIM_MRE_REGISTER_JOIN_PENDING_STATE);
    
    // Try to remove the entry    
    entry_try_remove();
}

// Note: applies for (S,G)
void
PimMre::set_register_join_state()
{
    if (! is_sg())
	return;
    
    if (is_register_join_state())
	return;			// Nothing changed
    
    _flags &= ~(PIM_MRE_REGISTER_JOIN_STATE
		| PIM_MRE_REGISTER_PRUNE_STATE
		| PIM_MRE_REGISTER_JOIN_PENDING_STATE);
    _flags |= PIM_MRE_REGISTER_JOIN_STATE;
}

// Note: applies for (S,G)
void
PimMre::set_register_prune_state()
{
    if (! is_sg())
	return;
    
    if (is_register_prune_state())
	return;			// Nothing changed
    
    _flags &= ~(PIM_MRE_REGISTER_JOIN_STATE
		| PIM_MRE_REGISTER_PRUNE_STATE
		| PIM_MRE_REGISTER_JOIN_PENDING_STATE);
    _flags |= PIM_MRE_REGISTER_PRUNE_STATE;
}

// Note: applies for (S,G)
void
PimMre::set_register_join_pending_state()
{
    if (! is_sg())
	return;
    
    if (is_register_join_pending_state())
	return;			// Nothing changed
    
    _flags &= ~(PIM_MRE_REGISTER_JOIN_STATE
		| PIM_MRE_REGISTER_PRUNE_STATE
		| PIM_MRE_REGISTER_JOIN_PENDING_STATE);
    _flags |= PIM_MRE_REGISTER_JOIN_PENDING_STATE;
}

// Note: applies for (S,G)
bool
PimMre::is_register_noinfo_state() const
{
    return (! (_flags & (PIM_MRE_REGISTER_JOIN_STATE
			 | PIM_MRE_REGISTER_PRUNE_STATE
			 | PIM_MRE_REGISTER_JOIN_PENDING_STATE)));
}

// Note: applies for (S,G)
bool
PimMre::is_register_join_state() const
{
    return (_flags & PIM_MRE_REGISTER_JOIN_STATE);
}

// Note: applies for (S,G)
bool
PimMre::is_register_prune_state() const
{
    return (_flags & PIM_MRE_REGISTER_PRUNE_STATE);
}

// Note: applies for (S,G)
bool
PimMre::is_register_join_pending_state() const
{
    return (_flags & PIM_MRE_REGISTER_JOIN_PENDING_STATE);
}

// Return true if state has changed, otherwise return false.
// Note: applies for (S,G)
bool
PimMre::recompute_is_could_register_sg()
{
    if (! is_sg())
	return (false);
    
    if (is_not_could_register_sg())
	goto not_could_register_sg_label;
    if (is_could_register_sg())
	goto could_register_sg_label;
    XLOG_ASSERT(false);
    return (false);
    
 not_could_register_sg_label:
    // CouldRegister -> true
    if (! compute_is_could_register_sg())
	return (false);		// Nothing changed
    
    if (is_register_noinfo_state()) {
	// Register NoInfo state -> Register Join state
	set_register_join_state();
	// Add reg tunnel
	add_register_tunnel();
    }
    // Set the new state
    set_could_register_sg();
    return (true);
    
 could_register_sg_label:
    // CouldRegister -> false
    if (compute_is_could_register_sg())
	return (false);		// Nothing changed
    
    if (is_register_join_state()
	|| is_register_join_pending_state()
	|| is_register_prune_state()) {
	// Register Join state -> Register NoInfo state
	set_register_noinfo_state();
	// Remove reg tunnel
	remove_register_tunnel();
    }
    // Set the new state
    set_not_could_register_sg();
    return (true);
}

void
PimMre::register_stop_timer_timeout()
{
    PimVif *pim_vif = NULL;
    
    if (! is_sg())
	return;
    
    if (is_register_noinfo_state())
	goto register_noinfo_state;
    if (is_register_join_state())
	goto register_join_state;
    if (is_register_join_pending_state())
	goto register_join_pending_state;
    if (is_register_prune_state())
	goto register_prune_state;
    
 register_noinfo_state:
    // Register NoInfo state
    // Ignore
    return;
    
 register_join_state:
    // Register Join state
    // Ignore
    return;
    
 register_join_pending_state:
    // Register JoinPending state
    // Register JoinPending state -> Register Join state
    set_register_join_state();
    // Add reg tunnel
    add_register_tunnel();
    return;
    
 register_prune_state:
    // Register Prune state
    // Register Prune state -> Register JoinPending state
    set_register_join_pending_state();
    // Stop timer(**) (** The RegisterStopTimer is set to Register_Probe_Time
    register_stop_timer() =
	pim_node().event_loop().new_oneoff_after(
	    TimeVal(PIM_REGISTER_PROBE_TIME_DEFAULT, 0),
	    callback(this, &PimMre::register_stop_timer_timeout));
    // Send Null Register
    pim_vif = pim_node().vif_find_direct(source_addr());
    if ((pim_vif != NULL) && (rp_addr_ptr() != NULL)) {
	pim_vif->pim_register_null_send(*rp_addr_ptr(),
					source_addr(),
					group_addr());
    }
    return;
}

// Note: applies for (S,G)
void
PimMre::receive_register_stop()
{
    if (! is_sg())
	return;
    
    TimeVal register_stop_tv;
    TimeVal register_probe_tv;
    
    if (is_register_noinfo_state())
	goto register_noinfo_state_label;
    if (is_register_join_state())
	goto register_join_state_label;
    if (is_register_join_pending_state())
	goto register_join_pending_state_label;
    if (is_register_prune_state())
	goto register_prune_state_label;
    
    XLOG_ASSERT(false);
    return;
    
 register_noinfo_state_label:
    // Register NoInfo state
    return;		// Ignore
    
 register_join_state_label:
    // Register Join state
    // Register Join state -> Register Prune state
    set_register_prune_state();
    // Remove reg tunnel
    remove_register_tunnel();
    // Set Register-Stop timer
    register_stop_tv.set(PIM_REGISTER_SUPPRESSION_TIME_DEFAULT, 0);
    register_stop_tv = positive_random_uniform(register_stop_tv, 0.5);
    register_probe_tv.set(PIM_REGISTER_PROBE_TIME_DEFAULT, 0);
    register_stop_tv -= register_probe_tv;
    register_stop_timer() =
	pim_node().event_loop().new_oneoff_after(
	    register_stop_tv,
	    callback(this, &PimMre::register_stop_timer_timeout));
    return;
    
 register_join_pending_state_label:
    // Register JoinPending state
    // Register JoinPending state -> Register Prune state
    set_register_prune_state();
    // Set Register-Stop timer
    register_stop_tv.set(PIM_REGISTER_SUPPRESSION_TIME_DEFAULT, 0);
    register_stop_tv = positive_random_uniform(register_stop_tv, 0.5);
    register_probe_tv.set(PIM_REGISTER_PROBE_TIME_DEFAULT, 0);
    register_stop_tv -= register_probe_tv;
    register_stop_timer() =
	pim_node().event_loop().new_oneoff_after(
	    register_stop_tv,
	    callback(this, &PimMre::register_stop_timer_timeout));
    return;
    
 register_prune_state_label:
    // Register Prune state
    return;		// Ignore
}

//
// Perform the "RP changed" action at the (S,G) register state-machine
// Note that the RP has already changed and assigned by the method that
// calls this one, hence we unconditionally take the "RP changed" actions.
//
// Note: applies for (S,G)
void
PimMre::rp_register_sg_changed()
{
    if (! is_sg())
	return;
    
    if (is_register_noinfo_state())
	goto register_noinfo_state_label;
    if (is_register_join_state())
	goto register_join_state_label;
    if (is_register_join_pending_state())
	goto register_join_pending_state_label;
    if (is_register_prune_state())
	goto register_prune_state_label;
    
    XLOG_ASSERT(false);
    return;
    
 register_noinfo_state_label:
    // Register NoInfo state
    return;		// Ignore
    
 register_join_state_label:
    // Register Join state
    // Register Join state -> Register Join state
    // Update Register tunnel
    update_register_tunnel();
    return;
    
 register_join_pending_state_label:
    // Register JoinPending state
    // Register JoinPending state -> Register Join state
    set_register_join_state();
    // Add reg tunnel
    add_register_tunnel();
    // Cancel Register-Stop timer
    register_stop_timer().unschedule();
    return;
    
 register_prune_state_label:
    // Register Prune state
    // Register Prune state -> Register Join state
    set_register_join_state();
    // Add reg tunnel
    add_register_tunnel();
    // Cancel Register-Stop timer
    register_stop_timer().unschedule();
    return;
}

// XXX: The Register vif implementation is not RP-specific, so it is
// sufficient to update the DownstreamJpState(S,G,VI) only.
// Note: applies for (S,G)
void
PimMre::add_register_tunnel()
{
    uint16_t register_vif_index;
    
    if (! is_sg())
	return;
    
    register_vif_index = pim_register_vif_index();
    if (register_vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    set_downstream_join_state(register_vif_index);
}

// XXX: The Register vif implementation is not RP-specific, so it is
// sufficient to update the DownstreamJpState(S,G,VI) only.
// Note: applies for (S,G)
void
PimMre::remove_register_tunnel()
{
    uint16_t register_vif_index;
    
    if (! is_sg())
	return;

    register_vif_index = pim_register_vif_index();
    if (register_vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    set_downstream_noinfo_state(register_vif_index);
}

// XXX: nothing to do because the Register vif implementation
// is not RP-specific.
// Note: applies for (S,G)
void
PimMre::update_register_tunnel()
{
    if (! is_sg())
	return;
    
    return;
}
