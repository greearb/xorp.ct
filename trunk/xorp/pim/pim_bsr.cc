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

#ident "$XORP: xorp/pim/pim_bsr.cc,v 1.7 2003/02/26 19:59:39 pavlin Exp $"


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
static void pim_bsr_rp_table_apply_rp_changes_timeout(void *data_pointer);
static void pim_bsr_clean_expire_bsr_zones_timeout(void *data_pointer);
static void bsr_zone_bsr_timer_timeout(void *data_pointer);
static void bsr_zone_scope_zone_expiry_timer_timeout(void *data_pointer);
static void bsr_rp_candidate_rp_expiry_timer_timeout(void *data_pointer);
static void bsr_zone_candidate_rp_advertise_timer_timeout(void *data_pointer);


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
    delete_pointers_list(_config_bsr_zone_list);
    delete_pointers_list(_active_bsr_zone_list);
    delete_pointers_list(_expire_bsr_zone_list);
    delete_pointers_list(_test_bsr_zone_list);
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
	    if (add_active_bsr_zone(*config_bsr_zone, error_msg) == NULL) {
		XLOG_ERROR("Cannot add configured Bootstrap zone %s: %s",
			   cstring(config_bsr_zone->zone_id()),
			   error_msg.c_str());
		stop();
		return (XORP_ERROR);
	    }
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
    PimVif *pim_vif_up = NULL;
    
    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    //
    // Perform misc. PIM-specific stop operations
    //
    
    //
    // Find the first vif that is UP, and use it to unicast the Cand-RP-Adv
    // messages.
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	pim_vif_up = pim_node().vif_find_by_vif_index(i);
	if (pim_vif_up == NULL)
	    continue;
	if (pim_vif_up->is_up())
	    break;
	pim_vif_up = NULL;
    }
    
    //
    // Send Bootstrap message with lowest priority
    // and Cand-RP-Adv message with holdtime of zero
    //
    list<BsrZone *>::iterator iter;
    for (iter = _config_bsr_zone_list.begin();
	 iter != _config_bsr_zone_list.end();
	 ++iter) {
	BsrZone *config_bsr_zone = *iter;
	BsrZone *active_bsr_zone;
	
	active_bsr_zone = find_active_bsr_zone(config_bsr_zone->zone_id());
	if (active_bsr_zone == NULL)
	    continue;		// No active BsrZone yet
	if (! active_bsr_zone->bsr_addr().is_unicast())
	    continue;		// We don't know the BSR address
	if (active_bsr_zone->i_am_bsr())
	    continue;		// I am the BSR, hence don't send the messages
	
	//
	// Cancel the Cand-RP-Advertise timer,
	// and send Cand-RP-Adv messages with holdtime of zero.
	//
	do {
	    if (! config_bsr_zone->candidate_rp_advertise_timer().is_set())
		break;		// We were not sending Cand-RP-Adv messages
	    
	    config_bsr_zone->candidate_rp_advertise_timer().cancel();
	    
	    if (config_bsr_zone->bsr_group_prefix_list().empty())
		break;		// No Cand-RP-Adv to cancel
	    
	    //
	    // Send Cand-RP-Adv messages with holdtime of zero
	    //
	    if (pim_vif_up == NULL) {
		XLOG_ERROR("Cannot send Cand-RP Adv message: "
			   "cannot find a vif that is UP and running");
		break;
	    }
	    config_bsr_zone->set_is_cancel(true);
	    pim_vif_up->pim_cand_rp_adv_send(active_bsr_zone->bsr_addr(),
					     *config_bsr_zone);
	    config_bsr_zone->set_is_cancel(false);
	} while (false);
	
	//
	// Send Bootstrap message with lowest priority
	//
	if (! config_bsr_zone->i_am_candidate_bsr())
	    continue;
	
	//
	// TODO: XXX: Sending BSM-cancel when in STATE_PENDING_BSR is not
	// in the spec (04/27/2002).
	//
	if (! ((active_bsr_zone->bsr_zone_state()
		== BsrZone::STATE_ELECTED_BSR)
	       || (active_bsr_zone->bsr_zone_state()
		   == BsrZone::STATE_PENDING_BSR))) {
	    continue;
	}
	
	active_bsr_zone->new_fragment_tag();
	active_bsr_zone->set_is_cancel(true);
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	    if (pim_vif == NULL)
		continue;
	    pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
					*active_bsr_zone);
	}
	active_bsr_zone->set_is_cancel(false);
    }
    
    // Remove the lists of active and expiring BsrZone entries
    delete_pointers_list(_active_bsr_zone_list);
    delete_pointers_list(_expire_bsr_zone_list);
    
    // Cancel unwanted timers
    _clean_expire_bsr_zones_timer.cancel();
    
    //
    // XXX: don't stop the _rp_table_apply_rp_changes_timer
    // because it is a one-time only timers that is used to
    // synchronize internal state.
    //
    
    return (XORP_OK);
}

// Unicast the Bootstrap message(s) to a (new) neighbor
int
PimBsr::unicast_pim_bootstrap(PimVif *pim_vif, const IPvX& nbr_addr) const
{
    list<BsrZone *>::const_iterator bsr_zone_iter;
    
    if (pim_vif->pim_nbr_find(nbr_addr) == NULL)
	return (XORP_ERROR);
    
    //
    // Unicast the messages with the remaining expiring BSR zones
    // Note that those must be sent BEFORE the active BSR zones,
    // so if somehow there is overlapping, the active BSR zones will
    // override the expiring BSR zones.
    //
    for (bsr_zone_iter = _expire_bsr_zone_list.begin();
	 bsr_zone_iter != _expire_bsr_zone_list.end();
	 ++bsr_zone_iter) {
	BsrZone *bsr_zone = *bsr_zone_iter;
	pim_vif->pim_bootstrap_send(nbr_addr, *bsr_zone);
    }
    
    //
    // Unicast the messages with the active BSR zones
    //
    for (bsr_zone_iter = _active_bsr_zone_list.begin();
	 bsr_zone_iter != _active_bsr_zone_list.end();
	 ++bsr_zone_iter) {
	BsrZone *bsr_zone = *bsr_zone_iter;
	BsrZone::bsr_zone_state_t bsr_zone_state = bsr_zone->bsr_zone_state();
	if ((bsr_zone_state == BsrZone::STATE_CANDIDATE_BSR)
	    || (bsr_zone_state == BsrZone::STATE_ELECTED_BSR)
	    || (bsr_zone_state == BsrZone::STATE_ACCEPT_PREFERRED)) {
	    pim_vif->pim_bootstrap_send(nbr_addr, *bsr_zone);
	}
    }
    
    return (XORP_OK);
}

//
// Return: a pointer to the BSR zone if it exists and there is no
// inconsistency, or if it can and was added, otherwise NULL.
// XXX: if this is the global, non-scoped zone, then
// the zone ID will be set to IPvXNet::ip_multicast_base_prefix()
//
BsrZone *
PimBsr::add_config_bsr_zone(const BsrZone& bsr_zone, string& error_msg)
{
    if (! can_add_config_bsr_zone(bsr_zone, error_msg))
	return (NULL);
    
    BsrZone *new_bsr_zone = new BsrZone(*this, bsr_zone);
    new_bsr_zone->set_config_bsr_zone(true);
    _config_bsr_zone_list.push_back(new_bsr_zone);
    
    return (new_bsr_zone);
}

