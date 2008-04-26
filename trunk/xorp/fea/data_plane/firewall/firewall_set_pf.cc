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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_PFVAR_H
#include <net/pfvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


#include "fea/firewall_manager.hh"

#include "firewall_set_pf.hh"


//
// Set firewall information into the underlying system.
//
// The mechanism to set the information is PF.
//

#ifdef HAVE_FIREWALL_PF

const string FirewallSetPf::_pf_device_name = "/dev/pf";

FirewallSetPf::FirewallSetPf(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallSet(fea_data_plane_manager),
      _fd(-1)
{
}

FirewallSetPf::~FirewallSetPf()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the PF mechanism to set "
		   "firewall information into the underlying system: %s",
		   error_msg.c_str());
    }
}

int
FirewallSetPf::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Open the PF device
    //
    _fd = open(_pf_device_name.c_str(), O_RDWR);
    if (_fd < 0) {
	error_msg = c_format("Could not open device %s for PF firewall: %s",
			     _pf_device_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Enable PF operation
    //
    if (ioctl(_fd, DIOCSTART) < 0) {
	// Test whether PF is already enabled
	if (errno != EEXIST) {
	    error_msg = c_format("Failed to enable PF firewall operation: %s",
				 strerror(errno));
	    close(_fd);
	    _fd = -1;
	    return (XORP_ERROR);
	}
    }

    _is_running = true;

    return (XORP_OK);
}

int
FirewallSetPf::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    //
    // Disable PF operation
    //
    if (ioctl(_fd, DIOCSTOP) < 0) {
	// Test whether PF is already disabled
	if (errno != ENOENT) {
	    error_msg = c_format("Failed to disable PF firewall operation: %s",
				 strerror(errno));
	    close(_fd);
	    _fd = -1;
	    return (XORP_ERROR);
	}
    }

    if (close(_fd) < 0) {
	_fd = -1;
	error_msg = c_format("Could not close device %s for PF firewall: %s",
			     _pf_device_name.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    _fd = -1;
    _is_running = false;

    return (XORP_OK);
}

int
FirewallSetPf::add_entry(const FirewallEntry& firewall_entry,
			 string& error_msg)
{
    FirewallTrie::iterator iter;
    FirewallTrie* ftp = NULL;
    uint32_t key = firewall_entry.rule_number();	// XXX: the map key

    if (firewall_entry.is_ipv4())
	ftp = &_firewall_entries4;
    else
	ftp = &_firewall_entries6;

    // XXX: If the entry already exists, then just update it
    iter = ftp->find(key);
    if (iter == ftp->end()) {
	ftp->insert(make_pair(key, firewall_entry));
    } else {
	FirewallEntry& fe_tmp = iter->second;
	fe_tmp = firewall_entry;
    }

    //
    // XXX: Push all entries, because the API for doing incremental changes
    // adds more complexity.
    //
    // If this becomes an issue, then we need to:
    // - Keep track of the kernel's rule numbers in user space
    // - Use ioctl(DIOCCHANGERULE) with PF_CHANGE_GET_TICKET
    //   to get the appropriate ticket.
    // - Use ioctl(DIOCCHANGERULE) as appropriate with one of the
    //   following actions and the appropriate kernel rule number:
    //   PF_CHANGE_ADD_HEAD, PF_CHANGE_ADD_TAIL, F_CHANGE_ADD_BEFORE,
    //   PF_CHANGE_ADD_AFTER
    //
    return (push_entries(error_msg));
}

int
FirewallSetPf::delete_entry(const FirewallEntry& firewall_entry,
			    string& error_msg)
{
    FirewallTrie::iterator iter;
    FirewallTrie* ftp = NULL;
    uint32_t key = firewall_entry.rule_number();	// XXX: the map key

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

    //
    // XXX: Push all entries, because the API for doing incremental changes
    // adds more complexity.
    //
    // If this becomes an issue, then we need to:
    // - Keep track of the kernel's rule numbers in user space
    // - Use ioctl(DIOCCHANGERULE) with PF_CHANGE_GET_TICKET
    //   to get the appropriate ticket.
    // - Use ioctl(DIOCCHANGERULE) with action PF_CHANGE_REMOVE
    //   and the appropriate kernel rule number.
    //
    return (push_entries(error_msg));

#if 0
    //
    // Use a single operation to delete the entry
    //
    bool is_add = false;
    uint32_t ticket = 0;
    if (add_delete_transaction_entry(is_add, ticket, firewall_entry, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
#endif // 0

    return (XORP_OK);
}

int
FirewallSetPf::push_entries(string& error_msg)
{
    FirewallTrie::iterator iter;
    uint32_t ticket = 0;

    //
    // Use a transaction to push the entries
    //
    if (start_transaction(ticket, error_msg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Add all entries one-by-one.
    //
    // Note that we have to push both the IPv4 and IPv6 entries,
    // because they share the same table.
    //
    for (iter = _firewall_entries4.begin();
	 iter != _firewall_entries4.end();
	 ++iter) {
	FirewallEntry& firewall_entry = iter->second;
	bool is_add = true;

	if (add_delete_transaction_entry(is_add, ticket, firewall_entry,
					 error_msg)
	    != XORP_OK) {
	    string dummy_error_msg;
	    abort_transaction(ticket, dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    for (iter = _firewall_entries6.begin();
	 iter != _firewall_entries6.end();
	 ++iter) {
	FirewallEntry& firewall_entry = iter->second;
	bool is_add = true;

	if (add_delete_transaction_entry(is_add, ticket, firewall_entry,
					 error_msg)
	    != XORP_OK) {
	    string dummy_error_msg;
	    abort_transaction(ticket, dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    if (commit_transaction(ticket, error_msg) != XORP_OK) {
	string dummy_error_msg;
	abort_transaction(ticket, dummy_error_msg);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetPf::add_delete_transaction_entry(bool is_add, uint32_t ticket, 
					    const FirewallEntry& firewall_entry,
					    string& error_msg)
{
    struct pfioc_rule pr;

    memset(&pr, 0, sizeof(pr));
    pr.ticket = ticket;

    //
    // Transcribe the XORP rule to a PF rule
    //

    //
    // Set the address family
    //
    pr.rule.af = firewall_entry.src_network().af();

    //
    // Set the rule number
    //
    // XXX: Do nothing, because PF doesn't have rule numbers
    //

    //
    // Set the action
    //
    if (is_add) {
	// Add the entry
	switch (firewall_entry.action()) {
	case FirewallEntry::ACTION_PASS:
	    pr.rule.action = PF_PASS;
	    break;
	case FirewallEntry::ACTION_DROP:
	    pr.rule.action = PF_DROP;
	    break;
	case FirewallEntry::ACTION_REJECT:
	    // XXX: PF doesn't distinguish between packet DROP and REJECT
	    pr.rule.action = PF_DROP;
	    break;
	case FirewallEntry::ACTION_ANY:
	case FirewallEntry::ACTION_NONE:
	    break;
	default:
	    XLOG_FATAL("Unknown firewall entry action code: %u",
		       firewall_entry.action());
	    break;
	}
    } else {
	// Delete the entry
	pr.action = PF_CHANGE_REMOVE;
    }

    //
    // Set the protocol
    //
    if (firewall_entry.ip_protocol() != FirewallEntry::IP_PROTOCOL_ANY)
	pr.rule.proto = firewall_entry.ip_protocol();
    else
	pr.rule.proto = 0;

    //
    // Set the source and destination network addresses
    //
    pr.rule.src.addr.type = PF_ADDR_ADDRMASK;
    pr.rule.src.addr.iflags = 0;		// XXX: PFI_AFLAG_NETWORK ??
    const IPvX& src_addr = firewall_entry.src_network().masked_addr();
    src_addr.copy_out(pr.rule.src.addr.v.a.addr.addr8);
    IPvX src_mask = IPvX::make_prefix(src_addr.af(),
				      firewall_entry.src_network().prefix_len());
    src_mask.copy_out(pr.rule.src.addr.v.a.mask.addr8);

    pr.rule.dst.addr.type = PF_ADDR_ADDRMASK;
    pr.rule.dst.addr.iflags = 0;		// XXX: PFI_AFLAG_NETWORK ??
    const IPvX& dst_addr = firewall_entry.dst_network().masked_addr();
    dst_addr.copy_out(pr.rule.dst.addr.v.a.addr.addr8);
    IPvX dst_mask = IPvX::make_prefix(dst_addr.af(),
				      firewall_entry.dst_network().prefix_len());
    dst_mask.copy_out(pr.rule.dst.addr.v.a.mask.addr8);

    //
    // Set the source and destination port number ranges
    //
    if ((pr.rule.proto == IPPROTO_TCP) || (pr.rule.proto == IPPROTO_UDP)) {
	pr.rule.src.port[0] = firewall_entry.src_port_begin();
	pr.rule.src.port[1] = firewall_entry.src_port_end();
	pr.rule.src.port_op = PF_OP_RRG;

	pr.rule.dst.port[0] = firewall_entry.dst_port_begin();
	pr.rule.dst.port[1] = firewall_entry.dst_port_end();
	pr.rule.dst.port_op = PF_OP_RRG;
    }

    //
    // Set the interface/vif
    //
    // XXX: On this platform, ifname == vifname
    //
    if (! firewall_entry.vifname().empty()) {
        strlcpy(pr.rule.ifname, firewall_entry.vifname().c_str(),
                sizeof(pr.rule.ifname));
    }

    //
    // Misc. other state
    //
    pr.rule.direction = PF_INOUT;

    // Add or delete the entry
    if (is_add) {
	if (ioctl(_fd, DIOCADDRULE, &pr) < 0) {
	    error_msg = c_format("Failed to add rule to a PF firewall "
				 "transaction: %s", strerror(errno));
	    return (XORP_ERROR);
	}
    } else {
	//
	// TODO: XXX: The code below doesn't work, because
	// pr.nr is not set to the number of rule to delete.
	// Therefore, we delete entries by pushing the whole table
	// (same as when we add entries).
	//
	XLOG_UNREACHABLE();

	// Get the ticket
	pr.action = PF_CHANGE_GET_TICKET;
	if (ioctl(_fd, DIOCCHANGERULE, &pr) < 0) {
	    error_msg = c_format("Failed to delete rule from a PF firewall "
				 "transaction: %s", strerror(errno));
	    return (XORP_ERROR);
	}
	pr.action = PF_CHANGE_REMOVE;

	if (ioctl(_fd, DIOCCHANGERULE, &pr) < 0) {
	    error_msg = c_format("Failed to delete rule from a PF firewall "
				 "transaction: %s", strerror(errno));
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
FirewallSetPf::set_table4(const list<FirewallEntry>& firewall_entry_list,
			  string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    _firewall_entries4.clear();

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	_firewall_entries4.insert(make_pair(firewall_entry.rule_number(),
					    firewall_entry));
    }

    return (push_entries(error_msg));
}

int
FirewallSetPf::delete_all_entries4(string& error_msg)
{
    _firewall_entries4.clear();

    return (push_entries(error_msg));
}

int
FirewallSetPf::set_table6(const list<FirewallEntry>& firewall_entry_list,
			  string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    _firewall_entries6.clear();

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	_firewall_entries6.insert(make_pair(firewall_entry.rule_number(),
					    firewall_entry));
    }

    return (push_entries(error_msg));
}

int
FirewallSetPf::delete_all_entries6(string& error_msg)
{
    _firewall_entries6.clear();

    return (push_entries(error_msg));
}

int
FirewallSetPf::start_transaction(uint32_t& ticket, string& error_msg)
{
    struct pfioc_trans trans;
    struct pfioc_trans::pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;

    if (ioctl(_fd, DIOCXBEGIN, &trans) < 0) {
	error_msg = c_format("Failed to begin transaction for adding rules "
			     "to PF firewall: %s", strerror(errno));
        return (XORP_ERROR);
    }
    // TODO: Do we need a 'pool ticket'?

    ticket = trans_e.ticket;

    return (XORP_OK);
}

int
FirewallSetPf::commit_transaction(uint32_t ticket, string& error_msg)
{
    struct pfioc_trans trans;
    struct pfioc_trans::pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;
    trans_e.ticket = ticket;

    if (ioctl(_fd, DIOCXCOMMIT, &trans) < 0) {
	error_msg = c_format("Failed to commit transaction for adding rules "
			     "to PF firewall: %s", strerror(errno));
        return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetPf::abort_transaction(uint32_t ticket, string& error_msg)
{
    struct pfioc_trans trans;
    struct pfioc_trans::pfioc_trans_e trans_e;

    memset(&trans, 0, sizeof(trans));
    memset(&trans_e, 0, sizeof(trans_e));

    trans.size = 1;
    trans.esize = sizeof(trans_e);
    trans.array = &trans_e;
    trans_e.rs_num = PF_RULESET_FILTER;
    trans_e.ticket = ticket;

    if (ioctl(_fd, DIOCXROLLBACK, &trans) < 0) {
        error_msg = c_format("Failed to abort transaction for adding rules "
			     "to PF firewall: %s", strerror(errno));
        return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_FIREWALL_PF
