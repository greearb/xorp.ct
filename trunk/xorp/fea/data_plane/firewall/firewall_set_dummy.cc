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

#ident "$XORP: xorp/fea/data_plane/firewall/firewall_set_dummy.cc,v 1.1 2008/04/26 00:59:47 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/firewall_manager.hh"

#include "firewall_set_dummy.hh"


//
// Set firewall information into the underlying system.
//
// The mechanism to set the information is Dummy (for testing purpose).
//

FirewallSetDummy::FirewallSetDummy(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallSet(fea_data_plane_manager)
{
}

FirewallSetDummy::~FirewallSetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Dummy mechanism to set "
		   "firewall information into the underlying system: %s",
		   error_msg.c_str());
    }
}

int
FirewallSetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
FirewallSetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
FirewallSetDummy::add_entry(const FirewallEntry& firewall_entry,
			    string& error_msg)
{
    FirewallTrie::iterator iter;
    FirewallTrie* ftp = NULL;
    uint32_t key = firewall_entry.rule_number();  // XXX: the map key

    UNUSED(error_msg);

    if (firewall_entry.is_ipv4())
	ftp = &_firewall_entries4;
    else
	ftp = &_firewall_entries6;

    //
    // XXX: If the entry already exists, then just update it.
    // Note that the replace_entry() implementation relies on this.
    //
    iter = ftp->find(key);
    if (iter == ftp->end()) {
	ftp->insert(make_pair(key, firewall_entry));
    } else {
	FirewallEntry& fe_tmp = iter->second;
	fe_tmp = firewall_entry;
    }

    return (XORP_OK);
}

int
FirewallSetDummy::replace_entry(const FirewallEntry& firewall_entry,
				string& error_msg)
{
    //
    // XXX: The add_entry() method implementation covers the replace_entry()
    // semantic as well.
    //
    return (add_entry(firewall_entry, error_msg));
}

int
FirewallSetDummy::delete_entry(const FirewallEntry& firewall_entry,
			       string& error_msg)
{
    FirewallTrie::iterator iter;
    FirewallTrie* ftp = NULL;
    uint32_t key = firewall_entry.rule_number();  // XXX: the map key

    if (firewall_entry.is_ipv4())
	ftp = &_firewall_entries4;
    else
	ftp = &_firewall_entries6;

    // Find the entry
    iter = ftp->find(key);
    if (iter == ftp->end()) {
	error_msg = c_format("Entry not found");
	return (XORP_ERROR);
    }
    ftp->erase(iter);

    return (XORP_OK);
}

int
FirewallSetDummy::set_table4(const list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    if (delete_all_entries4(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	_firewall_entries4.insert(make_pair(firewall_entry.rule_number(),
					    firewall_entry));
    }

    return (XORP_OK);
}

int
FirewallSetDummy::delete_all_entries4(string& error_msg)
{
    UNUSED(error_msg);

    _firewall_entries4.clear();

    return (XORP_OK);
}

int
FirewallSetDummy::set_table6(const list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    if (delete_all_entries6(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	_firewall_entries6.insert(make_pair(firewall_entry.rule_number(),
					    firewall_entry));
    }

    return (XORP_OK);
}

int
FirewallSetDummy::delete_all_entries6(string& error_msg)
{
    UNUSED(error_msg);

    _firewall_entries6.clear();

    return (XORP_OK);
}
