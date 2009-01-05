// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/firewall/firewall_get_dummy.cc,v 1.4 2008/10/09 00:57:11 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/firewall_manager.hh"

#include "firewall_get_dummy.hh"
#include "firewall_set_dummy.hh"


//
// Get information about firewall entries from the underlying system.
//
// The mechanism to obtain the information is Dummy (for testing purpose).
//

FirewallGetDummy::FirewallGetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallGet(fea_data_plane_manager)
{
}

FirewallGetDummy::~FirewallGetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to get "
		   "information about firewall entries from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FirewallGetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FirewallGetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FirewallGetDummy::get_table4(list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    FirewallSetDummy* firewall_set_dummy;
    FirewallSetDummy::FirewallTrie::const_iterator iter;

    firewall_set_dummy = dynamic_cast<FirewallSetDummy *>(fea_data_plane_manager().firewall_set());
    if (firewall_set_dummy == NULL) {
	error_msg = c_format("Firewall plugin mismatch: expected "
			     "Dummy firewall set plugin mot found");
	return (XORP_ERROR);
    }

    for (iter = firewall_set_dummy->firewall_entries4().begin();
	 iter != firewall_set_dummy->firewall_entries4().end();
	 ++iter) {
	const FirewallEntry& firewall_entry = iter->second;
	firewall_entry_list.push_back(firewall_entry);
    }

    return (XORP_OK);
}

int
FirewallGetDummy::get_table6(list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    FirewallSetDummy* firewall_set_dummy;
    FirewallSetDummy::FirewallTrie::const_iterator iter;

    firewall_set_dummy = dynamic_cast<FirewallSetDummy *>(fea_data_plane_manager().firewall_set());
    if (firewall_set_dummy == NULL) {
	error_msg = c_format("Firewall plugin mismatch: expected "
			     "Dummy firewall set plugin mot found");
	return (XORP_ERROR);
    }

    for (iter = firewall_set_dummy->firewall_entries6().begin();
	 iter != firewall_set_dummy->firewall_entries6().end();
	 ++iter) {
	const FirewallEntry& firewall_entry = iter->second;
	firewall_entry_list.push_back(firewall_entry);
    }

    return (XORP_OK);
}
