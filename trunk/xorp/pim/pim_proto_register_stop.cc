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

#ident "$XORP: xorp/pim/pim_proto_register_stop.cc,v 1.7 2003/09/30 18:27:06 pavlin Exp $"


//
// PIM PIM_REGISTER_STOP messages processing.
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


/**
 * PimVif::pim_register_stop_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_REGISTER_STOP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_register_stop_recv(PimNbr *pim_nbr,
			       const IPvX& src,
			       const IPvX& dst,
			       buffer_t *buffer)
{
    int rcvd_family;
    uint8_t group_addr_reserved_flags;
    uint8_t group_mask_len;
    IPvX source_addr(family()), group_addr(family());
    
    //
    // Parse the message
    //
    GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_mask_len,
			   group_addr_reserved_flags, buffer);
    GET_ENCODED_UNICAST_ADDR(rcvd_family, source_addr, buffer);
    
    if (! group_addr.is_multicast()) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "group address = %s must be multicast",
		     PIMTYPE2ASCII(PIM_REGISTER_STOP),
		     cstring(src), cstring(dst),
		     cstring(group_addr));
	return (XORP_ERROR);
    }

    if (group_addr.is_linklocal_multicast()
	|| group_addr.is_nodelocal_multicast()) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "group address = %s must not be be link or node-local "
		     "multicast",
		     PIMTYPE2ASCII(PIM_REGISTER_STOP),
		     cstring(src), cstring(dst),
		     cstring(group_addr));
	return (XORP_ERROR);
    }
    
    if (! (source_addr.is_unicast() || source_addr.is_zero())) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "source address = %s must be either unicast or zero",
		     PIMTYPE2ASCII(PIM_REGISTER_STOP),
		     cstring(src), cstring(dst),
		     cstring(source_addr));
	return (XORP_ERROR);
    }

    //
    // Process the Register-Stop data
    //
    pim_register_stop_process(src, source_addr, group_addr, group_mask_len);
    
    UNUSED(pim_nbr);
    return (XORP_OK);
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_REGISTER_STOP),
		 cstring(src), cstring(dst));
    ++_pimstat_rx_malformed_packet;
    return (XORP_ERROR);
    
 rcvd_mask_len_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid group mask length = %d",
		 PIMTYPE2ASCII(PIM_REGISTER_STOP),
		 cstring(src), cstring(dst),
		 group_mask_len);
    return (XORP_ERROR);
    
 rcvd_family_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid address family inside = %d",
		 PIMTYPE2ASCII(PIM_REGISTER_STOP),
		 cstring(src), cstring(dst),
		 rcvd_family);
    return (XORP_ERROR);
}

int
PimVif::pim_register_stop_process(const IPvX& rp_addr,
				  const IPvX& source_addr,
				  const IPvX& group_addr,
				  uint8_t group_mask_len)
{
    uint32_t	lookup_flags;
    PimMre	*pim_mre;
    
    lookup_flags = PIM_MRE_SG;
    
    if (group_mask_len != IPvX::addr_bitlen(family())) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "invalid group mask length = %d "
		     "instead of %u",
		     PIMTYPE2ASCII(PIM_REGISTER_STOP),
		     cstring(rp_addr), cstring(domain_wide_addr()),
		     group_mask_len, (uint32_t)IPvX::addr_bitlen(family()));
	return (XORP_ERROR);
    }
    
    if (! source_addr.is_zero()) {
	// (S,G) Register-Stop
	pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					 lookup_flags, 0);
	if (pim_mre == NULL) {
	    // XXX: we don't have this (S,G) state. Ignore
	    // TODO: print a warning, or do something else?
	    ++_pimstat_unknown_register_stop;
	    return (XORP_ERROR);
	}
	pim_mre->receive_register_stop();
	return (XORP_OK);
    }
    
    // (*,G) Register-Stop
    // Apply to all (S,G) entries for this group that are not in the NoInfo
    // state.
    // TODO: XXX: PAVPAVPAV: should schedule a timeslice task for this.
    PimMrtSg::const_gs_iterator iter_begin, iter_end, iter;
    iter_begin = pim_mrt().pim_mrt_sg().group_by_addr_begin(group_addr);
    iter_end = pim_mrt().pim_mrt_sg().group_by_addr_end(group_addr);
    for (iter = iter_begin; iter != iter_end; ++iter) {
	PimMre *pim_mre = iter->second;
	if (! pim_mre->is_register_noinfo_state())
	    pim_mre->receive_register_stop();
    }
    
    return (XORP_OK);
}

int
PimVif::pim_register_stop_send(const IPvX& dr_addr,
			       const IPvX& source_addr,
			       const IPvX& group_addr)
{
    uint8_t group_mask_len = IPvX::addr_bitlen(family());
    buffer_t *buffer = buffer_send_prepare();
    uint8_t group_addr_reserved_flags = 0;
    
    // Write all data to the buffer
    PUT_ENCODED_GROUP_ADDR(family(), group_addr, group_mask_len,
			   group_addr_reserved_flags, buffer);
    PUT_ENCODED_UNICAST_ADDR(family(), source_addr, buffer);
    
    return (pim_send(dr_addr, PIM_REGISTER_STOP, buffer));
    
 invalid_addr_family_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_REGISTER_STOP),
	       cstring(domain_wide_addr()), cstring(dr_addr),
	       family());
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_REGISTER_STOP),
	       cstring(domain_wide_addr()), cstring(dr_addr));
    return (XORP_ERROR);
}
