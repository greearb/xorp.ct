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

#ident "$XORP: xorp/pim/pim_mre_assert.cc,v 1.25 2003/06/28 00:27:33 pavlin Exp $"

//
// PIM Multicast Routing Entry Assert handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mre.hh"
#include "pim_mrt.hh"
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

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::i_am_assert_winner_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_wc->i_am_assert_winner_state();
    
    return (mifs);
}

// Note: works only for (S,G)
const Mifset&
PimMre::i_am_assert_winner_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = i_am_assert_winner_state();
    
    return (mifs);
}

// Note: applies for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::i_am_assert_loser_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_wc->i_am_assert_loser_state();
    
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::i_am_assert_loser_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = i_am_assert_loser_state();
    
    return (mifs);
}

// Note: applies for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::lost_assert_wc() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    if (! (is_wc() || is_sg() || is_sg_rpt())) {
	mifs.reset();
	return (mifs);
    }
    
    // XXX: AssertWinner(*,G,I) != NULL AND AssertWinner(*,G,I) != me
    mifs = i_am_assert_loser_wc();
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::lost_assert_sg() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    // XXX: AssertWinner(S,G,I) != NULL AND AssertWinner(S,G,I) != me
    mifs = i_am_assert_loser_sg();
    mifs &= assert_winner_metric_is_better_than_spt_assert_metric_sg();	// XXX
    vif_index = rpf_interface_s();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (S,G) and (S,G,rpt)
const Mifset&
PimMre::lost_assert_sg_rpt() const
{
    static Mifset mifs;
    const PimMre *pim_mre_sg = NULL;
    uint16_t vif_index;
    
    if (! (is_sg() || is_sg_rpt())) {
	mifs.reset();
	return (mifs);
    }
    
    mifs.reset();
    
    do {
	if (is_sg()) {
	    pim_mre_sg = this;
	    break;
	}
	if (is_sg_rpt()) {
	    pim_mre_sg = sg_entry();
	    break;
	}
	XLOG_UNREACHABLE();
    } while (false);
    
    // XXX: AssertWinner(S,G,I) != NULL AND AssertWinner(S,G,I) != me
    if (pim_mre_sg != NULL)
	mifs = pim_mre_sg->i_am_assert_loser_sg();
    
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    if (pim_mre_sg != NULL) {
	if (pim_mre_sg->is_spt()) {
	    vif_index = pim_mre_sg->rpf_interface_s();
	    if (vif_index != Vif::VIF_INDEX_INVALID)
		mifs.reset(vif_index);
	}
    }
    
    return (mifs);
}

// Note: applies only for (*,G) and (S,G)
void
PimMre::set_could_assert_state(uint16_t vif_index, bool v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (v) {
	if (is_could_assert_state(vif_index))
	    return;			// Nothing changed
	_could_assert_state.set(vif_index);
    } else {
	if (! is_could_assert_state(vif_index))
	    return;			// Nothing changed
	_could_assert_state.reset(vif_index);
    }
}

// Note: applies only for (*,G) and (S,G)
bool
PimMre::is_could_assert_state(uint16_t vif_index) const
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_could_assert_state.test(vif_index));
}

// Note: applies only for (*,G) and (S,G)
void
PimMre::set_assert_tracking_desired_state(uint16_t vif_index, bool v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (v) {
	if (is_assert_tracking_desired_state(vif_index))
	    return;			// Nothing changed
	_assert_tracking_desired_state.set(vif_index);
    } else {
	if (! is_assert_tracking_desired_state(vif_index))
	    return;			// Nothing changed
	_assert_tracking_desired_state.reset(vif_index);
    }
}

// Note: applies only for (*,G) and (S,G)
bool
PimMre::is_assert_tracking_desired_state(uint16_t vif_index) const
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_assert_tracking_desired_state.test(vif_index));
}

// Note: applies only for (*,G) and (S,G)
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
    
    i_am_assert_winner_bool = (my_metric > *assert_metric);
    
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

