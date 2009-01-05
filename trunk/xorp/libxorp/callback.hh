// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/callback.hh,v 1.23 2008/10/02 21:57:29 bms Exp $

#ifndef __LIBXORP_CALLBACK_HH__
#define __LIBXORP_CALLBACK_HH__

#include "libxorp/xorp.h"

#define INCLUDED_FROM_CALLBACK_HH

#ifdef DEBUG_CALLBACKS
#include "callback_debug.hh"
#else
#include "callback_nodebug.hh"
#endif

#undef INCLUDED_FROM_CALLBACK_HH

#endif // __LIBXORP_CALLBACK_HH__
