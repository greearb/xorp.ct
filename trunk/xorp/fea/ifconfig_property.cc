// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2009 XORP, Inc.
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
