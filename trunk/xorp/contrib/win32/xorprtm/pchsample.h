/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
 * vim:set sts=4 ts=8:
 *
 * Copyright (c) 2001-2008 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 *
 * $XORP: xorp/contrib/win32/xorprtm/pchsample.h,v 1.3 2007/02/16 22:45:32 pavlin Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#ifndef _PCHSAMPLE_H_
#define _PCHSAMPLE_H_

#include <winsock2.h>
#include <ws2tcpip.h>

#include <routprot.h>
#include <rtmv2.h>  
#include <iprtrmib.h>
#include <mgm.h>    

#include <mprerror.h>
#include <rtutils.h>

#include <stdio.h>
#include <wchar.h>

#include "xorprtm.h"

#include "list.h"  
#include "sync.h" 

#include "defs.h" 
#include "utils.h"

#include "xorprtm_internal.h"
#include "mibmgr.h"

#include "bsdroute.h"

#endif
