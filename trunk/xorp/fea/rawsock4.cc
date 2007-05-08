// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/rawsock4.cc,v 1.18 2007/02/16 22:45:49 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "rawsock4.hh"

/* ------------------------------------------------------------------------- */
/* RawSocket4 methods */

RawSocket4::RawSocket4(EventLoop& eventloop, uint32_t ip_protocol,
		       const IfTree& iftree)
    : RawSocket(eventloop, IPv4::af(), ip_protocol, iftree)
{
}

RawSocket4::~RawSocket4()
{
    string error_msg;

    RawSocket::stop(error_msg);
}

/* ------------------------------------------------------------------------- */
/* FilterRawSocket4 methods */

FilterRawSocket4::FilterRawSocket4(EventLoop& eventloop, uint32_t protocol,
				   const IfTree& iftree)
    : RawSocket4(eventloop, protocol, iftree)
{
}

FilterRawSocket4::~FilterRawSocket4()
{
    if (_filters.empty() == false) {
	string dummy_error_msg;
	RawSocket::stop(dummy_error_msg);

	do {
	    InputFilter* i = _filters.front();
	    _filters.erase(_filters.begin());
	    i->bye();
	} while (_filters.empty() == false);
    }
}

bool
FilterRawSocket4::add_filter(InputFilter* filter)
{
    if (filter == 0) {
	XLOG_FATAL("Adding a null filter");
	return false;
    }

    if (find(_filters.begin(), _filters.end(), filter) != _filters.end()) {
	debug_msg("filter already exists\n");
	return false;
    }

    _filters.push_back(filter);
    if (_filters.front() == filter) {
	string error_msg;
	if (RawSocket4::start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

bool
FilterRawSocket4::remove_filter(InputFilter* filter)
{
    list<InputFilter*>::iterator i;
    i = find(_filters.begin(), _filters.end(), filter);
    if (i == _filters.end()) {
	debug_msg("filter does not exist\n");
	return false;
    }

    _filters.erase(i);
    if (_filters.empty()) {
	string error_msg;
	if (RawSocket4::stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

int
FilterRawSocket4::proto_socket_write(const string&	if_name,
				     const string&	vif_name,
				     const IPv4&	src_address,
				     const IPv4&	dst_address,
				     int32_t		ip_ttl,
				     int32_t		ip_tos,
				     bool		ip_router_alert,
				     bool		ip_internet_control,
				     const vector<uint8_t>& payload,
				     string&		error_msg)
{
    // XXX: The extention headers are not used in IPv4
    vector<uint8_t> ext_headers_type;
    vector<vector<uint8_t> > ext_headers_payload;

    return (RawSocket4::proto_socket_write(if_name,
					   vif_name,
					   IPvX(src_address),
					   IPvX(dst_address),
					   ip_ttl,
					   ip_tos,
					   ip_router_alert,
					   ip_internet_control,
					   ext_headers_type,
					   ext_headers_payload,
					   payload,
					   error_msg));
}

void
FilterRawSocket4::process_recv_data(const string&	if_name,
				    const string&	vif_name,
				    const IPvX&		src_address,
				    const IPvX&		dst_address,
				    int32_t		ip_ttl,
				    int32_t		ip_tos,
				    bool		ip_router_alert,
				    bool		ip_internet_control,
				    const vector<uint8_t>& ext_headers_type,
				    const vector<vector<uint8_t> >& ext_headers_payload,
				    const vector<uint8_t>& payload)
{
    struct IPv4HeaderInfo header;

    // XXX: The extention headers are not used in IPv4
    UNUSED(ext_headers_type);
    UNUSED(ext_headers_payload);

    XLOG_ASSERT(src_address.is_ipv4());
    XLOG_ASSERT(dst_address.is_ipv4());

    header.if_name = if_name;
    header.vif_name = vif_name;
    header.src_address = src_address.get_ipv4();
    header.dst_address = dst_address.get_ipv4();
    header.ip_protocol = RawSocket4::ip_protocol();
    header.ip_ttl = ip_ttl;
    header.ip_tos = ip_tos;
    header.ip_router_alert = ip_router_alert;
    header.ip_internet_control = ip_internet_control;

    for (list<InputFilter*>::iterator i = _filters.begin();
	 i != _filters.end(); ++i) {
	(*i)->recv(header, payload);
    }
}

int
FilterRawSocket4::join_multicast_group(const string&	if_name,
				       const string&	vif_name,
				       const IPv4&	group_address,
				       const string&	receiver_name,
				       string&		error_msg)
{
    JoinedGroupsTable::iterator iter;

    //
    // Check the arguments
    //
    if (! group_address.is_multicast()) {
	error_msg = c_format("Cannot join group %s: not a multicast address",
			     group_address.str().c_str());
	return (XORP_ERROR);
    }
    if (if_name.empty()) {
	error_msg = c_format("Cannot join group %s: empty interface name",
			     group_address.str().c_str());
	return (XORP_ERROR);
    }
    if (vif_name.empty()) {
	error_msg = c_format("Cannot join group %s on interface %s: "
			     "empty vif name",
			     group_address.str().c_str(),
			     if_name.c_str());
	return (XORP_ERROR);
    }
    if (receiver_name.empty()) {
	error_msg = c_format("Cannot join group %s on interface %s vif %s: "
			     "empty receiver name",
			     group_address.str().c_str(),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }

    JoinedMulticastGroup init_jmg(if_name, vif_name, group_address);
    iter = _joined_groups_table.find(init_jmg);
    if (iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	if (RawSocket::join_multicast_group(if_name,
					    vif_name,
					    IPvX(group_address),
					    error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	_joined_groups_table.insert(make_pair(init_jmg, init_jmg));
	iter = _joined_groups_table.find(init_jmg);
    }
    XLOG_ASSERT(iter != _joined_groups_table.end());
    JoinedMulticastGroup& jmg = iter->second;

    jmg.add_receiver(receiver_name);

    return (XORP_OK);
}

int
FilterRawSocket4::leave_multicast_group(const string&	if_name,
					const string&	vif_name,
					const IPv4&	group_address,
					const string&	receiver_name,
					string&		error_msg)
{
    JoinedGroupsTable::iterator iter;

    JoinedMulticastGroup init_jmg(if_name, vif_name, group_address);
    iter = _joined_groups_table.find(init_jmg);
    if (iter == _joined_groups_table.end()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "the group was not joined",
			     group_address.str().c_str(),
			     if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    JoinedMulticastGroup& jmg = iter->second;

    jmg.delete_receiver(receiver_name);
    if (jmg.empty()) {
	//
	// The last receiver, hence leave the group
	//
	_joined_groups_table.erase(iter);
	if (RawSocket::leave_multicast_group(if_name,
					     vif_name,
					     IPvX(group_address),
					     error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}