// Note: applies only for (*,G)
int
PimMre::assert_process_wc(PimVif *pim_vif,
			  AssertMetric *assert_metric,
			  assert_state_t assert_state,
			  bool i_am_assert_winner_bool)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *new_assert_metric;
    
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
	if (*assert_metric > *assert_winner_metric_wc(vif_index)) {
	    // Receive preferred assert
	    goto a2;
	}
	if ((! i_am_assert_winner_bool)
	    && (assert_winner_metric_wc(vif_index)->addr()
		== assert_metric->addr())) {
	    // Receive acceptable assert from current winner
	    goto a2;
	}
	if ((i_am_assert_winner_bool)
	    && (assert_winner_metric_wc(vif_index)->addr()
		== assert_metric->addr())) {
	    // Receive inferior assert from current winner
	    goto a5;
	}
	break;
	
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    return (XORP_ERROR);
    
 a1:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, IPvX::ZERO(family()));
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_wc, vif_index));
    //  * Store self as AssertWinner(*,G,I)
    //  * Store rpt_assert_metric(G,I) as AssertWinnerMetric(*,G,I)
    new_assert_metric = new AssertMetric(*rpt_assert_metric(vif_index));
    set_assert_winner_metric_wc(vif_index, new_assert_metric);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
 a2:
    //  * Store new assert winner as AssertWinner(*,G,I) and assert winner
    //    metric as AssertWinnerMetric(*,G,I)
    new_assert_metric = new AssertMetric(*assert_metric);
    set_assert_winner_metric_wc(vif_index, new_assert_metric);
    //  * Set timer to Assert_Time
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_wc, vif_index));
    set_i_am_assert_loser_state(vif_index);
    return (XORP_OK);
    
 a3:
    //  * Send Assert(*,G)
    pim_vif->pim_assert_mre_send(this, IPvX::ZERO(family()));
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_wc, vif_index));
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
    // XXX: a4 is not triggered by receiving of an Assert message.
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (XORP_OK);
}

// Note: applies only for (S,G)
int
PimMre::assert_process_sg(PimVif *pim_vif,
			  AssertMetric *assert_metric,
			  assert_state_t assert_state,
			  bool i_am_assert_winner_bool)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *new_assert_metric;
    
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
	if (*assert_metric > *assert_winner_metric_sg(vif_index)) {
	    // Receive preferred assert
	    goto a2;
	}
	if ((! i_am_assert_winner_bool)
	    && (assert_winner_metric_sg(vif_index)->addr()
		== assert_metric->addr())) {
	    // Receive acceptable assert from current winner
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
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    return (XORP_ERROR);
    
 a1:
    //  * Send Assert(S,G)
    pim_vif->pim_assert_mre_send(this, source_addr());
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
    //  * Store self as AssertWinner(S,G,I)
    //  * Store spt_assert_metric(S,I) as AssertWinnerMetric(S,G,I)
    new_assert_metric = new AssertMetric(*spt_assert_metric(vif_index));
    set_assert_winner_metric_sg(vif_index, new_assert_metric);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
 a2:
    //  * Store new assert winner as AssertWinner(S,G,I) and assert winner
    //    metric as AssertWinnerMetric(S,G,I)
    new_assert_metric = new AssertMetric(*assert_metric);
    set_assert_winner_metric_sg(vif_index, new_assert_metric);
    //  * Set timer to Assert_Time
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
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
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
    
    // XXX: a4 is not triggered by receiving of an Assert message.
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (XORP_OK);
    
 a6:
    //  * Store new assert winner as AssertWinner(S,G,I) and assert winner
    //    metric as AssertWinnerMetric(S,G,I)
    new_assert_metric = new AssertMetric(*assert_metric);
    set_assert_winner_metric_sg(vif_index, new_assert_metric);
    //  * Set timer to Assert_Time
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
    //  * If I is RPF_interface(S) Set SPTbit(S,G) to TRUE
    if (vif_index == rpf_interface_s())
	set_spt(true);
    set_i_am_assert_loser_state(vif_index);
    return (XORP_OK);
}

// Note: applies only for (*,G)
int
PimMre::wrong_iif_data_arrived_wc(PimVif *pim_vif,
				  const IPvX& assert_source_addr,
				  bool& is_assert_sent)
{
    uint16_t vif_index = pim_vif->vif_index();
    
    if (! is_wc())
	return (XORP_ERROR);
    
    if (_asserts_rate_limit.test(vif_index))
	return (XORP_OK);	// XXX: we are rate-limiting the Asserts
    
    // Send Assert(*,G)
    if (! is_assert_sent) {
	pim_vif->pim_assert_mre_send(this, assert_source_addr);
	is_assert_sent = true;
    }
    
    // Set the bit-flag, and restart the rate-limited timer (if not running)
    // XXX: On average, the data packets trigger no more than one Assert
    // message per second.
    _asserts_rate_limit.set(vif_index);
    if (! _asserts_rate_limit_timer.scheduled()) {
	_asserts_rate_limit_timer = pim_node().eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &PimMre::asserts_rate_limit_timer_timeout));
    }
    
    return (XORP_OK);
}

