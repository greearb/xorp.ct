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

// $XORP: xorp/mfea/mfea_unix_kernel_messages.hh,v 1.1.1.1 2002/12/11 23:56:06 hodson Exp $

#ifndef __MFEA_MFEA_UNIX_KERNEL_MESSAGES_HH__
#define __MFEA_MFEA_UNIX_KERNEL_MESSAGES_HH__


//
// The header file for defining various UNIX kernel signal message types.
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
// Note that MFEA_UNIX_KERNEL_MESSAGE_BW_UPCAL is not sent by the
// Mfea to the protocol instances, because it is not always supported
// by the kernel. For this reason the Mfea implements an user-level
// work-around mechanism, and it uses specific methods to propagate
// this information to the protocol instances.
// 
#define MFEA_UNIX_KERNEL_MESSAGE_NOCACHE	1
#define MFEA_UNIX_KERNEL_MESSAGE_WRONGVIF	2
#define MFEA_UNIX_KERNEL_MESSAGE_WHOLEPKT	3
#define MFEA_UNIX_KERNEL_MESSAGE_BW_UPCALL	4


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

#endif // __MFEA_MFEA_UNIX_KERNEL_MESSAGES_HH__
