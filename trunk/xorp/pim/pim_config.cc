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

#ident "$XORP: xorp/pim/pim_config.cc,v 1.4 2003/02/25 01:38:48 pavlin Exp $"


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
PimNode::set_vif_is_tracking_support_disabled(const string& vif_name,
					      bool is_tracking_support_disabled)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->is_tracking_support_disabled().set(is_tracking_support_disabled);
    
    // Send immediately a Hello message with the new value
    pim_vif->pim_hello_send();
    
    return (XORP_OK);
}

int
PimNode::reset_vif_is_tracking_support_disabled(const string& vif_name)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    pim_vif->is_tracking_support_disabled().reset();
    
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
PimNode::add_config_cand_bsr_by_vif_name(const IPvXNet& scope_zone_id,
					 bool is_scope_zone,
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
    
    if (add_config_cand_bsr_by_addr(scope_zone_id,
				    is_scope_zone,
				    *pim_vif->addr_ptr(),
				    bsr_priority,
				    hash_masklen) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Add myself as a Cand-BSR.
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_bsr_by_addr(const IPvXNet& scope_zone_id,
				     bool is_scope_zone,
				     const IPvX& my_cand_bsr_addr,
				     uint8_t bsr_priority,
				     uint8_t hash_masklen)
{
    uint16_t fragment_tag = RANDOM(0xffff);
    string error_msg = "";
    PimScopeZoneId zone_id(scope_zone_id, is_scope_zone);
    
    // XXX: if hash_masklen is 0, then set its value to default
    if (hash_masklen == 0)
	hash_masklen = PIM_BOOTSTRAP_HASH_MASKLEN_DEFAULT(family());
    
    BsrZone new_bsr_zone(pim_bsr(), my_cand_bsr_addr, bsr_priority,
			 hash_masklen, fragment_tag);
    new_bsr_zone.set_zone_id(zone_id);
    new_bsr_zone.set_i_am_candidate_bsr(true, my_cand_bsr_addr, bsr_priority);
    
    if (pim_bsr().add_config_bsr_zone(new_bsr_zone, error_msg) == NULL) {
	XLOG_ERROR("Cannot add configure BSR with address %s "
		   "for zone %s: %s",
		   cstring(my_cand_bsr_addr), cstring(zone_id),
		   error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::delete_config_cand_bsr(const IPvXNet& scope_zone_id,
				bool is_scope_zone)
{
    BsrZone *bsr_zone = NULL;
    bool is_up = false;
    PimScopeZoneId zone_id(scope_zone_id, is_scope_zone);
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_config_bsr_zone(zone_id);
    if (bsr_zone == NULL) {
	XLOG_ERROR("Cannot delete configure BSR for zone %s: "
		   "zone not found",
		   cstring(zone_id));
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
	bsr_zone->set_i_am_candidate_bsr(false, IPvX::ZERO(family()), 0);
    }
    
    if (is_up)
	pim_bsr().start();	// XXX: restart the BSR
    
    return (XORP_OK);
}

//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_rp_by_vif_name(const IPvXNet& group_prefix,
					bool is_scope_zone,
					const string& vif_name,
					uint8_t rp_priority,
					uint16_t rp_holdtime)
{
    // XXX: Find the vif address
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
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
    
    if (add_config_cand_rp_by_addr(group_prefix,
				   is_scope_zone,
				   *pim_vif->addr_ptr(),
				   rp_priority,
				   rp_holdtime) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

//
// Add myself as a Cand-RP
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::add_config_cand_rp_by_addr(const IPvXNet& group_prefix,
				    bool is_scope_zone,
				    const IPvX& my_cand_rp_addr,
				    uint8_t rp_priority,
				    uint16_t rp_holdtime)
{
    BsrZone *config_bsr_zone = NULL;
    string error_msg = "";
    bool is_new_zone = false;
    
    if (! is_my_addr(my_cand_rp_addr)) {
	XLOG_ERROR("Cannot add configure Cand-RP with address %s: "
		   "not my address",
		   cstring(my_cand_rp_addr));
	return (XORP_ERROR);
    }
    
    config_bsr_zone = pim_bsr().find_config_bsr_zone_by_prefix(group_prefix,
							       is_scope_zone);
    
    if (config_bsr_zone == NULL) {
	BsrZone new_bsr_zone(pim_bsr(), PimScopeZoneId(group_prefix,
						       is_scope_zone));
	config_bsr_zone = pim_bsr().add_config_bsr_zone(new_bsr_zone,
							error_msg);
	if (config_bsr_zone == NULL) {
	    XLOG_ERROR("Cannot add configure Cand-RP for "
		       "zone group prefix %s (%s): %s",
		       cstring(group_prefix),
		       (is_scope_zone)? "scoped" : "non-scoped",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}
	is_new_zone = true;
    }
    
    if (config_bsr_zone->add_rp(group_prefix, is_scope_zone,
				my_cand_rp_addr, rp_priority,
				rp_holdtime, error_msg)
	== NULL) {
	XLOG_ERROR("Cannot add configure Cand-RP address %s for "
		   "zone group prefix %s (%s): %s",
		   cstring(my_cand_rp_addr),
		   cstring(group_prefix),
		   (is_scope_zone)? "scoped" : "non-scoped",
		   error_msg.c_str());
	if (is_new_zone)
	    pim_bsr().delete_config_bsr_zone(config_bsr_zone);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::delete_config_cand_rp_by_vif_name(const IPvXNet& group_prefix,
					   bool is_scope_zone,
					   const string& vif_name)
{
    // XXX: Find the vif address
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
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
    
    if (delete_config_cand_rp_by_addr(group_prefix,
				      is_scope_zone,
				      *pim_vif->addr_ptr()) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::delete_config_cand_rp_by_addr(const IPvXNet& group_prefix,
				       bool is_scope_zone,
				       const IPvX& my_cand_rp_addr)
{
    BsrZone *bsr_zone;
    BsrGroupPrefix *bsr_group_prefix;
    BsrRp *bsr_rp;
    bool is_up = false;
    
    //
    // Find the BSR zone
    //
    bsr_zone = pim_bsr().find_config_bsr_zone_by_prefix(group_prefix,
							is_scope_zone);
    if (bsr_zone == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for zone for "
		   "group prefix %s (%s): zone not found",
		   cstring(group_prefix),
		   (is_scope_zone)? "scoped" : "non-scoped");
	return (XORP_ERROR);
    }
    
    //
    // Find the BSR group prefix
    //
    bsr_group_prefix = bsr_zone->find_bsr_group_prefix(group_prefix);
    if (bsr_group_prefix == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for zone for "
		   "group prefix %s (%s): prefix not found",
		   cstring(group_prefix),
		   (is_scope_zone)? "scoped" : "non-scoped");
	return (XORP_ERROR);
    }
    
    //
    // Find the RP
    //
    bsr_rp = bsr_group_prefix->find_rp(my_cand_rp_addr);
    if (bsr_rp == NULL) {
	XLOG_ERROR("Cannot delete configure Cand-RP for zone for "
		   "group prefix %s (%s) and RP %s: RP not found",
		   cstring(group_prefix),
		   (is_scope_zone)? "scoped" : "non-scoped",
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
		   "not a multicast address",
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
PimNode::delete_config_rp(const IPvXNet& group_prefix, const IPvX& rp_addr)
{
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

//
// Send test Assert message on an interface.
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
//
int
PimNode::send_test_assert(const string& vif_name,
			  const IPvX& source_addr,
			  const IPvX& group_addr,
			  bool rpt_bit,
			  uint32_t metric_preference,
			  uint32_t metric)
{
    PimVif *pim_vif = vif_find_by_name(vif_name);
    
    if (pim_vif == NULL)
	return (XORP_ERROR);
    
    if (pim_vif->pim_assert_send(source_addr,
				 group_addr,
				 rpt_bit,
				 metric_preference,
				 metric) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_zone(const PimScopeZoneId& zone_id,
			   const IPvX& bsr_addr,
			   uint8_t bsr_priority,
			   uint8_t hash_masklen,
			   uint16_t fragment_tag)
{
    if (pim_bsr().add_test_bsr_zone(zone_id, bsr_addr, bsr_priority,
				    hash_masklen, fragment_tag)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_group_prefix(const PimScopeZoneId& zone_id,
				   const IPvXNet& group_prefix,
				   bool is_scope_zone,
				   uint8_t expected_rp_count)
{
    if (pim_bsr().add_test_bsr_group_prefix(zone_id, group_prefix,
					    is_scope_zone, expected_rp_count)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::add_test_bsr_rp(const PimScopeZoneId& zone_id,
			 const IPvXNet& group_prefix,
			 const IPvX& rp_addr,
			 uint8_t rp_priority,
			 uint16_t rp_holdtime)
{
    if (pim_bsr().add_test_bsr_rp(zone_id, group_prefix, rp_addr, rp_priority,
				  rp_holdtime)
	== NULL) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_bootstrap(const string& vif_name)
{
    if (pim_bsr().send_test_bootstrap(vif_name) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_bootstrap_by_dest(const string& vif_name,
				     const IPvX& dest_addr)
{
    if (pim_bsr().send_test_bootstrap_by_dest(vif_name, dest_addr) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
PimNode::send_test_cand_rp_adv()
{
    if (pim_bsr().send_test_cand_rp_adv() < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}
