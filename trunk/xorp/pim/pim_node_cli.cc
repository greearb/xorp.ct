// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_node_cli.cc,v 1.28 2004/06/10 22:41:32 hodson Exp $"


//
// PIM protocol CLI implementation
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mfc.hh"
#include "pim_mre.hh"
#include "pim_node.hh"
#include "pim_node_cli.hh"
#include "pim_vif.hh"


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
 * PimNodeCli::PimNodeCli:
 * @pim_node: The PIM node to use.
 * 
 * PimNodeCli constructor.
 **/
PimNodeCli::PimNodeCli(PimNode& pim_node)
    : ProtoNodeCli(pim_node.family(), pim_node.module_id()),
      _pim_node(pim_node)
{
    
}

PimNodeCli::~PimNodeCli()
{
    stop();
}

int
PimNodeCli::start()
{
    if (is_up() || is_pending_up())
	return (XORP_OK);

    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);

    if (add_all_cli_commands() < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimNodeCli::stop()
{
    int ret_code = XORP_OK;

    if (is_down())
	return (XORP_OK);

    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    if (delete_all_cli_commands() < 0)
	ret_code = XORP_ERROR;
    
    return (ret_code);
}

int
PimNodeCli::add_all_cli_commands()
{
    // XXX: command "show" must have been installed by the CLI itself.
    
    if (pim_node().is_ipv4()) {
	add_cli_dir_command("show pim",
			    "Display information about IPv4 PIM");

	add_cli_command("show pim bootstrap",
			"Display information about PIM IPv4 bootstrap routers",
			callback(this, &PimNodeCli::cli_show_pim_bootstrap));

	add_cli_command("show pim bootstrap rps",
			"Display information about PIM IPv4 bootstrap RPs",
			callback(this, &PimNodeCli::cli_show_pim_bootstrap_rps));

	add_cli_command("show pim interface",
			"Display information about PIM IPv4 interfaces",
			callback(this, &PimNodeCli::cli_show_pim_interface));

	add_cli_command("show pim interface address",
			"Display information about addresses of PIM IPv4 interfaces",
			callback(this, &PimNodeCli::cli_show_pim_interface_address));

	add_cli_command("show pim join",
			"Display information about PIM IPv4 groups",
			callback(this, &PimNodeCli::cli_show_pim_join));

	add_cli_command("show pim join all",
			"Display information about all PIM IPv4 groups",
			callback(this, &PimNodeCli::cli_show_pim_join_all));

	add_cli_command("show pim mfc",
			"Display information about PIM Multicast Forwarding Cache",
			callback(this, &PimNodeCli::cli_show_pim_mfc));

	add_cli_command("show pim neighbors",
			"Display information about PIM IPv4 neighbors",
			callback(this, &PimNodeCli::cli_show_pim_neighbors));

	add_cli_command("show pim mrib",
			"Display MRIB IPv4 information inside PIM",
			callback(this, &PimNodeCli::cli_show_pim_mrib));

	add_cli_command("show pim rps",
			"Display information about PIM IPv4 RPs",
			callback(this, &PimNodeCli::cli_show_pim_rps));

	add_cli_command("show pim scope",
			"Display information about PIM IPv4 scope zones",
			callback(this, &PimNodeCli::cli_show_pim_scope));
    }

    if (pim_node().is_ipv6()) {
	add_cli_dir_command("show pim6",
			    "Display information about IPv6 PIM");

	add_cli_command("show pim6 bootstrap",
			"Display information about PIM IPv6 bootstrap routers",
			callback(this, &PimNodeCli::cli_show_pim_bootstrap));

	add_cli_command("show pim6 bootstrap rps",
			"Display information about PIM IPv6 bootstrap RPs",
			callback(this, &PimNodeCli::cli_show_pim_bootstrap_rps));

	add_cli_command("show pim6 interface",
			"Display information about PIM IPv6 interfaces",
			callback(this, &PimNodeCli::cli_show_pim_interface));

	add_cli_command("show pim6 interface address",
			"Display information about addresses of PIM IPv6 interfaces",
			callback(this, &PimNodeCli::cli_show_pim_interface_address));

	add_cli_command("show pim6 join",
			"Display information about PIM IPv6 groups",
			callback(this, &PimNodeCli::cli_show_pim_join));

	add_cli_command("show pim6 join all",
			"Display information about all PIM IPv6 groups",
			callback(this, &PimNodeCli::cli_show_pim_join_all));

	add_cli_command("show pim6 mfc",
			"Display information about PIM Multicast Forwarding Cache",
			callback(this, &PimNodeCli::cli_show_pim_mfc));

	add_cli_command("show pim6 neighbors",
			"Display information about PIM IPv6 neighbors",
			callback(this, &PimNodeCli::cli_show_pim_neighbors));

	add_cli_command("show pim6 mrib",
			"Display MRIB IPv6 information inside PIM",
			callback(this, &PimNodeCli::cli_show_pim_mrib));

	add_cli_command("show pim6 rps",
			"Display information about PIM IPv6 RPs",
			callback(this, &PimNodeCli::cli_show_pim_rps));

	add_cli_command("show pim6 scope",
			"Display information about PIM IPv6 scope zones",
			callback(this, &PimNodeCli::cli_show_pim_scope));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim bootstrap [scope-zone-group-prefix [scoped]]"
// CLI COMMAND: "show pim6 bootstrap [scope-zone-group-prefix [scoped]]"
//
// Display information about PIM bootstrap routers
//
int
PimNodeCli::cli_show_pim_bootstrap(const vector<string>& argv)
{
    PimScopeZoneId zone_id(IPvXNet::ip_multicast_base_prefix(family()), false);
    bool is_zone_id_set = false;
    list<BsrZone *>::const_iterator zone_iter;
    
    // Check the optional arguments
    if (argv.size()) {
	try {
	    zone_id = PimScopeZoneId(argv[0].c_str(), false);
	    is_zone_id_set = true;
	    if (zone_id.scope_zone_prefix().masked_addr().af() != family()) {
		cli_print(c_format("ERROR: Address with invalid address family: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	    // Test if the second argument specifies scoped zone
	    if (argv.size() >= 2) {
		if (argv[1] == "scoped")
		    zone_id = PimScopeZoneId(argv[0].c_str(), true);
	    }
	} catch (InvalidString) {
	    cli_print(c_format("ERROR: Invalid zone ID: %s\n",
			       argv[0].c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print("Active zones:\n");
    cli_print(c_format("%-15s %3s %-15s %3s %-15s %7s %9s\n",
		       "BSR", "Pri", "LocalAddress", "Pri", "State", "Timeout",
		       "SZTimeout"));
    for (zone_iter = pim_node().pim_bsr().active_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().active_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	string zone_state_string = "Unknown";
	switch (bsr_zone->bsr_zone_state()) {
	case BsrZone::STATE_INIT:
	    zone_state_string = "Init";
	    break;
	case BsrZone::STATE_CANDIDATE_BSR:
	    zone_state_string = "Candidate";
	    break;
	case BsrZone::STATE_PENDING_BSR:
	    zone_state_string = "Pending";
	    break;
	case BsrZone::STATE_ELECTED_BSR:
	    zone_state_string = "Elected";
	    break;
	case BsrZone::STATE_NO_INFO:
	    zone_state_string = "NoInfo";
	    break;
	case BsrZone::STATE_ACCEPT_ANY:
	    zone_state_string = "AcceptAny";
	    break;
	case BsrZone::STATE_ACCEPT_PREFERRED:
	    zone_state_string = "AcceptPreferred";
	    break;
	default:
	    zone_state_string = "InvalidState";
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	    break;
	}
	int scope_zone_left_sec = -1;
	if (bsr_zone->const_scope_zone_expiry_timer().scheduled()) {
	    TimeVal tv_left;
	    bsr_zone->const_scope_zone_expiry_timer().time_remaining(tv_left);
	    scope_zone_left_sec = tv_left.sec();
	}
	int bsr_zone_left_sec = -1;
	if (bsr_zone->const_bsr_timer().scheduled()) {
	    TimeVal tv_left;
	    bsr_zone->const_bsr_timer().time_remaining(tv_left);
	    bsr_zone_left_sec = tv_left.sec();
	}
	cli_print(c_format("%-15s %3d %-15s %3d %-15s %7d %9d\n",
			   cstring(bsr_zone->bsr_addr()),
			   bsr_zone->bsr_priority(),
			   cstring(bsr_zone->my_bsr_addr()),
			   bsr_zone->my_bsr_priority(),
			   zone_state_string.c_str(),
			   bsr_zone_left_sec,
			   scope_zone_left_sec));
    }
    
    cli_print("Expiring zones:\n");
    cli_print(c_format("%-15s %3s %-15s %3s %-15s %7s %9s\n",
		       "BSR", "Pri", "LocalAddress", "Pri", "State", "Timeout",
		       "SZTimeout"));
    for (zone_iter = pim_node().pim_bsr().expire_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().expire_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	string zone_state_string = "Unknown";
	switch (bsr_zone->bsr_zone_state()) {
	case BsrZone::STATE_INIT:
	    zone_state_string = "Init";
	    break;
	case BsrZone::STATE_CANDIDATE_BSR:
	    zone_state_string = "Candidate";
	    break;
	case BsrZone::STATE_PENDING_BSR:
	    zone_state_string = "Pending";
	    break;
	case BsrZone::STATE_ELECTED_BSR:
	    zone_state_string = "Elected";
	    break;
	case BsrZone::STATE_NO_INFO:
	    zone_state_string = "NoInfo";
	    break;
	case BsrZone::STATE_ACCEPT_ANY:
	    zone_state_string = "AcceptAny";
	    break;
	case BsrZone::STATE_ACCEPT_PREFERRED:
	    zone_state_string = "AcceptPreferred";
	    break;
	default:
	    zone_state_string = "InvalidState";
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	    break;
	}
	int scope_zone_left_sec = -1;
	if (bsr_zone->const_scope_zone_expiry_timer().scheduled()) {
	    TimeVal tv_left;
	    bsr_zone->const_scope_zone_expiry_timer().time_remaining(tv_left);
	    scope_zone_left_sec = tv_left.sec();
	}
	int bsr_zone_left_sec = -1;
	if (bsr_zone->const_bsr_timer().scheduled()) {
	    TimeVal tv_left;
	    bsr_zone->const_bsr_timer().time_remaining(tv_left);
	    bsr_zone_left_sec = tv_left.sec();
	}
	cli_print(c_format("%-15s %3d %-15s %3d %-15s %7d %9d\n",
			   cstring(bsr_zone->bsr_addr()),
			   bsr_zone->bsr_priority(),
			   cstring(bsr_zone->my_bsr_addr()),
			   bsr_zone->my_bsr_priority(),
			   zone_state_string.c_str(),
			   bsr_zone_left_sec,
			   scope_zone_left_sec));
    }
    
    cli_print("Configured zones:\n");
    cli_print(c_format("%-15s %3s %-15s %3s %-15s %7s %9s\n",
		       "BSR", "Pri", "LocalAddress", "Pri", "State", "Timeout",
		       "SZTimeout"));
    for (zone_iter = pim_node().pim_bsr().config_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().config_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	string zone_state_string = "Unknown";
	switch (bsr_zone->bsr_zone_state()) {
	case BsrZone::STATE_INIT:
	    zone_state_string = "Init";
	    break;
	case BsrZone::STATE_CANDIDATE_BSR:
	    zone_state_string = "Candidate";
	    break;
	case BsrZone::STATE_PENDING_BSR:
	    zone_state_string = "Pending";
	    break;
	case BsrZone::STATE_ELECTED_BSR:
	    zone_state_string = "Elected";
	    break;
	case BsrZone::STATE_NO_INFO:
	    zone_state_string = "NoInfo";
	    break;
	case BsrZone::STATE_ACCEPT_ANY:
	    zone_state_string = "AcceptAny";
	    break;
	case BsrZone::STATE_ACCEPT_PREFERRED:
	    zone_state_string = "AcceptPreferred";
	    break;
	default:
	    zone_state_string = "InvalidState";
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	    break;
	}
	cli_print(c_format("%-15s %3d %-15s %3d %-15s %7d %9d\n",
			   cstring(bsr_zone->bsr_addr()),
			   bsr_zone->bsr_priority(),
			   cstring(bsr_zone->my_bsr_addr()),
			   bsr_zone->my_bsr_priority(),
			   zone_state_string.c_str(),
			   -1,
			   -1));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim bootstrap rps [scope-zone-group-prefix [scoped]]"
// CLI COMMAND: "show pim6 bootstrap rps [scope-zone-group-prefix [scoped]]"
//
// Display information about PIM bootstrap RPs
//
int
PimNodeCli::cli_show_pim_bootstrap_rps(const vector<string>& argv)
{
    PimScopeZoneId zone_id(IPvXNet::ip_multicast_base_prefix(family()), false);
    bool is_zone_id_set = false;
    list<BsrZone *>::const_iterator zone_iter;
    
    // Check the optional argument
    if (argv.size()) {
	try {
	    zone_id = PimScopeZoneId(argv[0].c_str(), false);
	    is_zone_id_set = true;
	    if (zone_id.scope_zone_prefix().masked_addr().af() != family()) {
		cli_print(c_format("ERROR: Address with invalid address family: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	    // Test if the second argument specifies scoped zone
	    if (argv.size() >= 2) {
		if (argv[1] == "scoped")
		    zone_id = PimScopeZoneId(argv[0].c_str(), true);
	    }
	} catch (InvalidString) {
	    cli_print(c_format("ERROR: Invalid zone ID: %s\n",
			       argv[0].c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print("Active RPs:\n");
    cli_print(c_format("%-15s %3s %7s %-18s %-15s %16s\n",
		       "RP", "Pri", "Timeout", "GroupPrefix", "BSR",
		       "CandRpAdvTimeout"));
    for (zone_iter = pim_node().pim_bsr().active_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().active_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
	for (group_prefix_iter = bsr_zone->bsr_group_prefix_list().begin();
	     group_prefix_iter != bsr_zone->bsr_group_prefix_list().end();
	     ++group_prefix_iter) {
	    BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	    
	    list<BsrRp *>::const_iterator rp_iter;
	    for (rp_iter = bsr_group_prefix->rp_list().begin();
		 rp_iter != bsr_group_prefix->rp_list().end();
		 ++rp_iter) {
		BsrRp *bsr_rp = *rp_iter;
		int left_sec = -1;
		if (bsr_rp->const_candidate_rp_expiry_timer().scheduled()) {
		    TimeVal tv_left;
		    bsr_rp->const_candidate_rp_expiry_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
		cli_print(c_format("%-15s %3d %7d %-18s %-15s %16d\n",
				   cstring(bsr_rp->rp_addr()),
				   bsr_rp->rp_priority(),
				   left_sec,
				   cstring(bsr_group_prefix->group_prefix()),
				   cstring(bsr_zone->bsr_addr()),
				   -1));
	    }
	}
    }

    cli_print("Expiring RPs:\n");
    cli_print(c_format("%-15s %3s %7s %-18s %-15s %16s\n",
		       "RP", "Pri", "Timeout", "GroupPrefix", "BSR",
		       "CandRpAdvTimeout"));
    for (zone_iter = pim_node().pim_bsr().expire_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().expire_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
	for (group_prefix_iter = bsr_zone->bsr_group_prefix_list().begin();
	     group_prefix_iter != bsr_zone->bsr_group_prefix_list().end();
	     ++group_prefix_iter) {
	    BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	    
	    list<BsrRp *>::const_iterator rp_iter;
	    for (rp_iter = bsr_group_prefix->rp_list().begin();
		 rp_iter != bsr_group_prefix->rp_list().end();
		 ++rp_iter) {
		BsrRp *bsr_rp = *rp_iter;
		int left_sec = -1;
		if (bsr_rp->const_candidate_rp_expiry_timer().scheduled()) {
		    TimeVal tv_left;
		    bsr_rp->const_candidate_rp_expiry_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
		cli_print(c_format("%-15s %3d %7d %-18s %-15s %16d\n",
				   cstring(bsr_rp->rp_addr()),
				   bsr_rp->rp_priority(),
				   left_sec,
				   cstring(bsr_group_prefix->group_prefix()),
				   cstring(bsr_zone->bsr_addr()),
				   -1));
	    }
	}
    }
    
    cli_print("Configured RPs:\n");
    cli_print(c_format("%-15s %3s %7s %-18s %-15s %16s\n",
		       "RP", "Pri", "Timeout", "GroupPrefix", "BSR",
		       "CandRpAdvTimeout"));
    for (zone_iter = pim_node().pim_bsr().config_bsr_zone_list().begin();
	 zone_iter != pim_node().pim_bsr().config_bsr_zone_list().end();
	 ++zone_iter) {
	BsrZone *bsr_zone = *zone_iter;
	
	if (is_zone_id_set && (bsr_zone->zone_id() != zone_id))
	    continue;
	
	list<BsrGroupPrefix *>::const_iterator group_prefix_iter;
	for (group_prefix_iter = bsr_zone->bsr_group_prefix_list().begin();
	     group_prefix_iter != bsr_zone->bsr_group_prefix_list().end();
	     ++group_prefix_iter) {
	    BsrGroupPrefix *bsr_group_prefix = *group_prefix_iter;
	    
	    list<BsrRp *>::const_iterator rp_iter;
	    for (rp_iter = bsr_group_prefix->rp_list().begin();
		 rp_iter != bsr_group_prefix->rp_list().end();
		 ++rp_iter) {
		BsrRp *bsr_rp = *rp_iter;
		int left_sec = -1;
		if (pim_node().is_my_addr(bsr_rp->rp_addr())
		    && (bsr_zone->const_candidate_rp_advertise_timer().scheduled())) {
		    TimeVal tv_left;
		    bsr_zone->const_candidate_rp_advertise_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
		
		cli_print(c_format("%-15s %3d %7d %-18s %-15s %16d\n",
				   cstring(bsr_rp->rp_addr()),
				   bsr_rp->rp_priority(),
				   -1,
				   cstring(bsr_group_prefix->group_prefix()),
				   cstring(bsr_zone->bsr_addr()),
				   left_sec));
	    }
	}
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim interface [interface-name]"
// CLI COMMAND: "show pim6 interface [interface-name]"
//
// Display information about the interfaces on which PIM is configured.
//
int
PimNodeCli::cli_show_pim_interface(const vector<string>& argv)
{
    string interface_name;

    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (pim_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %-8s %-6s %1s %-8s %8s %-15s %9s\n",
		       "Interface", "State", "Mode", "V", "PIMstate",
		       "Priority", "DRaddr", "Neighbors"));
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (pim_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	cli_print(c_format("%-12s %-8s %-6s %1d %-8s %8d %-15s %9d\n",
			   pim_vif->name().c_str(),
			   pim_vif->state_str().c_str(),
			   pim_vif->proto_is_pimsm()? "Sparse" : "Dense",
			   pim_vif->proto_version(),
			   // TODO: should we print "only P2P" if P2P link?
			   // pim_vif->is_p2p()? "P2P" : pim_vif->i_am_dr()? "DR" : "NotDR",
			   pim_vif->i_am_dr()? "DR" : "NotDR",
			   pim_vif->dr_priority().get(),
			   cstring(pim_vif->dr_addr()),
			   pim_vif->pim_nbrs_number()));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim interface address [interface-name]"
// CLI COMMAND: "show pim6 interface address [interface-name]"
//
// Display information about the addresses of PIM interfaces
//
int
PimNodeCli::cli_show_pim_interface_address(const vector<string>& argv)
{
    string interface_name;

    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (pim_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %-15s %-15s %-15s\n",
		       "Interface", "PrimaryAddr", "DomainWideAddr", "SecondaryAddr"));
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (pim_vif->name() == interface_name) {
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
	for (vif_addr_iter = pim_vif->addr_list().begin();
	     vif_addr_iter != pim_vif->addr_list().end();
	     ++vif_addr_iter) {
	    const VifAddr& vif_addr = *vif_addr_iter;
	    if (vif_addr.addr() == pim_vif->primary_addr())
		continue;
	    if (vif_addr.addr() == pim_vif->domain_wide_addr())
		continue;
	    secondary_addr_list.push_back(vif_addr.addr());
	}
	cli_print(c_format("%-12s %-15s %-15s %-15s\n",
			   pim_vif->name().c_str(),
			   cstring(pim_vif->primary_addr()),
			   cstring(pim_vif->domain_wide_addr()),
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
	    cli_print(c_format("%-12s %-15s %-15s %-15s\n",
			       " ",
			       " ",
			       " ",
			       cstring(secondary_addr)));
	}
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim join [group | group-range]"
// CLI COMMAND: "show pim6 join [group | group-range]"
//
// Display information about PIM groups.
//
int
PimNodeCli::cli_show_pim_join(const vector<string>& argv)
{
    IPvXNet group_range = IPvXNet::ip_multicast_base_prefix(family());
    
    // Check the optional argument
    if (argv.size()) {
	try {
	    group_range = IPvXNet(argv[0].c_str());
	} catch (InvalidString) {
	    try {
		group_range = IPvXNet(IPvX(argv[0].c_str()),
				      IPvX::addr_bitlen(family()));
	    } catch (InvalidString) {
		cli_print(c_format("ERROR: Invalid group range: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	}
	if (! group_range.is_multicast()) {
	    cli_print(c_format("ERROR: Group range is not multicast: %s\n",
			       cstring(group_range)));
	    return (XORP_ERROR);
	}
    }
    
    cli_print_pim_mre_entries(group_range, false);
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim join all [group | group-range]"
// CLI COMMAND: "show pim6 join all [group | group-range]"
//
// Display information about all PIM groups.
//
int
PimNodeCli::cli_show_pim_join_all(const vector<string>& argv)
{
    IPvXNet group_range = IPvXNet::ip_multicast_base_prefix(family());
    
    // Check the optional argument
    if (argv.size()) {
	try {
	    group_range = IPvXNet(argv[0].c_str());
	} catch (InvalidString) {
	    try {
		group_range = IPvXNet(IPvX(argv[0].c_str()),
				      IPvX::addr_bitlen(family()));
	    } catch (InvalidString) {
		cli_print(c_format("ERROR: Invalid group range: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	}
	if (! group_range.is_multicast()) {
	    cli_print(c_format("ERROR: Group range is not multicast: %s\n",
			       cstring(group_range)));
	    return (XORP_ERROR);
	}
    }
    
    cli_print_pim_mre_entries(group_range, true);
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim mfc [group | group-range]"
// CLI COMMAND: "show pim6 mfc [group | group-range]"
//
// Display information about PIM Multicast Forwarding Cache.
//
int
PimNodeCli::cli_show_pim_mfc(const vector<string>& argv)
{
    IPvXNet group_range = IPvXNet::ip_multicast_base_prefix(family());
    
    // Check the optional argument
    if (argv.size()) {
	try {
	    group_range = IPvXNet(argv[0].c_str());
	} catch (InvalidString) {
	    try {
		group_range = IPvXNet(IPvX(argv[0].c_str()),
				      IPvX::addr_bitlen(family()));
	    } catch (InvalidString) {
		cli_print(c_format("ERROR: Invalid group range: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	}
	if (! group_range.is_multicast()) {
	    cli_print(c_format("ERROR: Group range is not multicast: %s\n",
			       cstring(group_range)));
	    return (XORP_ERROR);
	}
    }
    
    cli_print_pim_mfc_entries(group_range);
    
    return (XORP_OK);
}

//
// Print the CLI output for the #PimMre entries.
//
void
PimNodeCli::cli_print_pim_mre_entries(const IPvXNet& group_range,
				      bool is_print_all)
{
    //
    // TODO: XXX: PAVPAVPAV: the printing below is very incomplete.
    //
    cli_print(c_format("%-15s %-15s %-15s %-5s\n",
		       "Group", "Source", "RP", "Flags"));
    
    //
    // The (*,*,RP) entries
    //
    PimMrtRp::const_sg_iterator iter_rp, iter_begin_rp, iter_end_rp;
    iter_begin_rp = pim_node().pim_mrt().pim_mrt_rp().sg_begin();
    iter_end_rp = pim_node().pim_mrt().pim_mrt_rp().sg_end();
    
    for (iter_rp = iter_begin_rp; iter_rp != iter_end_rp; ++iter_rp) {
	PimMre *pim_mre_rp = iter_rp->second;
	if (is_print_all || (! pim_mre_rp->is_not_joined_state()))
	    cli_print_pim_mre(pim_mre_rp);
    }
    
    //
    // The (*,G) entries
    //
    PimMrtG::const_gs_iterator iter_g, iter_begin_g, iter_end_g;
    iter_begin_g = pim_node().pim_mrt().pim_mrt_g().group_by_prefix_begin(group_range);
    iter_end_g = pim_node().pim_mrt().pim_mrt_g().group_by_prefix_end(group_range);
    
    for (iter_g = iter_begin_g; iter_g != iter_end_g; ++iter_g) {
	PimMre *pim_mre_g = iter_g->second;
	cli_print_pim_mre(pim_mre_g);
    }
    
    PimMrtSg::const_gs_iterator iter_s, iter_begin_s, iter_end_s;
    //
    // The (S,G,rpt) entries
    //
    iter_begin_s = pim_node().pim_mrt().pim_mrt_sg_rpt().group_by_prefix_begin(group_range);
    iter_end_s = pim_node().pim_mrt().pim_mrt_sg_rpt().group_by_prefix_end(group_range);
    for (iter_s = iter_begin_s; iter_s != iter_end_s; ++iter_s) {
	PimMre *pim_mre_s = iter_s->second;
	cli_print_pim_mre(pim_mre_s);
    }
    
    // The (S,G) entries
    iter_begin_s = pim_node().pim_mrt().pim_mrt_sg().group_by_prefix_begin(group_range);
    iter_end_s = pim_node().pim_mrt().pim_mrt_sg().group_by_prefix_end(group_range);
    for (iter_s = iter_begin_s; iter_s != iter_end_s; ++iter_s) {
	PimMre *pim_mre_s = iter_s->second;
	cli_print_pim_mre(pim_mre_s);
    }
}

//
// Print the CLI output for the #PimMfc entries.
//
void
PimNodeCli::cli_print_pim_mfc_entries(const IPvXNet& group_range)
{
    cli_print(c_format("%-15s %-15s %-15s\n",
		       "Group", "Source", "RP"));
    
    //
    // The PimMfc entries
    //
    PimMrtMfc::const_gs_iterator iter_mfc, iter_begin_mfc, iter_end_mfc;
    iter_begin_mfc = pim_node().pim_mrt().pim_mrt_mfc().group_by_prefix_begin(group_range);
    iter_end_mfc = pim_node().pim_mrt().pim_mrt_mfc().group_by_prefix_end(group_range);
    
    for (iter_mfc = iter_begin_mfc; iter_mfc != iter_end_mfc; ++iter_mfc) {
	PimMfc *pim_mfc = iter_mfc->second;
	cli_print_pim_mfc(pim_mfc);
    }
}

//
// Print the CLI output for a #PimMre entry.
//
void
PimNodeCli::cli_print_pim_mre(const PimMre *pim_mre)
{
    if (pim_mre == NULL)
	return;
    
    //
    // Compute the entry state flags
    //
    string entry_state_flags;
    if (pim_mre->is_sg())
	entry_state_flags += "SG ";
    if (pim_mre->is_sg_rpt())
	entry_state_flags += "SG_RPT ";
    if (pim_mre->is_wc())
	entry_state_flags += "WC ";
    if (pim_mre->is_rp())
	entry_state_flags += "RP ";
    if (pim_mre->is_spt())
	entry_state_flags += "SPT ";
    if (pim_mre->is_directly_connected_s())
	entry_state_flags += "DirectlyConnectedS ";
    
    //
    // Compute the upstream state
    //
    string upstream_state;
    if (pim_mre->is_rp() || pim_mre->is_wc() || pim_mre->is_sg()) {
	if (pim_mre->is_joined_state())
	    upstream_state += "Joined ";
	if (pim_mre->is_not_joined_state())
	    upstream_state += "NotJoined ";
    }
    if (pim_mre->is_sg_rpt()) {
	if (pim_mre->is_rpt_not_joined_state())
	    upstream_state += "RPTNotJoined ";
	if (pim_mre->is_pruned_state())
	    upstream_state += "Pruned ";
	if (pim_mre->is_not_pruned_state())
	    upstream_state += "NotPruned ";
    }
    
    //
    // Compute the Register state
    //
    string register_state;
    if (pim_mre->is_sg() && pim_mre->is_directly_connected_s()) {
	if (pim_mre->is_register_noinfo_state())
	    register_state += "RegisterNoinfo ";
	if (pim_mre->is_register_join_state())
	    register_state += "RegisterJoin ";
	if (pim_mre->is_register_prune_state())
	    register_state += "RegisterPrune ";
	if (pim_mre->is_register_join_pending_state())
	    register_state += "RegisterJoinPending ";
	if (pim_mre->is_could_register_sg())
	    register_state += "RegisterCouldRegister ";
	if (pim_mre->is_not_could_register_sg())
	    register_state += "RegisterNotCouldRegister ";
    }
    
    // Compute the RPF interface
    uint16_t vif_index_s, vif_index_rp;
    PimVif *iif_pim_vif_s = NULL;
    PimVif *iif_pim_vif_rp = NULL;
    
    vif_index_s = pim_mre->rpf_interface_s();
    vif_index_rp = pim_mre->rpf_interface_rp();
    iif_pim_vif_s = pim_node().vif_find_by_vif_index(vif_index_s);
    iif_pim_vif_rp = pim_node().vif_find_by_vif_index(vif_index_rp);
    
    cli_print(c_format("%-15s %-15s %-15s %-5s\n",
		       cstring(pim_mre->group_addr()),
		       cstring(pim_mre->source_addr()),
		       pim_mre->rp_addr_string().c_str(),
		       entry_state_flags.c_str()));
    if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Upstream interface (S):    %s\n",
			   (iif_pim_vif_s != NULL)?
			   iif_pim_vif_s->name().c_str() : "UNKNOWN"));
    }
    cli_print(c_format("    Upstream interface (RP):   %s\n",
		       (iif_pim_vif_rp != NULL)?
		       iif_pim_vif_rp->name().c_str() : "UNKNOWN"));
    cli_print(c_format("    Upstream MRIB next hop (RP): %s\n",
		       (pim_mre->nbr_mrib_next_hop_rp() != NULL)?
		       cstring(pim_mre->nbr_mrib_next_hop_rp()->primary_addr())
		       : "UNKNOWN"));
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Upstream MRIB next hop (S):  %s\n",
			   (pim_mre->nbr_mrib_next_hop_s() != NULL)?
			   cstring(pim_mre->nbr_mrib_next_hop_s()->primary_addr())
			   : "UNKNOWN"));
    }
    if (pim_mre->is_wc()) {
	cli_print(c_format("    Upstream RPF'(*,G):        %s\n",
			   (pim_mre->rpfp_nbr_wc() != NULL)?
			   cstring(pim_mre->rpfp_nbr_wc()->primary_addr())
			   : "UNKNOWN"));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Upstream RPF'(S,G):        %s\n",
			   (pim_mre->rpfp_nbr_sg() != NULL)?
			   cstring(pim_mre->rpfp_nbr_sg()->primary_addr())
			   : "UNKNOWN"));
    }
    if (pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Upstream RPF'(S,G,rpt):    %s\n",
			   (pim_mre->rpfp_nbr_sg_rpt() != NULL)?
			   cstring(pim_mre->rpfp_nbr_sg_rpt()->primary_addr())
			   : "UNKNOWN"));
    }
    cli_print(c_format("    Upstream state:            %s\n",
		       upstream_state.c_str()));
    if (pim_mre->is_sg() && pim_mre->is_directly_connected_s()) {
	cli_print(c_format("    Register state:            %s\n",
			   register_state.c_str()));
    }
    if (pim_mre->is_sg_rpt()) {
	int left_sec = -1;
	if (pim_mre->const_override_timer().scheduled()) {
	    TimeVal tv_left;
	    pim_mre->const_override_timer().time_remaining(tv_left);
	    left_sec = tv_left.sec();
	}
	cli_print(c_format("    Override timer:            %d\n",
			   left_sec));
    } else {
	int left_sec = -1;
	if (pim_mre->const_join_timer().scheduled()) {
	    TimeVal tv_left;
	    pim_mre->const_join_timer().time_remaining(tv_left);
	    left_sec = tv_left.sec();
	}
	cli_print(c_format("    Join timer:                %d\n",
			   left_sec));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    KAT(S,G) running:          %s\n",
			   pim_mre->is_keepalive_timer_running()?
			   "true" : "false"));
    }
    
    
    //
    // The downstream interfaces
    //
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Local receiver include WC: %s\n",
			   mifset_str(pim_mre->local_receiver_include_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Local receiver include SG: %s\n",
			   mifset_str(pim_mre->local_receiver_include_sg()).c_str()));
	cli_print(c_format("    Local receiver exclude SG: %s\n",
			   mifset_str(pim_mre->local_receiver_exclude_sg()).c_str()));
    }
    cli_print(c_format("    Joins RP:                  %s\n",
		       mifset_str(pim_mre->joins_rp()).c_str()));
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Joins WC:                  %s\n",
			   mifset_str(pim_mre->joins_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Joins SG:                  %s\n",
			   mifset_str(pim_mre->joins_sg()).c_str()));
    }
    if (pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Prunes SG_RPT:             %s\n",
			   mifset_str(pim_mre->prunes_sg_rpt()).c_str()));
    }
    cli_print(c_format("    Join state:                %s\n",
		       mifset_str(pim_mre->downstream_join_state()).c_str()));
    cli_print(c_format("    Prune state:               %s\n",
		       mifset_str(pim_mre->downstream_prune_state()).c_str()));
    cli_print(c_format("    Prune pending state:       %s\n",
		       mifset_str(pim_mre->downstream_prune_pending_state()).c_str()));
    if (pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Prune tmp state:           %s\n",
			   mifset_str(pim_mre->downstream_prune_tmp_state()).c_str()));
	cli_print(c_format("    Prune pending tmp state:   %s\n",
			   mifset_str(pim_mre->downstream_prune_pending_tmp_state()).c_str()));
    }
    if (pim_mre->is_wc() || pim_mre->is_sg()) {
	cli_print(c_format("    I am assert winner state:  %s\n",
			   mifset_str(pim_mre->i_am_assert_winner_state()).c_str()));
	cli_print(c_format("    I am assert loser state:   %s\n",
			   mifset_str(pim_mre->i_am_assert_loser_state()).c_str()));
    }
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Assert winner WC:          %s\n",
			   mifset_str(pim_mre->i_am_assert_winner_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Assert winner SG:          %s\n",
			   mifset_str(pim_mre->i_am_assert_winner_sg()).c_str()));
    }
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Assert lost WC:            %s\n",
			   mifset_str(pim_mre->lost_assert_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Assert lost SG:            %s\n",
			   mifset_str(pim_mre->lost_assert_sg()).c_str()));
    }
    if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Assert lost SG_RPT:        %s\n",
			   mifset_str(pim_mre->lost_assert_sg_rpt()).c_str()));
    }
    if (pim_mre->is_wc()) {
	cli_print(c_format("    Assert tracking WC:        %s\n",
			   mifset_str(pim_mre->assert_tracking_desired_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Assert tracking SG:        %s\n",
			   mifset_str(pim_mre->assert_tracking_desired_sg()).c_str()));
    }
    cli_print(c_format("    Could assert WC:           %s\n",
		       mifset_str(pim_mre->could_assert_wc()).c_str()));
    if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Could assert SG:           %s\n",
			   mifset_str(pim_mre->could_assert_sg()).c_str()));
    }
    cli_print(c_format("    I am DR:                   %s\n",
		       mifset_str(pim_mre->i_am_dr()).c_str()));
    cli_print(c_format("    Immediate olist RP:        %s\n",
		       mifset_str(pim_mre->immediate_olist_rp()).c_str()));
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    Immediate olist WC:        %s\n",
			   mifset_str(pim_mre->immediate_olist_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    Immediate olist SG:        %s\n",
			   mifset_str(pim_mre->immediate_olist_sg()).c_str()));
    }
    cli_print(c_format("    Inherited olist SG:        %s\n",
		       mifset_str(pim_mre->inherited_olist_sg()).c_str()));
    cli_print(c_format("    Inherited olist SG_RPT:    %s\n",
		       mifset_str(pim_mre->inherited_olist_sg_rpt()).c_str()));
    if (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	cli_print(c_format("    PIM include WC:            %s\n",
			   mifset_str(pim_mre->pim_include_wc()).c_str()));
    }
    if (pim_mre->is_sg()) {
	cli_print(c_format("    PIM include SG:            %s\n",
			   mifset_str(pim_mre->pim_include_sg()).c_str()));
	cli_print(c_format("    PIM exclude SG:            %s\n",
			   mifset_str(pim_mre->pim_exclude_sg()).c_str()));
    }
}

//
// Print the CLI output for a #PimMfc entry.
//
void
PimNodeCli::cli_print_pim_mfc(const PimMfc *pim_mfc)
{
    if (pim_mfc == NULL)
	return;
    
    // Compute the IIF interface
    PimVif *iif_pim_vif = pim_node().vif_find_by_vif_index(pim_mfc->iif_vif_index());
    
    cli_print(c_format("%-15s %-15s %-15s\n",
		       cstring(pim_mfc->group_addr()),
		       cstring(pim_mfc->source_addr()),
		       cstring(pim_mfc->rp_addr())));
    cli_print(c_format("    Incoming interface :      %s\n",
		       (iif_pim_vif != NULL)?
		       iif_pim_vif->name().c_str() : "UNKNOWN"));
    
    //
    // The outgoing interfaces
    //
    cli_print(c_format("    Outgoing interfaces:      %s\n",
		       mifset_str(pim_mfc->olist()).c_str()));
}

//
// CLI COMMAND: "show pim neighbors [interface-name]"
// CLI COMMAND: "show pim6 neighbors [interface-name]"
//
// Display information about PIM neighbors.
//
int
PimNodeCli::cli_show_pim_neighbors(const vector<string>& argv)
{
    string interface_name;

    // Check the optional argument
    if (argv.size()) {
	interface_name = argv[0];
	if (pim_node().vif_find_by_name(interface_name) == NULL) {
	    cli_print(c_format("ERROR: Invalid interface name: %s\n",
			       interface_name.c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-12s %10s %-15s %1s %-6s %8s %7s\n",
		       "Interface", "DRpriority", "NeighborAddr", "V", "Mode",
		       "Holdtime", "Timeout"));
    for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
	if (pim_vif == NULL)
	    continue;
	// Test if we should print this entry
	bool do_print = true;
	if (interface_name.size()) {
	    do_print = false;
	    if (pim_vif->name() == interface_name) {
		do_print = true;
	    }
	}
	if (! do_print)
	    continue;
	list<PimNbr *>::iterator iter;
	for (iter = pim_vif->pim_nbrs().begin();
	     iter != pim_vif->pim_nbrs().end();
	     ++iter) {
	    PimNbr *pim_nbr = *iter;
	    
	    string dr_priority_string;
	    if (pim_nbr->is_dr_priority_present())
		dr_priority_string = c_format("%d", pim_nbr->dr_priority());
	    else
		dr_priority_string = "none";
	    
	    string nbr_timeout_sec_string;
	    if (pim_nbr->const_neighbor_liveness_timer().scheduled()) {
		TimeVal tv_left;
		pim_nbr->const_neighbor_liveness_timer().time_remaining(tv_left);
		nbr_timeout_sec_string = c_format("%d", tv_left.sec());
	    } else {
		nbr_timeout_sec_string = "None";
	    }
	    
	    cli_print(c_format("%-12s %10s %-15s %1d %-6s %8d %7s\n",
			       pim_vif->name().c_str(),
			       dr_priority_string.c_str(),
			       cstring(pim_nbr->primary_addr()),
			       pim_nbr->proto_version(),
			       pim_vif->proto_is_pimsm()? "Sparse" : "Dense",
			       pim_nbr->hello_holdtime(),
			       nbr_timeout_sec_string.c_str()));
	    // Print the secondary addresses
	    list<IPvX>::const_iterator list_iter;
	    for (list_iter = pim_nbr->secondary_addr_list().begin();
		 list_iter != pim_nbr->secondary_addr_list().end();
		 ++list_iter) {
		const IPvX& secondary_addr = *list_iter;
		cli_print(c_format("%-12s %10s %-15s\n",
				   "",
				   "",
				   cstring(secondary_addr)));
	    }
	}
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim mrib [dest-address]"
// CLI COMMAND: "show pim6 mrib [dest-address]"
//
// Display MRIB information inside PIM.
//
int
PimNodeCli::cli_show_pim_mrib(const vector<string>& argv)
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
	Mrib *mrib = pim_node().pim_mrib_table().find(dest_address);
	if (mrib == NULL) {
	    cli_print(c_format("No matching MRIB entry for %s\n",
			       dest_address_name.c_str()));
	    return (XORP_ERROR);
	}
	cli_print(c_format("%-18s %-15s %-7s %-8s %10s %6s\n",
			   "DestPrefix", "NextHopRouter", "VifName",
			   "VifIndex", "MetricPref", "Metric"));
	string vif_name = "UNKNOWN";
	Vif *vif = pim_node().vif_find_by_vif_index(mrib->next_hop_vif_index());
	if (vif != NULL)
	    vif_name = vif->name();
	cli_print(c_format("%-18s %-15s %-7s %-8d %10d %6d\n",
			   cstring(mrib->dest_prefix()),
			   cstring(mrib->next_hop_router_addr()),
			   vif_name.c_str(),
			   mrib->next_hop_vif_index(),
			   mrib->metric_preference(),
			   mrib->metric()));
	return (XORP_OK);
    }
    
    cli_print(c_format("%-18s %-15s %-7s %-8s %10s %6s\n",
		       "DestPrefix", "NextHopRouter", "VifName",
		       "VifIndex", "MetricPref", "Metric"));
    PimMribTable::iterator iter;
    for (iter = pim_node().pim_mrib_table().begin();
	 iter != pim_node().pim_mrib_table().end();
	 ++iter) {
	Mrib *mrib = *iter;
	if (mrib == NULL)
	    continue;
	
	string vif_name = "UNKNOWN";
	Vif *vif = pim_node().vif_find_by_vif_index(mrib->next_hop_vif_index());
	if (vif != NULL)
	    vif_name = vif->name();
	cli_print(c_format("%-18s %-15s %-7s %-8d %10d %6d\n",
			   cstring(mrib->dest_prefix()),
			   cstring(mrib->next_hop_router_addr()),
			   vif_name.c_str(),
			   mrib->next_hop_vif_index(),
			   mrib->metric_preference(),
			   mrib->metric()));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim rps [group-address]"
// CLI COMMAND: "show pim6 rps [group-address]"
//
// Display information about PIM RPs
//
int
PimNodeCli::cli_show_pim_rps(const vector<string>& argv)
{
    PimRp *show_pim_rp = NULL;
    
    // Check the optional argument
    if (argv.size()) {
	try {
	    IPvX group_addr(argv[0].c_str());
	    if (group_addr.af() != family()) {
		cli_print(c_format("ERROR: Address with invalid address family: %s\n",
				   argv[0].c_str()));
		return (XORP_ERROR);
	    }
	    //
	    // Try to find the RP for that group.
	    //
	    show_pim_rp = pim_node().rp_table().rp_find(group_addr);
	    if (show_pim_rp == NULL) {
		cli_print(c_format("ERROR: no matching RP for group %s\n",
				   cstring(group_addr)));
		return (XORP_ERROR);
	    }
	} catch (InvalidString) {
	    cli_print(c_format("ERROR: Invalid group address: %s\n",
			       argv[0].c_str()));
	    return (XORP_ERROR);
	}
    }
    
    cli_print(c_format("%-15s %-9s %3s %8s %7s %12s %-18s\n",
		       "RP", "Type", "Pri", "Holdtime", "Timeout",
		       "ActiveGroups", "GroupPrefix"));
    list<PimRp *>::const_iterator iter;
    for (iter = pim_node().rp_table().rp_list().begin();
	 iter != pim_node().rp_table().rp_list().end();
	 ++iter) {
	PimRp *pim_rp = *iter;
	
	if ((show_pim_rp != NULL) && (show_pim_rp != pim_rp))
	    continue;
	
	// Get the string-name of the RP type (i.e., how we learned about it)
	string rp_type;
	switch (pim_rp->rp_learned_method()) {
	case PimRp::RP_LEARNED_METHOD_AUTORP:
	    rp_type = "auto-rp";
	    break;
	case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	    rp_type = "bootstrap";
	    break;
	case PimRp::RP_LEARNED_METHOD_STATIC:
	    rp_type = "static";
	    break;
	default:
	    rp_type = "unknown";
	    break;
	}
	
	//
	// Compute the 'holdtime' and 'timeout' for this RP (if applicable)
	//
	int holdtime = -1;
	int left_sec = -1;
	switch (pim_rp->rp_learned_method()) {
	case PimRp::RP_LEARNED_METHOD_AUTORP:
	    break;
	case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	    do {
		// Try first a scoped zone, then a non-scoped zone
		BsrRp *bsr_rp;
		bsr_rp = pim_node().pim_bsr().find_rp(pim_rp->group_prefix(),
						      true,
						      pim_rp->rp_addr());
		if (bsr_rp == NULL) {
		    bsr_rp = pim_node().pim_bsr().find_rp(pim_rp->group_prefix(),
							  false,
							  pim_rp->rp_addr());
		}
		if (bsr_rp == NULL)
		    break;
		holdtime = bsr_rp->rp_holdtime();
		if (bsr_rp->const_candidate_rp_expiry_timer().scheduled()) {
		    TimeVal tv_left;
		    bsr_rp->const_candidate_rp_expiry_timer().time_remaining(tv_left);
		    left_sec = tv_left.sec();
		}
	    } while (false);
	    break;
	case PimRp::RP_LEARNED_METHOD_STATIC:
	    break;
	default:
	    break;
	}
	
	cli_print(c_format("%-15s %-9s %3d %8d %7d %12u %-18s\n",
			   cstring(pim_rp->rp_addr()),
			   rp_type.c_str(),
			   pim_rp->rp_priority(),
			   holdtime,
			   left_sec,
			   (uint32_t)(pim_rp->pim_mre_wc_list().size()
				      + pim_rp->processing_pim_mre_wc_list().size()),
			   cstring(pim_rp->group_prefix())));
    }
    
    return (XORP_OK);
}

//
// CLI COMMAND: "show pim scope"
// CLI COMMAND: "show pim6 scope"
//
// Display information about PIM RPs
//
int
PimNodeCli::cli_show_pim_scope(const vector<string>& argv)
{
    // Check the optional argument
    if (argv.size()) {
	cli_print(c_format("ERROR: Unexpected argument: %s\n",
			   argv[0].c_str()));
	return (XORP_ERROR);
    }
    
    cli_print(c_format("%-43s %-14s\n",
		       "GroupPrefix", "Interface"));
    list<PimScopeZone>::const_iterator iter;
    for (iter = pim_node().pim_scope_zone_table().pim_scope_zone_list().begin();
	 iter != pim_node().pim_scope_zone_table().pim_scope_zone_list().end();
	 ++iter) {
	const PimScopeZone& pim_scope_zone = *iter;
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    if (pim_scope_zone.is_set(i)) {
		PimVif *pim_vif = pim_node().vif_find_by_vif_index(i);
		if (pim_vif == NULL)
		    continue;
		cli_print(c_format("%-43s %-14s\n",
				   cstring(pim_scope_zone.scope_zone_prefix()),
				   pim_vif->name().c_str()));
	    }
	}
    }
    
    return (XORP_OK);
}
