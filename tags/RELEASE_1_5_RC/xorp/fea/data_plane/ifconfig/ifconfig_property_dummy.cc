// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_property_dummy.cc,v 1.1 2007/12/28 05:12:37 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/ifconfig.hh"

#include "ifconfig_property_dummy.hh"


//
// Get information about the data plane property.
//
// The mechanism to obtain the information is Dummy (for testing purpose).
//


IfConfigPropertyDummy::IfConfigPropertyDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigProperty(fea_data_plane_manager)
{
}

IfConfigPropertyDummy::~IfConfigPropertyDummy()
{
}

bool
IfConfigPropertyDummy::test_have_ipv4() const
{
    return (true);
}

bool
IfConfigPropertyDummy::test_have_ipv6() const
{
    return (true);
}
