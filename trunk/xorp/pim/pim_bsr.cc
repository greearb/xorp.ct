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

#ident "$XORP: xorp/pim/pim_bsr.cc,v 1.2 2003/01/16 19:32:51 pavlin Exp $"


//
// PIM Bootstrap Router (BSR) mechanism implementation.
// PIM-SMv2 (draft-ietf-pim-sm-bsr-*)
//


#include <math.h>

#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/utils.hh"
#include "pim_proto.h"
#include "pim_bsr.hh"
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
static void pim_bsr_bsr_timer_timeout(void *data_pointer);
static void pim_bsr_scope_zone_expiry_timer_timeout(void *data_pointer);
static void pim_bsr_rp_table_apply_rp_changes_timeout(void *data_pointer);
static void bsr_rp_candidate_rp_expiry_timer_timeout(void *data_pointer);
static void bsr_rp_candidate_rp_advertise_timer_timeout(void *data_pointer);


/**
 * PimBsr::PimBsr:
 * @pim_node: The associated PIM node.
 * 
 * PimBsr constructor.
 **/
PimBsr::PimBsr(PimNode& pim_node)
    : ProtoUnit(pim_node.family(), pim_node.module_id()),
      _pim_node(pim_node)
{
    
}

/**
 * PimBsr::~PimBsr:
 * @void: 
 * 
 * PimBsr destructor.
 * 
 **/
PimBsr::~PimBsr(void)
{
    stop();
    
    // Free the lists
    delete_pointers_list(_active_bsr_zone_list);
    delete_pointers_list(_config_bsr_zone_list);
}

