// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/xrl_config.hh,v 1.7 2008/07/23 05:11:37 pavlin Exp $

#ifndef __RIP_XRL_CONFIG_H__
#define __RIP_XRL_CONFIG_H__

/**
 * Get name to address RIB by in XRL calls.
 */
const char* xrl_rib_name();

/**
 * Get name to address FEA by in XRL calls.
 */
const char* xrl_fea_name();

/**
 * Set name to address RIB by in XRL calls.
 *
 * NB this call should be made before any calls to xrl_rib_name().
 */
void set_xrl_rib_name(const char* rib_name);

/**
 * Set name to address FEA by in XRL calls.
 *
 * NB this call should be made before any calls to xrl_rib_name().
 */
void set_xrl_fea_name(const char* fea_name);

#endif // __RIP_XRL_CONFIG_H__