// Note: applies only for (S,G)
int
PimMre::wrong_iif_data_arrived_sg(PimVif *pim_vif,
				  const IPvX& assert_source_addr,
				  bool& is_assert_sent)
{
    uint16_t vif_index = pim_vif->vif_index();
    
    if (! is_sg())
	return (XORP_ERROR);
    XLOG_ASSERT(assert_source_addr == source_addr());
    
    if (_asserts_rate_limit.test(vif_index))
	return (XORP_OK);	// XXX: we are rate-limiting the Asserts
    
    // Send Assert(S,G)
    if (! is_assert_sent) {
	pim_vif->pim_assert_mre_send(this, source_addr());
	is_assert_sent = true;
    }
    
    // Set the bit-flag, and restart the rate-limited timer (if not running)
    // XXX: On average, the data packets trigger no more than one Assert
    // message per second.
    _asserts_rate_limit.set(vif_index);
    if (! _asserts_rate_limit_timer.scheduled()) {
	_asserts_rate_limit_timer = pim_node().eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &PimMre::asserts_rate_limit_timer_timeout));
    }
    
    return (XORP_OK);
}

void
PimMre::asserts_rate_limit_timer_timeout()
{
    if (! (is_sg() || is_wc()))
	return;
    
    // Reset the rate-limiting bits
    _asserts_rate_limit.reset();
    
    // XXX: try to remove the entry if it was created just for
    // the purpose of rate-limiting the triggered assert messages.
    entry_try_remove();
}

//
// Data packet received that may trigger an Assert.
//
// XXX: should be applied to (S,G) entry (if such exists). If the
// entry is not (S,G), then we assume that there is no such entry
// in the multicast routing table.
//
// Note: applies for all entries
int
PimMre::data_arrived_could_assert(PimVif *pim_vif,
				  const IPvX& src,
				  const IPvX& dst,
				  bool& is_assert_sent)
{
    uint16_t vif_index = pim_vif->vif_index();
    int ret_value;
    
    //
    // Data packet received that may trigger an Assert
    //
    // First try to apply this event to the (S,G) assert state machine.
    // Only if the (S,G) assert state machine is in NoInfo state, and
    // only if there was no change in the (S,G) assert state machine
    // a result of receiving this message, then apply it to the (*,G)
    // assert state machine.
    //
    do {
	bool is_sg_noinfo_old, is_sg_noinfo_new;
	
	//
	// XXX: strictly speaking, we should try to create
	// the (S,G) state, and explicitly compare the old
	// and new (S,G) assert state.
	// However, we use the observation that if there is no (S,G)
	// routing state, then the receiving of the data packet will not change
	// the (S,G) assert state mchine. Note that this observation is
	// based on the specific details of the (S,G) assert state machine.
	// In particular, the action in NoInfo state when
	// "Data arrives from S to G on I and CouldAssert(S,G,I)".
	// If there is no (S,G) routing state, then the SPTbit cannot
	// be true, and therefore CouldAssert(S,G,I) also cannot be true.
	//
	
	if (! is_sg())
	    break;
	
	is_sg_noinfo_old = is_assert_noinfo_state(vif_index);
	ret_value = data_arrived_could_assert_sg(pim_vif, src, is_assert_sent);
	is_sg_noinfo_new = is_assert_noinfo_state(vif_index);
	
	//
	// If there was transaction in the (S,G) assert state,
	// or if the new (S,G) assert state is not NoInfo, then
	// don't apply this event to the (*,G) assert state machine.
	// In other words, both the old and the new state in the
	// (S,G) assert state machine must be in NoInfo state to
	// apply the event to the (*,G) assert state
	// machine.
	//
	if (is_sg_noinfo_old && is_sg_noinfo_new)
	    break;
	return (ret_value);
    } while (false);
    
    //
    // No transaction occured in the (S,G) assert state machine, and 
    // it is in NoInfo state.
    // Apply the event to the (*,G) assert state machine.
    //
    if (is_wc()) {
	return (data_arrived_could_assert_wc(pim_vif, src, is_assert_sent));
    }
    
    PimMre *pim_mre_wc;
    pim_mre_wc = pim_mrt().pim_mre_find(src, dst, PIM_MRE_WC, PIM_MRE_WC);
    if (pim_mre_wc == NULL) {
	XLOG_ERROR("Internal error lookup/creating PIM multicast routing "
		   "entry for source = %s group = %s",
		   cstring(src),
		   cstring(dst));
	return (XORP_ERROR);
    }
    
    ret_value = pim_mre_wc->data_arrived_could_assert_wc(pim_vif, src,
							 is_assert_sent);
    
    // Try to remove the entry in case we don't need it
    pim_mre_wc->entry_try_remove();
    
    return (ret_value);
}

