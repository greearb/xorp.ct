// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_mre_assert.cc,v 1.6 2003/01/30 00:39:32 pavlin Exp $"

//
// PIM Multicast Routing Entry Assert handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mre.hh"
#include "pim_mrt.hh"
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


//
// Assert state
//
// Note: applies only for (*,G) and (S,G)
void
PimMre::set_assert_noinfo_state(uint16_t vif_index)
{
    if (! (is_wc() || is_sg()))
	return;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_assert_noinfo_state(vif_index))
	return;			// Nothing changed
    
    _i_am_assert_winner_state.reset(vif_index);
    _i_am_assert_loser_state.reset(vif_index);
    
    do {
	if (is_sg()) {
	    pim_mrt().add_task_assert_state_sg(vif_index,
					       source_addr(),
					       group_addr());
	    break;
	}
	if (is_wc()) {
	    pim_mrt().add_task_assert_state_wc(vif_index, group_addr());
	    break;
	}
	break;
    } while (false);
    
    // Try to remove the entry
    entry_try_remove();
}

// Note: applies only for (*,G) and (S,G)
void
PimMre::set_i_am_assert_winner_state(uint16_t vif_index)
{
    if (! (is_wc() || is_sg()))
	return;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_i_am_assert_winner_state(vif_index))
	return;			// Nothing changed
    
    _i_am_assert_winner_state.set(vif_index);
    _i_am_assert_loser_state.reset(vif_index);

    do {
	if (is_sg()) {
	    pim_mrt().add_task_assert_state_sg(vif_index,
					       source_addr(),
					       group_addr());
	    break;
	}
	if (is_wc()) {
	    pim_mrt().add_task_assert_state_wc(vif_index, group_addr());
	    break;
	}
	break;
    } while (false);
}

// Note: applies only for (*,G) and (S,G)
void
PimMre::set_i_am_assert_loser_state(uint16_t vif_index)
{
    if (! (is_wc() || is_sg()))
	return;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    // XXX: we always perform the setting, because we always want to add
    // the INPUT_STATE_ASSERT_STATE_* task in case the assert winner
    // has changed.
    // if (is_i_am_assert_loser_state(vif_index))
    //	return;			// Nothing changed
    
    _i_am_assert_winner_state.reset(vif_index);
    _i_am_assert_loser_state.set(vif_index);
    
    do {
	if (is_sg()) {
	    pim_mrt().add_task_assert_state_sg(vif_index,
					       source_addr(),
					       group_addr());
	    break;
	}
	if (is_wc()) {
	    pim_mrt().add_task_assert_state_wc(vif_index, group_addr());
	    break;
	}
	if (is_sg_rpt() || is_rp()) {
	    XLOG_ASSERT(false);
	    break;
	}
    } while (false);
}

// Note: applies only for (*,G) and (S,G)
bool
PimMre::is_assert_noinfo_state(uint16_t vif_index) const
{
    if (! (is_wc() || is_sg()))
	return (true);		// XXX
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (true);		// XXX
    
    return (! (_i_am_assert_winner_state.test(vif_index)
	       || _i_am_assert_loser_state.test(vif_index))
	);
}

// Note: applies only for (*,G) and (S,G)
bool
PimMre::is_i_am_assert_winner_state(uint16_t vif_index) const
{
    if (! (is_wc() || is_sg()))
	return (false);		// XXX
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_i_am_assert_winner_state.test(vif_index));
}

// Note: applies only for (*,G) and (S,G)
bool
PimMre::is_i_am_assert_loser_state(uint16_t vif_index) const
{
    if (! (is_wc() || is_sg()))
	return (false);		// XXX
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_i_am_assert_loser_state.test(vif_index));
}

