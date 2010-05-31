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

// $XORP: xorp/fea/mfea_kernel_messages.hh,v 1.9 2008/10/02 21:56:49 bms Exp $

#ifndef __FEA_MFEA_KERNEL_MESSAGES_HH__
#define __FEA_MFEA_KERNEL_MESSAGES_HH__


//
// The header file for defining various kernel signal message types.
//
// Those messages are typically sent from the Multicast FEA to the
// multicast routing protocols.
//


//
// Constants definitions
//

//
// The types of the kernel multicast signal messages
// 
// Note that MFEA_KERNEL_MESSAGE_BW_UPCALL is not sent by the
// Mfea to the protocol instances, because it is not always supported
// by the kernel. Therefore, the Mfea implements a work-around mechanism
// at user-level, and it uses separate channel to propagate
// this information to the protocol instances.
// 
#define MFEA_KERNEL_MESSAGE_NOCACHE	1
#define MFEA_KERNEL_MESSAGE_WRONGVIF	2
#define MFEA_KERNEL_MESSAGE_WHOLEPKT	3
#define MFEA_KERNEL_MESSAGE_BW_UPCALL	4


//
// Structures, typedefs and macros
//
//

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __FEA_MFEA_KERNEL_MESSAGES_HH__