// Note: applies only for (*,G)
int
PimMre::data_arrived_could_assert_wc(PimVif *pim_vif,
				     const IPvX& assert_source_addr,
				     bool& is_assert_sent)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *new_assert_metric;
    Mifset mifs;
    
    if (! is_wc())
	return (XORP_ERROR);
    
    if (is_assert_noinfo_state(vif_index))
	goto assert_noinfo_state_label;
    return (XORP_OK);
    
 assert_noinfo_state_label:
    // NoInfo state
    mifs = could_assert_wc();
    if (! mifs.test(vif_index)) {
	return (XORP_OK);	// CouldAssert(*,G,I) is false. Ignore.
    }
    goto a1;
    
 a1:
    //  * Send Assert(*,G)
    if (! is_assert_sent) {
	pim_vif->pim_assert_mre_send(this, assert_source_addr);
	is_assert_sent = true;
    }
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_wc, vif_index));
    //  * Store self as AssertWinner(*,G,I)
    //  * Store rpt_assert_metric(G,I) as AssertWinnerMetric(*,G,I)
    new_assert_metric = new AssertMetric(*rpt_assert_metric(vif_index));
    set_assert_winner_metric_wc(vif_index, new_assert_metric);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
}

// Note: applies only for (S,G)
int
PimMre::data_arrived_could_assert_sg(PimVif *pim_vif,
				     const IPvX& assert_source_addr,
				     bool& is_assert_sent)
{
    uint16_t vif_index = pim_vif->vif_index();
    AssertMetric *new_assert_metric;
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
	return (XORP_OK);	// CouldAssert(S,G,I) is false. Ignore.
    }
    goto a1;
    
 a1:
    //  * Send Assert(S,G)
    if (! is_assert_sent) {
	pim_vif->pim_assert_mre_send(this, source_addr());
	is_assert_sent = true;
    }
    //  * Set timer to (Assert_Time - Assert_Override_Interval)
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
    //  * Store self as AssertWinner(S,G,I)
    //  * Store spt_assert_metric(S,I) as AssertWinnerMetric(S,G,I)
    new_assert_metric = new AssertMetric(*spt_assert_metric(vif_index));
    set_assert_winner_metric_sg(vif_index, new_assert_metric);
    set_i_am_assert_winner_state(vif_index);
    return (XORP_OK);
}

