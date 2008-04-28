// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/data_plane/firewall/firewall_set_netfilter.cc,v 1.2 2008/04/27 23:08:05 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/stat.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_LINUX_NETFILTER_IPV4_IP_TABLES_H
#include <linux/netfilter_ipv4/ip_tables.h>
#endif
#ifdef HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif

#include "fea/firewall_manager.hh"

#include "firewall_set_netfilter.hh"


//
// Set firewall information into the underlying system.
//
// The mechanism to set the information is NETFILTER.
//

#ifdef HAVE_FIREWALL_NETFILTER

#ifndef _ALIGNBYTES
#define _ALIGNBYTES	(sizeof(long) - 1)
#endif
#ifndef _ALIGN
#define _ALIGN(p)	(((u_long)(p) + _ALIGNBYTES) &~ _ALIGNBYTES)
#endif

const string FirewallSetNetfilter::_netfilter_table_name = "filter";
const string FirewallSetNetfilter::_netfilter_match_tcp = "tcp";
const string FirewallSetNetfilter::_netfilter_match_udp = "udp";
const string FirewallSetNetfilter::_netfilter_chain_input = "INPUT";
const string FirewallSetNetfilter::_netfilter_chain_forward = "FORWARD";
const string FirewallSetNetfilter::_netfilter_chain_output = "OUTPUT";

//
// Local IPv4 structures
//

// IPv4: The error rule at the beginning and the end of a chain
struct local_ipv4_ipt_error_target {
    struct ipt_entry_target		entry_target;
    char				error[IPT_TABLE_MAXNAMELEN];
};

// IPv4: The start of a chain
struct local_ipv4_iptcb_chain_start {
    struct ipt_entry			entry;
    struct local_ipv4_ipt_error_target	error_target;
};

// IPv4: The end of a chain
struct local_ipv4_iptcb_chain_foot {
    struct ipt_entry			entry;
    struct ipt_standard_target		standard_target;
};

// IPv4: The end of a table
struct local_ipv4_iptcb_chain_error {
    struct ipt_entry			entry;
    struct local_ipv4_ipt_error_target	error_target;
};

// IPv6: The error rule at the beginning and the end
struct local_ipv6_ipt_error_target {
    struct ip6t_entry_target		entry_target;
    char				error[IP6T_TABLE_MAXNAMELEN];
};

// IPv6: The start of a chain
struct local_ipv6_iptcb_chain_start {
    struct ip6t_entry			entry;
    struct local_ipv6_ipt_error_target	error_target;
};

// IPv6: The end of a chain
struct local_ipv6_iptcb_chain_foot {
    struct ip6t_entry			entry;
    struct ip6t_standard_target		standard_target;
};

// IPv6: The end of a table
struct local_ipv6_iptcb_chain_error {
    struct ip6t_entry			entry;
    struct local_ipv6_ipt_error_target	error_target;
};


FirewallSetNetfilter::FirewallSetNetfilter(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallSet(fea_data_plane_manager),
      _s4(-1),
      _s6(-1),
      _num_entries(0),
      _head_offset(0),
      _foot_offset(0)
{
}

FirewallSetNetfilter::~FirewallSetNetfilter()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the NETFILTER mechanism to set "
		   "firewall information into the underlying system: %s",
		   error_msg.c_str());
    }
}

