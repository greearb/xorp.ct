// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_proto_graft_ack.cc,v 1.1.1.1 2002/12/11 23:56:12 hodson Exp $"


//
// PIM PIM_GRAFT_ACK messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
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