// Return: a pointer to the BSR zone if it exists and there is no
// inconsistency, or if it can and was added, otherwise NULL.
// XXX: if this is the global, non-scoped zone, then
// the zone ID will be set to IPvXNet::ip_multicast_base_prefix()
BsrZone *
PimBsr::add_active_bsr_zone(const BsrZone& bsr_zone, string& error_msg)
{
    BsrZone *active_bsr_zone = NULL;
    
    if (! can_add_active_bsr_zone(bsr_zone, error_msg))
	return (NULL);
    
    active_bsr_zone = find_active_bsr_zone(bsr_zone.zone_id());
    if (active_bsr_zone == NULL) {
	active_bsr_zone = new BsrZone(*this, bsr_zone.zone_id());
	active_bsr_zone->set_active_bsr_zone(true);
	_active_bsr_zone_list.push_back(active_bsr_zone);
    }
    
    active_bsr_zone->process_candidate_bsr(bsr_zone);
    
    if (active_bsr_zone->bsr_addr() != bsr_zone.bsr_addr()) {
	// Received message is not from the preferred BSR.
	return (active_bsr_zone);
    }
    
    //
    // Start (or restart) the Cand-RR Expiry Timer in 'active_bsr_zone'
    // for all RPs that were in 'bsr_zone'.
    //
    list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
    for (group_prefix_iter = bsr_zone.bsr_group_prefix_list().begin();
	 group_prefix_iter != bsr_zone.bsr_group_prefix_list().end();
	 ++group_prefix_iter) {
	BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	BsrGroupPrefix *active_bsr_group_prefix = active_bsr_zone->find_bsr_group_prefix(bsr_group_prefix->group_prefix());
	if (active_bsr_zone == NULL)
	    continue;
	list<BsrRp *>::const_iterator rp_iter;
	for (rp_iter = bsr_group_prefix->rp_list().begin();
	     rp_iter != bsr_group_prefix->rp_list().end();
	     ++rp_iter) {
	    BsrRp *bsr_rp = *rp_iter;
	    BsrRp *active_bsr_rp = active_bsr_group_prefix->find_rp(bsr_rp->rp_addr());
	    if (active_bsr_rp == NULL)
		continue;
	    if (active_bsr_zone->i_am_bsr()) {
		//
		// XXX: If I am the BSR, don't start the timer for my
		// own Cand-RPs
		continue;
	    }
	    active_bsr_rp->start_candidate_rp_expiry_timer();
	}
    }
    
    return (active_bsr_zone);
}

// Return: a pointer to the new BSR zone
BsrZone *
PimBsr::add_expire_bsr_zone(const BsrZone& bsr_zone)
{
    if (bsr_zone.bsr_group_prefix_list().empty())
	return (NULL);	// XXX: don't add if no prefixes
    
    //
    // Remove all the old RPs for same prefixes.
    //
    list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
    for (group_prefix_iter = bsr_zone.bsr_group_prefix_list().begin();
	 group_prefix_iter != bsr_zone.bsr_group_prefix_list().end();
	 ++group_prefix_iter) {
	const BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	delete_expire_bsr_zone_prefix(bsr_group_prefix->group_prefix(),
				      bsr_group_prefix->is_scope_zone());
    }
    
    BsrZone *expire_bsr_zone = new BsrZone(*this, bsr_zone);
    expire_bsr_zone->set_expire_bsr_zone(true);
    
    //
    // Cancel all timers for this zone.
    // Note that we do keep the C-RP Expiry timers running
    //
    expire_bsr_zone->bsr_timer().cancel();
    expire_bsr_zone->scope_zone_expiry_timer().cancel();
    expire_bsr_zone->candidate_rp_advertise_timer().cancel();
    
    // Add the zone to the list of expiring zones
    _expire_bsr_zone_list.push_back(expire_bsr_zone);
    
    return (expire_bsr_zone);
}

void
PimBsr::delete_config_bsr_zone(BsrZone *old_bsr_zone)
{
    // Remove from the list of configured zones
    _config_bsr_zone_list.remove(old_bsr_zone);
    
    delete old_bsr_zone;
}

void
PimBsr::delete_active_bsr_zone(BsrZone *old_bsr_zone)
{
    list<BsrZone *>::iterator iter;
    
    // Try to remove from the list of active zones
    iter = find(_active_bsr_zone_list.begin(),
		_active_bsr_zone_list.end(),
		old_bsr_zone);
    if (iter != _active_bsr_zone_list.end()) {
	_active_bsr_zone_list.erase(iter);
	delete_all_expire_bsr_zone_by_zone_id(old_bsr_zone->zone_id());
	delete old_bsr_zone;
	return;
    }
}

void
PimBsr::delete_expire_bsr_zone(BsrZone *old_bsr_zone)
{
    list<BsrZone *>::iterator iter;
    
    // Try to remove from the list of expiring zones
    iter = find(_expire_bsr_zone_list.begin(),
		_expire_bsr_zone_list.end(),
		old_bsr_zone);
    if (iter != _expire_bsr_zone_list.end()) {
	_expire_bsr_zone_list.erase(iter);
	delete old_bsr_zone;
	return;
    }
}

void
PimBsr::delete_all_expire_bsr_zone_by_zone_id(const PimScopeZoneId& zone_id)
{
    list<BsrZone *>::iterator iter, old_iter;
    
    // Try to remove all expire zones that match the specified zone ID
    for (iter = _expire_bsr_zone_list.begin();
	 iter != _expire_bsr_zone_list.end(); ) {
	BsrZone *bsr_zone = *iter;
	old_iter = iter;
	++iter;
	if (bsr_zone->zone_id() == zone_id) {
	    _expire_bsr_zone_list.erase(old_iter);
	    delete bsr_zone;
	}
    }
}

// Delete all group prefixes from the expiring zones.
void
PimBsr::delete_expire_bsr_zone_prefix(const IPvXNet& group_prefix,
				      bool is_scope_zone)
{
    list<BsrZone *>::iterator bsr_zone_iter, old_bsr_zone_iter;
    
    // Delete the group prefixes
    for (bsr_zone_iter = _expire_bsr_zone_list.begin();
	 bsr_zone_iter != _expire_bsr_zone_list.end();
	) {
	BsrZone *bsr_zone = *bsr_zone_iter;
	old_bsr_zone_iter = bsr_zone_iter;
	++bsr_zone_iter;
	BsrGroupPrefix *bsr_group_prefix;
	if (bsr_zone->zone_id().is_scope_zone() != is_scope_zone)
	    continue;
	bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
	if (bsr_group_prefix != NULL) {
	    bsr_zone->delete_bsr_group_prefix(bsr_group_prefix);
	    // Delete the expiring BSR zone if no more prefixes
	    if (bsr_zone->bsr_group_prefix_list().empty()) {
		_expire_bsr_zone_list.erase(old_bsr_zone_iter);
		delete bsr_zone;
	    }
	}
    }
}

// Find the BsrZone that corresponds to @zone_id
// from the list of configured BSR zones.
BsrZone	*
PimBsr::find_config_bsr_zone(const PimScopeZoneId& zone_id) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    for (iter_zone = _config_bsr_zone_list.begin();
	 iter_zone != _config_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	if (bsr_zone->zone_id() == zone_id)
	    return (bsr_zone);
    }
    
    return (NULL);
}

// Find the BsrZone that corresponds to @zone_id
// from the list of active BSR zones.
BsrZone	*
PimBsr::find_active_bsr_zone(const PimScopeZoneId& zone_id) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    for (iter_zone = _active_bsr_zone_list.begin();
	 iter_zone != _active_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	if (bsr_zone->zone_id() == zone_id)
	    return (bsr_zone);
    }
    
    return (NULL);
}

// Find the BsrZone that corresponds to @group_prefix
// and @is_scope_zone from the list of configured BSR zones.
BsrZone	*
PimBsr::find_config_bsr_zone_by_prefix(const IPvXNet& group_prefix,
				       bool is_scope_zone) const
{
    return (find_bsr_zone_by_prefix_from_list(_config_bsr_zone_list,
					      group_prefix, is_scope_zone));
}

