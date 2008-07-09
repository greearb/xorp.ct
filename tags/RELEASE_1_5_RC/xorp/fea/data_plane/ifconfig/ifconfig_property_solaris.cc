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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_property_solaris.cc,v 1.1 2007/12/28 05:12:37 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "fea/ifconfig.hh"

#include "ifconfig_property_solaris.hh"


//
// Get information about the data plane property.
//
// The mechanism to obtain the information is for Solaris systems.
//


#ifdef HOST_OS_SOLARIS

IfConfigPropertySolaris::IfConfigPropertySolaris(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigProperty(fea_data_plane_manager)
{
}

IfConfigPropertySolaris::~IfConfigPropertySolaris()
{
}

bool
IfConfigPropertySolaris::test_have_ipv4() const
{
    XorpFd s = comm_sock_open(AF_INET, SOCK_DGRAM, 0, 0);
    if (! s.is_valid())
	return (false);

    comm_close(s);

    return (true);
}

bool
IfConfigPropertySolaris::test_have_ipv6() const
{
#ifndef HAVE_IPV6
    return (false);
#else
    XorpFd s = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, 0);
    if (! s.is_valid())
	return (false);

    comm_close(s);
    return (true);
#endif // HAVE_IPV6
}

#endif // HOST_OS_SOLARIS
