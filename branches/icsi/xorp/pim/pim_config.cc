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

#ident "$XORP: xorp/pim/pim_config.cc,v 1.17 2002/12/09 18:29:25 hodson Exp $"


//
// TODO: a temporary solution for various PIM configuration
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_node.hh"
#include "pim_vif.hh"


int
PimNode::set_vif_proto_version(const string& vif_name, int proto_version)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    if (pim_vif->set_proto_version(proto_version) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}
 
int
PimNode::reset_vif_proto_version(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->set_proto_version(pim_vif->proto_version_default());
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_triggered_delay(const string& vif_name,
				       uint16_t hello_triggered_delay)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_triggered_delay().set(hello_triggered_delay);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_triggered_delay(const string vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_triggered_delay().reset();
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_period(const string& vif_name, uint16_t hello_period)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_period().set(hello_period);
    
    // Send immediately a Hello message, and schedule the next one
    // at random in the interval [0, hello_period)
    pim_vif->pim_hello_send();
    pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_period(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_period().reset();
    
    // Send immediately a Hello message, and schedule the next one
    // at random in the interval [0, hello_period)
    pim_vif->pim_hello_send();
    pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    
    return (XORP_OK);
}

int
PimNode::set_vif_hello_holdtime(const string& vif_name,
				uint16_t hello_holdtime)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_holdtime().set(hello_holdtime);
    
    // Send immediately a Hello message, and schedule the next one
    // at random in the interval [0, hello_period)
    pim_vif->pim_hello_send();
    pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_hello_holdtime(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->hello_holdtime().reset();
    
    // Send immediately a Hello message, and schedule the next one
    // at random in the interval [0, hello_period)
    pim_vif->pim_hello_send();
    pim_vif->hello_timer_start_random(pim_vif->hello_period().get(), 0);
    
    return (XORP_OK);
}

int
PimNode::set_vif_dr_priority(const string& vif_name, uint32_t dr_priority)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->dr_priority().set(dr_priority);
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    // (Re)elect the DR
    pim_vif->pim_dr_elect();
    
    return (XORP_OK);
}

int
PimNode::reset_vif_dr_priority(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->dr_priority().reset();
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();

    // (Re)elect the DR
    pim_vif->pim_dr_elect();
    
    return (XORP_OK);
}

int
PimNode::set_vif_lan_delay(const string& vif_name, uint16_t lan_delay)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->lan_delay().set(lan_delay);
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    return (XORP_OK);
}

int
PimNode::reset_vif_lan_delay(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->lan_delay().reset();
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    return (XORP_OK);
}

int
PimNode::set_vif_override_interval(const string& vif_name,
				   uint16_t override_interval)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->override_interval().set(override_interval);
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    return (XORP_OK);
}

int
PimNode::reset_vif_override_interval(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->override_interval().reset();
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    return (XORP_OK);
}

int
PimNode::set_vif_accept_nohello_neighbors(const string& vif_name,
					  bool accept_nohello_neighbors)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    if (accept_nohello_neighbors && (! pim_vif->is_p2p())) {
	XLOG_WARNING("Accepting no-Hello neighbors should not be enabled "
	    "on non-point-to-point interfaces");
    }
    
    pim_vif->accept_nohello_neighbors().set(accept_nohello_neighbors);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_accept_nohello_neighbors(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->accept_nohello_neighbors().reset();
    
    return (XORP_OK);
}

int
PimNode::set_vif_join_prune_period(const string& vif_name,
				   uint16_t join_prune_period)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->join_prune_period().set(join_prune_period);
    
    return (XORP_OK);
}