// Find the BsrZone that corresponds to @group_prefix
// and @is_scope_zone from the list of acgive BSR zones.
BsrZone	*
PimBsr::find_active_bsr_zone_by_prefix(const IPvXNet& group_prefix,
				       bool is_scope_zone) const
{
    return (find_bsr_zone_by_prefix_from_list(_active_bsr_zone_list,
					      group_prefix, is_scope_zone));
}

// Find the first RP that corresponds for the given group prefix, the
// scope-zone flag, and the RP address.
// Note that first we search the list of active zones, and then
// the list of expiring zones.
BsrRp *
PimBsr::find_rp(const IPvXNet& group_prefix, bool is_scope_zone,
		const IPvX& rp_addr) const
{
    BsrZone *bsr_zone;
    BsrGroupPrefix *bsr_group_prefix;
    BsrRp *bsr_rp;
    
    // Try the list of active zones
    bsr_zone = find_active_bsr_zone_by_prefix(group_prefix, is_scope_zone);
    if (bsr_zone != NULL) {
	bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
	if (bsr_group_prefix != NULL) {
	    bsr_rp = bsr_group_prefix->find_rp(rp_addr);
	    if (bsr_rp != NULL)
		return (bsr_rp);
	}
    }
    
    // Try any of the zones on the list of expiring zones.
    list<BsrZone *>::const_iterator iter;
    for (iter = _expire_bsr_zone_list.begin();
	 iter != _expire_bsr_zone_list.end();
	 ++iter) {
	bsr_zone = *iter;
	if (bsr_zone->zone_id().is_scope_zone() != is_scope_zone)
	    continue;
	bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
	if (bsr_group_prefix != NULL) {
	    bsr_rp = bsr_group_prefix->find_rp(rp_addr);
	    if (bsr_rp != NULL)
		return (bsr_rp);
	}
    }
    
    return (NULL);
}

void
PimBsr::add_rps_to_rp_table()
{
    list<BsrZone *>::iterator iter_zone;
    
    // Add from the list of active BSR zones
    for (iter_zone = _active_bsr_zone_list.begin();
	 iter_zone != _active_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	list<BsrGroupPrefix *>::const_iterator iter_prefix;
	for (iter_prefix = bsr_zone->bsr_group_prefix_list().begin();
	     iter_prefix != bsr_zone->bsr_group_prefix_list().end();
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
					     bsr_zone->hash_masklen(),
					     PimRp::RP_LEARNED_METHOD_BOOTSTRAP);
	    }
	}
    }
    
    // Add from the list of expiring BSR zones
    for (iter_zone = _expire_bsr_zone_list.begin();
	 iter_zone != _expire_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	list<BsrGroupPrefix *>::const_iterator iter_prefix;
	for (iter_prefix = bsr_zone->bsr_group_prefix_list().begin();
	     iter_prefix != bsr_zone->bsr_group_prefix_list().end();
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
					     bsr_zone->hash_masklen(),
					     PimRp::RP_LEARNED_METHOD_BOOTSTRAP);
	    }
	}
    }
    
    // Apply the changes of the RPs to the PIM multicast routing table.
    pim_node().rp_table().apply_rp_changes();
}

void
PimBsr::schedule_rp_table_apply_rp_changes()
{
    _rp_table_apply_rp_changes_timer.start(0,
					   0,
					   pim_bsr_rp_table_apply_rp_changes_timeout,
					   this);
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

//
// Delete all group prefixes that have no RPs.
// If an expiring zone has no group prefixes, it is deleted as well.
//
void
PimBsr::clean_expire_bsr_zones()
{
    list<BsrZone *>::iterator bsr_zone_iter;
    list<BsrGroupPrefix *>::const_iterator bsr_group_prefix_iter;
    
    for (bsr_zone_iter = _expire_bsr_zone_list.begin();
	 bsr_zone_iter != _expire_bsr_zone_list.end();
	) {
	BsrZone *bsr_zone = *bsr_zone_iter;
	++bsr_zone_iter;
	for (bsr_group_prefix_iter = bsr_zone->bsr_group_prefix_list().begin();
	     bsr_group_prefix_iter != bsr_zone->bsr_group_prefix_list().end();
	    ) {
	    BsrGroupPrefix *bsr_group_prefix = *bsr_group_prefix_iter;
	    ++bsr_group_prefix_iter;
	    if (! bsr_group_prefix->rp_list().empty())
		continue;
	    bsr_zone->delete_bsr_group_prefix(bsr_group_prefix);
	}
	if (bsr_zone->bsr_group_prefix_list().empty())
	    delete_expire_bsr_zone(bsr_zone);
    }
}

void
PimBsr::schedule_clean_expire_bsr_zones()
{
    _clean_expire_bsr_zones_timer.start(0,
					0,
					pim_bsr_clean_expire_bsr_zones_timeout,
					this);
}

static void
pim_bsr_clean_expire_bsr_zones_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    PimBsr& pim_bsr = *(PimBsr *)data_pointer;
    
    // Clean the expiring zones
    pim_bsr.clean_expire_bsr_zones();
    
}

// Find the BsrZone that corresponds to @is_scope_zone
// and @group_prefix from the list of configured BSR zones.
BsrZone	*
PimBsr::find_bsr_zone_by_prefix_from_list(
    const list<BsrZone *>& zone_list,
    const IPvXNet& group_prefix,
    bool is_scope_zone) const
{
    list<BsrZone *>::const_iterator iter_zone;
    BsrZone *best_bsr_zone = NULL;
    
    for (iter_zone = zone_list.begin();
	 iter_zone != zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	if (bsr_zone->zone_id().is_scope_zone() != is_scope_zone)
	    continue;
	if (! bsr_zone->zone_id().contains(group_prefix))
	    continue;
	if (best_bsr_zone == NULL) {
	    best_bsr_zone = bsr_zone;
	    continue;
	}
	// XXX: we don't really need to do longest-prefix match selection,
	// because by definition the zones are non-overlapping, but just
	// in case (e.g., if we anyway accept overlapping zones).
	if (best_bsr_zone->zone_id().scope_zone_prefix().prefix_len()
	    < bsr_zone->zone_id().scope_zone_prefix().prefix_len())
	    best_bsr_zone = bsr_zone;
    }
    
    return (best_bsr_zone);
}

//
// Return true if @bsr_zone can be added to the list of configured zones,
// otherwise false.
// If the return value is false, @error_msg contains a message that
// describes the error.
//
bool
PimBsr::can_add_config_bsr_zone(const BsrZone& bsr_zone,
				string& error_msg) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    error_msg = "";			// Reset the error message
    
    if (bsr_zone.i_am_candidate_bsr()) {
	if (! bsr_zone.my_bsr_addr().is_unicast()) {
	    error_msg = c_format("BSR address %s is not an unicast address",
				 cstring(bsr_zone.my_bsr_addr()));
	    return (false);
	}
	
	if (! _pim_node.is_my_addr(bsr_zone.my_bsr_addr())) {
	    error_msg = c_format("BSR address %s is not my address",
				 cstring(bsr_zone.my_bsr_addr()));
	    return (false);
	}
    }
    
    if (find_config_bsr_zone(bsr_zone.zone_id()) != NULL) {
	error_msg = c_format("already have scope zone %s",
			     cstring(bsr_zone.zone_id()));
	return (false);
    }
    
    //
    // Check for consistency
    //
    for (iter_zone = _config_bsr_zone_list.begin();
	 iter_zone != _config_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *config_bsr_zone = *iter_zone;
	
	//
	// Check that zones do not overlap
	//
	if (config_bsr_zone->i_am_candidate_bsr()
	    && bsr_zone.i_am_candidate_bsr()) {
	    if (config_bsr_zone->zone_id().is_overlap(bsr_zone.zone_id())) {
		error_msg = c_format("overlapping zones %s and %s",
				     cstring(config_bsr_zone->zone_id()),
				     cstring(bsr_zone.zone_id()));
		return (false);
	    }
	}
	//
	// So far so good. Continue checking...
	//
    }
    
    return (true);		// The new BsrZone can be added.
}

