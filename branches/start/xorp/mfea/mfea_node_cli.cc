// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/mfea/mfea_node_cli.cc,v 1.25 2002/12/09 18:29:17 hodson Exp $"


//
// MFEA (Multicast Forwarding Engine Abstraction) CLI implementation
//

#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_node.hh"
#include "mfea_node_cli.hh"
#include "mfea_vif.hh"


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
 * MfeaNodeCli::MfeaNodeCli:
 * @mfea_node: The MFEA node to use.
 * 
 * MfeaNodeCli constructor.
 **/
MfeaNodeCli::MfeaNodeCli(MfeaNode& mfea_node)
    : ProtoNodeCli(mfea_node.family(), mfea_node.module_id()),
      _mfea_node(mfea_node)
{
    
}

MfeaNodeCli::~MfeaNodeCli()
{
    stop();
}

int
MfeaNodeCli::start()
{
    if (add_all_cli_commands() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
MfeaNodeCli::stop()
{
    int ret_code = XORP_OK;
    
    if (delete_all_cli_commands() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
MfeaNodeCli::add_all_cli_commands()
{
    // XXX: command "show" must have been installed by the CLI itself.

    add_cli_dir_command("show mfea",	"Display information about MFEA");
    
    add_cli_command("show mfea interface",
		    "Display information about MFEA interfaces",
		    callback(this, &MfeaNodeCli::cli_show_mfea_interface));
    
    add_cli_command("show mfea interface address",
		    "Display information about addresses of MFEA interfaces",
		    callback(this, &MfeaNodeCli::cli_show_mfea_interface_address));
    
    add_cli_command("show mfea mrib",
		    "Display MRIB information inside MFEA",
		    callback(this, &MfeaNodeCli::cli_show_mfea_mrib));
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mfea interface [interface-name]"
//
// Display information about the interfaces the MFEA knows about.
//
int
MfeaNodeCli::cli_show_mfea_interface(const vector<string>& argv)
{
    string interface_name;
    
    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (mfea_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-16s%-9s%14s %-16s%-1s\n",
		       "Interface", "State", "Vif/PifIndex", "Addr", "Flags"));
    for (size_t i = 0; i < mfea_node().maxvifs(); i++) {
	MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(i);
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (mfea_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	
	//
	// Create a string with the interface flags
	//
	string vif_flags = "";
	if (mfea_vif->is_pim_register()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "PIM_REGISTER";
	}
	if (mfea_vif->is_p2p()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "P2P";
	}
	if (mfea_vif->is_loopback()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "LOOPBACK";
	}
	if (mfea_vif->is_multicast_capable()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "MULTICAST";
	}
	if (mfea_vif->is_broadcast_capable()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "BROADCAST";
	}
	if (mfea_vif->is_underlying_vif_up()) {
	    if (vif_flags.size())
		vif_flags += " ";
	    vif_flags += "KERN_UP";
	}
	
	//
	// Print the interface
	//
	list<VifAddr>::const_iterator iter = mfea_vif->addr_list().begin();
	string dd = c_format("%d/%d", mfea_vif->vif_index(), mfea_vif->pif_index());
	cli_print(c_format("%-16s%-9s%14s %-16s%-1s\n",
			   mfea_vif->name().c_str(),
			   mfea_vif->state_string(),
			   dd.c_str(),
			   (iter != mfea_vif->addr_list().end())?
			   cstring((*iter).addr()): "",
			   vif_flags.c_str()));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mfea interface address [interface-name]"
//
// Display information about the addresses of interfaces the MFEA knows about.
//
int
MfeaNodeCli::cli_show_mfea_interface_address(const vector<string>& argv)
{
    string interface_name;
    
    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (mfea_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    // XXX: the first field width should be 16 (for consistency with other
    // "show foo interface" commands, but this makes the line too long
    // to fit into 80 width-terminal.
    cli_print(c_format("%-14s%-16s%-19s%-16s%-15s\n",
		       "Interface", "Addr", "Subnet", "Broadcast", "P2Paddr"));
    for (size_t i = 0; i < mfea_node().maxvifs(); i++) {
	MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(i);
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (mfea_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	
	//
	// Print the first address
	//
	list<VifAddr>::const_iterator iter = mfea_vif->addr_list().begin();
	cli_print(c_format("%-14s%-16s%-19s%-16s%-15s\n",
			   mfea_vif->name().c_str(),
			   (iter != mfea_vif->addr_list().end())?
			   cstring((*iter).addr()): "",
			   (iter != mfea_vif->addr_list().end())?
			   cstring((*iter).subnet_addr()): "",
			   (iter != mfea_vif->addr_list().end())?
			   cstring((*iter).broadcast_addr()): "",
			   (iter != mfea_vif->addr_list().end())?
			   cstring((*iter).peer_addr()): ""));
	//
	// Print the rest of the addresses
	//
	if (iter != mfea_vif->addr_list().end())
	    ++iter;
	for ( ; iter != mfea_vif->addr_list().end(); ++iter) {
	    cli_print(c_format("%-14s%-16s%-19s%-16s%-15s\n",
			       " ",
			       cstring((*iter).addr()),
			       cstring((*iter).subnet_addr()),
			       cstring((*iter).broadcast_addr()),
			       cstring((*iter).peer_addr())));
	}
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show mfea mrib [dest-address]"
//
// Display MRIB information inside MFEA.
//
int
MfeaNodeCli::cli_show_mfea_mrib(const vector<string>& argv)
{
    string dest_address_name;
    IPvX dest_address(family());
    
    // Check the optional argument
    if (argv.size()) {
	dest_address_name = argv[0];
	try {
	    dest_address = IPvX(dest_address_name.c_str());
	} catch (InvalidString) {
	    cli_print(c_format("ERROR: Invalid destination address: %s\n",
			       dest_address_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    // Test if we should print a single entry only
    if (dest_address_name.size()) {
	Mrib *mrib = mfea_node().mrib_table().find(dest_address);
	if (mrib == NULL) {
	    cli_print(c_format("No matching MRIB entry for %s\n",
			       dest_address_name.c_str()));
	    return (XORP_ERROR);
	}
	cli_print(c_format("%-19s%-16s%-8s%-9s%8s%17s\n",
			   "DestPrefix", "NextHopRouter", "VifName", 
			   "VifIndex", "Metric", "MetricPreference"));
	string vif_name = "UNKNOWN";
	Vif *vif = mfea_node().vif_find_by_vif_index(mrib->next_hop_vif_index());
	if (vif != NULL)
	    vif_name = vif->name();
	cli_print(c_format("%-19s%-16s%-8s%-9d%8d%17d\n",
			   cstring(mrib->dest_prefix()),
			   cstring(mrib->next_hop_router_addr()),
			   vif_name.c_str(),
			   mrib->next_hop_vif_index(),
			   mrib->metric(),
			   mrib->metric_preference()));
	return (XORP_OK);
    }
    
    cli_print(c_format("%-19s%-16s%-8s%-9s%8s%17s\n",
		       "DestPrefix", "NextHopRouter", "VifName", "VifIndex",
		       "Metric", "MetricPreference"));
    MribTable::iterator iter;
    for (iter = mfea_node().mrib_table().begin();
	 iter != mfea_node().mrib_table().end();
	 ++iter) {
	Mrib *mrib = *iter;
	if (mrib == NULL)
	    continue;
	
	string vif_name = "UNKNOWN";
	Vif *vif = mfea_node().vif_find_by_vif_index(mrib->next_hop_vif_index());
	if (vif != NULL)
	    vif_name = vif->name();
	cli_print(c_format("%-19s%-16s%-8s%-9d%8d%17d\n",
			   cstring(mrib->dest_prefix()),
			   cstring(mrib->next_hop_router_addr()),
			   vif_name.c_str(),
			   mrib->next_hop_vif_index(),
			   mrib->metric(),
			   mrib->metric_preference()));
    }
    
    return (XORP_OK);
}
