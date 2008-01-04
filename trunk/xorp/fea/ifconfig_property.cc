// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/ifconfig_property.cc,v 1.1 2007/12/28 05:12:35 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_property.hh"


//
// Data plane property plugin (base class implementation).
//


IfConfigProperty::IfConfigProperty(
    FeaDataPlaneManager& fea_data_plane_manager)
    : _is_running(false),
      _ifconfig(fea_data_plane_manager.ifconfig()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _have_ipv4(false),
      _have_ipv6(false),
      _first_start(true)
{
}

IfConfigProperty::~IfConfigProperty()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for testing "
		   "the data plane property: %s",
		   error_msg.c_str());
    }
}

int
IfConfigProperty::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    if (_first_start) {
	//
	// Test if the system supports IPv4 and IPv6 respectively
	//
	_have_ipv4 = test_have_ipv4();
	_have_ipv6 = test_have_ipv6();

	_first_start = false;
    }

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigProperty::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}
