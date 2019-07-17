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




//
// MLD6IGMP protocol CLI implementation
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mld6igmp_group_record.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_node_cli.hh"
#include "mld6igmp_vif.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//



/**
 * Mld6igmpNodeCli::Mld6igmpNodeCli:
 * @mld6igmp_node: The MLD6IGMP node to use.
 * 
 * Mld6igmpNodeCli constructor.
 **/
Mld6igmpNodeCli::Mld6igmpNodeCli(Mld6igmpNode& mld6igmp_node)
    : ProtoNodeCli(mld6igmp_node.family(), mld6igmp_node.module_id()),
      _mld6igmp_node(mld6igmp_node)
{

}

Mld6igmpNodeCli::~Mld6igmpNodeCli()
{
    stop();
}

int
Mld6igmpNodeCli::start()
{
    if (! is_enabled())
	return (XORP_OK);

    if (is_up() || is_pending_up())
	return (XORP_OK);

    if (ProtoUnit::start() != XORP_OK)
	return (XORP_ERROR);

    if (add_all_cli_commands() != XORP_OK)
	return (XORP_ERROR);

    XLOG_INFO("CLI started");

    return (XORP_OK);
}

int
Mld6igmpNodeCli::stop()
{
    int ret_code = XORP_OK;

    if (is_down())
	return (XORP_OK);

    if (delete_all_cli_commands() != XORP_OK)
	ret_code = XORP_ERROR;

    XLOG_INFO("CLI stopped");

    return (ret_code);
}

