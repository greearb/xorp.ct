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

// $XORP: xorp/cli/cli_private.hh,v 1.1.1.1 2002/12/11 23:55:52 hodson Exp $


#ifndef __CLI_CLI_PRIVATE_HH__
#define __CLI_CLI_PRIVATE_HH__


//
// CLI implementation-specific definitions.
//


#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"


//
// Constants definitions
//
#define CLI_MAX_CONNECTIONS	129	// XXX: intentionally not 2^n number

#define XORP_CLI_WELCOME "Welcome, and may the Xorp be with you!"
#define XORP_CLI_PROMPT  "Xorp> "
#define XORP_CLI_PROMPT_ENABLE  "XORP> "

#ifndef CHAR_TO_CTRL
#define CHAR_TO_CTRL(c) ((c) & 0x1f)
#endif
#ifndef CHAR_TO_META
#define CHAR_TO_META(c) ((c) | 0x080)
#endif
    

//
// Structures/classes, typedefs and macros
//

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __CLI_CLI_PRIVATE_HH__
