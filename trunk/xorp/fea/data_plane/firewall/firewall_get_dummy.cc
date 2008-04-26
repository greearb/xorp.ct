// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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
			     "dummy firewall set plugin mot found");
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
			     "dummy firewall set plugin mot found");
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