// Note: applies only for (*,G) and (S,G) entries
int
PimMre::assert_process(PimVif *pim_vif, AssertMetric *assert_metric)
{
    uint16_t vif_index = pim_vif->vif_index();
    int ret_value;
    assert_state_t assert_state;
    bool i_am_assert_winner_bool;
    AssertMetric my_metric(pim_vif->addr());
    
    if (! (is_sg() || is_wc()))
	return (XORP_ERROR);
    
    my_metric.set_rpt_bit_flag(! is_spt());
    my_metric.set_metric_preference(is_spt()?
				    metric_preference_s()
				    : metric_preference_rp());
    my_metric.set_metric(is_spt()?
			 metric_s()
			 : metric_rp());
    
    i_am_assert_winner_bool = my_metric.is_better(assert_metric);
    
    assert_state = ASSERT_STATE_NOINFO;
    do {
	if (is_i_am_assert_winner_state(vif_index)) {
	    assert_state = ASSERT_STATE_WINNER;
	    break;
	}
	if (is_i_am_assert_loser_state(vif_index)) {
	    assert_state = ASSERT_STATE_LOSER;
	    break;
	}
    } while (false);
    
    ret_value = XORP_ERROR;
    if (is_sg()) {
	ret_value = assert_process_sg(pim_vif, assert_metric, assert_state,
				      i_am_assert_winner_bool);
    }
    if (is_wc()) {
	ret_value = assert_process_wc(pim_vif, assert_metric, assert_state,
				      i_am_assert_winner_bool);
    }
    
    return (ret_value);
}

int
PimMre::assert_process_wc(PimVif *pim_vif,
			  AssertMetric *assert_metric,
			  assert_state_t assert_state,
			  bool i_am_assert_winner_bool)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *save_assert_metric;
    
    if (! is_wc())
	return (XORP_ERROR);
    
    switch (assert_state) {
    case ASSERT_STATE_NOINFO:
	if (i_am_assert_winner_bool && assert_metric->rpt_bit_flag()
	    && could_assert_wc().test(vif_index)) {
	    goto a1;
	}
	if ( (! i_am_assert_winner_bool)
	     && assert_metric->rpt_bit_flag()
	     && assert_tracking_desired_wc().test(vif_index)) {
	    goto a2;
	}
	break;
	
    case ASSERT_STATE_WINNER:
	if (i_am_assert_winner_bool) {
	    // Whoever sent the assert is in error
	    goto a3;
	} else {
	    // Receive preferred assert
	    goto a2;
	}
	break;
	
    case ASSERT_STATE_LOSER:
	if (!i_am_assert_winner_bool) {
	    goto a2;
	}
	if (i_am_assert_winner_bool
	    && (assert_winner_metric_wc(vif_index)->addr()
		== assert_metric->addr())) {
	    goto a5;
	}
	break;
	
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    return (XORP_ERROR);
    
 a1:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, IPvX::ZERO(family()));
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_wc);
    //  * Store self as AssertWinner(*,G,I)
    //  * Store rpt_assert_metric(G,I) as AssertWinnerMetric(*,G,I)
    save_assert_metric = assert_winner_metric_wc(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *rpt_assert_metric(vif_index);
    } else {
	save_assert_metric = new AssertMetric(*rpt_assert_metric(vif_index));
	set_assert_winner_metric_wc(vif_index, save_assert_metric);
    }
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
 a2:
    //  * Store new assert winner as AssertWinner(*,G,I) and assert winner
    //    metric as AssertWinnerMetric(*,G,I)
    save_assert_metric = assert_winner_metric_wc(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *assert_metric;
    } else {
	save_assert_metric = new AssertMetric(*assert_metric);
	set_assert_winner_metric_wc(vif_index, save_assert_metric);
    }
    //  * Set timer to Assert_Time
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get(),
		       0,
		       &PimMre::assert_timer_timeout_wc);
    set_i_am_assert_loser_state(vif_index);
    return (XORP_OK);
    
 a3:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, IPvX::ZERO(family()));
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_wc);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
    // XXX: a4 is not triggered by receiving of an Assert message.
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (XORP_OK);
}