// Note: applies for all entries
const Mifset&
PimMre::could_assert_wc() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    mifs = joins_rp();
    if (is_wc() || is_sg() || is_sg_rpt()) {
	mifs |= joins_wc();
	mifs |= pim_include_wc();
    }
    
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_could_assert_wc()
{
    Mifset old_value, new_value, diff_value;
    
    if (! is_wc())
	return (false);
    
    old_value = could_assert_state();
    new_value = could_assert_wc();
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    diff_value = new_value ^ old_value;
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	if (diff_value.test(i))
	    process_could_assert_wc(i, new_value.test(i));
    }
    
    return (true);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::process_could_assert_wc(uint16_t vif_index, bool new_value)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_wc())
	return (false);
    
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
    goto a4;
    
 a4:
    //	* Send AssertCancel(*,G)
    pim_vif->pim_assert_cancel_send(this);
    
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (S,G) and (S,G,rpt)
const Mifset&
PimMre::could_assert_sg() const
{
    static Mifset mifs;
    Mifset mifs2;
    PimMre *pim_mre_sg_rpt;
    uint16_t vif_index;
    
    if (! (is_sg() || is_sg_rpt())) {
	mifs.reset();
	return (mifs);
    }
    
    if (is_sg_rpt()) {
	PimMre *pim_mre_sg = sg_entry();
	if (pim_mre_sg == NULL) {
	    // XXX: if no (S,G) entry, then SPTbit(S,G) is false, hence reset
	    mifs.reset();
	    return (mifs);
	}
	return (pim_mre_sg->could_assert_sg());
    }
    
    if (! is_spt()) {		// XXX: SPTbit(S,G) must be true
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_rp();
    mifs |= joins_wc();
    pim_mre_sg_rpt = sg_rpt_entry();
    if (pim_mre_sg_rpt != NULL)
	mifs &= ~(pim_mre_sg_rpt->prunes_sg_rpt());
    
    mifs2 = pim_include_wc();
    mifs2 &= ~pim_exclude_sg();
    mifs |= mifs2;
    
    mifs &= ~lost_assert_wc();
    mifs |= joins_sg();
    mifs |= pim_include_sg();
    
    vif_index = rpf_interface_s();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_could_assert_sg()
{
    Mifset old_value, new_value, diff_value;
    
    if (! is_sg())
	return (false);
    
    old_value = could_assert_state();
    new_value = could_assert_sg();
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    diff_value = new_value ^ old_value;
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	if (diff_value.test(i))
	    process_could_assert_sg(i, new_value.test(i));
    }
    
    return (true);
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::process_could_assert_sg(uint16_t vif_index, bool new_value)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_sg())
	return (false);
    
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
    goto a4;
    
 a4:
    //	* Send AssertCancel(S,G)
    pim_vif->pim_assert_cancel_send(this);
    
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (*,G)
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
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_wc, vif_index));
    set_i_am_assert_winner_state(vif_index);
    return;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return;
}

// Note: applies only for (*,G)
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
    _assert_timers[vif_index] =
	pim_node().eventloop().new_oneoff_after(
	    TimeVal(pim_vif->assert_time().get(), 0)
	    - TimeVal(pim_vif->assert_override_interval().get(), 0),
	    callback(this, &PimMre::assert_timer_timeout_sg, vif_index));
    set_i_am_assert_winner_state(vif_index);
    return;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return;
}

// Note: works for (*,G), (S,G)
AssertMetric *
PimMre::assert_winner_metric_wc(uint16_t vif_index) const
{
    const PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! (is_wc() || is_sg()))
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
	XLOG_UNREACHABLE();
	return (NULL);
    }
    
    return (assert_winner_metric(vif_index));
}

// Note: works for (*,G), (S,G)
void
PimMre::set_assert_winner_metric_wc(uint16_t vif_index, AssertMetric *v)
{
    PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! (is_wc() || is_sg()))
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

// Note: works for (S,G)
void
PimMre::set_assert_winner_metric_sg(uint16_t vif_index, AssertMetric *v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_sg()) {
	XLOG_UNREACHABLE();
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
	    if ((assert_metric == NULL) || (*v > *assert_metric)) {
		set_value = true;
	    }
	}
	set_assert_winner_metric_is_better_than_spt_assert_metric_sg(
	    vif_index, set_value);
    } while (false);
}

// Note: applies only for (*,G) and (S,G)
void
PimMre::set_assert_winner_metric(uint16_t vif_index, AssertMetric *v)
{
    AssertMetric *old_assert_metric;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;

    old_assert_metric = _assert_winner_metrics[vif_index];
    if (old_assert_metric == v)
	return;		// Nothing changed
    
    if (old_assert_metric != NULL)
	delete old_assert_metric;
    _assert_winner_metrics[vif_index] = v;
}

// Note: works for (*,G), (S,G)
void
PimMre::delete_assert_winner_metric_wc(uint16_t vif_index)
{
    PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! (is_wc() || is_sg()))
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