/**
 * PimBsr::start:
 * @void: 
 * 
 * Start the PIM Bootsrap protocol.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
PimBsr::start(void)
{
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    //
    // Activate all configured BSR zones
    //
    list<BsrZone *>::iterator iter;
    for (iter = _config_bsr_zone_list.begin();
	 iter != _config_bsr_zone_list.end();
	 ++iter) {
	BsrZone *config_bsr_zone = *iter;
	
	if (config_bsr_zone->i_am_candidate_bsr()) {
	    string error_msg = "";
	    add_active_bsr_zone(*config_bsr_zone, error_msg);
	}
	config_bsr_zone->start_candidate_rp_advertise_timer();
    }
    
    return (XORP_OK);
}

/**
 * PimBsr::stop:
 * @void: 
 * 
 * Stop the PIM Bootstrap protocol.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimBsr::stop(void)
{
    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    // Perform misc. PIM-specific stop operations
    
    //
    // Send Bootstrap message with lowest priority
    // and Cand-RP-Adv message with holdtime of zero
    //
    list<BsrZone *>::iterator iter;
    for (iter = _active_bsr_zone_list.begin();
	 iter != _active_bsr_zone_list.end();
	 ++iter) {
	BsrZone *config_bsr_zone = *iter;
	BsrZone *active_bsr_zone = NULL;
	
	//
	// Cancel the Cand-RP-Advertise timer
	//
	config_bsr_zone->candidate_rp_advertise_timer().cancel();
	
	//
	// Send Cand-RP-Adv messages with holdtime of zero
	//
	if (! config_bsr_zone->bsr_group_prefix_list().empty()) {
	    //
	    // XXX: create a copy of the configured zone and modify
	    // the holdtime of the Cand-RPs in that zone.
	    //
	    BsrZone tmp_bsr_zone(*this, *config_bsr_zone);
	    list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
	    for (group_prefix_iter = tmp_bsr_zone.bsr_group_prefix_list().begin();
		 group_prefix_iter != tmp_bsr_zone.bsr_group_prefix_list().end();
		 ++group_prefix_iter) {
		BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
		list<BsrRp *>::const_iterator rp_iter;
		for (rp_iter = bsr_group_prefix->rp_list().begin();
		     rp_iter != bsr_group_prefix->rp_list().end();
		     ++rp_iter) {
		    BsrRp *bsr_rp = *rp_iter;
		    bsr_rp->set_rp_holdtime(0);
		}
	    }
	    //
	    // Send the Cand-RP Adv messages
	    //
	    // Find the first vif that is UP and use it to send the messages
	    PimVif *pim_vif = NULL;
	    for (size_t i = 0; i < pim_node().maxvifs(); i++) {
		pim_vif = pim_node().vif_find_by_vif_index(i);
		if (pim_vif == NULL)
		    continue;
		if (! pim_vif->is_up())
		    continue;
		break;	// Found
	    }
	    if (pim_vif != NULL) {
		pim_vif->pim_cand_rp_adv_send(tmp_bsr_zone);
	    } else {
		XLOG_ERROR("Cannot send Cand-RP Adv message: "
			   "cannot find a vif that is UP and running");
	    }
 	}
	
	//
	// Send Bootstrap message with lowest priority
	//
	if (! config_bsr_zone->i_am_candidate_bsr())
	    continue;
	active_bsr_zone = find_bsr_zone_from_list(
	    active_bsr_zone_list(),
	    config_bsr_zone->is_admin_scope_zone(),
	    config_bsr_zone->admin_scope_zone_id());
	if (active_bsr_zone == NULL)
	    continue;
	if (! ((active_bsr_zone->bsr_zone_state()
		== BsrZone::STATE_ELECTED_BSR)
	       || (active_bsr_zone->bsr_zone_state()
		   == BsrZone::STATE_PENDING_BSR))) {
	    continue;
	}
	// TODO: XXX: Sending BSM when in STATE_PENDING_BSR is not
	// in the spec (04/27/2002).
	
	//
	// Create a copy of the BsrZone to send and set
	// the sending priority to lowest.
	//
	// XXX: for simplicity, we create copies of both the active and
	// the configured BsrZone, even though we send only one of them.
	BsrZone send_active_bsr_zone(*this, *active_bsr_zone);
	BsrZone send_config_bsr_zone(*this, *config_bsr_zone);
	BsrZone *send_bsr_zone = NULL;
	if (active_bsr_zone->bsr_zone_state() == BsrZone::STATE_PENDING_BSR)
	    send_bsr_zone = &send_config_bsr_zone;
	else
	    send_bsr_zone = &send_active_bsr_zone;
	// Set the priority to lowest
	send_bsr_zone->set_i_am_candidate_bsr(
	    active_bsr_zone->i_am_candidate_bsr(),
	    active_bsr_zone->my_bsr_addr(),
	    PIM_BOOTSTRAP_LOWEST_PRIORITY);
	
	send_bsr_zone->new_fragment_tag();
	for (uint16_t vif_index = 0; vif_index < pim_node().maxvifs();
	     vif_index++) {
	    PimVif *pim_vif = pim_node().vif_find_by_vif_index(vif_index);
	    if (pim_vif == NULL)
		continue;
	    pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
					*send_bsr_zone);
	}
    }
    
    //
    // TODO: XXX: PAVPAVPAV: Send Cand-RP-Adv with lowest priority
    //
    
    // Remove the list of active BsrZone entries
    delete_pointers_list(_active_bsr_zone_list);
    
    return (XORP_OK);
}

// Return: a pointer to the BSR zone if it exists and there is no
// inconsistency, or if it can and was added, otherwise NULL.
// XXX: if this is the global, non-scoped zone, then
// the zone ID will be set to IPvXNet::ip_multicast_base_prefix()
BsrZone *
PimBsr::add_active_bsr_zone(BsrZone& bsr_zone, string& error_msg)
{
    BsrZone *org_bsr_zone = NULL;
    
    if (! can_add_bsr_zone_to_list(_active_bsr_zone_list, bsr_zone, error_msg))
	return (NULL);
    
    org_bsr_zone = find_bsr_zone_from_list(_active_bsr_zone_list,
					   bsr_zone.is_admin_scope_zone(),
					   bsr_zone.admin_scope_zone_id());
    if (org_bsr_zone == NULL) {
	org_bsr_zone = new BsrZone(*this, bsr_zone);
	_active_bsr_zone_list.push_back(org_bsr_zone);
    }
    
    org_bsr_zone->process_candidate_bsr(bsr_zone);
    
    //
    // Start (or restart) the Cand-RR Expiry Timer in 'org_bsr_zone'
    // for all RPs that were in 'bsr_zone'.
    //
    list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
    for (group_prefix_iter = bsr_zone.bsr_group_prefix_list().begin();
	 group_prefix_iter != bsr_zone.bsr_group_prefix_list().end();
	 ++group_prefix_iter) {
	BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	BsrGroupPrefix *org_bsr_group_prefix
	    = org_bsr_zone->find_bsr_group_prefix(bsr_group_prefix->group_prefix());
	if (org_bsr_group_prefix == NULL)
	    continue;
	list<BsrRp *>::const_iterator rp_iter;
	for (rp_iter = bsr_group_prefix->rp_list().begin();
	     rp_iter != bsr_group_prefix->rp_list().end();
	     ++rp_iter) {
	    BsrRp *bsr_rp = *rp_iter;
	    BsrRp *org_bsr_rp = org_bsr_group_prefix->find_rp(bsr_rp->rp_addr());
	    if (org_bsr_rp == NULL)
		continue;
	    if ((org_bsr_zone->bsr_addr() == org_bsr_zone->my_bsr_addr())
		&& (pim_node().is_my_addr(org_bsr_rp->rp_addr()))) {
		//
		// XXX: If I am the BSR, don't start the timer for my
		// own Cand-RPs
		continue;
	    }
	    org_bsr_rp->start_candidate_rp_expiry_timer();
	}
    }
    
    return (org_bsr_zone);
}

void
PimBsr::delete_active_bsr_zone(BsrZone *old_bsr_zone)
{
    // Remove from the list of active zones
    _active_bsr_zone_list.remove(old_bsr_zone);
    
    delete old_bsr_zone;
}

// Return: a pointer to the BSR zone if it exists and there is no
// inconsistency, or if it can and was added, otherwise NULL.
// XXX: if this is the global, non-scoped zone, then
// the zone ID will be set to IPvXNet::ip_multicast_base_prefix()
BsrZone *
PimBsr::add_config_bsr_zone(const BsrZone& bsr_zone, string& error_msg)
{
    if (! can_add_bsr_zone_to_list(_config_bsr_zone_list, bsr_zone,
				   error_msg))
	return (NULL);
    
    if (find_bsr_zone_from_list(_config_bsr_zone_list,
				bsr_zone.is_admin_scope_zone(),
				bsr_zone.admin_scope_zone_id()) != NULL) {
	error_msg = c_format("already have scope zone %s (%s)",
			     cstring(bsr_zone.admin_scope_zone_id()),
			     bsr_zone.is_admin_scope_zone()?
			     "scoped" : "non-scoped");
	return (NULL);
    }
    
    BsrZone *new_bsr_zone = new BsrZone(*this, bsr_zone);
    _config_bsr_zone_list.push_back(new_bsr_zone);
    
    return (new_bsr_zone);
}

void
PimBsr::delete_config_bsr_zone(BsrZone *old_bsr_zone)
{
    // Remove from the list of configured zones
    _config_bsr_zone_list.remove(old_bsr_zone);
    
    delete old_bsr_zone;
}

// Find the BsrZone that corresponds to @is_admin_scope_zone
// and @group_prefix from the list of active BSR zones.
BsrZone	*
PimBsr::find_active_bsr_zone_by_prefix(bool is_admin_scope_zone,
				       const IPvXNet& group_prefix) const
{
    return (find_bsr_zone_by_prefix_from_list(
	_active_bsr_zone_list, is_admin_scope_zone, group_prefix));
}

// Find the BsrZone that corresponds to @is_admin_scope_zone
// and @group_prefix from the list of configured BSR zones.
BsrZone	*
PimBsr::find_config_bsr_zone_by_prefix(bool is_admin_scope_zone,
				       const IPvXNet& group_prefix) const
{
    return (find_bsr_zone_by_prefix_from_list(
	_config_bsr_zone_list, is_admin_scope_zone, group_prefix));
}

// Find the BsrZone that corresponds to @is_admin_scope_zone
// and @group_prefix from the list of configured BSR zones.
BsrZone	*
PimBsr::find_bsr_zone_by_prefix_from_list(
    const list<BsrZone *>& zone_list,
    bool is_admin_scope_zone,
    const IPvXNet& group_prefix) const
{
    list<BsrZone *>::const_iterator iter_zone;
    BsrZone *best_bsr_zone = NULL;
    
    for (iter_zone = zone_list.begin();
	 iter_zone != zone_list.end();
	 ++iter_zone) {
	BsrZone& bsr_zone = *(*iter_zone);
	if (bsr_zone.is_admin_scope_zone() != is_admin_scope_zone)
	    continue;
	if (! bsr_zone.admin_scope_zone_id().contains(group_prefix))
	    continue;
	if (best_bsr_zone == NULL) {
	    best_bsr_zone = &bsr_zone;
	    continue;
	}
	// XXX: we don't really need to do longest-prefix match selection,
	// because by definition the zones are non-overlapping, but just
	// in case (e.g., if we anyway accept overlapping zones).
	if (best_bsr_zone->admin_scope_zone_id().prefix_len()
	    < bsr_zone.admin_scope_zone_id().prefix_len())
	    best_bsr_zone = &bsr_zone;
    }
    
    return (best_bsr_zone);
}

void
PimBsr::schedule_rp_table_apply_rp_changes()
{
    _rp_table_apply_rp_changes_timer.start(0,
					   0,
					   pim_bsr_rp_table_apply_rp_changes_timeout,
					   this);
}

void
PimBsr::add_rps_to_rp_table()
{
    list<BsrZone *>::iterator iter_zone;
    for (iter_zone = active_bsr_zone_list().begin();
	 iter_zone != active_bsr_zone_list().end();
	 ++iter_zone) {
	BsrZone& bsr_zone = *(*iter_zone);
	list<BsrGroupPrefix *>::const_iterator iter_prefix;
	for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	     iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	     ++iter_prefix) {
	    BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	    if (bsr_group_prefix->received_rp_count()
		< bsr_group_prefix->expected_rp_count()) {
		// Not enough RPs
		continue;
	    }
	    list<BsrRp *>::const_iterator iter_rp;
	    for (iter_rp = bsr_group_prefix->rp_list().begin();
		 iter_rp != bsr_group_prefix->rp_list().end();
		 ++iter_rp) {
		BsrRp *bsr_rp = *iter_rp;
		pim_node().rp_table().add_rp(bsr_rp->rp_addr(),
					     bsr_rp->rp_priority(),
					     bsr_group_prefix->group_prefix(),
					     bsr_zone.hash_masklen(),
					     PimRp::RP_LEARNED_METHOD_BOOTSTRAP);
	    }
	}
    }
    // Apply the changes of the RPs to the PIM multicast routing table.
    pim_node().rp_table().apply_rp_changes();
}


//
// Return true if @bsr_zone can be added to @zone_list, otherwise false.
// If the return value is false, @error_msg contains a message that
// describes the error.
//
bool
PimBsr::can_add_bsr_zone_to_list(const list<BsrZone *>& zone_list,
				 const BsrZone& bsr_zone,
				 string& error_msg) const
{
    list<BsrZone *>::const_iterator iter_zone;
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    list<BsrRp *>::const_iterator iter_rp;
    
    error_msg = "";			// Reset the error message
    
    //
    // Check for consistency
    //
    for (iter_zone = zone_list.begin(); iter_zone != zone_list.end();
	 ++iter_zone) {
	BsrZone *org_bsr_zone = *iter_zone;
	
	if (bsr_zone.admin_scope_zone_id()
	    != org_bsr_zone->admin_scope_zone_id()) {
	    //
	    // Check that zones do not overlap
	    //
	    if (bsr_zone.admin_scope_zone_id().is_overlap(
		org_bsr_zone->admin_scope_zone_id())) {
		error_msg = c_format("overlapping zones %s and %s",
				     cstring(bsr_zone.admin_scope_zone_id()),
				     cstring(org_bsr_zone->admin_scope_zone_id()));
		return (false);
	    }
	    continue;
	}
	
	if (bsr_zone.is_admin_scope_zone()
	    != org_bsr_zone->is_admin_scope_zone()) {
	    //
	    // Check that the zone doesn't change
	    // its scoped/non-scoped characteristic
	    //
	    error_msg = c_format("inconsistent zones: "
	    			 "zone %s changed from %s to %s",
	    			 cstring(org_bsr_zone->admin_scope_zone_id()),
	    			 org_bsr_zone->is_admin_scope_zone()?
	    			 "'scoped'" : "'non-scoped'",
	    			 bsr_zone.is_admin_scope_zone()?
	    			 "'scoped'" : "'non-scoped'");
	    return (false);
	}
	
	if (bsr_zone.bsr_addr() != org_bsr_zone->bsr_addr()) {
	    // A message from a different Bootstrap router.
	    // Such message is OK, regardless whether the new message
	    // can be added or not. Continue checking...
	    continue;
	}
	
	// A message from the same Bootstrap router. Check whether
	// a fragment from the same message, or a new message.
	if (bsr_zone.fragment_tag() != org_bsr_zone->fragment_tag()) {
	    // A new message. The old message will be removed, hence
	    // we can always add the new one. Continue checking...
	    continue;
	}
	
	//
	// A fragment from the same message
	//
	
	// Check the new fragment priority for consistency.
	if (bsr_zone.bsr_priority() != org_bsr_zone->bsr_priority()) {
	    error_msg = c_format("inconsistent fragment: "
				 "new fragment for zone %s has priority %d; "
				 "old fragment has priority %d",
				 cstring(org_bsr_zone->admin_scope_zone_id()),
				 bsr_zone.bsr_priority(),
				 org_bsr_zone->bsr_priority());
	    return (false);
	}
	
	// Check the new fragment hash masklen for consistency
	if (bsr_zone.hash_masklen() != org_bsr_zone->hash_masklen()) {
	    error_msg = c_format("inconsistent fragment: "
				 "new fragment for zone %s has hash masklen %d; "
				 "old fragment has hash masklen %d",
				 cstring(org_bsr_zone->admin_scope_zone_id()),
				 bsr_zone.hash_masklen(),
				 org_bsr_zone->hash_masklen());
	    return (false);
	}
	
	// Check the group prefixes for consistency
	for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	     iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	     ++iter_prefix) {
	    BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	    BsrGroupPrefix *org_bsr_group_prefix
		= org_bsr_zone->find_bsr_group_prefix(
		    bsr_group_prefix->group_prefix());
	    if (org_bsr_group_prefix == NULL)
		continue;
	    // Check the number of expected RPs
	    if (bsr_group_prefix->expected_rp_count()
		!= org_bsr_group_prefix->expected_rp_count()) {
		error_msg = c_format("inconsistent 'RP count': "
				     "new fragment for zone %s has "
				     "'RP count' of %d; "
				     "in the old fragment the count is %d",
				     cstring(org_bsr_zone->admin_scope_zone_id()),
				     bsr_group_prefix->expected_rp_count(),
				     org_bsr_group_prefix->expected_rp_count());
		return (false);
	    }
	}
	
	//
	// Check the list of received RPs
	//
	for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	     iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	     ++iter_prefix) {
	    BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	    BsrGroupPrefix *org_bsr_group_prefix
		= org_bsr_zone->find_bsr_group_prefix(
		    bsr_group_prefix->group_prefix());
	    if (org_bsr_group_prefix == NULL) {
		// A new group prefix. Fine.
		continue;
	    }
	    // Check that the new RPs are not repeating, and that the total
	    // number of RPs is not too large.
	    uint32_t rp_count_sum = org_bsr_group_prefix->received_rp_count();
	    for (iter_rp = bsr_group_prefix->rp_list().begin();
		 iter_rp != bsr_group_prefix->rp_list().end();
		 ++iter_rp) {
		BsrRp *bsr_rp = *iter_rp;
		if (org_bsr_group_prefix->find_rp(bsr_rp->rp_addr()) != NULL) {
		    error_msg = c_format("BSR message fragment for zone %s "
					 "already contains entry for RP %s",
					 cstring(org_bsr_zone->admin_scope_zone_id()),
					 cstring(bsr_rp->rp_addr()));
		    return (false);
		}
		rp_count_sum++;
	    }
	    if (rp_count_sum > org_bsr_group_prefix->expected_rp_count()) {
		error_msg = c_format("inconsistent 'fragment RP count': "
				     "sum of old and new fragments count "
				     "for zone %s is too large: %u while "
				     "the expected count is %u",
				     cstring(org_bsr_zone->admin_scope_zone_id()),
				     rp_count_sum,
				     org_bsr_group_prefix->expected_rp_count());
		return (false);
	    }
	}
	
	//
	// So far so good. Continue checking...
	//
    }
    
    return (true);		// The new BsrZone can be added.
}

// Return true if the information inside 'bsr_zone' is already contained
// into some of the zones in 'zone_list', otherwise return false.
bool
PimBsr::contains_bsr_zone_info(const list<BsrZone *>& zone_list,
			       const BsrZone& bsr_zone) const
{
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    list<BsrRp *>::const_iterator iter_rp;
    BsrZone *org_bsr_zone;

    org_bsr_zone = find_bsr_zone_from_list(
	zone_list,
	bsr_zone.is_admin_scope_zone(),
	bsr_zone.admin_scope_zone_id());
    if (org_bsr_zone == NULL)
	return (false);

    //
    // Check if same BSR zone
    //
    if (bsr_zone.bsr_addr() != org_bsr_zone->bsr_addr()) {
	// A message from a different Bootstrap router
	return (false);
    }
    
    if (bsr_zone.fragment_tag() != org_bsr_zone->fragment_tag()) {
	// A new message from the same Bootstrap router
	return (false);
    }

    if (bsr_zone.bsr_priority() != org_bsr_zone->bsr_priority()) {
	// Fragment priority is different
	return (false);
    }

    if (bsr_zone.hash_masklen() != org_bsr_zone->hash_masklen()) {    
	// The hash masklen is different
	return (false);
    }
    
    //
    // Check if all group prefixes exist already
    //
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	BsrGroupPrefix *org_bsr_group_prefix
	    = org_bsr_zone->find_bsr_group_prefix(
		bsr_group_prefix->group_prefix());
	if (org_bsr_group_prefix == NULL) {
	    // A new group prefix
	    return (false);
	}
	    
	if (bsr_group_prefix->is_admin_scope_zone()
	    != org_bsr_group_prefix->is_admin_scope_zone()) {
	    // Difference in scope zones.
	    // Redundant check, but anyway...
	    return (false);
	}

	if (bsr_group_prefix->expected_rp_count()
	    != org_bsr_group_prefix->expected_rp_count()) {
	    // Different number of expected RPs
	    return (false);
	}
	
	//
	// Check the list of received RPs
	//
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    BsrRp *org_bsr_rp;
	    org_bsr_rp = org_bsr_group_prefix->find_rp(bsr_rp->rp_addr());
	    
	    if (org_bsr_rp == NULL) {
		// A new RP
		return (false);
	    }
	    
	    if (bsr_rp->rp_priority() != org_bsr_rp->rp_priority()) {
		// Different RP priority
		return (false);
	    }
	    
	    if (bsr_rp->rp_holdtime() != org_bsr_rp->rp_holdtime()) {
		// Different RP holdtime
		return (false);
	    }
	}
    }
    
    return (true);		// The new information is redundant
}


// Search @zone_list for a BSR zone with same @is_admin_scope_zone
// and @admin_scope_zone_id
BsrZone *
PimBsr::find_bsr_zone_from_list(const list<BsrZone *>& zone_list,
				bool is_admin_scope_zone,
				const IPvXNet& admin_scope_zone_id) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    for (iter_zone = zone_list.begin(); iter_zone != zone_list.end();
	 ++iter_zone) {
	BsrZone& bsr_zone = *(*iter_zone);
	if ((bsr_zone.is_admin_scope_zone() == is_admin_scope_zone)
	    && (bsr_zone.admin_scope_zone_id() == admin_scope_zone_id))
	    return (&bsr_zone);
    }
    
    return (NULL);
}

BsrZone::BsrZone(PimBsr& pim_bsr, bool is_admin_scope_zone,
		 const IPvXNet& admin_scope_zone_id)
    : _pim_bsr(pim_bsr),
      _bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _bsr_priority(0),
      _hash_masklen(PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(_pim_bsr.family())),
      _fragment_tag(RANDOM(0xffff)),
      _is_admin_scope_zone(is_admin_scope_zone),
      _admin_scope_zone_id(admin_scope_zone_id),
      _i_am_candidate_bsr(false),		// XXX: disable by default
      _my_bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _my_bsr_priority(0),			// XXX: lowest priority
      _is_bsm_forward(false),
      _is_bsm_originate(false),
      _is_accepted_previous_bsm(false)
{
    set_bsr_zone_state(BsrZone::STATE_INIT);
}

BsrZone::BsrZone(PimBsr& pim_bsr, const IPvX& bsr_addr, uint8_t bsr_priority,
		 uint8_t hash_masklen, uint16_t fragment_tag)
    : _pim_bsr(pim_bsr),
      _bsr_addr(bsr_addr),
      _bsr_priority(bsr_priority),
      _hash_masklen(hash_masklen),
      _fragment_tag(fragment_tag),
      _is_admin_scope_zone(false),	// XXX: no scope zone by default
      _admin_scope_zone_id(IPvXNet::ip_multicast_base_prefix(_pim_bsr.family())),
      _i_am_candidate_bsr(false),		// XXX: disable by default
      _my_bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _my_bsr_priority(0),			// XXX: lowest priority
      _is_bsm_forward(false),
      _is_bsm_originate(false),
      _is_accepted_previous_bsm(false)
{
    set_bsr_zone_state(BsrZone::STATE_INIT);
}

BsrZone::BsrZone(PimBsr& pim_bsr, const BsrZone& bsr_zone)
    : _pim_bsr(pim_bsr),
      _bsr_addr(bsr_zone.bsr_addr()),
      _bsr_priority(bsr_zone.bsr_priority()),
      _hash_masklen(bsr_zone.hash_masklen()),
      _fragment_tag(bsr_zone.fragment_tag()),
      _is_admin_scope_zone(bsr_zone.is_admin_scope_zone()),
      _admin_scope_zone_id(bsr_zone.admin_scope_zone_id()),
      _bsr_zone_state(bsr_zone.bsr_zone_state()),
      _i_am_candidate_bsr(bsr_zone.i_am_candidate_bsr()),
      _my_bsr_addr(bsr_zone.my_bsr_addr()),
      _my_bsr_priority(bsr_zone.my_bsr_priority()),
      _is_bsm_forward(bsr_zone.is_bsm_forward()),
      _is_bsm_originate(bsr_zone.is_bsm_originate()),
      _is_accepted_previous_bsm(bsr_zone.is_accepted_previous_bsm())
{
    // Conditionally set the Bootstrap timer
    if (bsr_zone.const_bsr_timer().is_set()) {
	struct timeval left_timeval;
	bsr_zone.const_bsr_timer().left_timeval(&left_timeval);
	_bsr_timer.start(TIMEVAL_SEC(&left_timeval),
			 TIMEVAL_USEC(&left_timeval),
			 pim_bsr_bsr_timer_timeout,
			 this);
    }
    
    // Conditionally set the Scone-Zone Expiry timer
    if (bsr_zone.const_scope_zone_expiry_timer().is_set()) {
	struct timeval left_timeval;
	bsr_zone.const_scope_zone_expiry_timer().left_timeval(&left_timeval);
	_scope_zone_expiry_timer.start(TIMEVAL_SEC(&left_timeval),
				       TIMEVAL_USEC(&left_timeval),
				       pim_bsr_scope_zone_expiry_timer_timeout,
				       this);
    }
    
    //
    // XXX: Do not conditionally set the C-RP Advertise timer, because
    // it is explicitly start at configured BsrZone only.
    //
    
    //
    // Replace the list of group prefixes
    //
    replace_bsr_group_prefix_list(bsr_zone);
}

//
// Search the list of active BSR zones and find the matching active BSR zone
// same as @this.
//
BsrZone *
BsrZone::find_matching_active_bsr_zone() const
{
    BsrZone *active_bsr_zone = _pim_bsr.find_bsr_zone_from_list(
	_pim_bsr.active_bsr_zone_list(),
	is_admin_scope_zone(),
	admin_scope_zone_id());
    
    return (active_bsr_zone);
}

//
// Replace the list of group prefixes
//
void
BsrZone::replace_bsr_group_prefix_list(const BsrZone& bsr_zone)
{
    list<BsrGroupPrefix *>::const_iterator iter;
    
    delete_pointers_list(_bsr_group_prefix_list);
    
    for (iter = bsr_zone.bsr_group_prefix_list().begin();
	 iter != bsr_zone.bsr_group_prefix_list().end();
	 ++iter) {
	const BsrGroupPrefix *bsr_group_prefix = *iter;
	BsrGroupPrefix *bsr_group_prefix_copy
	    = new BsrGroupPrefix(*this, *bsr_group_prefix);
	_bsr_group_prefix_list.push_back(bsr_group_prefix_copy);
    }
    
    // Set the new BSR
    _bsr_addr = bsr_zone.bsr_addr();
    _bsr_priority = bsr_zone.bsr_priority();
    _hash_masklen = bsr_zone.hash_masklen();
    _fragment_tag = bsr_zone.fragment_tag();
}

//
// Merge the list of group prefixes
//
void
BsrZone::merge_bsr_group_prefix_list(const BsrZone& bsr_zone)
{
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    list<BsrRp *>::const_iterator iter_rp;
    
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	BsrGroupPrefix *org_bsr_group_prefix
	    = find_bsr_group_prefix(bsr_group_prefix->group_prefix());
	if (org_bsr_group_prefix == NULL) {
	    // A new group prefix. Add it to the list.
	    BsrGroupPrefix *new_bsr_group_prefix;
	    new_bsr_group_prefix = new BsrGroupPrefix(*bsr_group_prefix);
	    _bsr_group_prefix_list.push_back(new_bsr_group_prefix);
	    continue;
	}
	// Merge the information about the new RPs.
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    string error_msg = "";	// TODO: XXX: use it
	    BsrRp *bsr_rp = *iter_rp;
	    BsrRp *org_bsr_rp = org_bsr_group_prefix->find_rp(bsr_rp->rp_addr());
	    if (org_bsr_rp != NULL) {
		org_bsr_rp->set_rp_priority(bsr_rp->rp_priority());
		org_bsr_rp->set_rp_holdtime(bsr_rp->rp_holdtime());
		continue;
	    }
	    org_bsr_group_prefix->add_rp(bsr_rp->rp_addr(),
					 bsr_rp->rp_priority(),
					 bsr_rp->rp_holdtime(),
					 error_msg);
	}
    }
}

void
BsrZone::set_admin_scope_zone(bool is_admin_scope_zone,
			      const IPvXNet& admin_scope_zone_id)
{
    _is_admin_scope_zone = is_admin_scope_zone;
    _admin_scope_zone_id = admin_scope_zone_id;
}

BsrZone::~BsrZone()
{
    delete_pointers_list(_bsr_group_prefix_list);
}

bool
BsrZone::is_consistent(string& error_msg) const
{
    error_msg = "";			// Reset the error message
    
    //
    // Check that no two group prefixes are exactly same
    //
    list<BsrGroupPrefix *>::const_iterator iter1, iter2;
    for (iter1 = _bsr_group_prefix_list.begin();
	 iter1 != _bsr_group_prefix_list.end();
	 ++iter1) {
	const BsrGroupPrefix *bsr_group_prefix1 = *iter1;
	iter2 = iter1;
	for (++iter2; iter2 != _bsr_group_prefix_list.end(); ++iter2) {
	    const BsrGroupPrefix *bsr_group_prefix2 = *iter2;
	    if (bsr_group_prefix1->group_prefix()
		== bsr_group_prefix2->group_prefix()) {
		error_msg = c_format("group prefix %s received more than once",
				     cstring(bsr_group_prefix1->group_prefix()));
		return (false);		// No two group prefixes should be same
	    }
	}
    }
    
    //
    // XXX: we could check that an RP address is listed no more than once
    // per group prefix. However, listing an RP more that once is not
    // very harmful: the lastest listing of the RP overrides the previous.
    //
    
    if (! is_admin_scope_zone())
	return (true);
    
    //
    // Scope zone. The first group prefix must be the group range
    // for the entire scope range.
    //
    list<BsrGroupPrefix *>::const_iterator iter;
    iter = _bsr_group_prefix_list.begin();
    if (iter == _bsr_group_prefix_list.end())
	return (true);		// OK: no group prefixes; probably a message
				// to expire the BSR.
    
    const BsrGroupPrefix *bsr_group_prefix = *iter;
    
    //
    // Test that the remaining group prefixes are a subset
    // of 'admin_scope_zone_id'.
    for (++iter;
	 iter != _bsr_group_prefix_list.end();
	 ++iter) {
	bsr_group_prefix = *iter;
	if (! admin_scope_zone_id().contains(
	    bsr_group_prefix->group_prefix())) {
	    error_msg = c_format("group prefix %s is not contained in "
				 "scope zone %s",
				 cstring(bsr_group_prefix->group_prefix()),
				 cstring(admin_scope_zone_id()));
	    return (false);	// ERROR: group prefix is not contained.
	}
    }
    
    return (true);		// OK
}

void
BsrZone::set_i_am_candidate_bsr(bool i_am_candidate_bsr,
				const IPvX& my_bsr_addr,
				uint8_t my_bsr_priority)
{
    _i_am_candidate_bsr = i_am_candidate_bsr;
    _my_bsr_addr = my_bsr_addr;
    _my_bsr_priority = my_bsr_priority;
    
    //
    // Set the other fields appropriately if I am the elected BSR
    //
    if (_i_am_candidate_bsr && (_bsr_addr == _my_bsr_addr)) {
	_bsr_priority = _my_bsr_priority;
    }
}

BsrRp *
BsrZone::add_rp(bool is_admin_scope_zone_init,
		const IPvXNet& group_prefix,
		const IPvX& rp_addr,
		uint8_t rp_priority,
		uint16_t rp_holdtime,
		string& error_msg)
{
    BsrGroupPrefix *bsr_group_prefix;
    BsrRp *bsr_rp = NULL;
    
    error_msg = "";
    
    // Check for consistency
    if (is_admin_scope_zone() != is_admin_scope_zone_init) {
	error_msg = c_format("inconsistent scope zones: "
			     "the BSR zone (%s) is %s, but the RP prefix (%s) is %s",
			     cstring(admin_scope_zone_id()),
			     is_admin_scope_zone()?
			     "scoped" : "non-scoped",
			     cstring(group_prefix),
			     is_admin_scope_zone_init?
			     "scoped" : "non-scoped");
	return (NULL);
    }
    if (! admin_scope_zone_id().contains(group_prefix)) {
	error_msg = c_format("BSR scope zone %s does not contain RP prefix %s",
			     cstring(admin_scope_zone_id()),
			     cstring(group_prefix));
	return (NULL);
    }
    
    bsr_group_prefix = find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL) {
	bsr_group_prefix = add_bsr_group_prefix(is_admin_scope_zone_init,
						group_prefix, 0, error_msg);
	if (bsr_group_prefix == NULL)
	    return (NULL);
    }
    
    bsr_rp = bsr_group_prefix->find_rp(rp_addr);
    if (bsr_rp != NULL) {
	// Matching BsrRp entry found. Update it.
	bsr_rp->set_rp_priority(rp_priority);
	bsr_rp->set_rp_holdtime(rp_holdtime);
	return (bsr_rp);
    }
    
    // Create state for the new BsrRp
    if (bsr_group_prefix->expected_rp_count()
	== bsr_group_prefix->received_rp_count()) {
	if (bsr_group_prefix->expected_rp_count() == ((uint8_t)~0)) {
	    // XXX: too many RPs already
	    // TODO: if the BSR, it should perform more intelligent
	    // selection about which RPs to keep and which to remove
	    // when the number of all Cand-RP is too large.
	    return (NULL);
	}
	bsr_group_prefix->set_expected_rp_count(
	    bsr_group_prefix->expected_rp_count() + 1);	// XXX: ugly
    }
    bsr_rp = bsr_group_prefix->add_rp(rp_addr, rp_priority, rp_holdtime,
				      error_msg);
    
    return (bsr_rp);
}

//
// Find an RP for a BsrZone
//
BsrRp *
BsrZone::find_rp(const IPvXNet& group_prefix, const IPvX& rp_addr) const
{
    BsrGroupPrefix *bsr_group_prefix = find_bsr_group_prefix(group_prefix);
    
    if (bsr_group_prefix == NULL)
	return (NULL);
    
    return (bsr_group_prefix->find_rp(rp_addr));
}

// Return: a pointer to the BSR prefix if it exists and there is no
// inconsistency, or if it can and was added, otherwise NULL.
BsrGroupPrefix *
BsrZone::add_bsr_group_prefix(bool is_admin_scope_zone_init,
			      const IPvXNet& group_prefix_init,
			      uint8_t expected_rp_count_init,
			      string& error_msg)
{
    BsrGroupPrefix *bsr_group_prefix;
    
    error_msg = "";
    
    // Check for consistency
    if (is_admin_scope_zone() != is_admin_scope_zone_init) {
	error_msg = c_format("scope zone mismatch: trying to add "
			     "group prefix %s that is %s "
			     "to zone %s that is %s",
			     cstring(group_prefix_init),
			     is_admin_scope_zone_init? "scoped" : "non-scoped",
			     cstring(admin_scope_zone_id()),
			     is_admin_scope_zone()? "scoped" : "non-scoped");
	return (NULL);
    }
    if (! admin_scope_zone_id().contains(group_prefix_init)) {
	error_msg = c_format("scope zone mismatch: trying to add "
			     "group prefix %s to zone %s: "
			     "the zone does not contain this prefix",
			     cstring(group_prefix_init),
			     cstring(admin_scope_zone_id()));
	return (NULL);
    }
    
    bsr_group_prefix = find_bsr_group_prefix(group_prefix_init);
    
    if (bsr_group_prefix != NULL) {
	if (bsr_group_prefix->is_admin_scope_zone()
	    != is_admin_scope_zone_init) {
	    // Inconsistency with the scope zone
	    XLOG_ASSERT(false);
	    error_msg = c_format("scope zone mismatch: trying to add "
				 "group prefix %s that is %s "
				 "to zone %s that is %s: "
				 "already have group prefix %s that is %s",
				 cstring(group_prefix_init),
				 is_admin_scope_zone_init?
				 "scoped" : "non-scoped",
				 cstring(admin_scope_zone_id()),
				 is_admin_scope_zone()?
				 "scoped" : "non-scoped",
				 cstring(bsr_group_prefix->group_prefix()),
				 bsr_group_prefix->is_admin_scope_zone()?
				 "scoped" : "non-scoped");
	    return (NULL);
	}
	if (expected_rp_count_init > bsr_group_prefix->expected_rp_count()) {
	    // XXX: increase the expected RP count
	    // TODO: is it the right thing to do?
	    bsr_group_prefix->set_expected_rp_count(expected_rp_count_init);
	}
	return (bsr_group_prefix);
    }
    
    bsr_group_prefix = new BsrGroupPrefix(*this,
					  is_admin_scope_zone_init,
					  group_prefix_init,
					  expected_rp_count_init);
    
    _bsr_group_prefix_list.push_back(bsr_group_prefix);
    
    return (bsr_group_prefix);
}

void
BsrZone::delete_bsr_group_prefix(BsrGroupPrefix *bsr_group_prefix)
{
    _bsr_group_prefix_list.remove(bsr_group_prefix);
    
    delete bsr_group_prefix;
}

// Return: a pointer to the BSR prefix if a matching prefix found,
// otherwise NULL.
BsrGroupPrefix *
BsrZone::find_bsr_group_prefix(const IPvXNet& group_prefix) const
{
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    
    for (iter_prefix = bsr_group_prefix_list().begin();
	 iter_prefix != bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	if (bsr_group_prefix->group_prefix() == group_prefix)
	    return (bsr_group_prefix);
    }
    
    return (NULL);
}

// Process a Cand-BSR information for that zone.
// Return true if @bsr_zone is the new BSR, otherwise false.
// XXX: assumes that if necessary, 'Forward BSM' event
// will happen in the parent function.
bool
BsrZone::process_candidate_bsr(BsrZone& bsr_zone)
{
    // TODO: handle the case when reconfiguring the local BSR on-the-fly
    // (not in the spec).
    
    if (bsr_zone.is_accepted_previous_bsm())
	set_accepted_previous_bsm(true);
    
    if (bsr_zone_state() == BsrZone::STATE_INIT) {
	// A new entry
	
	//
	// Set my_bsr_addr and my_bsr_priority if I am a Cand-BSR for this
	// zone.
	//
	BsrZone *config_bsr_zone
	    = pim_bsr().find_bsr_zone_from_list(
		pim_bsr().config_bsr_zone_list(),
		is_admin_scope_zone(),
		admin_scope_zone_id());
	if ((config_bsr_zone != NULL)
	    && config_bsr_zone->i_am_candidate_bsr()) {
	    // I am a Cand-BSR for this zone
	    // -> P-BSR state
	    set_bsr_zone_state(BsrZone::STATE_PENDING_BSR);
	    set_i_am_candidate_bsr(
		config_bsr_zone->i_am_candidate_bsr(),
		config_bsr_zone->bsr_addr(),
		config_bsr_zone->bsr_priority());
	    // Set BS Timer to BS Timeout
	    // TODO: XXX: PAVPAVPAV: send the very first Bootstrap message
	    // after a very short random interval instead?
	    // E.g. after 1/10th of the BS Timeout?
	    // TODO: XXX: PAVPAVPAV: for testing purpose, the interval
	    // is shorter: 1/30th
	    // (Not in the spec).
	    _bsr_timer.start_random(
		PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT/30,
		0,
		pim_bsr_bsr_timer_timeout,
		this,
		0.5);
	    return (false);
	} else {
	    // I am not a Cand-BSR for this zone
	    if (is_admin_scope_zone())
		set_bsr_zone_state(BsrZone::STATE_NO_INFO);
	    else
		set_bsr_zone_state(BsrZone::STATE_ACCEPT_ANY);
	    // XXX: fall-through with the processing below
	    XLOG_ASSERT(! i_am_candidate_bsr());
	}
    }
    
    if (i_am_candidate_bsr())
	goto candidate_bsr_state_machine_label;
    goto non_candidate_bsr_state_machine_label;
    
    
 candidate_bsr_state_machine_label:
    //
    // Per-Scope-Zone State-machine for a Cand-BSR
    //
    if (bsr_zone_state() == BsrZone::STATE_CANDIDATE_BSR)
	goto bsr_zone_state_candidate_bsr_label;
    if (bsr_zone_state() == BsrZone::STATE_PENDING_BSR)
	goto bsr_zone_state_pending_bsr_label;
    if (bsr_zone_state() == BsrZone::STATE_ELECTED_BSR)
	goto bsr_zone_state_elected_bsr_label;
    // Invalid state
    XLOG_ASSERT(false);
    return (false);
    
 bsr_zone_state_candidate_bsr_label:
    //
    // Candidate BSR state
    //
    if (is_new_bsr_preferred(bsr_zone) || is_new_bsr_same_priority(bsr_zone)) {
	// XXX: some of actions below are not in the spec
	// Receive Preferred BSM or BSM from Elected BSR
	// -> C-BSR state
	set_bsr_zone_state(BsrZone::STATE_CANDIDATE_BSR);
	// Forward BSM  : will happen in the parent function
	bsr_zone.set_bsm_forward(true);
	if (is_new_bsr_preferred(bsr_zone)) {
	    // Store RP-Set
	    replace_bsr_group_prefix_list(bsr_zone);
	    timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	} else {
	    // Update the Elected BSR 
	    if (fragment_tag() == bsr_zone.fragment_tag()) {
		// Merge RP-Set
		merge_bsr_group_prefix_list(bsr_zone);
	    } else {
		// Store RP-Set
		replace_bsr_group_prefix_list(bsr_zone);
	    }
	}
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 pim_bsr_bsr_timer_timeout, this);
	return (true);
    }
    if (bsr_addr() == bsr_zone.bsr_addr()) {
	// Receive Non-preferred BSM from Elected BSR
	// -> P-BSR state
	set_bsr_zone_state(BsrZone::STATE_PENDING_BSR);
	// Update Elected BSR preference (TODO: XXX: not in the spec)
	_bsr_addr = bsr_zone.bsr_addr();
	_bsr_priority = bsr_zone.bsr_priority();
	// Set BS Timer to rand_override
	struct timeval rand_override;
	randomized_override_interval(_my_bsr_addr, _my_bsr_priority,
				     &rand_override);
	_bsr_timer.start(TIMEVAL_SEC(&rand_override),
			  TIMEVAL_USEC(&rand_override),
			  pim_bsr_bsr_timer_timeout,
			  this);
	return (false);
    }
    // Receive Non-preferred BSM. Ignore.
    return (false);
    
 bsr_zone_state_pending_bsr_label:
    // Pending BSR state
    if (is_new_bsr_preferred(bsr_zone)) {
	// Receive Preferred BSM
	// -> C-BSR state
	set_bsr_zone_state(BsrZone::STATE_CANDIDATE_BSR);
	// Forward BSM  : will happen in the parent function
	bsr_zone.set_bsm_forward(true);
	// Store RP-Set
	replace_bsr_group_prefix_list(bsr_zone);
	timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 pim_bsr_bsr_timer_timeout, this);
	return (true);
    }
    // Receive Non-preferred BSM
    // -> P-BSR state
    set_bsr_zone_state(BsrZone::STATE_PENDING_BSR);
    return (false);
    
 bsr_zone_state_elected_bsr_label:
    // Elected BSR state
    if (is_new_bsr_preferred(bsr_zone)) {
	// Receive Preferred BSM
	// -> C-BSR state
	set_bsr_zone_state(BsrZone::STATE_CANDIDATE_BSR);
	// Forward BSM  : will happen in the parent function
	bsr_zone.set_bsm_forward(true);
	// Store RP-Set
	replace_bsr_group_prefix_list(bsr_zone);
	timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 pim_bsr_bsr_timer_timeout, this);
	return (true);
    }
    // Receive Non-preferred BSM
    // -> E-BSR state
    set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Originate BSM
    _is_bsm_originate = true;
    // Set BS Timer to BS Period
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
		     pim_bsr_bsr_timer_timeout, this);
    return (false);
    
    
 non_candidate_bsr_state_machine_label:
    //
    // Per-Scope-Zone State-machine for a router not configured as Cand-BSR
    //
    if (bsr_zone_state() == BsrZone::STATE_NO_INFO)
	goto bsr_zone_state_no_info_label;
    if (bsr_zone_state() == BsrZone::STATE_ACCEPT_ANY)
	goto bsr_zone_state_accept_any_label;
    if (bsr_zone_state() == BsrZone::STATE_ACCEPT_PREFERRED)
	goto bsr_zone_state_accept_preferred_label;
    // Invalid state
    XLOG_ASSERT(false);
    return (false);
    
 bsr_zone_state_no_info_label:
    // No Info state
    // -> AP state
    set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
    // Forward BSM  : will happen in the parent function
    bsr_zone.set_bsm_forward(true);
    // Store RP-Set
    replace_bsr_group_prefix_list(bsr_zone);
    timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
    // Set BS Timer to BS Timeout
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
		     pim_bsr_bsr_timer_timeout, this);
    // Set SZ Timer to SZ Timeout
    _scope_zone_expiry_timer.start(PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
				   pim_bsr_scope_zone_expiry_timer_timeout,
				   this);
    return (true);
    
 bsr_zone_state_accept_any_label:
    // Accept Any state
    // -> AP State
    set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
    // Forward BSM  : will happen in the parent function
    bsr_zone.set_bsm_forward(true);
    // Store RP-Set
    replace_bsr_group_prefix_list(bsr_zone);
    timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
    // Set BS Timer to BS Timeout
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
		     pim_bsr_bsr_timer_timeout, this);
    // Set SZ Timer to SZ Timeout
    _scope_zone_expiry_timer.start(PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
				   pim_bsr_scope_zone_expiry_timer_timeout,
				   this);
    return (true);
    
 bsr_zone_state_accept_preferred_label:
    // Accept Preferred state
    if (is_new_bsr_preferred(bsr_zone) || is_new_bsr_same_priority(bsr_zone)) {
	// XXX: some of the actions below are not in the spec
	// Receive Preferred BSM or BSM from Elected BSR
	// -> AP State
	set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
	// Forward BSM  : will happen in the parent function
	bsr_zone.set_bsm_forward(true);
	if (is_new_bsr_preferred(bsr_zone)) {
	    // Store RP-Set
	    replace_bsr_group_prefix_list(bsr_zone);
	    timeout_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	} else {
	    // Update the Elected BSR
	    if (fragment_tag() == bsr_zone.fragment_tag()) {
		// Merge RP-Set
		merge_bsr_group_prefix_list(bsr_zone);
	    } else {
		// Store RP-Set
		replace_bsr_group_prefix_list(bsr_zone);
	    }
	}
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 pim_bsr_bsr_timer_timeout, this);
	// Set SZ Timer to SZ Timeout
	_scope_zone_expiry_timer.start(
	    PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
	    pim_bsr_scope_zone_expiry_timer_timeout,
	    this);
	return (true);
    }
    // Receive Non-preferred BSM
    // -> AP state
    set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
    return (false);
}

//
// TODO: XXX: PAVPAVPAV: fix the spec for IPv6 (and probably even IPv4)
//
void
BsrZone::randomized_override_interval(const IPvX& my_addr,
				      uint8_t my_priority,
				      struct timeval *result_timeval) const
{
    double addr_delay, delay;
    uint8_t best_priority = max(bsr_priority(), my_priority);
    uint8_t priority_diff;
    uint8_t my_addr_array[sizeof(IPvX)];
    uint8_t stored_addr_array[sizeof(IPvX)];
    uint32_t my_addr_int, stored_addr_int;

    // Get the address values
    my_addr.copy_out(my_addr_array);
    bsr_addr().copy_out(stored_addr_array);
    
    // Get the integer value of the first 4 octets of the addresses
    // TODO: at the time of writing (draft-ietf-pim-sm-bsr-01.ps),
    // the Randomized Override Interval is unclear about how to compute
    // its value for IPv6 addresses (see page 11), therefore to make
    // those formulas applicable to IPv6 as well we use only the first
    // four octets of the IPv6 addresses.
    my_addr_int = 0;
    stored_addr_int = 0;
    for (uint32_t i = 0; i < sizeof(my_addr_int); i++) {
	my_addr_int = (my_addr_int << 8) + my_addr_array[i];
	stored_addr_int = (stored_addr_int << 8) + stored_addr_array[i];
    }
    
    // Compute AddrDelay
    if (bsr_priority() == my_priority) {
	uint32_t addr_diff;
	if (stored_addr_int > my_addr_int)
	    addr_diff = stored_addr_int - my_addr_int;
	else
	    addr_diff = 1;		// XXX
	
	addr_delay = log((double)addr_diff)/log((double)2);	// log2()
	addr_delay /= 16;
    } else {
	addr_delay = 2 - ((double)my_addr_int/(1 << 31));
    }
    
    if (best_priority >= my_priority)
	priority_diff = best_priority - my_priority;
    else
	priority_diff = 0;		// XXX
    
    delay = PIM_BOOTSTRAP_RAND_OVERRIDE_DEFAULT
	+ 2*log((double)(1 + priority_diff))
	+ addr_delay;
    
    if (result_timeval != NULL) {
	uint32_t sec, usec;
	
	sec = (uint32_t)delay;
	usec = (uint32_t)((delay - sec)*1000000);
	TIMEVAL_SET(result_timeval, sec, usec);
    }
}

//
// (Re)start the BsrTimer so it will to expire immediately.
//
void
BsrZone::timeout_bsr_timer()
{
    _bsr_timer.start(0, 0, pim_bsr_bsr_timer_timeout, this);
}

static void
pim_bsr_bsr_timer_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    PimNode& pim_node = bsr_zone.pim_bsr().pim_node();
    
    if (bsr_zone.bsr_zone_state() == BsrZone::STATE_CANDIDATE_BSR)
	goto bsr_zone_state_candidate_bsr_label;
    if (bsr_zone.bsr_zone_state() == BsrZone::STATE_PENDING_BSR)
	goto bsr_zone_state_pending_bsr_label;
    if (bsr_zone.bsr_zone_state() == BsrZone::STATE_ELECTED_BSR)
	goto bsr_zone_state_elected_bsr_label;
    if (bsr_zone.bsr_zone_state() == BsrZone::STATE_ACCEPT_PREFERRED)
	goto bsr_zone_state_accept_preferred_label;
    // Invalid state
    XLOG_ASSERT(false);
    return;
    
 bsr_zone_state_candidate_bsr_label:
    // Candidate BSR state
    // -> P-BSR state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_PENDING_BSR);
    // Set BS Timer to rand_override
    struct timeval rand_override;
    bsr_zone.randomized_override_interval(bsr_zone.my_bsr_addr(),
					  bsr_zone.my_bsr_priority(),
					  &rand_override);
    bsr_zone.bsr_timer().start(TIMEVAL_SEC(&rand_override),
			       TIMEVAL_USEC(&rand_override),
			       pim_bsr_bsr_timer_timeout,
			       &bsr_zone);
    
    return;
    
 bsr_zone_state_pending_bsr_label:
    // Pending BSR state
    // -> E-BSR state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Store RP-Set
    {
	BsrZone *config_bsr_zone
	    = bsr_zone.pim_bsr().find_bsr_zone_from_list(
		bsr_zone.pim_bsr().config_bsr_zone_list(),
		bsr_zone.is_admin_scope_zone(),
		bsr_zone.admin_scope_zone_id());
	XLOG_ASSERT(config_bsr_zone != NULL);
	// TODO: XXX: PAVPAVPAV: need to be careful with the above assert in
	// case we have reconfigured the BSR on-the-fly
	bsr_zone.replace_bsr_group_prefix_list(*config_bsr_zone);
	// TODO: XXX: PAVPAVPAV: do we want to set the fragment_tag to
	// something new here?
	
	// Add the RPs to the RP table
	bsr_zone.pim_bsr().add_rps_to_rp_table();
    }
    // Originate BSM
    bsr_zone.new_fragment_tag();
    for (uint16_t vif_index = 0; vif_index < pim_node.maxvifs(); vif_index++) {
	PimVif *pim_vif = pim_node.vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    continue;
	pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
				    bsr_zone);
    }
    
    // Set BS Timer to BS Period
    bsr_zone.bsr_timer().start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
			       pim_bsr_bsr_timer_timeout, &bsr_zone);
    return;
    
 bsr_zone_state_elected_bsr_label:
    // Elected BSR state
    // -> E-BSR state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Originate BSM
    bsr_zone.new_fragment_tag();
    for (uint16_t vif_index = 0; vif_index < pim_node.maxvifs(); vif_index++) {
	PimVif *pim_vif = pim_node.vif_find_by_vif_index(vif_index);
	if (pim_vif == NULL)
	    continue;
	pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
				    bsr_zone);
    }
    // Set BS Timer to BS Period
    bsr_zone.bsr_timer().start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
			       pim_bsr_bsr_timer_timeout, &bsr_zone);
    return;
    
 bsr_zone_state_accept_preferred_label:
    // Accept Preferred state
    // -> AA State
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ACCEPT_ANY);
    return;
}

static void
pim_bsr_scope_zone_expiry_timer_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    
    if (bsr_zone.bsr_zone_state() == BsrZone::STATE_ACCEPT_ANY)
	goto bsr_zone_state_accept_any_label;
    // Invalid state
    XLOG_ASSERT(false);
    return;
    
 bsr_zone_state_accept_any_label:
    // No Info state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_NO_INFO);
    // Cancel timers
    // Clear state
    bsr_zone.pim_bsr().delete_active_bsr_zone(&bsr_zone);
    return;
}

// Time to apply the changes to the RP Table
static void
pim_bsr_rp_table_apply_rp_changes_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    PimBsr& pim_bsr = *(PimBsr *)data_pointer;
    
    // Apply the changes
    pim_bsr.pim_node().rp_table().apply_rp_changes();
}

// Return true if the received BSR is preferred than the current one.
bool
BsrZone::is_new_bsr_preferred(const BsrZone& bsr_zone) const
{
    const IPvX& compare_bsr_addr = bsr_addr();
    uint8_t compare_bsr_priority = bsr_priority();
    
    if (bsr_zone.bsr_priority() > compare_bsr_priority)
	return (true);
    if (bsr_zone.bsr_priority() < compare_bsr_priority)
	return (false);
    if (bsr_zone.bsr_addr() > compare_bsr_addr)
	return (true);
    
    return (false);
}

// Return true if the received BSR is same priority.
bool
BsrZone::is_new_bsr_same_priority(const BsrZone& bsr_zone) const
{
    const IPvX& compare_bsr_addr = bsr_addr();
    uint8_t compare_bsr_priority = bsr_priority();
    
    if ((bsr_zone.bsr_priority() == compare_bsr_priority)
	&& (bsr_zone.bsr_addr() == compare_bsr_addr))
	return (true);
    
    return (false);
}

BsrGroupPrefix::BsrGroupPrefix(BsrZone& bsr_zone,
			       bool is_admin_scope_zone,
			       const IPvXNet& group_prefix, 
			       uint8_t expected_rp_count)
    : _bsr_zone(bsr_zone),
      _is_admin_scope_zone(is_admin_scope_zone),
      _group_prefix(group_prefix),
      _expected_rp_count(expected_rp_count)
{
    _received_rp_count = 0;
}

BsrGroupPrefix::BsrGroupPrefix(BsrZone& bsr_zone,
			       const BsrGroupPrefix& bsr_group_prefix)
    : _bsr_zone(bsr_zone),
      _is_admin_scope_zone(bsr_group_prefix.is_admin_scope_zone()),
      _group_prefix(bsr_group_prefix.group_prefix()),
      _expected_rp_count(bsr_group_prefix.expected_rp_count()),
      _received_rp_count(bsr_group_prefix.received_rp_count())
{
    //
    // Copy the list of RPs
    //
    list<BsrRp *>::const_iterator iter;
    for (iter = bsr_group_prefix.rp_list().begin();
	 iter != bsr_group_prefix.rp_list().end();
	 ++iter) {
	const BsrRp *bsr_rp = *iter;
	BsrRp *bsr_rp_copy = new BsrRp(*this, *bsr_rp);
	_rp_list.push_back(bsr_rp_copy);
    }
}

BsrGroupPrefix::~BsrGroupPrefix()
{
    delete_pointers_list(_rp_list);
}

// Return: a pointer to the RP entry if it can be added, otherwise NULL.
BsrRp *
BsrGroupPrefix::add_rp(const IPvX& rp_addr, uint8_t rp_priority,
		       uint16_t rp_holdtime, string& error_msg)
{
    BsrRp *bsr_rp = NULL;
    
    if (_received_rp_count >= _expected_rp_count) {
	error_msg = c_format("cannot add RP %s to BSR group prefix %s: "
			     "received RP count %d is not smaller than "
			     "expected RP count %d",
			     cstring(rp_addr),
			     cstring(group_prefix()),
			     _received_rp_count,
			     _expected_rp_count);
	return (NULL);
    }
    
    // Search if we already have an entry for this RP
    for (list<BsrRp *>::iterator iter = _rp_list.begin();
	 iter != _rp_list.end();
	 ++iter) {
	bsr_rp = *iter;
	if (bsr_rp->rp_addr() != rp_addr)
	    continue;
	// Matching RP entry found. Update its holdtime and priority
	bsr_rp->set_rp_priority(rp_priority);
	bsr_rp->set_rp_holdtime(rp_holdtime);
	return (bsr_rp);
    }
    
    // No matching entry found. Add a new one.
    bsr_rp = new BsrRp(*this, rp_addr, rp_priority, rp_holdtime);
    _rp_list.push_back(bsr_rp);
    _received_rp_count++;
    return (bsr_rp);
}

void
BsrGroupPrefix::delete_rp(BsrRp *bsr_rp)
{
    // TODO: XXX: PAVPAVPAV: do we want to do anything else here
    // or in function(s) that call delete_rp()??
    
    for (list<BsrRp *>::iterator iter = _rp_list.begin();
	 iter != _rp_list.end();
	 ++iter) {
	if (bsr_rp != *iter)
	    continue;
	// Entry found. Remove it.
	set_received_rp_count(received_rp_count() - 1);
	_rp_list.erase(iter);
	
	// Delete the RP entry from the RP table
	bsr_zone().pim_bsr().pim_node().rp_table().delete_rp(
	    bsr_rp->rp_addr(),
	    group_prefix(),
	    PimRp::RP_LEARNED_METHOD_BOOTSTRAP);
	bsr_zone().pim_bsr().schedule_rp_table_apply_rp_changes();
	
	delete bsr_rp;
	return;
    }
}

BsrRp *
BsrGroupPrefix::find_rp(const IPvX& rp_addr) const
{
    // Search if we already have an entry for this RP
    for (list<BsrRp *>::const_iterator iter = _rp_list.begin();
	 iter != _rp_list.end();
	 ++iter) {
	BsrRp *bsr_rp = *iter;
	if (bsr_rp->rp_addr() == rp_addr)
	    return (bsr_rp);
    }
    
    return (NULL);
}

BsrRp::BsrRp(BsrGroupPrefix& bsr_group_prefix, const IPvX& rp_addr,
	     uint8_t rp_priority, uint16_t rp_holdtime)
    : _bsr_group_prefix(bsr_group_prefix),
      _rp_addr(rp_addr),
      _rp_priority(rp_priority),
      _rp_holdtime(rp_holdtime)
{
    
}

BsrRp::BsrRp(BsrGroupPrefix& bsr_group_prefix, const BsrRp& bsr_rp)
    : _bsr_group_prefix(bsr_group_prefix),
      _rp_addr(bsr_rp.rp_addr()),
      _rp_priority(bsr_rp.rp_priority()),
      _rp_holdtime(bsr_rp.rp_holdtime())
{
    // Conditionally set the Cand-RP Expiry timer
    if (bsr_rp.const_candidate_rp_expiry_timer().is_set()) {
	struct timeval left_timeval;
	bsr_rp.const_candidate_rp_expiry_timer().left_timeval(&left_timeval);
	_candidate_rp_expiry_timer.start(TIMEVAL_SEC(&left_timeval),
					 TIMEVAL_USEC(&left_timeval),
					 bsr_rp_candidate_rp_expiry_timer_timeout,
					 this);
    }
}

void
BsrRp::start_candidate_rp_expiry_timer()
{
    _candidate_rp_expiry_timer.start(_rp_holdtime, 0,
				     bsr_rp_candidate_rp_expiry_timer_timeout,
				     this);
}

// Time to expire the Cand-RP
static void
bsr_rp_candidate_rp_expiry_timer_timeout(void *data_pointer)
{
    BsrRp *bsr_rp = (BsrRp *)data_pointer;
    
    bsr_rp->bsr_group_prefix().delete_rp(bsr_rp);
}

//
// Start the Candidate-RP-Advertise timer
//
void
BsrZone::start_candidate_rp_advertise_timer()
{
    // TODO: instead of PIM_CAND_RP_ADV_PERIOD_DEFAULT we should use
    // a configurable value
    _candidate_rp_advertise_timer.start(
	PIM_CAND_RP_ADV_PERIOD_DEFAULT,
	0,
	bsr_rp_candidate_rp_advertise_timer_timeout,
	this);
}

//
// Timeout the Candidate-RP-Advertise timer (i.e., schedule it to expire NOW).
// XXX: for simplicitly, we may call this method on active BsrZone.
// However, we use only configured BsrZone to send Cand-RP-Adv.
// Therefore, we need to search for the corresponding config BsrZone and
// expire the timer in that one instead.
// XXX: if no Cand-RP is configured, silently ignore it.
//
void
BsrZone::timeout_candidate_rp_advertise_timer()
{
    //
    // Search for the corresponding config BsrZone
    //
    BsrZone *config_bsr_zone = pim_bsr().find_bsr_zone_from_list(
	pim_bsr().config_bsr_zone_list(),
	is_admin_scope_zone(),
	admin_scope_zone_id());
    
    if (config_bsr_zone == NULL) {
	// Probably I am not configured as a Cand-RP. Ignore.
	return;
    }
    config_bsr_zone->candidate_rp_advertise_timer().start(
	0,
	0,
	bsr_rp_candidate_rp_advertise_timer_timeout,
	config_bsr_zone);
}

// Time to send a Cand-RP-Advertise message to the BSR
static void
bsr_rp_candidate_rp_advertise_timer_timeout(void *data_pointer)
{
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    PimNode& pim_node = bsr_zone.pim_bsr().pim_node();
    PimVif *pim_vif = NULL;

    // Find the first vif that is UP and use it to send the messages
    for (size_t i = 0; i < pim_node.maxvifs(); i++) {
	pim_vif = pim_node.vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	if (! pim_vif->is_up())
	    continue;
	break;	// Found
    }
    if (pim_vif != NULL) {
	pim_vif->pim_cand_rp_adv_send(bsr_zone);
    } else {
	XLOG_ERROR("Cannot send periodic Cand-RP Adv message: "
		   "cannot find a vif that is UP and running");
    }
    
    // Restart the timer
    bsr_zone.start_candidate_rp_advertise_timer();
}
