// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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


//
// FEA (Forwarding Engine Abstraction) XRL-based I/O implementation.
//


#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "xrl_fea_io.hh"


XrlFeaIO::XrlFeaIO(EventLoop& eventloop)
    : FeaIO(eventloop)
{
}

XrlFeaIO::~XrlFeaIO()
{
    shutdown();
}

int
XrlFeaIO::startup()
{
    return (FeaIO::startup());
}

int
XrlFeaIO::shutdown()
{
    return (FeaIO::shutdown());
}

bool
XrlFeaIO::is_running() const
{
    return (FeaIO::is_running());
}
