// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP$"

#include "config.h"
#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "xrl_io.hh"

bool
XrlIO::send(const char* interface, const char *vif,
	    const char *src,
	    const char *dst,
	    uint8_t* data, uint32_t len)
{
    debug_msg("send(%s,%s,%s,%s,%p,%d\n",
	      interface, vif, src, dst, data, len);

    return true;
}

bool
XrlIO::register_receive(ReceiveCallback cb)
{
    _cb = cb;

    return true;
}

bool
XrlIO::add_route()
{
    return true;
}

bool
XrlIO::delete_route()
{
    return true;
}


