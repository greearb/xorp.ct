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

// $XORP: xorp/libxorp/callback.hh,v 1.13 2004/03/05 22:12:49 hodson Exp $

#ifndef __LIBXORP_CALLBACK_HH__
#define __LIBXORP_CALLBACK_HH__

#define INCLUDED_FROM_CALLBACK_HH

#include "config.h"
#ifdef DEBUG_CALLBACKS
#include "callback_debug.hh"
#else
#include "callback_nodebug.hh"
#endif

#undef INCLUDED_FROM_CALLBACK_HH

#endif // __LIBXORP_CALLBACK_HH__
