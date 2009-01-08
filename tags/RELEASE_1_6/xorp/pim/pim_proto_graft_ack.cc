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

#ident "$XORP: xorp/pim/pim_proto_graft_ack.cc,v 1.10 2008/10/02 21:57:55 bms Exp $"


//
// PIM PIM_GRAFT_ACK messages processing.
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
 * PimVif::pim_graft_ack_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_GRAFT_ACK message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_graft_ack_recv(PimNbr *pim_nbr,
			   const IPvX& , // src
			   const IPvX& , // dst
			   buffer_t *buffer)
{
    UNUSED(pim_nbr);
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(buffer);
    
    return (XORP_OK);
}