//
// Return true if @bsr_zone can be added to the list of active zones,
// otherwise false.
// If the return value is false, @error_msg contains a message that
// describes the error.
//
bool
PimBsr::can_add_active_bsr_zone(const BsrZone& bsr_zone,
				string& error_msg) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    error_msg = "";			// Reset the error message
    
    //
    // Check for consistency
    //
    for (iter_zone = _active_bsr_zone_list.begin();
	 iter_zone != _active_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *active_bsr_zone = *iter_zone;
	
	//
	// Check that zones do not overlap
	//
	if (bsr_zone.zone_id() != active_bsr_zone->zone_id()) {
	    if (bsr_zone.zone_id().is_overlap(active_bsr_zone->zone_id())) {
		error_msg = c_format("overlapping zones %s and %s",
				     cstring(active_bsr_zone->zone_id()),
				     cstring(bsr_zone.zone_id()));
		return (false);
	    }
	    continue;
	}
	
	if (bsr_zone.bsr_addr() != active_bsr_zone->bsr_addr()) {
	    // A message from a different Bootstrap router.
	    // Such message is OK, because it may replace the old one.
	    continue;
	}
	
	// A message from the same Bootstrap router. Check whether
	// a fragment from the same message, or a new message.
	if (bsr_zone.fragment_tag() != active_bsr_zone->fragment_tag()) {
	    // A new message. The new message will replace the old message.
	    continue;
	}
	
	//
	// A fragment from the same message
	//
	
	if (! active_bsr_zone->can_merge_rp_set(bsr_zone, error_msg))
	    return (false);
	
	//
	// So far so good. Continue checking...
	//
    }
    
    return (true);		// The new BsrZone can be added.
}

//
// Test-related methods
//
BsrZone	*
PimBsr::add_test_bsr_zone(const PimScopeZoneId& zone_id, const IPvX& bsr_addr,
			  uint8_t bsr_priority, uint8_t hash_masklen,
			  uint16_t fragment_tag)
{
    BsrZone *new_bsr_zone = new BsrZone(*this, bsr_addr, bsr_priority,
					hash_masklen, fragment_tag);
    new_bsr_zone->set_zone_id(zone_id);
    new_bsr_zone->set_test_bsr_zone(true);
    _test_bsr_zone_list.push_back(new_bsr_zone);
    
    return (new_bsr_zone);
}

BsrZone	*
PimBsr::find_test_bsr_zone(const PimScopeZoneId& zone_id) const
{
    list<BsrZone *>::const_iterator iter_zone;
    
    for (iter_zone = _test_bsr_zone_list.begin();
	 iter_zone != _test_bsr_zone_list.end();
	 ++iter_zone) {
	BsrZone *bsr_zone = *iter_zone;
	if (bsr_zone->zone_id() == zone_id)
	    return (bsr_zone);
    }
    
    return (NULL);
}

BsrGroupPrefix *
PimBsr::add_test_bsr_group_prefix(const PimScopeZoneId& zone_id,
				  const IPvXNet& group_prefix,
				  bool is_scope_zone,
				  uint8_t expected_rp_count)
{
    BsrZone *bsr_zone = NULL;
    BsrGroupPrefix *bsr_group_prefix = NULL;
    
    bsr_zone = find_test_bsr_zone(zone_id);
    if (bsr_zone == NULL)
	return (NULL);
    
    bsr_group_prefix = bsr_zone->add_bsr_group_prefix(group_prefix,
						      is_scope_zone,
						      expected_rp_count);
    return (bsr_group_prefix);
}

BsrRp *
PimBsr::add_test_bsr_rp(const PimScopeZoneId& zone_id,
			const IPvXNet& group_prefix,
			const IPvX& rp_addr,
			uint8_t rp_priority,
			uint16_t rp_holdtime)
{
    BsrZone *bsr_zone = NULL;
    BsrGroupPrefix *bsr_group_prefix = NULL;
    BsrRp *bsr_rp = NULL;
    
    bsr_zone = find_test_bsr_zone(zone_id);
    if (bsr_zone == NULL)
	return (NULL);
    
    bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL)
	return (NULL);
    
    bsr_rp = bsr_group_prefix->add_rp(rp_addr, rp_priority, rp_holdtime);
    return (bsr_rp);
}

int
PimBsr::send_test_bootstrap(const string& vif_name)
{
    return (send_test_bootstrap_by_dest(vif_name, IPvX::PIM_ROUTERS(family())));
}