/**
 * Enable the node operation.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
Mld6igmpNodeCli::enable()
{
    ProtoUnit::enable();

    XLOG_INFO("CLI enabled");
}

/**
 * Disable the node operation.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
Mld6igmpNodeCli::disable()
{
    stop();
    ProtoUnit::disable();

    XLOG_INFO("CLI disabled");
}

int
Mld6igmpNodeCli::add_all_cli_commands()
{
    // XXX: command "show" must have been installed by the CLI itself.
    
    if (mld6igmp_node().proto_is_igmp()) {
	add_cli_dir_command("show igmp", "Display information about IGMP");
	
	add_cli_command("show igmp group",
			"Display information about IGMP group membership",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_group));
	add_cli_command("show igmp interface",
			"Display information about IGMP interfaces",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_interface));
	add_cli_command("show igmp interface address",
			"Display information about addresses of IGMP interfaces",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_interface_address));
    }

    if (mld6igmp_node().proto_is_mld6()) {
	add_cli_dir_command("show mld", "Display information about MLD");
	
	add_cli_command("show mld group",
			"Display information about MLD group membership",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_group));
	add_cli_command("show mld interface",
			"Display information about MLD interfaces",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_interface));
	add_cli_command("show mld interface address",
			"Display information about addresses of MLD interfaces",
			callback(this, &Mld6igmpNodeCli::cli_show_mld6igmp_interface_address));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mld interface [interface-name]"
// CLI COMMAND: "show igmp interface [interface-name]"
//
// Display information about the interfaces on which MLD/IGMP is configured.
//
int
Mld6igmpNodeCli::cli_show_mld6igmp_interface(const vector<string>& argv)
{
    string interface_name;
    
    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (mld6igmp_node().vif_find_by_name(interface_name.c_str()) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %-8s %-15s %7s %7s %6s\n",
		       "Interface", "State", "Querier",
		       "Timeout", "Version", "Groups"));
    for (uint32_t i = 0; i < mld6igmp_node().maxvifs(); i++) {
	Mld6igmpVif *mld6igmp_vif = mld6igmp_node().vif_find_by_vif_index(i);
	if (mld6igmp_vif == NULL)
	    continue;
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (mld6igmp_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	string querier_timeout_sec_string;
	if (mld6igmp_vif->const_other_querier_timer().scheduled()) {
	    TimeVal tv;
	    mld6igmp_vif->const_other_querier_timer().time_remaining(tv);
	    querier_timeout_sec_string = c_format("%d",
						  XORP_INT_CAST(tv.sec()));
	} else {
	    querier_timeout_sec_string = "None";
	}
	
	cli_print(c_format("%-12s %-8s %-15s %7s %7d %6u\n",
			   mld6igmp_vif->name().c_str(),
			   mld6igmp_vif->state_str().c_str(),
			   cstring(mld6igmp_vif->querier_addr()),
			   querier_timeout_sec_string.c_str(),
			   mld6igmp_vif->proto_version(),
			   XORP_UINT_CAST(mld6igmp_vif->group_records().size())));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mld interface address [interface-name]"
// CLI COMMAND: "show igmp interface address [interface-name]"
//
// Display information about the addresses of MLD/IGMP interfaces
//
int
Mld6igmpNodeCli::cli_show_mld6igmp_interface_address(const vector<string>& argv)
{
    string interface_name;

    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (mld6igmp_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %-15s %-15s\n",
		       "Interface", "PrimaryAddr", "SecondaryAddr"));
    for (uint32_t i = 0; i < mld6igmp_node().maxvifs(); i++) {
	Mld6igmpVif *mld6igmp_vif = mld6igmp_node().vif_find_by_vif_index(i);
	if (mld6igmp_vif == NULL)
	    continue;
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (mld6igmp_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	
	//
	// Create a list with all secondary addresses
	//
	list<IPvX> secondary_addr_list;
	list<VifAddr>::const_iterator vif_addr_iter;
	for (vif_addr_iter = mld6igmp_vif->addr_list().begin();
	     vif_addr_iter != mld6igmp_vif->addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    if (vif_addr.addr() == mld6igmp_vif->primary_addr())
		continue;
	    secondary_addr_list.push_back(vif_addr.addr());
	}
	cli_print(c_format("%-12s %-15s %-15s\n",
			   mld6igmp_vif->name().c_str(),
			   cstring(mld6igmp_vif->primary_addr()),
			   (secondary_addr_list.size())?
			   cstring(secondary_addr_list.front()): ""));
	// Pop the first secondary address
	if (secondary_addr_list.size())
	    secondary_addr_list.pop_front();

	//
	// Print the rest of the secondary addresses
	//
	list<IPvX>::iterator secondary_addr_iter;
	for (secondary_addr_iter = secondary_addr_list.begin();
	     secondary_addr_iter != secondary_addr_list.end();
	     ++secondary_addr_iter) {
	    IPvX& secondary_addr = *secondary_addr_iter;
	    cli_print(c_format("%-12s %-15s %-15s\n",
			       " ",
			       " ",
			       cstring(secondary_addr)));
	}
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mld group [group-name [...]]"
// CLI COMMAND: "show igmp group [group-name [...]]"
//
// Display information about MLD/IGMP group membership.
//
int
Mld6igmpNodeCli::cli_show_mld6igmp_group(const vector<string>& argv)
{
    vector<IPvX> groups;
    
    // Check the (optional) arguments, and create an array of groups to test
    for (size_t i = 0; i < argv.size(); i++) {
	try {
	    IPvX g(argv[i].c_str());
	    if (g.af() != family()) {
		cli_print(c_format("ERROR: Address with invalid address family: %s\n",
				   argv[i].c_str()));
		return (XORP_ERROR);
	    }
	    if (! g.is_multicast()) {
		cli_print(c_format("ERROR: Not a multicast address: %s\n",
				   argv[i].c_str()));
		return (XORP_ERROR);
	    }
	    groups.push_back(g);
	} catch (InvalidString&) {
	    cli_print(c_format("ERROR: Invalid IP address: %s\n",
			       argv[i].c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %-15s %-15s %-12s %7s %1s %5s\n",
		       "Interface", "Group", "Source",
		       "LastReported", "Timeout", "V", "State"));
    for (uint32_t i = 0; i < mld6igmp_node().maxvifs(); i++) {
	const Mld6igmpVif *mld6igmp_vif = mld6igmp_node().vif_find_by_vif_index(i);
	if (mld6igmp_vif == NULL)
	    continue;
	Mld6igmpGroupSet::const_iterator group_iter;
	for (group_iter = mld6igmp_vif->group_records().begin();
	     group_iter != mld6igmp_vif->group_records().end();
	     ++group_iter) {
	    const Mld6igmpGroupRecord *group_record = group_iter->second;
	    Mld6igmpSourceSet::const_iterator source_iter;
	    int version = 0;
	    string state;

	    // Test if we should print this entry
	    bool do_print = true;
	    if (groups.size()) {
		do_print = false;
		for (size_t j = 0; j < groups.size(); j++) {
		    if (groups[j] == group_record->group()) {
			do_print = true;
			break;
		    }
		}
	    }
	    if (! do_print)
		continue;

	    // Calcuate the group entry version
	    do {
		version = 0;
		if (mld6igmp_vif->is_igmpv1_mode(group_record)) {
		    version = 1;
		    break;
		}
		if (mld6igmp_vif->is_igmpv2_mode(group_record)) {
		    version = 2;
		    break;
		}
		if (mld6igmp_vif->is_igmpv3_mode(group_record)) {
		    version = 3;
		    break;
		}
		if (mld6igmp_vif->is_mldv1_mode(group_record)) {
		    version = 1;
		    break;
		}
		if (mld6igmp_vif->is_mldv2_mode(group_record)) {
		    version = 2;
		    break;
		}
		break;
	    } while (false);
	    XLOG_ASSERT(version > 0);

	    //
	    // The state:
	    // - "I" = INCLUDE (for group entry)
	    // - "E" = EXCLUDE (for group entry)
	    // - "F" = Forward (for source entry)
	    // - "D" = Don't forward (for source entry)
	    //

	    // The group state
	    if (group_record->is_include_mode())
		state = "I";
	    if (group_record->is_exclude_mode())
		state = "E";

	    // Print the group-specific output
	    cli_print(c_format("%-12s %-15s %-15s %-12s %7d %1d %5s\n",
			       mld6igmp_vif->name().c_str(),
			       cstring(group_record->group()),
			       cstring(IPvX::ZERO(family())),
			       cstring(group_record->last_reported_host()),
			       XORP_INT_CAST(group_record->timeout_sec()),
			       version, state.c_str()));

	    // Print the sources to forward
	    state = "F";
	    for (source_iter = group_record->do_forward_sources().begin();
		 source_iter != group_record->do_forward_sources().end();
		 ++source_iter) {
		const Mld6igmpSourceRecord *source_record = source_iter->second;
		cli_print(c_format("%-12s %-15s %-15s %-12s %7d %1d %5s\n",
				   mld6igmp_vif->name().c_str(),
				   cstring(group_record->group()),
				   cstring(source_record->source()),
				   cstring(group_record->last_reported_host()),
				   XORP_INT_CAST(source_record->timeout_sec()),
				   version, state.c_str()));
	    }

	    // Print the sources not to forward
	    state = "D";
	    for (source_iter = group_record->dont_forward_sources().begin();
		 source_iter != group_record->dont_forward_sources().end();
		 ++source_iter) {
		const Mld6igmpSourceRecord *source_record = source_iter->second;
		cli_print(c_format("%-12s %-15s %-15s %-12s %7d %1d %5s\n",
				   mld6igmp_vif->name().c_str(),
				   cstring(group_record->group()),
				   cstring(source_record->source()),
				   cstring(group_record->last_reported_host()),
				   XORP_INT_CAST(source_record->timeout_sec()),
				   version, state.c_str()));
	    }
	}
    }
    
    return (XORP_OK);
}