int
FirewallSetNetfilter::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Check if we have the necessary permission.
    // If not, print a warning and continue without firewall.
    //
    if (geteuid() != 0) {
	XLOG_WARNING("Cannot start the NETFILTER firewall mechanism: "
		     "must be root; continuing without firewall setup");
	return (XORP_OK);
    }

    //
    // Open a raw IPv4 socket that NETFILTER uses for communication
    //
    _s4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_s4 < 0) {
	error_msg = c_format("Could not open a raw IPv4 socket for NETFILTER "
			     "firewall: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

#ifdef HAVE_IPV6
    //
    // Open a raw IPv6 socket that NETFILTER uses for communication
    //
    _s6 = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (_s6 < 0) {
	error_msg = c_format("Could not open a raw IPv6 socket for NETFILTER "
			     "firewall: %s",
			     strerror(errno));
	comm_close(_s4);
	_s4 = -1;
	return (XORP_ERROR);
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
FirewallSetNetfilter::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	ret_value4 = comm_close(_s4);
	_s4 = -1;
	if (ret_value4 != XORP_OK) {
	    error_msg = c_format("Could not close raw IPv4 socket for "
				 "NETFILTER firewall: %s",
				 strerror(errno));
	}
    }

    if (_s6 >= 0) {
	ret_value6 = comm_close(_s6);
	_s6 = -1;
	if ((ret_value6 != XORP_OK) && (ret_value4 == XORP_OK)) {
	    error_msg = c_format("Could not close raw IPv6 socket for "
				 "NETFILTER firewall: %s",
				 strerror(errno));
	}
    }

    if ((ret_value4 != XORP_OK) || (ret_value6 != XORP_OK))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FirewallSetNetfilter::update_entries(
    const list<FirewallEntry>& added_entries,
    const list<FirewallEntry>& replaced_entries,
    const list<FirewallEntry>& deleted_entries,
    string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;
    bool has_ipv4 = false;
    bool has_ipv6 = false;

    //
    // The entries to add
    //
    for (iter = added_entries.begin();
	 iter != added_entries.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	if (firewall_entry.is_ipv4())
	    has_ipv4 = true;
	if (firewall_entry.is_ipv6())
	    has_ipv6 = true;
	if (add_entry(firewall_entry, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // The entries to replace
    //
    for (iter = replaced_entries.begin();
	 iter != replaced_entries.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	if (firewall_entry.is_ipv4())
	    has_ipv4 = true;
	if (firewall_entry.is_ipv6())
	    has_ipv6 = true;
	if (replace_entry(firewall_entry, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // The entries to delete
    //
    for (iter = deleted_entries.begin();
	 iter != deleted_entries.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	if (firewall_entry.is_ipv4())
	    has_ipv4 = true;
	if (firewall_entry.is_ipv6())
	    has_ipv6 = true;
	if (delete_entry(firewall_entry, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Push the entries
    //
    if (has_ipv4) {
	if (push_entries4(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }
    if (has_ipv6) {
	if (push_entries6(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetNetfilter::set_table4(const list<FirewallEntry>& firewall_entry_list,
				 string& error_msg)
{
    list<FirewallEntry> empty_list;

    _firewall_entries4.clear();

    return (update_entries(firewall_entry_list, empty_list, empty_list,
			   error_msg));
}

int
FirewallSetNetfilter::set_table6(const list<FirewallEntry>& firewall_entry_list,
				 string& error_msg)
{
    list<FirewallEntry> empty_list;

    _firewall_entries6.clear();

    return (update_entries(firewall_entry_list, empty_list, empty_list,
			   error_msg));
}

int
FirewallSetNetfilter::delete_all_entries4(string& error_msg)
{
    _firewall_entries4.clear();

    return (push_entries4(error_msg));
}

int
FirewallSetNetfilter::delete_all_entries6(string& error_msg)
{
    _firewall_entries6.clear();

    return (push_entries6(error_msg));
}

int
FirewallSetNetfilter::add_entry(const FirewallEntry& firewall_entry,
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
FirewallSetNetfilter::replace_entry(const FirewallEntry& firewall_entry,
				    string& error_msg)
{
    //
    // XXX: The add_entry() method implementation covers the replace_entry()
    // semantic as well.
    //
    return (add_entry(firewall_entry, error_msg));
}

int
FirewallSetNetfilter::delete_entry(const FirewallEntry& firewall_entry,
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
FirewallSetNetfilter::push_entries4(string& error_msg)
{
    vector<uint8_t> buffer;
    size_t size;
    size_t next_data_index = 0;
    struct ipt_replace* ipr;
    socklen_t socklen;

    //
    // Calculate the required buffer space and allocate the buffer
    //
    size = _ALIGN(sizeof(struct ipt_entry))
	+ _ALIGN(sizeof(struct ipt_entry_match))
	+ _ALIGN(sizeof(struct ipt_standard_target));
    size *= _firewall_entries4.size();
    size += _ALIGN(sizeof(struct ipt_replace));
    size += 3 * _ALIGN(sizeof(struct local_ipv4_iptcb_chain_start));
    size += 3 * _ALIGN(sizeof(struct local_ipv4_iptcb_chain_foot));
    size += _ALIGN(sizeof(struct local_ipv4_iptcb_chain_error));

    buffer.resize(size);
    for (size_t i = 0; i < buffer.size(); i++)
	buffer[i] = 0;

    //
    // Get information about the old table
    //
    struct ipt_getinfo info;
    memset(&info, 0, sizeof(info));
    strlcpy(info.name, _netfilter_table_name.c_str(), sizeof(info.name));
    socklen = sizeof(info);
    if (getsockopt(_s4, IPPROTO_IP, IPT_SO_GET_INFO, &info, &socklen) < 0) {
	error_msg = c_format("Could not get the NETFILTER IPv4 firewall table: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Add a buffer to get information about the old counters
    //
    size = sizeof(struct ipt_counters) * info.num_entries;
    vector<uint8_t> counters_buffer(size);
    for (size_t i = 0; i < counters_buffer.size(); i++)
	counters_buffer[i] = 0;

    //
    // Initialize the replacement header
    //
    ipr = reinterpret_cast<struct ipt_replace *>(&buffer[0]);
    strlcpy(ipr->name, _netfilter_table_name.c_str(), sizeof(ipr->name));
    ipr->valid_hooks = info.valid_hooks;
    ipr->num_counters = info.num_entries;
    ipr->num_entries = 0;
    ipr->counters = reinterpret_cast<struct ipt_counters *>(&counters_buffer[0]);
    next_data_index += sizeof(*ipr);
    _num_entries = 0;

    //
    // Add the chains: INPUT, FORWARD, OUTPUT
    //

    // The INPUT chain
    if (encode_chain4(_netfilter_chain_input, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP_LOCAL_IN] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP_LOCAL_IN] = _foot_offset - sizeof(*ipr);

    // The FORWARD chain
    if (encode_chain4(_netfilter_chain_forward, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP_FORWARD] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP_FORWARD] = _foot_offset - sizeof(*ipr);

    // The OUTPUT chain
    if (encode_chain4(_netfilter_chain_output, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP_LOCAL_OUT] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP_LOCAL_OUT] = _foot_offset - sizeof(*ipr);

    //
    // Append the error rule at the end of the table
    //
    {
	struct local_ipv4_iptcb_chain_error* error;

	uint8_t* ptr = &buffer[next_data_index];
	error = reinterpret_cast<struct local_ipv4_iptcb_chain_error*>(ptr);
	error->entry.target_offset = sizeof(error->entry);
	error->entry.next_offset = sizeof(error->entry)
	    + _ALIGN(sizeof(error->error_target));
	error->error_target.entry_target.u.user.target_size
	    = _ALIGN(sizeof(error->error_target));
	strlcpy(error->error_target.entry_target.u.user.name,
		IPT_ERROR_TARGET,
		sizeof(error->error_target.entry_target.u.user.name));
	strlcpy(error->error_target.error, "ERROR",
		sizeof(error->error_target.error));

	next_data_index += error->entry.next_offset;
	_num_entries++;
    }

    //
    // Finish the initialization of the replacement header
    //
    ipr->num_entries = _num_entries;
    _num_entries = 0;
    ipr->size = next_data_index - sizeof(*ipr);

    socklen = next_data_index;
    if (setsockopt(_s4, IPPROTO_IP, IPT_SO_SET_REPLACE, &buffer[0], socklen)
	< 0) {
	error_msg = c_format("Could not set NETFILTER firewall entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetNetfilter::push_entries6(string& error_msg)
{
    vector<uint8_t> buffer;
    size_t size;
    size_t next_data_index = 0;
    struct ip6t_replace* ipr;
    socklen_t socklen;

    //
    // Calculate the required buffer space and allocate the buffer
    //
    size = _ALIGN(sizeof(struct ip6t_entry))
	+ _ALIGN(sizeof(struct ip6t_entry_match))
	+ _ALIGN(sizeof(struct ip6t_standard_target));
    size *= _firewall_entries6.size();
    size += _ALIGN(sizeof(struct ip6t_replace));
    size += 3 * _ALIGN(sizeof(struct local_ipv6_iptcb_chain_start));
    size += 3 * _ALIGN(sizeof(struct local_ipv6_iptcb_chain_foot));
    size += _ALIGN(sizeof(struct local_ipv6_iptcb_chain_error));

    buffer.resize(size);
    for (size_t i = 0; i < buffer.size(); i++)
	buffer[i] = 0;

    //
    // Get information about the old table
    //
    struct ip6t_getinfo info;
    memset(&info, 0, sizeof(info));
    strlcpy(info.name, _netfilter_table_name.c_str(), sizeof(info.name));
    socklen = sizeof(info);
    if (getsockopt(_s6, IPPROTO_IPV6, IP6T_SO_GET_INFO, &info, &socklen) < 0) {
	error_msg = c_format("Could not get the NETFILTER IPv6 firewall table: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Add a buffer to get information about the old counters
    //
    size = sizeof(struct ip6t_counters) * info.num_entries;
    vector<uint8_t> counters_buffer(size);
    for (size_t i = 0; i < counters_buffer.size(); i++)
	counters_buffer[i] = 0;

    //
    // Initialize the replacement header
    //
    ipr = reinterpret_cast<struct ip6t_replace *>(&buffer[0]);
    strlcpy(ipr->name, _netfilter_table_name.c_str(), sizeof(ipr->name));
    ipr->valid_hooks = info.valid_hooks;
    ipr->num_counters = info.num_entries;
    ipr->num_entries = 0;
    ipr->counters = reinterpret_cast<struct ip6t_counters *>(&counters_buffer[0]);
    next_data_index += sizeof(*ipr);
    _num_entries = 0;

    //
    // Add the chains: INPUT, FORWARD, OUTPUT
    //

    // The INPUT chain
    if (encode_chain6(_netfilter_chain_input, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP6_LOCAL_IN] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP6_LOCAL_IN] = _foot_offset - sizeof(*ipr);

    // The FORWARD chain
    if (encode_chain6(_netfilter_chain_forward, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP6_FORWARD] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP6_FORWARD] = _foot_offset - sizeof(*ipr);

    // The OUTPUT chain
    if (encode_chain6(_netfilter_chain_output, buffer, next_data_index,
		      error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    ipr->hook_entry[NF_IP6_LOCAL_OUT] = _head_offset - sizeof(*ipr);
    ipr->underflow[NF_IP6_LOCAL_OUT] = _foot_offset - sizeof(*ipr);

    //
    // Append the error rule at the end of the table
    //
    {
	struct local_ipv6_iptcb_chain_error* error;

	uint8_t* ptr = &buffer[next_data_index];
	error = reinterpret_cast<struct local_ipv6_iptcb_chain_error*>(ptr);
	error->entry.target_offset = sizeof(error->entry);
	error->entry.next_offset = sizeof(error->entry)
	    + _ALIGN(sizeof(error->error_target));
	error->error_target.entry_target.u.user.target_size
	    = _ALIGN(sizeof(error->error_target));
	strlcpy(error->error_target.entry_target.u.user.name,
		IP6T_ERROR_TARGET,
		sizeof(error->error_target.entry_target.u.user.name));
	strlcpy(error->error_target.error, "ERROR",
		sizeof(error->error_target.error));

	next_data_index += error->entry.next_offset;
	_num_entries++;
    }

    //
    // Finish the initialization of the replacement header
    //
    ipr->num_entries = _num_entries;
    _num_entries = 0;
    ipr->size = next_data_index - sizeof(*ipr);

    //
    // Write the data to the underlying system
    //
    socklen = next_data_index;
    if (setsockopt(_s6, IPPROTO_IPV6, IP6T_SO_SET_REPLACE, &buffer[0], socklen)
	< 0) {
	error_msg = c_format("Could not set NETFILTER firewall entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetNetfilter::encode_chain4(const string& chain_name,
				    vector<uint8_t>& buffer,
				    size_t& next_data_index,
				    string& error_msg)
{
    uint8_t* ptr;

    //
    // Add the chain header for user-defined chains
    //
    // XXX: We don't use user-defined chains.
    //
    bool is_user_defined_chain = false;

    _head_offset = next_data_index;		// XXX

    if (is_user_defined_chain) {
	struct local_ipv4_iptcb_chain_start* head;
	string chain_name;	// XXX: user-defined

	ptr = &buffer[next_data_index];
	head = reinterpret_cast<struct local_ipv4_iptcb_chain_start *>(ptr);
	head->entry.target_offset = sizeof(head->entry);
	head->entry.next_offset = sizeof(head->entry)
	    + _ALIGN(sizeof(head->error_target));
	strlcpy(head->error_target.entry_target.u.user.name, IPT_ERROR_TARGET,
		sizeof(head->error_target.entry_target.u.user.name));
	head->error_target.entry_target.u.target_size
	    = _ALIGN(sizeof(head->error_target));
	strlcpy(head->error_target.error, chain_name.c_str(),
		sizeof(head->error_target.error));

	next_data_index += head->entry.next_offset;
	_num_entries++;
    }

    //
    // Encode all entries one-by-one if the FORWARD chain
    //
    if (chain_name == _netfilter_chain_forward) {
	FirewallTrie::const_iterator iter;
	for (iter = _firewall_entries4.begin();
	     iter != _firewall_entries4.end();
	     ++iter) {
	    const FirewallEntry& firewall_entry = iter->second;
	    if (encode_entry4(firewall_entry, buffer, next_data_index,
			      error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
    }

    //
    // Add the chain footer
    //
    {
	struct local_ipv4_iptcb_chain_foot* foot;

	_foot_offset = next_data_index;		// XXX

	ptr = &buffer[next_data_index];
	foot = reinterpret_cast<struct local_ipv4_iptcb_chain_foot *>(ptr);
	foot->entry.target_offset = sizeof(foot->entry);
	foot->entry.next_offset = sizeof(foot->entry)
	    + _ALIGN(sizeof(foot->standard_target));
	strlcpy(foot->standard_target.target.u.user.name, IPT_STANDARD_TARGET,
		sizeof(foot->standard_target.target.u.user.name));
	foot->standard_target.target.u.target_size
	    = _ALIGN(sizeof(foot->standard_target));
	if (is_user_defined_chain)
	    foot->standard_target.verdict = IPT_RETURN;
	else
	    foot->standard_target.verdict = -NF_ACCEPT - 1;

	next_data_index += foot->entry.next_offset;
	_num_entries++;
    }

    return (XORP_OK);
}

int
FirewallSetNetfilter::encode_chain6(const string& chain_name,
				    vector<uint8_t>& buffer,
				    size_t& next_data_index,
				    string& error_msg)
{
    uint8_t* ptr;

    //
    // Add the chain header for user-defined chains
    //
    // XXX: We don't use user-defined chains.
    //
    bool is_user_defined_chain = false;

    _head_offset = next_data_index;		// XXX

    if (is_user_defined_chain) {
	struct local_ipv6_iptcb_chain_start* head;
	string chain_name;	// XXX: user-defined

	ptr = &buffer[next_data_index];
	head = reinterpret_cast<struct local_ipv6_iptcb_chain_start *>(ptr);
	head->entry.target_offset = sizeof(head->entry);
	head->entry.next_offset = sizeof(head->entry)
	    + _ALIGN(sizeof(head->error_target));
	strlcpy(head->error_target.entry_target.u.user.name, IP6T_ERROR_TARGET,
		sizeof(head->error_target.entry_target.u.user.name));
	head->error_target.entry_target.u.target_size
	    = _ALIGN(sizeof(head->error_target));
	strlcpy(head->error_target.error, chain_name.c_str(),
		sizeof(head->error_target.error));

	next_data_index += head->entry.next_offset;
	_num_entries++;
    }

    //
    // Encode all entries one-by-one if the FORWARD chain
    //
    if (chain_name == _netfilter_chain_forward) {
	FirewallTrie::const_iterator iter;
	for (iter = _firewall_entries6.begin();
	     iter != _firewall_entries6.end();
	     ++iter) {
	    const FirewallEntry& firewall_entry = iter->second;
	    if (encode_entry6(firewall_entry, buffer, next_data_index,
			      error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
    }

    //
    // Add the chain footer
    //
    {
	struct local_ipv6_iptcb_chain_foot* foot;

	_foot_offset = next_data_index;		// XXX

	ptr = &buffer[next_data_index];
	foot = reinterpret_cast<struct local_ipv6_iptcb_chain_foot *>(ptr);
	foot->entry.target_offset = sizeof(foot->entry);
	foot->entry.next_offset = sizeof(foot->entry)
	    + _ALIGN(sizeof(foot->standard_target));
	strlcpy(foot->standard_target.target.u.user.name, IP6T_STANDARD_TARGET,
		sizeof(foot->standard_target.target.u.user.name));
	foot->standard_target.target.u.target_size
	    = _ALIGN(sizeof(foot->standard_target));
	foot->standard_target.verdict = IP6T_RETURN;
	if (is_user_defined_chain)
	    foot->standard_target.verdict = IP6T_RETURN;
	else
	    foot->standard_target.verdict = -NF_ACCEPT - 1;

	next_data_index += foot->entry.next_offset;
	_num_entries++;
    }

    return (XORP_OK);
}

int
FirewallSetNetfilter::encode_entry4(const FirewallEntry& firewall_entry,
				    vector<uint8_t>& buffer,
				    size_t& next_data_index,
				    string& error_msg)
{
    struct ipt_entry* ipt = NULL;
    struct ipt_entry_match* iem = NULL;
    struct ipt_standard_target* ist = NULL;
    uint8_t* ptr;

    UNUSED(error_msg);

    ptr = &buffer[next_data_index];
    ipt = reinterpret_cast<struct ipt_entry *>(ptr);

    //
    // Transcribe the XORP rule to an NETFILTER rule
    //
    ipt->target_offset = sizeof(*ipt);	// XXX: netfilter doesn't use _ALIGN()

    //
    // Set the rule number
    //
    // XXX: Do nothing, because NETFILTER doesn't have rule numbers
    //

    //
    // Set the protocol
    if (firewall_entry.ip_protocol() != FirewallEntry::IP_PROTOCOL_ANY)
	ipt->ip.proto = firewall_entry.ip_protocol();
    else
	ipt->ip.proto = 0;

    //
    // Set the source and destination network addresses
    //
    const IPvX& src_addr = firewall_entry.src_network().masked_addr();
    src_addr.copy_out(ipt->ip.src);
    IPvX src_mask = IPvX::make_prefix(src_addr.af(),
				      firewall_entry.src_network().prefix_len());
    src_mask.copy_out(ipt->ip.smsk);

    const IPvX& dst_addr = firewall_entry.dst_network().masked_addr();
    dst_addr.copy_out(ipt->ip.dst);
    IPvX dst_mask = IPvX::make_prefix(dst_addr.af(),
				      firewall_entry.dst_network().prefix_len());
    dst_mask.copy_out(ipt->ip.dmsk);

    //
    // Set the interface/vif
    //
    // XXX: On this platform, ifname == vifname
    //
    if (! firewall_entry.vifname().empty()) {
	strlcpy(ipt->ip.iniface, firewall_entry.vifname().c_str(),
		sizeof(ipt->ip.iniface));
    }

    //
    // Set the source and destination port number ranges
    //
    if ((ipt->ip.proto == IPPROTO_TCP) || (ipt->ip.proto == IPPROTO_UDP)) {
	ptr = reinterpret_cast<uint8_t *>(ipt);
	ptr += ipt->target_offset;
	iem = reinterpret_cast<struct ipt_entry_match *>(ptr);
	iem->u.match_size = _ALIGN(sizeof(*iem));
	switch (ipt->ip.proto) {
	case IPPROTO_TCP:
	{
	    struct ipt_tcp* ip_tcp;
	    ip_tcp = reinterpret_cast<struct ipt_tcp *>(iem->data);
	    ip_tcp->spts[0] = firewall_entry.src_port_begin();
	    ip_tcp->spts[1] = firewall_entry.src_port_end();
	    ip_tcp->dpts[0] = firewall_entry.dst_port_begin();
	    ip_tcp->dpts[1] = firewall_entry.dst_port_end();

	    strlcpy(iem->u.user.name, _netfilter_match_tcp.c_str(),
		    sizeof(iem->u.user.name));
	    iem->u.match_size += _ALIGN(sizeof(*ip_tcp));
	    break;
	}
	case IPPROTO_UDP:
	{
	    struct ipt_udp* ip_udp;
	    ip_udp = reinterpret_cast<struct ipt_udp *>(iem->data);
	    ip_udp->spts[0] = firewall_entry.src_port_begin();
	    ip_udp->spts[1] = firewall_entry.src_port_end();
	    ip_udp->dpts[0] = firewall_entry.dst_port_begin();
	    ip_udp->dpts[1] = firewall_entry.dst_port_end();

	    strlcpy(iem->u.user.name, _netfilter_match_udp.c_str(),
		    sizeof(iem->u.user.name));
	    iem->u.match_size += _ALIGN(sizeof(*ip_udp));
	    break;
	}
	default:
	    break;
	}
	ipt->target_offset += iem->u.match_size;
    }

    //
    // Set the action
    //
    ptr = reinterpret_cast<uint8_t *>(ipt);
    ptr += ipt->target_offset;
    ist = reinterpret_cast<struct ipt_standard_target *>(ptr);
    ist->target.u.user.target_size = XT_ALIGN(sizeof(*ist));
    strlcpy(ist->target.u.user.name, IPT_STANDARD_TARGET,
	    sizeof(ist->target.u.user.name));
    switch (firewall_entry.action()) {
    case FirewallEntry::ACTION_PASS:
	ist->verdict = -NF_ACCEPT - 1;
	break;
    case FirewallEntry::ACTION_DROP:
	ist->verdict = -NF_DROP - 1;
	break;
    case FirewallEntry::ACTION_REJECT:
	// XXX: NETFILTER doesn't distinguish between packet DROP and REJECT
	ist->verdict = -NF_DROP - 1;
	break;
    case FirewallEntry::ACTION_ANY:
    case FirewallEntry::ACTION_NONE:
	break;
    default:
	XLOG_FATAL("Unknown firewall entry action code: %u",
		   firewall_entry.action());
	break;
    }
    ipt->next_offset = ipt->target_offset + ist->target.u.user.target_size;

    _num_entries++;
    next_data_index += ipt->next_offset;

    return (XORP_OK);
}

int
FirewallSetNetfilter::encode_entry6(const FirewallEntry& firewall_entry,
				    vector<uint8_t>& buffer,
				    size_t& next_data_index,
				    string& error_msg)
{
    struct ip6t_entry* ipt = NULL;
    struct ip6t_entry_match* iem = NULL;
    struct ip6t_standard_target* ist = NULL;
    uint8_t* ptr;

    UNUSED(error_msg);

    ptr = &buffer[next_data_index];
    ipt = reinterpret_cast<struct ip6t_entry *>(ptr);

    //
    // Transcribe the XORP rule to an NETFILTER rule
    //
    ipt->target_offset = sizeof(*ipt);	// XXX: netfilter doesn't use _ALIGN()

    //
    // Set the rule number
    //
    // XXX: Do nothing, because NETFILTER doesn't have rule numbers
    //

    //
    // Set the protocol
    if (firewall_entry.ip_protocol() != FirewallEntry::IP_PROTOCOL_ANY)
	ipt->ipv6.proto = firewall_entry.ip_protocol();
    else
	ipt->ipv6.proto = 0;

    //
    // Set the source and destination network addresses
    //
    const IPvX& src_addr = firewall_entry.src_network().masked_addr();
    src_addr.copy_out(ipt->ipv6.src);
    IPvX src_mask = IPvX::make_prefix(src_addr.af(),
				      firewall_entry.src_network().prefix_len());
    src_mask.copy_out(ipt->ipv6.smsk);

    const IPvX& dst_addr = firewall_entry.dst_network().masked_addr();
    dst_addr.copy_out(ipt->ipv6.dst);
    IPvX dst_mask = IPvX::make_prefix(dst_addr.af(),
				      firewall_entry.dst_network().prefix_len());
    dst_mask.copy_out(ipt->ipv6.dmsk);

    //
    // Set the interface/vif
    //
    // XXX: On this platform, ifname == vifname
    //
    if (! firewall_entry.vifname().empty()) {
	strlcpy(ipt->ipv6.iniface, firewall_entry.vifname().c_str(),
		sizeof(ipt->ipv6.iniface));
    }

    //
    // Set the source and destination port number ranges
    //
    if ((ipt->ipv6.proto == IPPROTO_TCP) || (ipt->ipv6.proto == IPPROTO_UDP)) {
	ptr = reinterpret_cast<uint8_t *>(ipt);
	ptr += ipt->target_offset;
	iem = reinterpret_cast<struct ip6t_entry_match *>(ptr);
	iem->u.match_size = _ALIGN(sizeof(*iem));
	switch (ipt->ipv6.proto) {
	case IPPROTO_TCP:
	{
	    struct ip6t_tcp* ip_tcp;
	    ip_tcp = reinterpret_cast<struct ip6t_tcp *>(iem->data);
	    ip_tcp->spts[0] = firewall_entry.src_port_begin();
	    ip_tcp->spts[1] = firewall_entry.src_port_end();
	    ip_tcp->dpts[0] = firewall_entry.dst_port_begin();
	    ip_tcp->dpts[1] = firewall_entry.dst_port_end();

	    strlcpy(iem->u.user.name, _netfilter_match_tcp.c_str(),
		    sizeof(iem->u.user.name));
	    iem->u.match_size += _ALIGN(sizeof(*ip_tcp));
	    break;
	}
	case IPPROTO_UDP:
	{
	    struct ip6t_udp* ip_udp;
	    ip_udp = reinterpret_cast<struct ip6t_udp *>(iem->data);
	    ip_udp->spts[0] = firewall_entry.src_port_begin();
	    ip_udp->spts[1] = firewall_entry.src_port_end();
	    ip_udp->dpts[0] = firewall_entry.dst_port_begin();
	    ip_udp->dpts[1] = firewall_entry.dst_port_end();

	    strlcpy(iem->u.user.name, _netfilter_match_udp.c_str(),
		    sizeof(iem->u.user.name));
	    iem->u.match_size += _ALIGN(sizeof(*ip_udp));
	    break;
	}
	default:
	    break;
	}
	ipt->target_offset += iem->u.match_size;
    }

    //
    // Set the action
    //
    ptr = reinterpret_cast<uint8_t *>(ipt);
    ptr += ipt->target_offset;
    ist = reinterpret_cast<struct ip6t_standard_target *>(ptr);
    ist->target.u.user.target_size = XT_ALIGN(sizeof(*ist));
    strlcpy(ist->target.u.user.name, IP6T_STANDARD_TARGET,
	    sizeof(ist->target.u.user.name));
    switch (firewall_entry.action()) {
    case FirewallEntry::ACTION_PASS:
	ist->verdict = -NF_ACCEPT - 1;
	break;
    case FirewallEntry::ACTION_DROP:
	ist->verdict = -NF_DROP - 1;
	break;
    case FirewallEntry::ACTION_REJECT:
	// XXX: NETFILTER doesn't distinguish between packet DROP and REJECT
	ist->verdict = -NF_DROP - 1;
	break;
    case FirewallEntry::ACTION_ANY:
    case FirewallEntry::ACTION_NONE:
	break;
    default:
	XLOG_FATAL("Unknown firewall entry action code: %u",
		   firewall_entry.action());
	break;
    }
    ipt->next_offset = ipt->target_offset + ist->target.u.user.target_size;

    _num_entries++;
    next_data_index += ipt->next_offset;

    return (XORP_OK);
}

#endif // HAVE_FIREWALL_NETFILTER
