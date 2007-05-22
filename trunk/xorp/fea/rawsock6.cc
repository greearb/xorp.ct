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

#ident "$XORP: xorp/fea/rawsock6.cc,v 1.19 2007/05/19 01:52:41 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "rawsock6.hh"

/* ------------------------------------------------------------------------- */
/* RawSocket6 methods */

RawSocket6::RawSocket6(EventLoop& eventloop, uint8_t ip_protocol,
		       const IfTree& iftree)
    : RawSocket(eventloop, IPv6::af(), ip_protocol, iftree)
{
}

RawSocket6::~RawSocket6()
{
    string error_msg;

    RawSocket::stop(error_msg);
}

/* ------------------------------------------------------------------------- */
/* FilterRawSocket6 methods */

FilterRawSocket6::FilterRawSocket6(EventLoop& eventloop, uint8_t ip_protocol,
				   const IfTree& iftree)
    : RawSocket6(eventloop, ip_protocol, iftree)
{
}

FilterRawSocket6::~FilterRawSocket6()
{
    if (_input_filters.empty() == false) {
	string dummy_error_msg;
	RawSocket6::stop(dummy_error_msg);

	do {
	    InputFilter* i = _input_filters.front();
	    _input_filters.erase(_input_filters.begin());
	    i->bye();
	} while (_input_filters.empty() == false);
    }
}

bool
FilterRawSocket6::add_filter(InputFilter* filter)
{
    if (filter == 0) {
	XLOG_FATAL("Adding a null filter");
	return false;
    }

    if (find(_input_filters.begin(), _input_filters.end(), filter)
	!= _input_filters.end()) {
	debug_msg("filter already exists\n");
	return false;
    }

    _input_filters.push_back(filter);
    if (_input_filters.front() == filter) {
	string error_msg;
	if (RawSocket6::start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

bool
FilterRawSocket6::remove_filter(InputFilter* filter)
{
    list<InputFilter*>::iterator i;
    i = find(_input_filters.begin(), _input_filters.end(), filter);
    if (i == _input_filters.end()) {
	debug_msg("filter does not exist\n");
	return false;
    }

    _input_filters.erase(i);
    if (_input_filters.empty()) {
	string error_msg;
	if (RawSocket6::stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

int
FilterRawSocket6::proto_socket_write(const string&	if_name,
				     const string&	vif_name,
				     const IPv6&	src_address,
				     const IPv6&	dst_address,
				     int32_t		ip_ttl,
				     int32_t		ip_tos,
				     bool		ip_router_alert,
				     bool		ip_internet_control,
				     const vector<uint8_t>& ext_headers_type,
				     const vector<vector<uint8_t> >& ext_headers_payload,
				     const vector<uint8_t>& payload,
				     string&		error_msg)
{
    return (RawSocket6::proto_socket_write(if_name,
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
FilterRawSocket6::process_recv_data(const string&	if_name,
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
    struct IPv6HeaderInfo header;

    XLOG_ASSERT(src_address.is_ipv6());
    XLOG_ASSERT(dst_address.is_ipv6());

    header.if_name = if_name;
    header.vif_name = vif_name;
    header.src_address = src_address.get_ipv6();
    header.dst_address = dst_address.get_ipv6();
    header.ip_protocol = RawSocket6::ip_protocol();
    header.ip_ttl = ip_ttl;
    header.ip_tos = ip_tos;
    header.ip_router_alert = ip_router_alert;
    header.ip_internet_control = ip_internet_control;
    header.ext_headers_type = ext_headers_type;
    header.ext_headers_payload = ext_headers_payload;

    for (list<InputFilter*>::iterator i = _input_filters.begin();
	 i != _input_filters.end(); ++i) {
	(*i)->recv(header, payload);
    }
}

int
FilterRawSocket6::join_multicast_group(const string&	if_name,
				       const string&	vif_name,
				       const IPv6&	group_address,
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
FilterRawSocket6::leave_multicast_group(const string&	if_name,
					const string&	vif_name,
					const IPv6&	group_address,
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