int
PimNode::reset_vif_join_prune_period(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->join_prune_period().reset();
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_bsr_by_vif_name(bool is_admin_scope_zone,
					 const IPvXNet& admin_scope_zone_id,
					 const string& vif_name,
					 uint8_t bsr_priority,
					 uint8_t hash_masklen)
{
    // XXX: Find the vif address
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot add configure BSR with vif %s: no such vif",
		   vif_name.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->addr_ptr() == NULL) {
	XLOG_ERROR("Cannot add configure BSR with vif %s: "
		   "the vif has no configured address",
		   vif_name.c_str());
	return (XORP_ERROR);	// The vif has no address yet
    }

    // XXX: if hash_masklen is 0, then set its value to default
    if (hash_masklen == 0)
	hash_masklen = PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(family());
    
    if (add_config_cand_bsr_by_addr(is_admin_scope_zone,
				    admin_scope_zone_id,
				    *pim_vif->addr_ptr(),
				    bsr_priority,
				    hash_masklen) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_bsr_by_addr(bool is_admin_scope_zone,
				     const IPvXNet& admin_scope_zone_id,
				     const IPvX& my_cand_bsr_addr,
				     uint8_t bsr_priority,
				     uint8_t hash_masklen)
{
    if (! is_my_addr(my_cand_bsr_addr)) {
	XLOG_ERROR("Cannot add configure BSR with address %s: not my address",
		   cstring(my_cand_bsr_addr));
	return (XORP_ERROR);
    }
    
    if (! my_cand_bsr_addr.is_unicast()) {
	XLOG_ERROR("Cannot add configure BSR with address %s: "
		   "not an unicast address",
		   cstring(my_cand_bsr_addr));
	return (XORP_ERROR);
    }
    
    // XXX: add myself as a Cand-BSR
    do {
	BsrZone *config_bsr_zone = NULL;
	uint16_t fragment_tag = RANDOM(0xffff);
	string error_msg = "";
	
	// XXX: if hash_masklen is 0, then set its value to default
	if (hash_masklen == 0)
	    hash_masklen = PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(family());
	
	config_bsr_zone = pim_bsr().find_bsr_zone_from_list(
	    pim_bsr().config_bsr_zone_list(),
	    is_admin_scope_zone,
	    admin_scope_zone_id);
	
	if (config_bsr_zone != NULL) {
	    XLOG_ERROR("Cannot add configure BSR for %s zone %s: "
		       "already have such zone",
		       (is_admin_scope_zone)? "scoped" : "non-scoped",
		       cstring(admin_scope_zone_id));
	    return (XORP_ERROR);
	}
	
	BsrZone new_bsr_zone(pim_bsr(), my_cand_bsr_addr, bsr_priority,
			     hash_masklen, fragment_tag);
	new_bsr_zone.set_admin_scope_zone(is_admin_scope_zone,
					  admin_scope_zone_id);
	new_bsr_zone.set_i_am_candidate_bsr(true, my_cand_bsr_addr,
					    bsr_priority);
	config_bsr_zone = pim_bsr().add_config_bsr_zone(new_bsr_zone,
							error_msg);
	
	if (config_bsr_zone == NULL) {
	    XLOG_ERROR("Cannot add configure BSR with address %s: %s",
		       cstring(my_cand_bsr_addr), error_msg.c_str());
	    return (XORP_ERROR);
	}
    } while (false);
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_config_cand_bsr(bool is_admin_scope_zone,
				const IPvXNet& admin_scope_zone_id)
{
    BsrZone *bsr_zone;
    bool is_up = false;
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_bsr_zone_from_list(
	pim_bsr().config_bsr_zone_list(),
	is_admin_scope_zone,
	admin_scope_zone_id);
    if (bsr_zone == NULL) {
	XLOG_ERROR("Cannot delete configure BSR for %s zone %s: "
		   "zone not found",
		   (is_admin_scope_zone)? "scoped" : "non-scoped",
		   cstring(admin_scope_zone_id));
	return (XORP_ERROR);
    }
    
    //
    // Stop the BSR, delete the BSR zone, and restart the BSR if necessary
    //
    is_up = pim_bsr().is_up();
    pim_bsr().stop();
    
    if (bsr_zone->bsr_group_prefix_list().empty()) {
	// No Cand-RP, therefore delete the zone.
	pim_bsr().delete_config_bsr_zone(bsr_zone);	
    } else {
	// There is Cand-RP configuration, therefore only reset the Cand-BSR
	// configuration.
	bsr_zone->set_i_am_candidate_bsr(false,
					 IPvX::ZERO(family()),
					 0);
    }
    
    if (is_up)
	pim_bsr().start();	// XXX: restart the BSR
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_rp_by_vif_name(bool is_admin_scope_zone,
					const IPvXNet& group_prefix,
					const string& vif_name,
					uint8_t rp_priority,
					uint16_t rp_holdtime)
{
    // XXX: Find the vif address
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot add configure Cand-RP "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot add configure Cand-RP with vif %s: no such vif",
		   vif_name.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->addr_ptr() == NULL) {
	XLOG_ERROR("Cannot add configure Cand-RP with vif %s: "
		   "the vif has no configured address",
		   vif_name.c_str());
	return (XORP_ERROR);	// The vif has no address yet
    }
    
    if (add_config_cand_rp_by_addr(is_admin_scope_zone,
				   group_prefix,
				   *pim_vif->addr_ptr(),
				   rp_priority,
				   rp_holdtime) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_rp_by_addr(bool is_admin_scope_zone,
				    const IPvXNet& group_prefix,
				    const IPvX& my_cand_rp_addr,
				    uint8_t rp_priority,
				    uint16_t rp_holdtime)
{
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot add configure Cand-RP "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (! is_my_addr(my_cand_rp_addr)) {
	XLOG_ERROR("Cannot add configure Cand-RP with address %s: "
		   "not my address",
		   cstring(my_cand_rp_addr));
	return (XORP_ERROR);
    }
    
    if (! my_cand_rp_addr.is_unicast()) {
	XLOG_ERROR("Cannot add configure Cand-RP with address %s: "
		   "not an unicast address",
		   cstring(my_cand_rp_addr));
	return (XORP_ERROR);
    }
    
    // XXX: add myself as a Cand-RP
    do {
	BsrZone *config_bsr_zone = NULL;
	BsrRp *bsr_rp = NULL;
	string error_msg = "";
	
	config_bsr_zone = pim_bsr().find_bsr_zone_by_prefix_from_list(
	    pim_bsr().config_bsr_zone_list(),
	    is_admin_scope_zone,
	    group_prefix);
	
	if (config_bsr_zone == NULL) {
	    BsrZone new_bsr_zone(pim_bsr(), is_admin_scope_zone, group_prefix);
	    config_bsr_zone = pim_bsr().add_config_bsr_zone(new_bsr_zone,
							    error_msg);
	    if (config_bsr_zone == NULL) {
		XLOG_ERROR("Cannot add configure Cand-RP for %s "
			   "zone group prefix %s: %s",
			   (is_admin_scope_zone)? "scoped" : "non-scoped",
			   cstring(group_prefix),
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	}
	XLOG_ASSERT(config_bsr_zone != NULL);
	
	bsr_rp = config_bsr_zone->add_rp(
	    is_admin_scope_zone,
	    group_prefix,
	    my_cand_rp_addr,
	    rp_priority,
	    rp_holdtime,
	    error_msg);
	
	if (bsr_rp == NULL) {
	    if (config_bsr_zone == NULL) {
		XLOG_ERROR("Cannot add configure Cand-RP address %s for %s "
			   "zone group prefix %s: %s",
			   cstring(my_cand_rp_addr),
			   (is_admin_scope_zone)? "scoped" : "non-scoped",
			   cstring(group_prefix),
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    
	    XLOG_ERROR("Cannot add add Cand-RP %s for group prefix %s",
		       cstring(my_cand_rp_addr),
		       cstring(IPvXNet::ip_multicast_base_prefix(family())));
	    return (XORP_ERROR);
	}
    } while (false);
    
    return (XORP_OK);
}

int
PimNode::delete_config_cand_rp_by_vif_name(bool is_admin_scope_zone,
					   const IPvXNet& group_prefix,
					   const string& vif_name)
{
    // XXX: Find the vif address
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot delete configure Cand-RP "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (pim_vif == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP with vif %s: no such vif",
		   vif_name.c_str());
	return (XORP_ERROR);
    }
    
    if (pim_vif->addr_ptr() == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP with vif %s: "
		   "the vif has no configured address",
		   vif_name.c_str());
	return (XORP_ERROR);	// The vif has no address yet
    }
    
    if (delete_config_cand_rp_by_addr(is_admin_scope_zone,
				      group_prefix,
				      *pim_vif->addr_ptr()) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::delete_config_cand_rp_by_addr(bool is_admin_scope_zone,
				       const IPvXNet& group_prefix,
				       const IPvX& my_cand_rp_addr)
{
    BsrZone *bsr_zone;
    BsrGroupPrefix *bsr_group_prefix;
    BsrRp *bsr_rp;
    bool is_up = false;
    
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot delete configure Cand-RP "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (! my_cand_rp_addr.is_unicast()) {
	XLOG_ERROR("Cannot delete configure Cand-RP with address %s: "
		   "not an unicast address",
		   cstring(my_cand_rp_addr));
	return (XORP_ERROR);
    }
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_config_bsr_zone_by_prefix(
	is_admin_scope_zone,
	group_prefix);
    if (bsr_zone == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for %s zone for "
		   "group prefix %s: zone not found",
		   (is_admin_scope_zone)? "scoped" : "non-scoped",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    //
    // Find the BSR group prefix
    //
    bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for %s zone for "
		   "group prefix %s: prefix not found",
		   (is_admin_scope_zone)? "scoped" : "non-scoped",
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    //
    // Find the BSR RP.
    //
    bsr_rp = bsr_group_prefix->find_rp(my_cand_rp_addr);
    if (bsr_rp == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for %s zone for "
		   "group prefix %s and RP %s: RP not found",
		   (is_admin_scope_zone)? "scoped" : "non-scoped",
		   cstring(group_prefix),
		   cstring(my_cand_rp_addr));
	return (XORP_ERROR);
    }
    
    //
    // Stop the BSR, delete the RP zone, and restart the BSR if necessary
    //
    is_up = pim_bsr().is_up();
    pim_bsr().stop();
    
    bsr_group_prefix->delete_rp(bsr_rp);
    bsr_rp = NULL;
    // Delete the BSR group prefix if not needed anymore
    if (bsr_group_prefix->rp_list().empty()) {
	bsr_zone->delete_bsr_group_prefix(bsr_group_prefix);
	bsr_group_prefix = NULL;
    }
    if (bsr_zone->bsr_group_prefix_list().empty()
	&& (! bsr_zone->i_am_candidate_bsr())) {
	// No Cand-RP, and no Cand-BSR, therefore delete the zone.
	pim_bsr().delete_config_bsr_zone(bsr_zone);
	bsr_zone = NULL;
    }
    
    if (is_up)
	pim_bsr().start();	// XXX: restart the BSR
    
    return (XORP_OK);
}

//
// Add a statically configured RP
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_rp(const IPvXNet& group_prefix,
		       const IPvX& rp_addr,
		       uint8_t rp_priority,
		       uint8_t hash_masklen)
{
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot add configure RP with address %s "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(rp_addr),
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (! rp_addr.is_unicast()) {
	XLOG_ERROR("Cannot add configure RP with address %s: "
		   "not an unicast address",
		   cstring(rp_addr));
	return (XORP_ERROR);
    }
    
    // XXX: if hash_masklen is 0, then set its value to default
    if (hash_masklen == 0)
	hash_masklen = PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(family());

    if (rp_table().add_rp(rp_addr, rp_priority, group_prefix, hash_masklen,
			  PimRp::RP_LEARNED_METHOD_STATIC)
	== NULL) {
	XLOG_ERROR("Cannot add configure RP with address %s "
		   "and priority %d for group prefix %s",
		   cstring(rp_addr),
		   rp_priority,
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Delete a statically configured RP
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_config_rp(const IPvXNet& group_prefix,
			  const IPvX& rp_addr)
{
    if (! group_prefix.is_multicast()) {
	XLOG_ERROR("Cannot delete configure RP with address %s "
		   "for group prefix %s: "
		   "not a multicast group prefix",
		   cstring(rp_addr),
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    if (! rp_addr.is_unicast()) {
	XLOG_ERROR("Cannot delete configure RP with address %s: "
		   "not an unicast address",
		   cstring(rp_addr));
	return (XORP_ERROR);
    }
    
    if (rp_table().delete_rp(rp_addr, group_prefix,
			     PimRp::RP_LEARNED_METHOD_STATIC)
	!= XORP_OK) {
	XLOG_ERROR("Cannot delete configure RP with address %s "
		   "for group prefix %s",
		   cstring(rp_addr),
		   cstring(group_prefix));
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Finish with the static RP configuration.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::config_rp_done()
{
    rp_table().apply_rp_changes();
    
    return (XORP_OK);
}

//
// Add a J/P entry to the _test_jp_header
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_test_jp_entry(const IPvX& source_addr, const IPvX& group_addr,
			   uint32_t group_masklen,
			   mrt_entry_type_t mrt_entry_type,
			   action_jp_t action_jp, uint16_t holdtime,
			   bool new_group_bool)
{
    int ret_value;
    
    ret_value = _test_jp_header.jp_entry_add(source_addr, group_addr,
					     group_masklen, mrt_entry_type,
					     action_jp, holdtime,
					     new_group_bool);
    
    return (ret_value);
}

//
// Send the accumulated state in the _test_jp_header
// XXX: the _test_jp_header is reset by the sending method
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::send_test_jp_entry(const IPvX& nbr_addr)
{
    int ret_value;
    PimNbr *pim_nbr = pim_nbr_find(nbr_addr);
    
    if (pim_nbr == NULL)
	return (XORP_ERROR);
    
    PimVif& pim_vif = pim_nbr->pim_vif();
    ret_value = pim_vif.pim_join_prune_send(pim_nbr, &_test_jp_header);
    
    return (ret_value);
}