int
PimBsr::send_test_bootstrap_by_dest(const string& vif_name,
				    const IPvX& dest_addr)
{
    PimVif *pim_vif = pim_node().vif_find_by_name(vif_name);
    int ret_value = XORP_ERROR;
    list<BsrZone *>::iterator bsr_iter;
    
    if (pim_vif == NULL) {
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    for (bsr_iter = _test_bsr_zone_list.begin();
	 bsr_iter != _test_bsr_zone_list.end();
	 ++bsr_iter) {
	BsrZone *bsr_zone = *bsr_iter;
	if (pim_vif->pim_bootstrap_send(dest_addr, *bsr_zone)
	    < 0) {
	    ret_value = XORP_ERROR;
	    goto ret_label;
	}
    }
    ret_value = XORP_OK;
    goto ret_label;
    
 ret_label:
    delete_pointers_list(_test_bsr_zone_list);
    return (ret_value);
}

int
PimBsr::send_test_cand_rp_adv()
{
    PimVif *pim_vif = NULL;
    int ret_value = XORP_ERROR;
    list<BsrZone *>::iterator bsr_iter;
    
    //
    // Find the first vif that is UP, and use it to unicast the Cand-RP-Adv
    // messages.
    //
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	pim_vif = pim_node().vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	if (! pim_vif->is_up())
	    continue;
	break;	// Found
    }
    if (pim_vif == NULL) {
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    for (bsr_iter = _test_bsr_zone_list.begin();
	 bsr_iter != _test_bsr_zone_list.end();
	 ++bsr_iter) {
	BsrZone *bsr_zone = *bsr_iter;
	if (pim_vif->pim_cand_rp_adv_send(bsr_zone->bsr_addr(), *bsr_zone)
	    < 0) {
	    ret_value = XORP_ERROR;
	    goto ret_label;
	}
    }
    ret_value = XORP_OK;
    goto ret_label;
    
 ret_label:
    delete_pointers_list(_test_bsr_zone_list);
    return (ret_value);
}


BsrZone::BsrZone(PimBsr& pim_bsr, const BsrZone& bsr_zone)
    : _pim_bsr(pim_bsr),
      _is_config_bsr_zone(bsr_zone.is_config_bsr_zone()),
      _is_active_bsr_zone(bsr_zone.is_active_bsr_zone()),
      _is_expire_bsr_zone(bsr_zone.is_expire_bsr_zone()),
      _is_test_bsr_zone(bsr_zone.is_test_bsr_zone()),
      _bsr_addr(bsr_zone.bsr_addr()),
      _bsr_priority(bsr_zone.bsr_priority()),
      _hash_masklen(bsr_zone.hash_masklen()),
      _fragment_tag(bsr_zone.fragment_tag()),
      _is_accepted_message(bsr_zone.is_accepted_message()),
      _is_unicast_message(bsr_zone.is_unicast_message()),
      _unicast_message_src(bsr_zone.unicast_message_src()),
      _zone_id(bsr_zone.zone_id()),
      _i_am_candidate_bsr(bsr_zone.i_am_candidate_bsr()),
      _my_bsr_addr(bsr_zone.my_bsr_addr()),
      _my_bsr_priority(bsr_zone.my_bsr_priority()),
      _is_bsm_forward(bsr_zone.is_bsm_forward()),
      _is_bsm_originate(bsr_zone.is_bsm_originate()),
      _is_cancel(bsr_zone.is_cancel())
{
    set_bsr_zone_state(bsr_zone.bsr_zone_state());
    
    // Conditionally set the Bootstrap timer
    if (bsr_zone.const_bsr_timer().is_set()) {
	struct timeval left_timeval;
	bsr_zone.const_bsr_timer().left_timeval(&left_timeval);
	_bsr_timer.start(TIMEVAL_SEC(&left_timeval),
			 TIMEVAL_USEC(&left_timeval),
			 bsr_zone_bsr_timer_timeout,
			 this);
    }
    
    // Conditionally set the Scone-Zone Expiry timer
    if (bsr_zone.const_scope_zone_expiry_timer().is_set()) {
	struct timeval left_timeval;
	bsr_zone.const_scope_zone_expiry_timer().left_timeval(&left_timeval);
	_scope_zone_expiry_timer.start(TIMEVAL_SEC(&left_timeval),
				       TIMEVAL_USEC(&left_timeval),
				       bsr_zone_scope_zone_expiry_timer_timeout,
				       this);
    }
    
    //
    // XXX: Do not set (conditionally) the C-RP Advertise timer, because
    // it is explicitly start at configured BsrZone only.
    //
    
    //
    // Copy the RP-set
    //
    list<BsrGroupPrefix *>::const_iterator iter;
    for (iter = bsr_zone.bsr_group_prefix_list().begin();
	 iter != bsr_zone.bsr_group_prefix_list().end();
	 ++iter) {
	const BsrGroupPrefix *bsr_group_prefix = *iter;
	BsrGroupPrefix *bsr_group_prefix_copy
	    = new BsrGroupPrefix(*this, *bsr_group_prefix);
	_bsr_group_prefix_list.push_back(bsr_group_prefix_copy);
    }
}

BsrZone::BsrZone(PimBsr& pim_bsr, const PimScopeZoneId& zone_id)
    : _pim_bsr(pim_bsr),
      _is_config_bsr_zone(false),
      _is_active_bsr_zone(false),
      _is_expire_bsr_zone(false),
      _is_test_bsr_zone(false),
      _bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _bsr_priority(0),
      _hash_masklen(PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(_pim_bsr.family())),
      _fragment_tag(RANDOM(0xffff)),
      _is_accepted_message(false),
      _is_unicast_message(false),
      _unicast_message_src(IPvX::ZERO(_pim_bsr.family())),
      _zone_id(zone_id),
      _i_am_candidate_bsr(false),		// XXX: disable by default
      _my_bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _my_bsr_priority(0),			// XXX: lowest priority
      _is_bsm_forward(false),
      _is_bsm_originate(false),
      _is_cancel(false)
{
    set_bsr_zone_state(BsrZone::STATE_INIT);
}

BsrZone::BsrZone(PimBsr& pim_bsr, const IPvX& bsr_addr, uint8_t bsr_priority,
		 uint8_t hash_masklen, uint16_t fragment_tag)
    : _pim_bsr(pim_bsr),
      _is_config_bsr_zone(false),
      _is_active_bsr_zone(false),
      _is_expire_bsr_zone(false),
      _is_test_bsr_zone(false),
      _bsr_addr(bsr_addr),
      _bsr_priority(bsr_priority),
      _hash_masklen(hash_masklen),
      _fragment_tag(fragment_tag),
      _is_accepted_message(false),
      _is_unicast_message(false),
      _unicast_message_src(IPvX::ZERO(_pim_bsr.family())),
      _zone_id(PimScopeZoneId(IPvXNet::ip_multicast_base_prefix(_pim_bsr.family()),
			      false)), // XXX: no scope zone by default
      _i_am_candidate_bsr(false),		// XXX: disable by default
      _my_bsr_addr(IPvX::ZERO(_pim_bsr.family())),
      _my_bsr_priority(0),			// XXX: lowest priority
      _is_bsm_forward(false),
      _is_bsm_originate(false),
      _is_cancel(false)
{
    set_bsr_zone_state(BsrZone::STATE_INIT);
}

BsrZone::~BsrZone()
{
    delete_pointers_list(_bsr_group_prefix_list);
}

void
BsrZone::set_config_bsr_zone(bool v)
{
    _is_config_bsr_zone = v;
    if (v) {
	_is_active_bsr_zone = false;
	_is_expire_bsr_zone = false;
	_is_test_bsr_zone = false;
    }
}

void
BsrZone::set_active_bsr_zone(bool v)
{
    _is_active_bsr_zone = v;
    if (v) {
	_is_config_bsr_zone = false;
	_is_expire_bsr_zone = false;
	_is_test_bsr_zone = false;
    }
}

void
BsrZone::set_expire_bsr_zone(bool v)
{
    _is_expire_bsr_zone = v;
    if (v) {
	_is_config_bsr_zone = false;
	_is_active_bsr_zone = false;
	_is_test_bsr_zone = false;
    }
}

void
BsrZone::set_test_bsr_zone(bool v)
{
    _is_test_bsr_zone = v;
    if (v) {
	_is_config_bsr_zone = false;
	_is_active_bsr_zone = false;
	_is_expire_bsr_zone = false;
    }
}

bool
BsrZone::is_consistent(string& error_msg) const
{
    error_msg = "";		// Reset the error message
    list<BsrGroupPrefix *>::const_iterator iter1, iter2;
    
    // Check the Bootstrap router address
    if (! bsr_addr().is_unicast()) {
	error_msg = c_format("invalid Bootstrap router address: %s",
			     cstring(bsr_addr()));
	return (false);
    }
    
    //
    // Check that all group prefixes are multicast, and all RPs are unicast
    //
    for (iter1 = _bsr_group_prefix_list.begin();
	 iter1 != _bsr_group_prefix_list.end();
	 ++iter1) {
	const BsrGroupPrefix *bsr_group_prefix = *iter1;
	
	if (! bsr_group_prefix->group_prefix().is_multicast()) {
	    error_msg = c_format("invalid group prefix: %s",
				 cstring(bsr_group_prefix->group_prefix()));
	    return (false);
	}
	
	list<BsrRp *>::const_iterator rp_iter;
	for (rp_iter = bsr_group_prefix->rp_list().begin();
	     rp_iter != bsr_group_prefix->rp_list().end();
	     ++rp_iter) {
	    BsrRp *bsr_rp = *rp_iter;
	    
	    if (! bsr_rp->rp_addr().is_unicast()) {
		error_msg = c_format("invalid RP address: %s",
				     cstring(bsr_rp->rp_addr()));
		return (false);
	    }
	}
    }
    
    //
    // Check that no two group prefixes are exactly same
    //
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
		return (false);
	    }
	}
    }
    
    //
    // XXX: we could check that an RP address is listed no more than once
    // per group prefix. However, listing an RP more that once is not
    // very harmful: the lastest listing of the RP overrides the previous.
    //
    
    if (! zone_id().is_scope_zone())
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
    
    //
    // Test that the remaining group prefixes are a subset of 'zone_id'.
    //
    for (++iter; iter != _bsr_group_prefix_list.end(); ++iter) {
	const BsrGroupPrefix *bsr_group_prefix = *iter;
	if (! zone_id().contains(bsr_group_prefix->group_prefix())) {
	    error_msg = c_format("group prefix %s is not contained in "
				 "scope zone %s",
				 cstring(bsr_group_prefix->group_prefix()),
				 cstring(zone_id()));
	    return (false);	// ERROR: group prefix is not contained.
	}
    }
    
    return (true);		// OK
}