int
PimMre::assert_process_sg(PimVif *pim_vif,
			  AssertMetric *assert_metric,
			  assert_state_t assert_state,
			  bool i_am_assert_winner_bool)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *save_assert_metric;
    
    if (! is_sg())
	return (XORP_ERROR);
    
    switch (assert_state) {
    case ASSERT_STATE_NOINFO:
	if (i_am_assert_winner_bool && (! assert_metric->rpt_bit_flag())
	    && could_assert_sg().test(vif_index)) {
	    goto a1;
	}
	if (assert_metric->rpt_bit_flag()
	    && could_assert_sg().test(vif_index)) {
	    goto a1;
	}
	if ( (! i_am_assert_winner_bool)
	     && (! assert_metric->rpt_bit_flag())
	     && assert_tracking_desired_sg().test(vif_index)) {
	    goto a6;	// TODO: or probably a2??
	}
	break;
	
    case ASSERT_STATE_WINNER:
	if (i_am_assert_winner_bool) {
	    // Whoever sent the assert is in error
	    goto a3;
	} else {
	    // Receive preferred assert
	    // TODO: this may affect the value of is_join_desired_sg();
	    goto a2;
	}
	break;
	
    case ASSERT_STATE_LOSER:
	if (! i_am_assert_winner_bool) {
	    goto a2;
	}
	if ((i_am_assert_winner_bool)
	    && (assert_winner_metric_sg(vif_index)->addr()
		== assert_metric->addr())) {
	    // Receive inferior assert from current winner
	    goto a5;
	}
	break;
	
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    return (XORP_ERROR);
    
 a1:
    //  * Send Assert(S,G)
    pim_vif->pim_assert_mre_send(this, source_addr());
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    //  * Store self as AssertWinner(S,G,I)
    //  * Store spt_assert_metric(S,I) as AssertWinnerMetric(S,G,I)
    save_assert_metric = assert_winner_metric_sg(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *spt_assert_metric(vif_index);
    } else {
	save_assert_metric = new AssertMetric(*spt_assert_metric(vif_index));
	set_assert_winner_metric_sg(vif_index, save_assert_metric);
    }
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
 a2:
    //  * Store new assert winner as AssertWinner(S,G,I) and assert winner
    //    metric as AssertWinnerMetric(S,G,I)
    save_assert_metric = assert_winner_metric_sg(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *assert_metric;
    } else {
	save_assert_metric = new AssertMetric(*assert_metric);
	set_assert_winner_metric_sg(vif_index, save_assert_metric);
    }
    //  * Set timer to Assert_Time
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    set_i_am_assert_loser_state(vif_index);
    // XXX: JoinDesired(S,G) and PruneDesired(S,G,rpt) may have changed
    // TODO: XXX: PAVPAVPAV: make sure that performing sequentially
    // the actions for each recomputation is OK.
    // TODO: XXX: PAVPAVPAV: OK TO COMMENT?
    // recompute_is_join_desired_sg();
    // if (sg_rpt_entry() != NULL)
    //	sg_rpt_entry()->recompute_is_prune_desired_sg_rpt();
    return (XORP_OK);
    
 a3:
    //  * Send Assert(S,G)
    pim_vif->pim_assert_mre_send(this, source_addr());
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
    // XXX: a4 is not triggered by receiving of an Assert message.
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (XORP_OK);
    
 a6:
    //  * Store new assert winner as AssertWinner(S,G,I) and assert winner
    //    metric as AssertWinnerMetric(S,G,I)
    save_assert_metric = assert_winner_metric_sg(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *assert_metric;
    } else {
	save_assert_metric = new AssertMetric(*assert_metric);
	set_assert_winner_metric_sg(vif_index, save_assert_metric);
    }
    //  * Set timer to Assert_Time
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    //  * If I is RPF_interface(S) Set SPTbit(S,G) to TRUE
    if (vif_index == rpf_interface_s())
	set_spt(true);
    set_i_am_assert_loser_state(vif_index);
    return (XORP_OK);
}

