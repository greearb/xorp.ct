// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/fibconfig_forwarding.cc,v 1.4 2008/07/23 05:10:08 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig.hh"
#include "fibconfig_forwarding.hh"


//
// Configure unicast forwarding plugin (base class implementation).
//


FibConfigForwarding::FibConfigForwarding(
    FeaDataPlaneManager& fea_data_plane_manager)
    : _is_running(false),
      _fibconfig(fea_data_plane_manager.fibconfig()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _orig_unicast_forwarding_enabled4(false),
      _orig_unicast_forwarding_enabled6(false),
      _orig_accept_rtadv_enabled6(false),
      _first_start(true)
{
}

FibConfigForwarding::~FibConfigForwarding()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for manipulating "
		   "the forwarding table information: %s",
		   error_msg.c_str());
    }
}

int
FibConfigForwarding::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (_first_start) {
	//
	// Get the old state from the underlying system
	//
	if (fea_data_plane_manager().have_ipv4()) {
	    if (unicast_forwarding_enabled4(_orig_unicast_forwarding_enabled4,
					    error_msg)
		!= XORP_OK) {
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
#ifdef HAVE_IPV6
	if (fea_data_plane_manager().have_ipv6()) {
	    if (unicast_forwarding_enabled6(_orig_unicast_forwarding_enabled6,
					    error_msg)
		!= XORP_OK) {
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	    if (accept_rtadv_enabled6(_orig_accept_rtadv_enabled6, error_msg)
		!= XORP_OK) {
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
#endif // HAVE_IPV6

	_first_start = false;
    }

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigForwarding::stop(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Restore the old forwarding state in the underlying system.
    //
    // XXX: Note that if the XORP forwarding entries are retained on shutdown,
    // then we don't restore the state.
    //
    if (fea_data_plane_manager().have_ipv4()) {
	if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown4()) {
	    if (set_unicast_forwarding_enabled4(_orig_unicast_forwarding_enabled4,
						error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
    }
#ifdef HAVE_IPV6
    if (fea_data_plane_manager().have_ipv6()) {
	if (! fibconfig().unicast_forwarding_entries_retain_on_shutdown6()) {
	    if (set_unicast_forwarding_enabled6(_orig_unicast_forwarding_enabled6,
						error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	    if (set_accept_rtadv_enabled6(_orig_accept_rtadv_enabled6,
					  error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = false;

    return (ret_value);
}