bool
BsrZone::can_merge_rp_set(const BsrZone& bsr_zone, string& error_msg) const
{
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    list<BsrRp *>::const_iterator iter_rp;
    
    //
    // Check the new fragment priority for consistency
    //
    if (bsr_priority() != bsr_zone.bsr_priority()) {
	error_msg = c_format("inconsistent fragment: "
			     "old fragment for zone %s has priority %d; "
			     "new fragment has priority %d",
			     cstring(zone_id()),
			     bsr_priority(),
			     bsr_zone.bsr_priority());
	return (false);
    }
    
    //
    // Check the new fragment hash masklen for consistency
    //
    if (hash_masklen() != bsr_zone.hash_masklen()) {
	error_msg = c_format("inconsistent fragment: "
			     "old fragment for zone %s has hash masklen %d; "
			     "new fragment has hash masklen %d",
			     cstring(zone_id()),
			     hash_masklen(),
			     bsr_zone.hash_masklen());
	return (false);
    }
    
    //
    // Check the group prefixes for consistency
    //
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	const BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	const BsrGroupPrefix *active_bsr_group_prefix = find_bsr_group_prefix(
	    bsr_group_prefix->group_prefix());
	if (active_bsr_group_prefix == NULL)
	    continue;		// A new group prefix. Fine.
	
	//
	// Check the number of expected RPs
	//
	if (active_bsr_group_prefix->expected_rp_count()
	    != bsr_group_prefix->expected_rp_count()) {
	    error_msg = c_format("inconsistent 'RP count': "
				 "old fragment for zone %s has "
				 "'RP count' of %d; "
				 "in the new fragment the count is %d",
				 cstring(zone_id()),
				 active_bsr_group_prefix->expected_rp_count(),
				 bsr_group_prefix->expected_rp_count());
	    return (false);
	}
	
	//
	// Check the list of received RPs
	//
	
	//
	// Check that the new RPs are not repeating, and that the total
	// number of RPs is not too large.
	//
	uint32_t rp_count_sum = active_bsr_group_prefix->received_rp_count();
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    if (active_bsr_group_prefix->find_rp(bsr_rp->rp_addr()) != NULL) {
		error_msg = c_format("BSR message fragment for zone %s "
				     "already contains entry for RP %s",
				     cstring(zone_id()),
				     cstring(bsr_rp->rp_addr()));
		return (false);
	    }
	    rp_count_sum++;
	}
	if (rp_count_sum > active_bsr_group_prefix->expected_rp_count()) {
	    error_msg = c_format("inconsistent 'fragment RP count': "
				 "sum of old and new fragments count "
				 "for zone %s is too large: %u while "
				 "the expected count is %u",
				 cstring(zone_id()),
				 rp_count_sum,
				 active_bsr_group_prefix->expected_rp_count());
	    return (false);
	}
    }
    
    return (true);
}

//
// Add the list of group prefixes
//
void
BsrZone::merge_rp_set(const BsrZone& bsr_zone)
{
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    list<BsrRp *>::const_iterator iter_rp;
    
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	BsrGroupPrefix *org_bsr_group_prefix = find_bsr_group_prefix(
	    bsr_group_prefix->group_prefix());
	if (org_bsr_group_prefix == NULL) {
	    // A new group prefix. Add it to the list.
	    BsrGroupPrefix *new_bsr_group_prefix;
	    new_bsr_group_prefix = new BsrGroupPrefix(*bsr_group_prefix);
	    _bsr_group_prefix_list.push_back(new_bsr_group_prefix);
	    continue;
	}
	// Add the information about the new RPs.
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    org_bsr_group_prefix->add_rp(bsr_rp->rp_addr(),
					 bsr_rp->rp_priority(),
					 bsr_rp->rp_holdtime());
	}
    }
}

//
// Store the RP-set
//
void
BsrZone::store_rp_set(const BsrZone& bsr_zone)
{
    //
    // Add a copy of this zone to the expiring zone list
    //
    if (is_active_bsr_zone())
	pim_bsr().add_expire_bsr_zone(*this);
    
    //
    // Delete the old prefixes
    //
    delete_pointers_list(_bsr_group_prefix_list);
    
    //
    // Copy the RP-set
    //
    list<BsrGroupPrefix *>::const_iterator iter;
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
    _is_accepted_message = bsr_zone.is_accepted_message(),
    _is_unicast_message = bsr_zone.is_unicast_message();
    _unicast_message_src = bsr_zone.unicast_message_src();
    
    //
    // Remove all the old RPs for same new prefixes
    //
    if (is_active_bsr_zone()) {
	list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
	for (group_prefix_iter = bsr_group_prefix_list().begin();
	     group_prefix_iter != bsr_group_prefix_list().end();
	     ++group_prefix_iter) {
	    const BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	    pim_bsr().delete_expire_bsr_zone_prefix(bsr_group_prefix->group_prefix(),
						    bsr_group_prefix->is_scope_zone());
	}
    }
}

// Create a new BSR prefix, and return a pointer to it.
BsrGroupPrefix *
BsrZone::add_bsr_group_prefix(const IPvXNet& group_prefix_init,
			      bool is_scope_zone_init,
			      uint8_t expected_rp_count)
{
    BsrGroupPrefix *bsr_group_prefix;
    
    bsr_group_prefix = new BsrGroupPrefix(*this,
					  group_prefix_init,
					  is_scope_zone_init,
					  expected_rp_count);
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
BsrZone::process_candidate_bsr(const BsrZone& bsr_zone)
{
    XLOG_ASSERT(is_active_bsr_zone());
    
    // TODO: handle the case when reconfiguring the local BSR on-the-fly
    // (not in the spec).
    
    if (bsr_zone_state() == BsrZone::STATE_INIT) {
	//
	// A new entry
	//
	
	//
	// Set my_bsr_addr and my_bsr_priority if I am a Cand-BSR for this
	// zone.
	//
	BsrZone *config_bsr_zone = pim_bsr().find_config_bsr_zone(zone_id());
	if ((config_bsr_zone != NULL)
	    && config_bsr_zone->i_am_candidate_bsr()) {
	    // I am a Cand-BSR for this zone
	    // -> P-BSR state
	    set_bsr_zone_state(BsrZone::STATE_PENDING_BSR);
	    set_i_am_candidate_bsr(config_bsr_zone->i_am_candidate_bsr(),
				   config_bsr_zone->bsr_addr(),
				   config_bsr_zone->bsr_priority());
	    // Set BS Timer to BS Timeout
	    // TODO: XXX: PAVPAVPAV: send the very first Bootstrap message
	    // after a very short random interval instead?
	    // E.g. after 1/10th of the BS Timeout?
	    // TODO: XXX: PAVPAVPAV: for testing purpose, the interval
	    // is shorter: 1/10th	(XXX: before was 1/30th)
	    // (Not in the spec).
	    _bsr_timer.start_random(
		PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT / 10,
		0,
		bsr_zone_bsr_timer_timeout,
		this,
		0.5);
	    return (false);
	} else {
	    // I am not a Cand-BSR for this zone
	    if ((config_bsr_zone != NULL) || (! zone_id().is_scope_zone()))
		set_bsr_zone_state(BsrZone::STATE_ACCEPT_ANY);
	    else
		set_bsr_zone_state(BsrZone::STATE_NO_INFO);
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
	set_bsm_forward(true);
	if (is_new_bsr_preferred(bsr_zone)) {
	    // Store RP-Set
	    store_rp_set(bsr_zone);
	    expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	} else {
	    // Update the Elected BSR 
	    if (fragment_tag() == bsr_zone.fragment_tag()) {
		// Merge RP-Set
		merge_rp_set(bsr_zone);
	    } else {
		// Store RP-Set
		store_rp_set(bsr_zone);
	    }
	}
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 bsr_zone_bsr_timer_timeout, this);
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
			 bsr_zone_bsr_timer_timeout,
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
	set_bsm_forward(true);
	// Store RP-Set
	store_rp_set(bsr_zone);
	expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 bsr_zone_bsr_timer_timeout, this);
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
	set_bsm_forward(true);
	// Store RP-Set
	store_rp_set(bsr_zone);
	expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 bsr_zone_bsr_timer_timeout, this);
	return (true);
    }
    // Receive Non-preferred BSM
    // -> E-BSR state
    set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Originate BSM
    _is_bsm_originate = true;
    // Set BS Timer to BS Period
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
		     bsr_zone_bsr_timer_timeout, this);
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
    set_bsm_forward(true);
    // Store RP-Set
    store_rp_set(bsr_zone);
    expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
    // Set BS Timer to BS Timeout
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
		     bsr_zone_bsr_timer_timeout, this);
    // Set SZ Timer to SZ Timeout
    _scope_zone_expiry_timer.start(PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
				   bsr_zone_scope_zone_expiry_timer_timeout,
				   this);
    return (true);
    
 bsr_zone_state_accept_any_label:
    // Accept Any state
    // -> AP State
    set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
    // Forward BSM  : will happen in the parent function
    set_bsm_forward(true);
    // Store RP-Set
    store_rp_set(bsr_zone);
    expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
    // Set BS Timer to BS Timeout
    _bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
		     bsr_zone_bsr_timer_timeout, this);
    // Set SZ Timer to SZ Timeout
    _scope_zone_expiry_timer.start(PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
				   bsr_zone_scope_zone_expiry_timer_timeout,
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
	set_bsm_forward(true);
	if (is_new_bsr_preferred(bsr_zone)) {
	    // Store RP-Set
	    store_rp_set(bsr_zone);
	    expire_candidate_rp_advertise_timer();	// Send my Cand-RP-Adv
	} else {
	    // Update the Elected BSR
	    if (fragment_tag() == bsr_zone.fragment_tag()) {
		// Merge RP-Set
		merge_rp_set(bsr_zone);
	    } else {
		// Store RP-Set
		store_rp_set(bsr_zone);
	    }
	}
	// Set BS Timer to BS Timeout
	_bsr_timer.start(PIM_BOOTSTRAP_BOOTSTRAP_TIMEOUT_DEFAULT, 0,
			 bsr_zone_bsr_timer_timeout, this);
	// Set SZ Timer to SZ Timeout
	_scope_zone_expiry_timer.start(
	    PIM_BOOTSTRAP_SCOPE_ZONE_TIMEOUT_DEFAULT, 0,
	    bsr_zone_scope_zone_expiry_timer_timeout,
	    this);
	return (true);
    }
    // Receive Non-preferred BSM
    // -> AP state
    set_bsr_zone_state(BsrZone::STATE_ACCEPT_PREFERRED);
    return (false);
}

