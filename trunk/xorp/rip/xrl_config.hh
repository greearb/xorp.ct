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

// $XORP: xorp/devnotes/template.hh,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $

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
