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

#ident "$XORP: xorp/fea/rawsock4.cc,v 1.22 2007/05/23 00:55:57 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "iftree.hh"
#include "rawsock4.hh"

/* ------------------------------------------------------------------------- */
/* RawSocket4 methods */

RawSocket4::RawSocket4(EventLoop& eventloop, uint8_t ip_protocol,
		       const IfTree& iftree)
    : IoIpSocket(eventloop, IPv4::af(), ip_protocol, iftree)
{
}

RawSocket4::~RawSocket4()
{
    string error_msg;

    IoIpSocket::stop(error_msg);
}

/* ------------------------------------------------------------------------- */
/* FilterRawSocket4 methods */

FilterRawSocket4::FilterRawSocket4(EventLoop& eventloop, uint8_t ip_protocol,
				   const IfTree& iftree)
    : RawSocket4(eventloop, ip_protocol, iftree)
{
}

FilterRawSocket4::~FilterRawSocket4()
{
    if (_input_filters.empty() == false) {
	string dummy_error_msg;
	IoIpSocket::stop(dummy_error_msg);

	do {
	    InputFilter* i = _input_filters.front();
	    _input_filters.erase(_input_filters.begin());
	    i->bye();
	} while (_input_filters.empty() == false);
    }
}

bool
FilterRawSocket4::add_filter(InputFilter* filter)
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
    i = find(_input_filters.begin(), _input_filters.end(), filter);
    if (i == _input_filters.end()) {
	debug_msg("filter does not exist\n");
	return false;
    }

    _input_filters.erase(i);
    if (_input_filters.empty()) {
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

    for (list<InputFilter*>::iterator i = _input_filters.begin();
	 i != _input_filters.end(); ++i) {
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
	if (IoIpSocket::join_multicast_group(if_name,
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
	if (IoIpSocket::leave_multicast_group(if_name,
					      vif_name,
					      IPvX(group_address),
					      error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

//
// Filter class for checking incoming raw packets and checking whether
// to forward them.
//
class VifInputFilter4 : public FilterRawSocket4::InputFilter {
public:
    VifInputFilter4(RawSocket4Manager&	raw_socket_manager,
		    FilterRawSocket4&	rs,
		    const string&	receiver_name,
		    const string&	if_name,
		    const string&	vif_name,
		    uint8_t		ip_protocol)
	: FilterRawSocket4::InputFilter(raw_socket_manager, receiver_name,
					ip_protocol),
	  _rs(rs),
	  _if_name(if_name),
	  _vif_name(vif_name),
	  _enable_multicast_loopback(false)
    {}

    virtual ~VifInputFilter4() {
	leave_all_multicast_groups();
    }

    void set_enable_multicast_loopback(bool v) { _enable_multicast_loopback = v; }

    void recv(const struct IPv4HeaderInfo& header,
	      const vector<uint8_t>& payload)
    {
	// Check the protocol
	if ((ip_protocol() != 0) && (ip_protocol() != header.ip_protocol)) {
	    debug_msg("Ignore packet with protocol %u (watching for %u)\n",
		      XORP_UINT_CAST(header.ip_protocol),
		      XORP_UINT_CAST(ip_protocol()));
	    return;
	}

	// Check the interface name
	if ((! _if_name.empty()) && (_if_name != header.if_name)) {
	    debug_msg("Ignore packet with interface %s (watching for %s)\n",
		      header.if_name.c_str(),
		      _if_name.c_str());
	    return;
	}

	// Check the vif name
	if ((! _vif_name.empty()) && (_vif_name != header.vif_name)) {
	    debug_msg("Ignore packet with vif %s (watching for %s)\n",
		      header.vif_name.c_str(),
		      _vif_name.c_str());
	    return;
	}

	// Check if multicast loopback is enabled
	if (header.dst_address.is_multicast()
	    && is_my_address(header.src_address)
	    && (! _enable_multicast_loopback)) {
	    debug_msg("Ignore packet with src %s dst %s: "
		      "multicast loopback is disabled\n",
		      header.src_address.str().c_str(),
		      header.dst_address.str().c_str());
	    return;
	}

	// Forward the packet
	raw_socket_manager().recv_and_forward(receiver_name(), header,
					      payload);
    }

    void bye() {}
    const string& if_name() const { return _if_name; }
    const string& vif_name() const { return _vif_name; }

    int join_multicast_group(const IPv4& group_address, string& error_msg) {
	if (_rs.join_multicast_group(if_name(), vif_name(), group_address,
				     receiver_name(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	_joined_multicast_groups.insert(group_address);
	return (XORP_OK);
    }

    int leave_multicast_group(const IPv4& group_address, string& error_msg) {
	_joined_multicast_groups.erase(group_address);
	if (_rs.leave_multicast_group(if_name(), vif_name(), group_address,
				      receiver_name(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }

    void leave_all_multicast_groups() {
	string error_msg;
	while (! _joined_multicast_groups.empty()) {
	    const IPv4& group_address = *(_joined_multicast_groups.begin());
	    leave_multicast_group(group_address, error_msg);
	}
    }

protected:
    bool is_my_address(const IPv4& addr) const {
	const IfTreeInterface* ifp = NULL;
	const IfTreeVif* vifp = NULL;

	if (_rs.find_interface_vif_by_addr(IPvX(addr), ifp, vifp) != true) {
	    return (false);
	}
	if (ifp->enabled() && vifp->enabled()) {
	    const IfTreeAddr4* ap = vifp->find_addr(addr);
	    if (ap != NULL) {
		if (ap->enabled())
		    return (true);
	    }
	}
	return (false);
    }

    FilterRawSocket4&	_rs;
    const string	_if_name;
    const string	_vif_name;
    set<IPv4>		_joined_multicast_groups;
    bool		_enable_multicast_loopback;
};


// ----------------------------------------------------------------------------
// RawSocket4Manager code

RawSocket4Manager::RawSocket4Manager(EventLoop&		eventloop,
				     const IfTree&	iftree)
    : _eventloop(eventloop),
      _iftree(iftree)
{
}

RawSocket4Manager::~RawSocket4Manager()
{
    erase_filters(_filters.begin(), _filters.end());
}

void
RawSocket4Manager::erase_filters_by_name(const string& receiver_name)
{
    erase_filters(_filters.lower_bound(receiver_name),
		  _filters.upper_bound(receiver_name));
}

void
RawSocket4Manager::erase_filters(const FilterBag4::iterator& begin,
				 const FilterBag4::iterator& end)
{
    FilterBag4::iterator fi(begin);
    while (fi != end) {
	FilterRawSocket4::InputFilter* filter = fi->second;

	SocketTable4::iterator sti = _sockets.find(filter->ip_protocol());
	XLOG_ASSERT(sti != _sockets.end());
	FilterRawSocket4* rs = sti->second;
	XLOG_ASSERT(rs != NULL);

	rs->remove_filter(filter);
	delete filter;

	_filters.erase(fi++);
    }
}

int
RawSocket4Manager::send(const string&	if_name,
			const string&	vif_name,
			const IPv4&	src_address,
			const IPv4&	dst_address,
			uint8_t		ip_protocol,
			int32_t		ip_ttl,
			int32_t		ip_tos,
			bool		ip_router_alert,
			bool		ip_internet_control,
			const vector<uint8_t>& payload,
			string&		error_msg)
{
    // Find the socket associated with this protocol
    SocketTable4::iterator sti = _sockets.find(ip_protocol);
    if (sti == _sockets.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    FilterRawSocket4* rs = sti->second;
    XLOG_ASSERT(rs != NULL);

    if (rs->proto_socket_write(if_name,
			       vif_name,
			       src_address,
			       dst_address,
			       ip_ttl,
			       ip_tos,
			       ip_router_alert,
			       ip_internet_control,
			       payload,
			       error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
RawSocket4Manager::register_receiver(const string&	receiver_name,
				     const string&	if_name,
				     const string&	vif_name,
				     uint8_t		ip_protocol,
				     bool		enable_multicast_loopback,
				     string&		error_msg)
{
    VifInputFilter4* filter;

    error_msg = "";

    //
    // Look in the SocketTable for a socket matching this protocol.
    // If a socket does not yet exist, create one.
    //
    SocketTable4::iterator sti = _sockets.find(ip_protocol);
    FilterRawSocket4* rs = NULL;
    if (sti == _sockets.end()) {
	rs = new FilterRawSocket4(_eventloop, ip_protocol, iftree());
	_sockets[ip_protocol] = rs;
    } else {
	rs = sti->second;
    }
    XLOG_ASSERT(rs != NULL);

    //
    // Search if we have already the filter
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<VifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Already have this filter
	    filter->set_enable_multicast_loopback(enable_multicast_loopback);
	    return (XORP_OK);
	}
    }

    //
    // Create the filter
    //
    filter = new VifInputFilter4(*this, *rs, receiver_name, if_name,
				 vif_name, ip_protocol);
    filter->set_enable_multicast_loopback(enable_multicast_loopback);

    // Add the filter to the appropriate raw socket
    rs->add_filter(filter);

    // Add the filter to those associated with receiver_name
    _filters.insert(FilterBag4::value_type(receiver_name, filter));

    return (XORP_OK);
}

int
RawSocket4Manager::unregister_receiver(const string&	receiver_name,
				       const string&	if_name,
				       const string&	vif_name,
				       uint8_t		ip_protocol,
				       string&		error_msg)
{
    //
    // Find the socket associated with this protocol
    //
    SocketTable4::iterator sti = _sockets.find(ip_protocol);
    if (sti == _sockets.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    FilterRawSocket4* rs = sti->second;
    XLOG_ASSERT(rs != NULL);

    //
    // Walk through list of filters looking for matching interface and vif
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter4* filter;
	filter = dynamic_cast<VifInputFilter4*>(fi->second);

	// If filter found, remove it and delete it
	if ((filter != NULL) &&
	    (filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    
	    // Remove the filter
	    rs->remove_filter(filter);

	    // Remove the filter from the group associated with this receiver
	    _filters.erase(fi);

	    // Destruct the filter
	    delete filter;

	    //
	    // Reference counting: if there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    //
	    if (rs->no_input_filters()) {
		_sockets.erase(ip_protocol);
		delete rs;
	    }

	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot find registration for receiver %s "
			 "interface %s and vif %s",
			 receiver_name.c_str(),
			 if_name.c_str(),
			 vif_name.c_str());
    return (XORP_ERROR);
}

int
RawSocket4Manager::join_multicast_group(const string&	receiver_name,
					const string&	if_name,
					const string&	vif_name,
					uint8_t		ip_protocol,
					const IPv4&	group_address,
					string&		error_msg)
{
    //
    // Search if we have already the filter
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter4* filter;
	filter = dynamic_cast<VifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Filter found
	    if (filter->join_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot join group %s on interface %s vif %s "
			 "protocol %u receiver %s: not registered",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ip_protocol),
			 receiver_name.c_str());
    return (XORP_ERROR);
}

int
RawSocket4Manager::leave_multicast_group(const string&	receiver_name,
					 const string&	if_name,
					 const string&	vif_name,
					 uint8_t	ip_protocol,
					 const IPv4&	group_address,
					 string&	error_msg)
{
    //
    // Search if we have already the filter
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter4* filter;
	filter = dynamic_cast<VifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Filter found
	    if (filter->leave_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot leave group %s on interface %s vif %s "
			 "protocol %u receiver %s: not registered",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ip_protocol),
			 receiver_name.c_str());
    return (XORP_ERROR);
}