// Return true if I am the BSR
bool
BsrZone::i_am_bsr() const
{
    return (i_am_candidate_bsr() && (bsr_addr() == my_bsr_addr()));
}

// Return true if the received BSR is preferred than the current one.
bool
BsrZone::is_new_bsr_preferred(const BsrZone& bsr_zone) const
{
    IPvX compare_bsr_addr = bsr_addr();
    uint8_t compare_bsr_priority = bsr_priority();
    
    //
    // If I am in "Pending BSR" state, then use my own address and priority
    //
    if (bsr_zone_state() == BsrZone::STATE_PENDING_BSR) {
	compare_bsr_addr = my_bsr_addr();
	compare_bsr_priority = my_bsr_priority();
    }
    
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
    IPvX compare_bsr_addr = bsr_addr();
    uint8_t compare_bsr_priority = bsr_priority();

    //
    // If I am in "Pending BSR" state, then use my own address and priority
    //
    if (bsr_zone_state() == BsrZone::STATE_PENDING_BSR) {
	compare_bsr_addr = my_bsr_addr();
	compare_bsr_priority = my_bsr_priority();
    }
    
    if ((bsr_zone.bsr_priority() == compare_bsr_priority)
	&& (bsr_zone.bsr_addr() == compare_bsr_addr))
	return (true);
    
    return (false);
}

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
    double my_addr_double, stored_addr_double;
    size_t addr_bitlen = IPvX::addr_bitlen(_pim_bsr.family());
    size_t addr_size = IPvX::addr_size(_pim_bsr.family());
    
    // Get the address values
    my_addr.copy_out(my_addr_array);
    bsr_addr().copy_out(stored_addr_array);
    
    // Get the (double) value of the addresses
    my_addr_double = 0.0;
    stored_addr_double = 0.0;
    for (size_t i = 0; i < addr_size; i++) {
	my_addr_double = my_addr_double * 256 + (double)my_addr_array[i];
	stored_addr_double = stored_addr_double * 256 + (double)stored_addr_array[i];
    }
    
    // Compute AddrDelay
    if (bsr_priority() == my_priority) {
	double addr_diff;
	if (stored_addr_double > my_addr_double)
	    addr_diff = stored_addr_double - my_addr_double;
	else
	    addr_diff = 1.0;		// XXX
	
	addr_delay = log(addr_diff) / log((double)2.0);		// log2()
	addr_delay /= (addr_bitlen / 2);			// 16 for IPv4
    } else {
	addr_delay = 2 - (my_addr_double / pow((double)2.0, (double)(addr_bitlen - 1)));
    }
    XLOG_ASSERT((addr_delay >= 0.0) && (addr_delay <= 2.0));
    
    if (best_priority >= my_priority)
	priority_diff = best_priority - my_priority;
    else
	priority_diff = 0;		// XXX
    
    delay = PIM_BOOTSTRAP_RAND_OVERRIDE_DEFAULT
	+ 2*(log((double)(1 + priority_diff))/log((double)2.0))	// log2()
	+ addr_delay;
    
    if (result_timeval != NULL) {
	uint32_t sec, usec;
	
	sec = (uint32_t)delay;
	usec = (uint32_t)((delay - sec)*1000000);
	TIMEVAL_SET(result_timeval, sec, usec);
    }
}

//
// (Re)start the BsrTimer so it will expire immediately.
//
void
BsrZone::timeout_bsr_timer()
{
    _bsr_timer.start(0, 0, bsr_zone_bsr_timer_timeout, this);
}

static void
bsr_zone_bsr_timer_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    PimNode& pim_node = bsr_zone.pim_bsr().pim_node();
    
    XLOG_ASSERT(bsr_zone.is_active_bsr_zone());
    
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
			       bsr_zone_bsr_timer_timeout,
			       &bsr_zone);
    
    return;
    
 bsr_zone_state_pending_bsr_label:
    // Pending BSR state
    // -> E-BSR state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Store RP-Set
    {
	BsrZone *config_bsr_zone = bsr_zone.pim_bsr().find_config_bsr_zone(
	    bsr_zone.zone_id());
	XLOG_ASSERT(config_bsr_zone != NULL);
	// TODO: XXX: PAVPAVPAV: need to be careful with the above assert in
	// case we have reconfigured the BSR on-the-fly
	bsr_zone.store_rp_set(*config_bsr_zone);
	// TODO: XXX: PAVPAVPAV: do we want to set the fragment_tag to
	// something new here?
	
	// Add the RPs to the RP table
	bsr_zone.pim_bsr().add_rps_to_rp_table();
    }
    // Originate BSM
    bsr_zone.new_fragment_tag();
    for (uint16_t i = 0; i < pim_node.maxvifs(); i++) {
	PimVif *pim_vif = pim_node.vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
				    bsr_zone);
    }
    
    // Set BS Timer to BS Period
    bsr_zone.bsr_timer().start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
			       bsr_zone_bsr_timer_timeout, &bsr_zone);
    return;
    
 bsr_zone_state_elected_bsr_label:
    // Elected BSR state
    // -> E-BSR state
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ELECTED_BSR);
    // Originate BSM
    bsr_zone.new_fragment_tag();
    for (uint16_t i = 0; i < pim_node.maxvifs(); i++) {
	PimVif *pim_vif = pim_node.vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(pim_vif->family()),
				    bsr_zone);
    }
    // Set BS Timer to BS Period
    bsr_zone.bsr_timer().start(PIM_BOOTSTRAP_BOOTSTRAP_PERIOD_DEFAULT, 0,
			       bsr_zone_bsr_timer_timeout, &bsr_zone);
    return;
    
 bsr_zone_state_accept_preferred_label:
    // Accept Preferred state
    // -> AA State
    bsr_zone.set_bsr_zone_state(BsrZone::STATE_ACCEPT_ANY);
    return;
}

