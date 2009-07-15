// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net




//
// PIM PIM_GRAFT messages processing.
//


#include "pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

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
 * PimVif::pim_graft_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_GRAFT message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_graft_recv(PimNbr *pim_nbr,
		       const IPvX& src,
		       const IPvX& , // dst
		       buffer_t *buffer)
{
    int ret_value;
    buffer_t *buffer2;
    string dummy_error_msg;
    
    //
    // Must unicast back a Graft-Ack to the originator of this Graft.
    //
    buffer2 = buffer_send_prepare();
    BUFFER_PUT_DATA(BUFFER_DATA_HEAD(buffer), buffer2,
		    BUFFER_DATA_SIZE(buffer));
    ret_value = pim_send(domain_wide_addr(), src, PIM_GRAFT_ACK, buffer2,
			 dummy_error_msg);
    
    UNUSED(pim_nbr);
    // UNUSED(dst);
    
    return (ret_value);
    
    // Various error processing
 buflen_error:
    XLOG_UNREACHABLE();
    dummy_error_msg = c_format("TX %s from %s to %s: "
			       "packet cannot fit into sending buffer",
			       PIMTYPE2ASCII(PIM_GRAFT_ACK),
			       cstring(domain_wide_addr()), cstring(src));
    XLOG_ERROR("%s", dummy_error_msg.c_str());
    return (XORP_ERROR);
}

#if 0		// TODO: XXX: implement/use it
int
PimVif::pim_graft_send(const IPvX& dst, buffer_t *buffer)
{
    int ret_value;
    IPvX src = dst.is_unicast()? domain_wide_addr() : primary_addr();
    
    ret_value = pim_send(src, dst, PIM_GRAFT, buffer);
    
    return (ret_value);

    // Various error processing
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_GRAFT),
	       cstring(src), cstring(dst));
}
#endif /* 0 */