// Note: works for (S,G)
void
PimMre::delete_assert_winner_metric_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_sg()) {
	XLOG_UNREACHABLE();
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

// Note: applies only for (*,G) and (S,G)
void
PimMre::delete_assert_winner_metric(uint16_t vif_index)
{
    set_assert_winner_metric(vif_index, NULL);
}

// Note: applies only for (S,G)
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

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_winner_nbr_sg_gen_id_changed(uint16_t vif_index,
						      const IPvX& nbr_addr)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index)) {
	if (assert_winner_metric_sg(vif_index)->addr() == nbr_addr)
	    goto a5;
	// This is not the assert winner. Ignore.
	return (false);
    }
    // All other states: ignore the change.
    return (false);
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_winner_nbr_wc_gen_id_changed(uint16_t vif_index,
						      const IPvX& nbr_addr)
{
    PimVif *pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    
    if (pim_vif == NULL)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index)) {
	if (assert_winner_metric_wc(vif_index)->addr() == nbr_addr)
	    goto a5;
	// This is not the assert winner. Ignore.
	return (false);
    }
    // All other states: ignore the change.
    return (false);
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies for (S,G)
const Mifset&
PimMre::assert_tracking_desired_sg() const
{
    static Mifset mifs;
    Mifset mifs2;
    PimMre *pim_mre_sg_rpt;
    uint16_t vif_index;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_rp();
    mifs |= joins_wc();
    
    pim_mre_sg_rpt = sg_rpt_entry();
    if (pim_mre_sg_rpt != NULL)
	mifs &= ~(pim_mre_sg_rpt->prunes_sg_rpt());
    
    mifs2 = pim_include_wc();
    mifs2 &= ~pim_exclude_sg();
    mifs |= mifs2;

    mifs2 &= ~lost_assert_wc();
    mifs |= joins_sg();

    mifs2 = i_am_dr();
    mifs2 |= i_am_assert_winner_sg();
    mifs2 &= local_receiver_include_sg();
    mifs |= mifs2;
    
    if (is_join_desired_sg()) {
	vif_index = rpf_interface_s();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    if (is_join_desired_wc() && (! is_spt())) {
	vif_index = rpf_interface_rp();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    
    return (mifs);
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_tracking_desired_sg()
{
    Mifset old_value, new_value, diff_value;
    
    if (! is_sg())
	return (false);
    
    old_value = assert_tracking_desired_state();
    new_value = assert_tracking_desired_sg();
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    diff_value = new_value ^ old_value;
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	if (diff_value.test(i))
	    process_assert_tracking_desired_sg(i, new_value.test(i));
    }
    
    return (true);
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::process_assert_tracking_desired_sg(uint16_t vif_index, bool new_value)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    // Set the new value
    set_assert_tracking_desired_state(vif_index, new_value);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (true);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (new_value)
	return (true);		// AssertTrackingDesired(S,G,I) -> TRUE: ignore
    // AssertTrackingDesired(S,G,I) -> FALSE
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies for (*,G)
const Mifset&
PimMre::assert_tracking_desired_wc() const
{
    static Mifset mifs;
    Mifset mifs2;
    uint16_t vif_index;
    
    if (! is_wc()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = could_assert_wc();
    
    mifs2 = i_am_dr();
    mifs2 |= i_am_assert_winner_wc();
    mifs2 &= local_receiver_include_wc();
    mifs |= mifs2;
    
    if (is_rpt_join_desired_g()) {
	vif_index = rpf_interface_rp();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    
    return (mifs);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_tracking_desired_wc()
{
    Mifset old_value, new_value, diff_value;
    
    if (! is_wc())
	return (false);
    
    old_value = assert_tracking_desired_state();
    new_value = assert_tracking_desired_wc();
    if (new_value == old_value)
	return (false);			// Nothing changed
    
    diff_value = new_value ^ old_value;
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	if (diff_value.test(i))
	    process_assert_tracking_desired_wc(i, new_value.test(i));
    }
    
    return (true);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::process_assert_tracking_desired_wc(uint16_t vif_index, bool new_value)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    // Set the new value
    set_assert_tracking_desired_state(vif_index, new_value);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (true);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (new_value)
	return (true);		// AssertTrackinDesired(*,G,I) -> TRUE: ignore
    // AssertTrackingDesired(*,G,I) -> FALSE
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (S,G)
AssertMetric *
PimMre::spt_assert_metric(uint16_t vif_index) const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    PimVif *pim_vif;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! is_sg())
	return (NULL);
    
    pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (NULL);
    
    assert_metric.set_addr(pim_vif->addr());
    assert_metric.set_rpt_bit_flag(false);
    assert_metric.set_metric_preference(metric_preference_s());
    assert_metric.set_metric(metric_s());
    
    return (&assert_metric);
}

// Note: applies only for (*,G) and (S,G)
AssertMetric *
PimMre::rpt_assert_metric(uint16_t vif_index) const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    PimVif *pim_vif;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! (is_wc() || is_sg()))
	return (NULL);
    
    pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (NULL);
    
    assert_metric.set_addr(pim_vif->addr());
    assert_metric.set_rpt_bit_flag(true);
    assert_metric.set_metric_preference(metric_preference_rp());
    assert_metric.set_metric(metric_rp());
    
    return (&assert_metric);
}

// Note: applies only for (*,G) and (S,G) (but is used only for (S,G))
AssertMetric *
PimMre::infinite_assert_metric() const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    
    // XXX: in case of Assert, the IP address with minimum value is the loser
    assert_metric.set_addr(IPvX::ZERO(family()));
    assert_metric.set_rpt_bit_flag(true);
    assert_metric.set_metric_preference(PIM_ASSERT_MAX_METRIC_PREFERENCE);
    assert_metric.set_metric(PIM_ASSERT_MAX_METRIC);
    
    return (&assert_metric);
}

// Note: applies only for (S,G) and (S,G,rpt)
uint32_t
PimMre::metric_preference_s() const
{
    Mrib *mrib = mrib_s();
    
    if (mrib != NULL)
	return (mrib->metric_preference());
    
    return (PIM_ASSERT_MAX_METRIC_PREFERENCE);
}

// Note: applies for all entries
uint32_t
PimMre::metric_preference_rp() const
{
    Mrib *mrib = mrib_rp();
    
    if (mrib != NULL)
	return (mrib->metric_preference());
    
    return (PIM_ASSERT_MAX_METRIC_PREFERENCE);
}

// Note: applies only for (S,G) and (S,G,rpt)
uint32_t
PimMre::metric_s() const
{
    Mrib *mrib = mrib_s();
    
    if (mrib != NULL)
	return (mrib->metric());
    
    return (PIM_ASSERT_MAX_METRIC);
}

// Note: applies for all entries
uint32_t
PimMre::metric_rp() const
{
    Mrib *mrib = mrib_rp();
    
    if (mrib != NULL)
	return (mrib->metric());
    
    return (PIM_ASSERT_MAX_METRIC);
}

// Note: applies only for (S,G)
AssertMetric *
PimMre::my_assert_metric_sg(uint16_t vif_index) const
{
    Mifset mifs;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! is_sg())
	return (NULL);
    
    mifs = could_assert_sg();
    if (mifs.test(vif_index))
	return (spt_assert_metric(vif_index));
    
    mifs = could_assert_wc();
    if (mifs.test(vif_index))
	return (rpt_assert_metric(vif_index));
    
    return (infinite_assert_metric());
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_my_assert_metric_sg(uint16_t vif_index)
{
    AssertMetric *my_assert_metric, *winner_metric;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    my_assert_metric = my_assert_metric_sg(vif_index);
    winner_metric = assert_winner_metric_sg(vif_index);
    XLOG_ASSERT(winner_metric != NULL);
    XLOG_ASSERT(my_assert_metric != NULL);
    XLOG_ASSERT(my_assert_metric->addr() != winner_metric->addr());
    // Test if my metric has become better
    if (! (*my_assert_metric > *winner_metric))
	return (false);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_my_assert_metric_wc(uint16_t vif_index)
{
    AssertMetric *my_assert_metric, *winner_metric;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    my_assert_metric = rpt_assert_metric(vif_index);
    winner_metric = assert_winner_metric_wc(vif_index);
    XLOG_ASSERT(winner_metric != NULL);	// TODO: XXX: PAVPAVPAV: is this assert OK? E.g, what about if loser to (S,G) Winner?
    XLOG_ASSERT(my_assert_metric != NULL);
    XLOG_ASSERT(my_assert_metric->addr() != winner_metric->addr());
    // Test if my metric has become better
    if (! (*my_assert_metric > *winner_metric))
	return (false);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

//
// "RPF_interface(S) stops being I"
//
// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_rpf_interface_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (rpf_interface_s() == vif_index)
	return (false);			// Nothing changed
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

//
// "RPF_interface(RP(G)) stops being I"
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_rpf_interface_wc(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (rpf_interface_rp() == vif_index)
	return (false);			// Nothing changed
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (S,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_receive_join_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Note: applies only for (*,G)
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_receive_join_wc(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    // All other states: ignore the change.
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    set_assert_noinfo_state(vif_index);
    return (true);
}
