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

#ident "$XORP: xorp/pim/pim_proto_bootstrap.cc,v 1.10 2004/02/22 04:07:44 pavlin Exp $"


//
// PIM PIM_BOOTSTRAP messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
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


/**
 * PimVif::pim_bootstrap_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_BOOTSTRAP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_bootstrap_recv(PimNbr *pim_nbr, const IPvX& src,
			   const IPvX& dst, buffer_t *buffer)
{
    int		rcvd_family;
    PimBsr&	pim_bsr = pim_node().pim_bsr();
    bool	is_unicast_message = false;
    IPvX	bsr_addr(family());
    uint8_t	hash_mask_len, bsr_priority;
    uint16_t	fragment_tag;
    uint8_t	group_mask_len = 0;
    BsrZone	*bsr_zone = NULL;
    BsrZone	*active_bsr_zone;
    size_t	group_prefix_n = 0;
    string	error_msg = "";
    int		ret_value = XORP_ERROR;
    
    //
    // Parse the head of the message
    //
    BUFFER_GET_HOST_16(fragment_tag, buffer);
    BUFFER_GET_OCTET(hash_mask_len, buffer);
    BUFFER_GET_OCTET(bsr_priority, buffer);
    GET_ENCODED_UNICAST_ADDR(rcvd_family, bsr_addr, buffer);
    if (! bsr_addr.is_unicast()) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "invalid Bootstrap router "
		     "address: %s. "
		     "Ignoring whole message.",
		     PIMTYPE2ASCII(PIM_BOOTSTRAP),
		     cstring(src), cstring(dst),
		     cstring(bsr_addr));
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    //
    // Message check
    //
    // XXX: the following checks were performed earlier:
    //      - There is Hello state for BSM.src_ip_address
    //      - The sender is directly connected
    // RPF check
    if (dst == IPvX::PIM_ROUTERS(family())) {
	PimNbr *pim_nbr_rpf = pim_node().pim_nbr_rpf_find(bsr_addr);
	if (pim_nbr_rpf != pim_nbr) {
	    // RPF failed
	    ret_value = XORP_ERROR;
	    ++_pimstat_rx_bsr_not_rpf_interface;
	    goto ret_label;
	}
    } else if (pim_node().vif_find_by_addr(dst) != NULL) {
	// BSM.dst_ip_address is one of my addresses
	is_unicast_message = true;
    } else {
	// Either the destination is not the right multicast group, or
	// somehow an unicast packet for somebody else came to me.
	// A third possibility is the destition unicast address is
	// one of mine, but it is not enabled for multicast routing.
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    //
    // Process the rest of the message
    //
    
    //
    // Create a new BsrZone that would contain all information
    // from this message.
    //
    bsr_zone = new BsrZone(pim_bsr, bsr_addr, bsr_priority, hash_mask_len,
			   fragment_tag);
    bsr_zone->set_is_accepted_message(true);
    bsr_zone->set_is_unicast_message(is_unicast_message, src);
    
    //
    // Main loop for message parsing
    //
    while (BUFFER_DATA_SIZE(buffer) > 0) {
	IPvX	group_addr(family());
	uint8_t	group_addr_reserved_flags;
	uint8_t	rp_count, fragment_rp_count;
	bool	is_scope_zone;
	BsrGroupPrefix *bsr_group_prefix = NULL;
	
	//
	// The Group (prefix) address
	//
	group_prefix_n++;
	GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_mask_len,
			       group_addr_reserved_flags, buffer);
	IPvXNet group_prefix(group_addr, group_mask_len);
	
	// Set the scope zone
	if (group_addr_reserved_flags & EGADDR_Z_BIT)
	    is_scope_zone = true;
	else
	    is_scope_zone = false;
	if (is_scope_zone && (group_prefix_n == 1)) {
	    PimScopeZoneId zone_id(group_prefix, is_scope_zone);
	    bsr_zone->set_zone_id(zone_id);
	}
	
	BUFFER_GET_OCTET(rp_count, buffer);
	BUFFER_GET_OCTET(fragment_rp_count, buffer);
	BUFFER_GET_SKIP(2, buffer);		// Reserved
	
	// Add the group prefix
	bsr_group_prefix = bsr_zone->add_bsr_group_prefix(group_prefix,
							  is_scope_zone,
							  rp_count);
	XLOG_ASSERT(bsr_group_prefix != NULL);
	
	//
	// The set of RPs
	//
	while (fragment_rp_count--) {
	    IPvX	rp_addr(family());
	    uint8_t	rp_priority;
	    uint16_t	rp_holdtime;
	    
	    GET_ENCODED_UNICAST_ADDR(rcvd_family, rp_addr, buffer);
	    BUFFER_GET_HOST_16(rp_holdtime, buffer);
	    BUFFER_GET_OCTET(rp_priority, buffer);
	    BUFFER_GET_SKIP(1, buffer);		// Reserved
	    bsr_group_prefix->add_rp(rp_addr, rp_priority, rp_holdtime);
	}
    }
    
    //
    // Test if this is not a bogus unicast Bootstrap message
    //
    if (is_unicast_message) {
	// Find if accepted previous BSM for same scope
	active_bsr_zone = pim_bsr.find_active_bsr_zone(bsr_zone->zone_id());
	if ((active_bsr_zone != NULL)
	    && (active_bsr_zone->is_accepted_message())) {
	    if (! (active_bsr_zone->is_unicast_message()
		   && (active_bsr_zone->unicast_message_src()
		       == bsr_zone->unicast_message_src())
		   && (active_bsr_zone->fragment_tag()
		       == bsr_zone->fragment_tag()))) {
		//
		// Any previous BSM for this scope has been accepted.
		// The packet was unicast, but this wasn't a quick refresh
		// on startup. Drop the BS message silently.
		//
		ret_value = XORP_ERROR;
		goto ret_label;
	    }
	}
    }
    
    //
    // Test whether the message is crossing Admin Scope border
    //
    if (pim_node().pim_scope_zone_table().is_scoped(bsr_zone->zone_id(),
						    pim_nbr->vif_index())) {
	// if (the interface the message arrived on is an Admin Scope
	//     border for the BSM.first_group_address) {
	//   drop the BS message silently
	// }
	// TODO: XXX: PAVPAVPAV: print a warning if the scoped zone
	// is NOT the local scoped zone!!
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    
    //
    // Test if the received data is consistent
    //
    if (! bsr_zone->is_consistent(error_msg)) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "inconsistent Bootstrap zone %s: %s",
		     PIMTYPE2ASCII(PIM_BOOTSTRAP),
		     cstring(src), cstring(dst),
		     cstring(bsr_zone->zone_id()),
		     error_msg.c_str());
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    //
    // Try to add the received BSM
    //
    active_bsr_zone = pim_bsr.add_active_bsr_zone(*bsr_zone, error_msg);
    if (active_bsr_zone == NULL) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "cannot add Bootstrap zone %s: %s",
		     PIMTYPE2ASCII(PIM_BOOTSTRAP),
		     cstring(src), cstring(dst),
		     cstring(bsr_zone->zone_id()),
		     error_msg.c_str());
	ret_value = XORP_ERROR;
	goto ret_label;
    }
    
    // Find the active BSR zone
    active_bsr_zone = pim_bsr.find_active_bsr_zone(bsr_zone->zone_id());
    
    //
    // If needed, originate my own BSR message
    //
    if ((active_bsr_zone != NULL)
	&& (active_bsr_zone->is_bsm_originate())) {
	active_bsr_zone->new_fragment_tag();
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	    if (pim_vif == NULL)
		continue;
	    pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(family()),
					*active_bsr_zone);
	}
	// Housekeeping: reset the bsm_originate flag.
	active_bsr_zone->set_bsm_originate(false);
    }
    
    //
    // If needed, forward the Bootstrap message
    //
    if ((active_bsr_zone != NULL) && (active_bsr_zone->is_bsm_forward())) {
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	    if (pim_vif == NULL)
		continue;
	    // XXX: always forward-back on the iif, because the 06 I-D is wrong
	    // if (pim_vif == this)
	    //	continue;		// Don't forward back on the iif
	    // XXX: use the BSR zone that was received to forward the message
	    pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(family()),
					*bsr_zone);
	}
	// Housekeeping: reset the bsm_forward flag.
	active_bsr_zone->set_bsm_forward(false);
    }
    
    
    //
    // Apply the new RP info
    //
    pim_bsr.add_rps_to_rp_table();
    
    ret_value = XORP_OK;
    goto ret_label;
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst));
    ++_pimstat_rx_malformed_packet;
    ret_value = XORP_ERROR;
    goto ret_label;
    
 rcvd_mask_len_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid group mask length = %d",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst),
		 group_mask_len);
    ret_value = XORP_ERROR;
    goto ret_label;
    
 rcvd_family_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid address family inside = %d",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst),
		 rcvd_family);
    ret_value = XORP_ERROR;
    goto ret_label;
    
 ret_label:
    if (bsr_zone != NULL)
	delete bsr_zone;
    return (ret_value);
}

// Return: %XORP_OK on success, otherwise %XORP_ERROR
int
PimVif::pim_bootstrap_send(const IPvX& dst_addr, const BsrZone& bsr_zone)
{
    size_t avail_buffer_size = 0;
    IPvX src_addr = primary_addr();

    if (dst_addr.is_unicast())
	src_addr = domain_wide_addr();
    
    if (bsr_zone.bsr_addr() == dst_addr)
	return (XORP_ERROR);	// Never send-back to the BSR
    
    if (bsr_zone.bsr_addr() == IPvX::ZERO(family()))
	return (XORP_ERROR);	// There is no BSR yet
    
    if (pim_nbrs_number() == 0)
	return (XORP_ERROR);	// No PIM neighbors yet, hence don't send it
    
    if (pim_node().pim_scope_zone_table().is_scoped(bsr_zone.zone_id(),
						    vif_index())) {
	return (XORP_ERROR);	// Don't across scope zone boundary
    }
    
    buffer_t *buffer = pim_bootstrap_send_prepare(src_addr, dst_addr,
						  bsr_zone, true);
    if (buffer == NULL)
	return (XORP_ERROR);
    
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	size_t needed_size = 0;
	size_t received_rp_count = bsr_group_prefix->received_rp_count();
	size_t max_fragment_rp_count = 0;
	size_t send_fragment_rp_count = 0;
	size_t send_remain_rp_count = 0;
	uint8_t group_addr_reserved_flags = 0;
	
	if (bsr_group_prefix->is_scope_zone())
	    group_addr_reserved_flags |= EGADDR_Z_BIT;
	
	needed_size =
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t);
	needed_size +=
	    received_rp_count * (
		ENCODED_UNICAST_ADDR_SIZE(family())
		+ sizeof(uint16_t)
		+ sizeof(uint8_t)
		+ sizeof(uint8_t));
	if (needed_size > BUFFER_AVAIL_TAIL(buffer)) {
	    //
	    // Try to do fragmentation among group prefixes for this zone
	    //
	    if (iter_prefix != bsr_zone.bsr_group_prefix_list().begin()) {
		// Send the accumulated prefixes so far
		if (pim_send(src_addr, dst_addr, PIM_BOOTSTRAP, buffer) < 0)
		    return (XORP_ERROR);
		buffer = pim_bootstrap_send_prepare(src_addr, dst_addr,
						    bsr_zone, false);
		if (buffer == NULL)
		    return (XORP_ERROR);
	    }
	}
	
	//
	// Compute the max. number of RPs we can add to the message
	//
	avail_buffer_size = BUFFER_AVAIL_TAIL(buffer);
	if (avail_buffer_size <
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t)) {
	    goto buflen_error;
	}
	avail_buffer_size -=
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t);
	
	max_fragment_rp_count = avail_buffer_size /
	    (ENCODED_UNICAST_ADDR_SIZE(family())
	     + sizeof(uint16_t)
	     + sizeof(uint8_t)
	     + sizeof(uint8_t));
	if (max_fragment_rp_count == 0)
	    goto buflen_error;
	send_fragment_rp_count = max_fragment_rp_count;
	if (send_fragment_rp_count > received_rp_count)
	    send_fragment_rp_count = received_rp_count;
	send_remain_rp_count = received_rp_count;
	
	PUT_ENCODED_GROUP_ADDR(family(),
			       bsr_group_prefix->group_prefix().masked_addr(),
			       bsr_group_prefix->group_prefix().prefix_len(),
			       group_addr_reserved_flags, buffer);
	// RP count and Fragment RP count
	BUFFER_PUT_OCTET(bsr_group_prefix->expected_rp_count(), buffer);
	BUFFER_PUT_OCTET(send_fragment_rp_count, buffer);
	BUFFER_PUT_HOST_16(0, buffer);			// Reserved
	// The list of RPs
	list<BsrRp *>::const_iterator iter_rp;
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    
	    if (send_fragment_rp_count == 0) {
		//
		// Send the accumulated RPs so far
		//
		if (pim_send(src_addr, dst_addr, PIM_BOOTSTRAP, buffer) < 0)
		    return (XORP_ERROR);
		buffer = pim_bootstrap_send_prepare(src_addr, dst_addr,
						    bsr_zone, false);
		if (buffer == NULL)
		    return (XORP_ERROR);
		
		avail_buffer_size = BUFFER_AVAIL_TAIL(buffer);
		if (avail_buffer_size <
		    ENCODED_GROUP_ADDR_SIZE(family())
		    + sizeof(uint8_t)
		    + sizeof(uint8_t)
		    + sizeof(uint16_t)) {
		    goto buflen_error;
		}
		avail_buffer_size -=
		    ENCODED_GROUP_ADDR_SIZE(family())
		    + sizeof(uint8_t)
		    + sizeof(uint8_t)
		    + sizeof(uint16_t);
		
		max_fragment_rp_count = avail_buffer_size /
		    (ENCODED_UNICAST_ADDR_SIZE(family())
		     + sizeof(uint16_t)
		     + sizeof(uint8_t)
		     + sizeof(uint8_t));
		if (max_fragment_rp_count == 0)
		    goto buflen_error;
		send_fragment_rp_count = max_fragment_rp_count;
		if (send_fragment_rp_count > send_remain_rp_count)
		    send_fragment_rp_count = send_remain_rp_count;
		
		PUT_ENCODED_GROUP_ADDR(family(),
				       bsr_group_prefix->group_prefix().masked_addr(),
				       bsr_group_prefix->group_prefix().prefix_len(),
				       group_addr_reserved_flags, buffer);
		// RP count and Fragment RP count
		BUFFER_PUT_OCTET(bsr_group_prefix->expected_rp_count(),
				 buffer);
		BUFFER_PUT_OCTET(send_fragment_rp_count, buffer);
		BUFFER_PUT_HOST_16(0, buffer);		// Reserved

	    }
	    send_fragment_rp_count--;
	    send_remain_rp_count--;
	    
	    //
	    // Put the RP information
	    //
	    PUT_ENCODED_UNICAST_ADDR(family(), bsr_rp->rp_addr(), buffer);
	    // If this is canceling Bootstrap message, set the Holdtime
	    // to zero for my RP addresses.
	    if (bsr_zone.is_cancel()
		&& (pim_node().is_my_addr(bsr_rp->rp_addr()))) {
		BUFFER_PUT_HOST_16(0, buffer);
	    } else {
		BUFFER_PUT_HOST_16(bsr_rp->rp_holdtime(), buffer);
	    }
	    BUFFER_PUT_OCTET(bsr_rp->rp_priority(), buffer);
	    BUFFER_PUT_OCTET(0, buffer);		// Reserved
	}
    }
    
    //
    // Send the lastest fragment
    //
    if (pim_send(src_addr, dst_addr, PIM_BOOTSTRAP, buffer) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
    
 invalid_addr_family_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(src_addr), cstring(dst_addr),
	       family());
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(src_addr), cstring(dst_addr));
    return (XORP_ERROR);
}

//
// Prepare the zone-specific data and add it to the buffer to send.
//
buffer_t *
PimVif::pim_bootstrap_send_prepare(const IPvX& src_addr, const IPvX& dst_addr,
				   const BsrZone& bsr_zone,
				   bool is_first_fragment)
{
    buffer_t *buffer = buffer_send_prepare(_buffer_send_bootstrap);
    uint8_t hash_mask_len = bsr_zone.hash_mask_len();
    uint8_t group_addr_reserved_flags = 0;
    
    //
    // Write zone-related data to the buffer
    //
    BUFFER_PUT_HOST_16(bsr_zone.fragment_tag(), buffer);
    BUFFER_PUT_OCTET(hash_mask_len, buffer);
    if (bsr_zone.is_cancel())
	BUFFER_PUT_OCTET(PIM_BOOTSTRAP_LOWEST_PRIORITY, buffer);
    else
	BUFFER_PUT_OCTET(bsr_zone.bsr_priority(), buffer);
    PUT_ENCODED_UNICAST_ADDR(family(), bsr_zone.bsr_addr(), buffer);
    
    //
    // Test whether we need a prefix for the entire admin scope
    // range with no RPs.
    //
    do {
	if (! bsr_zone.zone_id().is_scope_zone())
	    break;
	
	list<BsrGroupPrefix *>::const_iterator iter_prefix
	    = bsr_zone.bsr_group_prefix_list().begin();
	if (iter_prefix == bsr_zone.bsr_group_prefix_list().end())
	    break;
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	if (is_first_fragment
	    && (bsr_group_prefix->group_prefix()
		== bsr_zone.zone_id().scope_zone_prefix()))
	    break;	// XXX: the admin scope range will be added later
	
	//
	// Add the entire admin scope range with no RPs.
	//
	group_addr_reserved_flags = 0;
	group_addr_reserved_flags |= EGADDR_Z_BIT;
	PUT_ENCODED_GROUP_ADDR(family(),
			       bsr_zone.zone_id().scope_zone_prefix().masked_addr(),
			       bsr_zone.zone_id().scope_zone_prefix().prefix_len(),
			       group_addr_reserved_flags, buffer);
	BUFFER_PUT_OCTET(0, buffer);		// RP count
	BUFFER_PUT_OCTET(0, buffer);		// Fragment RP count
	BUFFER_PUT_HOST_16(0, buffer);		// Reserved
    } while (false);
    
    return (buffer);

 invalid_addr_family_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(src_addr), cstring(dst_addr),
	       family());
    return (NULL);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(src_addr), cstring(dst_addr));
    return (NULL);
}
