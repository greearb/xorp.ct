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

// $XORP: xorp/fea/mfea_kernel_messages.hh,v 1.1 2003/05/15 23:10:30 pavlin Exp $

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