int
PimMre::wrong_iif_data_arrived_wc(PimVif *pim_vif,
				  const IPvX& assert_source_addr)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *save_assert_metric;
    Mifset mifs;

    if (! is_wc()) {		// TODO: OK to enable only for is_wc() ?
	return (XORP_ERROR);
    }
    
    if (is_assert_noinfo_state(vif_index))
	goto assert_noinfo_state_label;
    return (XORP_OK);
    
 assert_noinfo_state_label:
    // NoInfo state
    mifs = could_assert_wc();
    if (! mifs.test(vif_index)) {
	// XXX: "wrong iif" packet arrived, but it is not on one of our
	// outgoing interfaces. Ignore.
	// TODO: if the kernel API allows, disable the "wrong iif"
	// signalling from the kernel.
	// TODO: here probably should be the code for completing
	// the switch to the shortest-path.
	return (XORP_OK);
    }
    set_i_am_assert_winner_state(vif_index);
    goto a1;

 a1:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, assert_source_addr);
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_wc);
    //  * Store self as AssertWinner(*,G,I)
    //  * Store rpt_assert_metric(G,I) as AssertWinnerMetric(*,G,I)
    save_assert_metric = assert_winner_metric_wc(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *rpt_assert_metric(vif_index);
    } else {
	save_assert_metric = new AssertMetric(*rpt_assert_metric(vif_index));
	set_assert_winner_metric_wc(vif_index, save_assert_metric);
    }
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
}

int
PimMre::wrong_iif_data_arrived_sg(PimVif *pim_vif,
				  const IPvX& assert_source_addr)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *save_assert_metric;
    Mifset mifs;
    
    if (! is_sg())
	return (XORP_ERROR);
    XLOG_ASSERT(assert_source_addr == source_addr());
    
    if (is_assert_noinfo_state(vif_index))
	goto assert_noinfo_state_label;
    return (XORP_OK);
    
 assert_noinfo_state_label:
    // NoInfo state
    mifs = could_assert_sg();
    if (! mifs.test(vif_index)) {
	// XXX: "wrong iif" packet arrived, but it is not on one of our
	// outgoing interfaces. Ignore.
	// TODO: if the kernel API allows, disable the "wrong iif"
	// signalling from the kernel.
	// TODO: here probably should be the code for completing
	// the switch to the shortest-path.
	return (XORP_OK);
    }
    set_i_am_assert_winner_state(vif_index);
    goto a1;
    
 a1:
    //  * Send Assert(S,G)
    pim_vif->pim_assert_mre_send(this, source_addr());
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    //  * Store self as AssertWinner(S,G,I)
    //  * Store spt_assert_metric(S,I) as AssertWinnerMetric(S,G,I)
    save_assert_metric = assert_winner_metric_sg(vif_index);
    if (save_assert_metric != NULL) {
	*save_assert_metric = *spt_assert_metric(vif_index);
    } else {
	save_assert_metric = new AssertMetric(*spt_assert_metric(vif_index));
	set_assert_winner_metric_sg(vif_index, save_assert_metric);
    }
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_could_assert_wc(uint16_t vif_index)
{
    bool old_value, new_value;
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_wc())		// TODO: OK to enable is_wc() only?
	return (false);
    
    old_value = is_could_assert_state(vif_index);
    new_value = could_assert_wc().test(vif_index);
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    // Set the new value
    set_could_assert_state(vif_index, new_value);
    
    if (is_i_am_assert_winner_state(vif_index))
	goto assert_winner_state_label;
    // All other states: ignore the change.
    return (true);
    
 assert_winner_state_label:
    // IamAssertWinner state
    if (new_value)
	return (true);		// CouldAssert(*,G,I) -> TRUE: ignore
    // CouldAssert(*,G,I) -> FALSE
    set_assert_noinfo_state(vif_index);
    goto a4;
    
 a4:
    //	* Send AssertCancel(*,G)
    pim_vif->pim_assert_cancel_send(this);
    
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_could_assert_sg(uint16_t vif_index)
{
    bool old_value, new_value;
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_sg())
	return (false);
    
    old_value = is_could_assert_state(vif_index);
    new_value = could_assert_sg().test(vif_index);
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    // Set the new value
    set_could_assert_state(vif_index, new_value);
    
    if (is_i_am_assert_winner_state(vif_index))
	goto assert_winner_state_label;
    // All other states: ignore the change.
    return (true);
    
 assert_winner_state_label:
    // IamAssertWinner state
    if (new_value)
	return (true);		// CouldAssert(S,G,I) -> TRUE: ignore
    // CouldAssert(S,G,I) -> FALSE
    set_assert_noinfo_state(vif_index);
    goto a4;
    
 a4:
    //	* Send AssertCancel(S,G)
    pim_vif->pim_assert_cancel_send(this);
    
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