static void
bsr_zone_scope_zone_expiry_timer_timeout(void *data_pointer)
{
    if (data_pointer == NULL)
	return;
    
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    XLOG_ASSERT(bsr_zone.is_active_bsr_zone());
    
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
    if (i_am_bsr()) {
	_bsr_priority = _my_bsr_priority;
    }
}

BsrRp *
BsrZone::add_rp(const IPvXNet& group_prefix,
		bool is_scope_zone_init,
		const IPvX& rp_addr,
		uint8_t rp_priority,
		uint16_t rp_holdtime,
		string& error_msg)
{
    BsrGroupPrefix *bsr_group_prefix;
    BsrRp *bsr_rp = NULL;
    
    error_msg = "";
    
    if (! group_prefix.is_multicast()) {
	error_msg = c_format("group prefix %s is not a multicast address",
			     cstring(group_prefix));
	return (NULL);
    }
    
    if (! rp_addr.is_unicast()) {
	error_msg = c_format("RP address %s is not an unicast address",
		   cstring(rp_addr));
	return (NULL);
    }
    
    
    // Check for consistency
    if ((zone_id().is_scope_zone() != is_scope_zone_init)
	|| (! zone_id().contains(group_prefix))) {
	error_msg = c_format("scope zone %s does not contain prefix %s",
			     cstring(zone_id()),
			     cstring(group_prefix));
	return (NULL);
    }
    
    bsr_group_prefix = find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL) {
	bsr_group_prefix = add_bsr_group_prefix(group_prefix,
						is_scope_zone_init,
						0);
	XLOG_ASSERT(bsr_group_prefix != NULL);
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
    bsr_rp = bsr_group_prefix->add_rp(rp_addr, rp_priority, rp_holdtime);
    
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
	bsr_zone_candidate_rp_advertise_timer_timeout,
	this);
}

//
// Expire the Candidate-RP-Advertise timer (i.e., schedule it to expire NOW).
// However, we use only configured BsrZone to send Cand-RP-Adv.
// Therefore, we need to search for the corresponding config BsrZone and
// expire the timer in that one instead.
// XXX: if no Cand-RP is configured, silently ignore it.
//
void
BsrZone::expire_candidate_rp_advertise_timer()
{
    //
    // Search for the corresponding config BsrZone
    //
    BsrZone *config_bsr_zone = pim_bsr().find_config_bsr_zone(zone_id());
    
    if (config_bsr_zone == NULL) {
	// Probably I am not configured as a Cand-RP. Ignore.
	return;
    }
    config_bsr_zone->candidate_rp_advertise_timer().start(
	0,
	0,
	bsr_zone_candidate_rp_advertise_timer_timeout,
	config_bsr_zone);
}

// Time to send a Cand-RP-Advertise message to the BSR
static void
bsr_zone_candidate_rp_advertise_timer_timeout(void *data_pointer)
{
    BsrZone& bsr_zone = *(BsrZone *)data_pointer;
    PimNode& pim_node = bsr_zone.pim_bsr().pim_node();
    PimVif *pim_vif = NULL;
    const BsrZone *active_bsr_zone = NULL;
    
    //
    // Find the active BsrZone
    //
    active_bsr_zone = bsr_zone.pim_bsr().find_active_bsr_zone(bsr_zone.zone_id());
    do {
	if (active_bsr_zone == NULL)
	    break;		// No active BsrZone yet
	
	if (! active_bsr_zone->bsr_addr().is_unicast())
	    break;		// We don't know the BSR address
	
	if (active_bsr_zone->i_am_bsr())
	    break;		// I am the BSR, hence don't send the message
	
	//
	// Find the first vif that is UP, and use it to unicast the Cand-RP-Adv
	// messages.
	//
	for (uint16_t i = 0; i < pim_node.maxvifs(); i++) {
	    pim_vif = pim_node.vif_find_by_vif_index(i);
	    if (pim_vif == NULL)
		continue;
	    if (! pim_vif->is_up())
		continue;
	    break;	// Found
	}
	if (pim_vif == NULL) {
	    XLOG_ERROR("Cannot send periodic Cand-RP Adv message: "
		       "cannot find a vif that is UP and running");
	    break;
	}
	
	pim_vif->pim_cand_rp_adv_send(active_bsr_zone->bsr_addr(), bsr_zone);
    } while (false);
    
    // Restart the timer
    bsr_zone.start_candidate_rp_advertise_timer();
}

BsrGroupPrefix::BsrGroupPrefix(BsrZone& bsr_zone,
			       const IPvXNet& group_prefix,
			       bool is_scope_zone,
			       uint8_t expected_rp_count)
    : _bsr_zone(bsr_zone),
      _group_prefix(group_prefix),
      _is_scope_zone(is_scope_zone),
      _expected_rp_count(expected_rp_count)
{
    _received_rp_count = 0;
}

BsrGroupPrefix::BsrGroupPrefix(BsrZone& bsr_zone,
			       const BsrGroupPrefix& bsr_group_prefix)
    : _bsr_zone(bsr_zone),
      _group_prefix(bsr_group_prefix.group_prefix()),
      _is_scope_zone(bsr_group_prefix.is_scope_zone()),
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
    list<BsrRp *>::iterator iter;
    
    // Remove the RPs one-by-one
    do {
	iter = _rp_list.begin();
	if (iter == _rp_list.end())
	    break;
	BsrRp *bsr_rp = *iter;
	delete_rp(bsr_rp);
    } while (true);
}

// Return: a pointer to the RP entry if it can be added, otherwise NULL.
BsrRp *
BsrGroupPrefix::add_rp(const IPvX& rp_addr, uint8_t rp_priority,
		       uint16_t rp_holdtime)
{
    BsrRp *bsr_rp = find_rp(rp_addr);
    
    if (bsr_rp != NULL) {
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
    for (list<BsrRp *>::iterator iter = _rp_list.begin();
	 iter != _rp_list.end();
	 ++iter) {
	if (bsr_rp != *iter)
	    continue;
	// Entry found. Remove it.
	set_received_rp_count(received_rp_count() - 1);
	_rp_list.erase(iter);

	//
	// Schedule the task to clean the expiring bsr zones
	//
	if (bsr_zone().is_expire_bsr_zone()) {
	    bsr_zone().pim_bsr().schedule_clean_expire_bsr_zones();
	}
	
	//
	// If we don't have this RP anymore, then delete the RP entry
	// from the RP table
	//
	if (bsr_zone().is_expire_bsr_zone()
	    || bsr_zone().is_active_bsr_zone()) {
	    if (bsr_zone().pim_bsr().find_rp(group_prefix(),
					     is_scope_zone(),
					     bsr_rp->rp_addr())
		== NULL) {
		bsr_zone().pim_bsr().pim_node().rp_table().delete_rp(
		    bsr_rp->rp_addr(),
		    group_prefix(),
		    PimRp::RP_LEARNED_METHOD_BOOTSTRAP);
		bsr_zone().pim_bsr().schedule_rp_table_apply_rp_changes();
	    }
	}
	
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