void
PimMre::assert_timer_timeout_wc(uint16_t vif_index)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return;
    
    if (! is_wc())
	return;
    
    if (is_i_am_assert_winner_state(vif_index))
	goto a3;
    if (is_i_am_assert_loser_state(vif_index))
	goto a5;
    // Assert NoInfo state
    return;
    
 a3:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, IPvX::ZERO(family()));
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_wc);
    set_i_am_assert_winner_state(vif_index);
    return;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return;
}

// TODO: unite actions a3 and a5 below with the original actions earlier
void
PimMre::assert_timer_timeout_sg(uint16_t vif_index)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return;
    
    if (! is_sg())
	return;
    
    if (is_i_am_assert_winner_state(vif_index))
	goto a3;
    if (is_i_am_assert_loser_state(vif_index))
	goto a5;
    // Assert NoInfo state
    return;
    
 a3:
    //  * Send Assert(S,G)
    pim_vif->pim_assert_mre_send(this, source_addr());
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    mifset_timer_start(assert_timers,
		       vif_index,
		       pim_vif->assert_time().get() - pim_vif->assert_override_interval().get(),
		       0,
		       &PimMre::assert_timer_timeout_sg);
    set_i_am_assert_winner_state(vif_index);
    return;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return;
}

// Note: works for (*,G), (S,G), (S,G,rpt)
AssertMetric *
PimMre::assert_winner_metric_wc(uint16_t vif_index) const
{
    const PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL)
	    return (NULL);
    }
    
    return (pim_mre_wc->assert_winner_metric(vif_index));
}

// Note: works for (S,G)
AssertMetric *
PimMre::assert_winner_metric_sg(uint16_t vif_index) const
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! is_sg()) {
	XLOG_ASSERT(false);
	return (NULL);
    }
    
    return (assert_winner_metric(vif_index));
}

void
PimMre::set_assert_winner_metric_wc(uint16_t vif_index, AssertMetric *v)
{
    PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL)
	    return;
    }
    
    pim_mre_wc->set_assert_winner_metric(vif_index, v);
}

void
PimMre::set_assert_winner_metric_sg(uint16_t vif_index, AssertMetric *v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_sg()) {
	XLOG_ASSERT(false);
	return;
    }
    
    set_assert_winner_metric(vif_index, v);
    
    //
    // Set/reset the 'assert_winner_metric_is_better_than_spt_assert_metric_sg'
    // state.
    //
    do {
	bool set_value = false;
	if (v != NULL) {
	    AssertMetric *assert_metric = spt_assert_metric(vif_index);
	    if ((assert_metric == NULL) || (v->is_better(assert_metric))) {
		set_value = true;
	    }
	}
	set_assert_winner_metric_is_better_than_spt_assert_metric_sg(
	    vif_index, set_value);
    } while (false);
}

void
PimMre::delete_assert_winner_metric_wc(uint16_t vif_index)
{
    PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL)
	    return;
    }
    
    pim_mre_wc->delete_assert_winner_metric(vif_index);
}

void
PimMre::delete_assert_winner_metric_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_sg()) {
	XLOG_ASSERT(false);
	return;
    }
    
    delete_assert_winner_metric(vif_index);
    
    //
    // Reset the 'assert_winner_metric_is_better_than_spt_assert_metric_sg'
    // state.
    //
    set_assert_winner_metric_is_better_than_spt_assert_metric_sg(
	vif_index, false);
}

void
PimMre::delete_assert_winner_metric(uint16_t vif_index)
{
    AssertMetric *assert_metric = assert_winner_metric(vif_index);
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (assert_metric != NULL) {
	set_assert_winner_metric(vif_index, NULL);
	delete assert_metric;
    }
}

void
PimMre::set_assert_winner_metric_is_better_than_spt_assert_metric_sg(
    uint16_t vif_index, bool v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (v)
	_assert_winner_metric_is_better_than_spt_assert_metric_sg.set(vif_index);
    else
	_assert_winner_metric_is_better_than_spt_assert_metric_sg.reset(vif_index);
}
