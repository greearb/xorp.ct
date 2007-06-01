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

#ident "$XORP: xorp/fea/io_ip_manager.cc,v 1.2 2007/05/26 02:10:26 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "iftree.hh"
#include "io_ip_manager.hh"

//
// Filter class for checking incoming raw packets and checking whether
// to forward them.
//
class VifInputFilter : public IoIpComm::InputFilter {
public:
    VifInputFilter(IoIpManager&		io_ip_manager,
		   IoIpComm&		io_ip_comm,
		   const string&	receiver_name,
		   const string&	if_name,
		   const string&	vif_name,
		   uint8_t		ip_protocol)
	: IoIpComm::InputFilter(io_ip_manager, receiver_name, ip_protocol),
	  _io_ip_comm(io_ip_comm),
	  _if_name(if_name),
	  _vif_name(vif_name),
	  _enable_multicast_loopback(false)
    {}

    virtual ~VifInputFilter() {
	leave_all_multicast_groups();
    }

    void set_enable_multicast_loopback(bool v) {
	_enable_multicast_loopback = v;
    }

    void recv(const struct IPvXHeaderInfo& header,
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
	io_ip_manager().send_to_receiver(receiver_name(), header, payload);
    }

    void recv_system_multicast_upcall(const vector<uint8_t>& payload) {
	// XXX: nothing to do
	UNUSED(payload);
    }

    void bye() {}
    const string& if_name() const { return _if_name; }
    const string& vif_name() const { return _vif_name; }

    int join_multicast_group(const IPvX& group_address, string& error_msg) {
	if (_io_ip_comm.join_multicast_group(if_name(), vif_name(),
					     group_address, receiver_name(),
					     error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	_joined_multicast_groups.insert(group_address);
	return (XORP_OK);
    }

    int leave_multicast_group(const IPvX& group_address, string& error_msg) {
	_joined_multicast_groups.erase(group_address);
	if (_io_ip_comm.leave_multicast_group(if_name(), vif_name(),
					      group_address, receiver_name(),
					      error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }

    void leave_all_multicast_groups() {
	string error_msg;
	while (! _joined_multicast_groups.empty()) {
	    const IPvX& group_address = *(_joined_multicast_groups.begin());
	    leave_multicast_group(group_address, error_msg);
	}
    }

protected:
    bool is_my_address(const IPvX& addr) const {
	const IfTreeInterface* ifp = NULL;
	const IfTreeVif* vifp = NULL;

	if (_io_ip_comm.find_interface_vif_by_addr(IPvX(addr), ifp, vifp)
	    != true) {
	    return (false);
	}
	if (! (ifp->enabled() && vifp->enabled()))
	    return (false);
	if (addr.is_ipv4()) {
	    const IfTreeAddr4* ap = vifp->find_addr(addr.get_ipv4());
	    if ((ap != NULL) && (ap->enabled()))
		return (true);
	    return (false);
	}
	if (addr.is_ipv6()) {
	    const IfTreeAddr6* ap = vifp->find_addr(addr.get_ipv6());
	    if ((ap != NULL) && (ap->enabled()))
		return (true);
	    return (false);
	}
	return (false);
    }

    IoIpComm&		_io_ip_comm;
    const string	_if_name;
    const string	_vif_name;
    set<IPvX>           _joined_multicast_groups;
    bool		_enable_multicast_loopback;
};

//
// Filter class for checking system multicast upcalls and forwarding them.
//
class SystemMulticastUpcallFilter : public IoIpComm::InputFilter {
public:
    SystemMulticastUpcallFilter(IoIpManager&	io_ip_manager,
				IoIpComm&	io_ip_comm,
				uint8_t		ip_protocol,
				IoIpManager::UpcallReceiverCb& receiver_cb)
	: IoIpComm::InputFilter(io_ip_manager, "", ip_protocol),
	  _io_ip_comm(io_ip_comm),
	  _receiver_cb(receiver_cb)
    {}

    virtual ~SystemMulticastUpcallFilter() {}

    void set_receiver_cb(IoIpManager::UpcallReceiverCb& receiver_cb) {
	_receiver_cb = receiver_cb;
    }

    void recv(const struct IPvXHeaderInfo& header,
	      const vector<uint8_t>& payload)
    {
	// XXX: nothing to do
	UNUSED(header);
	UNUSED(payload);
    }

    void recv_system_multicast_upcall(const vector<uint8_t>& payload) {
	// Forward the upcall
	if (! _receiver_cb.is_empty())
	    _receiver_cb->dispatch(&payload[0], payload.size());
    }

    void bye() {}

protected:
    IoIpComm&		_io_ip_comm;
    IoIpManager::UpcallReceiverCb	_receiver_cb;
};

/* ------------------------------------------------------------------------- */
/* IoIpComm methods */

IoIpComm::IoIpComm(EventLoop& eventloop, int family, uint8_t ip_protocol,
		   const IfTree& iftree)
    : IoIpSocket(eventloop, family, ip_protocol, iftree)
{
}

IoIpComm::~IoIpComm()
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
IoIpComm::add_filter(InputFilter* filter)
{
    if (filter == NULL) {
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
	if (IoIpComm::start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

bool
IoIpComm::remove_filter(InputFilter* filter)
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
	if (IoIpSocket::stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

int
IoIpComm::proto_socket_write(const string&	if_name,
			     const string&	vif_name,
			     const IPvX&	src_address,
			     const IPvX&	dst_address,
			     int32_t		ip_ttl,
			     int32_t		ip_tos,
			     bool		ip_router_alert,
			     bool		ip_internet_control,
			     const vector<uint8_t>& ext_headers_type,
			     const vector<vector<uint8_t> >& ext_headers_payload,
			     const vector<uint8_t>& payload,
			     string&		error_msg)
{
    return (IoIpSocket::proto_socket_write(if_name,
					   vif_name,
					   src_address,
					   dst_address,
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
IoIpComm::process_recv_data(const string&	if_name,
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
    struct IPvXHeaderInfo header;

    header.if_name = if_name;
    header.vif_name = vif_name;
    header.src_address = src_address;
    header.dst_address = dst_address;
    header.ip_protocol = IoIpSocket::ip_protocol();
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

void
IoIpComm::process_system_multicast_upcall(const vector<uint8_t>& payload)
{
    for (list<InputFilter*>::iterator i = _input_filters.begin();
	 i != _input_filters.end(); ++i) {
	(*i)->recv_system_multicast_upcall(payload);
    }
}

int
IoIpComm::join_multicast_group(const string&	if_name,
			       const string&	vif_name,
			       const IPvX&	group_address,
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
					     group_address,
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
IoIpComm::leave_multicast_group(const string&	if_name,
				const string&	vif_name,
				const IPvX&	group_address,
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
					      group_address,
					      error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

// ----------------------------------------------------------------------------
// IoIpManager code

IoIpManager::IoIpManager(EventLoop&	eventloop,
			 const IfTree&	iftree)
    : _eventloop(eventloop),
      _iftree(iftree),
      _send_to_receiver_base(NULL)
{
}

IoIpManager::~IoIpManager()
{
    erase_filters(_comm_table4, _filters4, _filters4.begin(), _filters4.end());
    erase_filters(_comm_table6, _filters6, _filters6.begin(), _filters6.end());
}

IoIpManager::CommTable&
IoIpManager::comm_table_by_family(int family)
{
    if (family == IPv4::af())
	return (_comm_table4);
    if (family == IPv6::af())
	return (_comm_table6);

    XLOG_FATAL("Invalid address family: %d", family);
    return (_comm_table4);
}

IoIpManager::FilterBag&
IoIpManager::filters_by_family(int family)
{
    if (family == IPv4::af())
	return (_filters4);
    if (family == IPv6::af())
	return (_filters6);

    XLOG_FATAL("Invalid address family: %d", family);
    return (_filters4);
}

void
IoIpManager::send_to_receiver(const string&			receiver_name,
			      const struct IPvXHeaderInfo&	header,
			      const vector<uint8_t>&		payload)
{
    if (_send_to_receiver_base != NULL)
	_send_to_receiver_base->send_to_receiver(receiver_name, header,
						 payload);
}

void
IoIpManager::erase_filters_by_name(const string& receiver_name, int family)
{
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    erase_filters(comm_table, filters,
		  filters.lower_bound(receiver_name),
		  filters.upper_bound(receiver_name));
}

void
IoIpManager::erase_filters(CommTable& comm_table, FilterBag& filters,
			   const FilterBag::iterator& begin,
			   const FilterBag::iterator& end)
{
    FilterBag::iterator fi(begin);
    while (fi != end) {
	IoIpComm::InputFilter* filter = fi->second;

	CommTable::iterator cti = comm_table.find(filter->ip_protocol());
	XLOG_ASSERT(cti != comm_table.end());
	IoIpComm* io_ip_comm = cti->second;
	XLOG_ASSERT(io_ip_comm != NULL);

	io_ip_comm->remove_filter(filter);
	delete filter;

	filters.erase(fi++);

	//
	// Reference counting: if there are now no listeners on
	// this protocol socket (and hence no filters), remove it
	// from the table and delete it.
	//
	if (io_ip_comm->no_input_filters()) {
	    comm_table.erase(io_ip_comm->ip_protocol());
	    delete io_ip_comm;
	}
    }
}

int
IoIpManager::send(const string&		if_name,
		  const string&		vif_name,
		  const IPvX&		src_address,
		  const IPvX&		dst_address,
		  uint8_t		ip_protocol,
		  int32_t		ip_ttl,
		  int32_t		ip_tos,
		  bool			ip_router_alert,
		  bool			ip_internet_control,
		  const vector<uint8_t>& ext_headers_type,
		  const vector<vector<uint8_t> >& ext_headers_payload,
		  const vector<uint8_t>& payload,
		  string&		error_msg)
{
    CommTable& comm_table = comm_table_by_family(src_address.af());

    // Find the IoIpComm associated with this protocol
    CommTable::iterator cti = comm_table.find(ip_protocol);
    if (cti == comm_table.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    IoIpComm* io_ip_comm = cti->second;
    XLOG_ASSERT(io_ip_comm != NULL);

    return (io_ip_comm->proto_socket_write(if_name,
					   vif_name,
					   src_address,
					   dst_address,
					   ip_ttl,
					   ip_tos,
					   ip_router_alert,
					   ip_internet_control,
					   ext_headers_type,
					   ext_headers_payload,
					   payload,
					   error_msg));
}

int
IoIpManager::register_receiver(int		family,
			       const string&	receiver_name,
			       const string&	if_name,
			       const string&	vif_name,
			       uint8_t		ip_protocol,
			       bool		enable_multicast_loopback,
			       string&		error_msg)
{
    VifInputFilter* filter;
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    error_msg = "";

    //
    // Look in the CommTable for an entry matching this protocol.
    // If an entry does not yet exist, create one.
    //
    CommTable::iterator cti = comm_table.find(ip_protocol);
    IoIpComm* io_ip_comm = NULL;
    if (cti == comm_table.end()) {
	io_ip_comm = new IoIpComm(_eventloop, family, ip_protocol, iftree());
	comm_table[ip_protocol] = io_ip_comm;
    } else {
	io_ip_comm = cti->second;
    }
    XLOG_ASSERT(io_ip_comm != NULL);

    //
    // Walk through list of filters looking for matching filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	//
	// Search if we have already the filter
	//
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
    filter = new VifInputFilter(*this, *io_ip_comm, receiver_name, if_name,
				vif_name, ip_protocol);
    filter->set_enable_multicast_loopback(enable_multicast_loopback);

    // Add the filter to the appropriate IoIpComm entry
    io_ip_comm->add_filter(filter);

    // Add the filter to those associated with receiver_name
    filters.insert(FilterBag::value_type(receiver_name, filter));

    return (XORP_OK);
}

int
IoIpManager::unregister_receiver(int		family,
				 const string&	receiver_name,
				 const string&	if_name,
				 const string&	vif_name,
				 uint8_t	ip_protocol,
				 string&	error_msg)
{
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    //
    // Find the IoIpComm entry associated with this protocol
    //
    CommTable::iterator cti = comm_table.find(ip_protocol);
    if (cti == comm_table.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    IoIpComm* io_ip_comm = cti->second;
    XLOG_ASSERT(io_ip_comm != NULL);

    //
    // Walk through list of filters looking for matching interface and vif
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	// If filter found, remove it and delete it
	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {

	    // Remove the filter
	    io_ip_comm->remove_filter(filter);

	    // Remove the filter from the group associated with this receiver
	    filters.erase(fi);

	    // Destruct the filter
	    delete filter;

	    //
	    // Reference counting: if there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    //
	    if (io_ip_comm->no_input_filters()) {
		comm_table.erase(ip_protocol);
		delete io_ip_comm;
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
IoIpManager::join_multicast_group(const string&	receiver_name,
				  const string&	if_name,
				  const string&	vif_name,
				  uint8_t	ip_protocol,
				  const IPvX&	group_address,
				  string&	error_msg)
{
    FilterBag& filters = filters_by_family(group_address.af());

    //
    // Search if we have already the filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
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
IoIpManager::leave_multicast_group(const string&	receiver_name,
				   const string&	if_name,
				   const string&	vif_name,
				   uint8_t		ip_protocol,
				   const IPvX&		group_address,
				   string&		error_msg)
{
    FilterBag& filters = filters_by_family(group_address.af());

    //
    // Search if we have already the filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
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

int
IoIpManager::register_system_multicast_upcall_receiver(
    int		family,
    uint8_t	ip_protocol,
    IoIpManager::UpcallReceiverCb receiver_cb,
    XorpFd&	receiver_fd,
    string&	error_msg)
{
    SystemMulticastUpcallFilter* filter;
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    error_msg = "";

    //
    // Look in the CommTable for an entry matching this protocol.
    // If an entry does not yet exist, create one.
    //
    CommTable::iterator cti = comm_table.find(ip_protocol);
    IoIpComm* io_ip_comm = NULL;
    if (cti == comm_table.end()) {
	io_ip_comm = new IoIpComm(_eventloop, family, ip_protocol, iftree());
	comm_table[ip_protocol] = io_ip_comm;
    } else {
	io_ip_comm = cti->second;
    }
    XLOG_ASSERT(io_ip_comm != NULL);

    //
    // Walk through list of filters looking for matching filter
    //
    string receiver_name;		// XXX: empty receiver name
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<SystemMulticastUpcallFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not an upcall filter

	//
	// Search if we have already the filter
	//
	if (filter->ip_protocol() == ip_protocol) {
	    // Already have this filter
	    filter->set_receiver_cb(receiver_cb);
	    receiver_fd = io_ip_comm->proto_socket_in();
	    return (XORP_OK);
	}
    }

    //
    // Create the filter
    //
    filter = new SystemMulticastUpcallFilter(*this, *io_ip_comm, ip_protocol,
					     receiver_cb);

    // Add the filter to the appropriate IoIpComm entry
    io_ip_comm->add_filter(filter);

    // Add the filter to those associated with empty receiver_name
    filters.insert(FilterBag::value_type(receiver_name, filter));

    receiver_fd = io_ip_comm->proto_socket_in();

    return (XORP_OK);
}

int
IoIpManager::unregister_system_multicast_upcall_receiver(
    int		family,
    uint8_t	ip_protocol,
    string&	error_msg)
{
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    //
    // Find the IoIpComm entry associated with this protocol
    //
    CommTable::iterator cti = comm_table.find(ip_protocol);
    if (cti == comm_table.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    IoIpComm* io_ip_comm = cti->second;
    XLOG_ASSERT(io_ip_comm != NULL);

    //
    // Walk through list of filters looking for matching filter
    //
    string receiver_name;		// XXX: empty receiver name
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	SystemMulticastUpcallFilter* filter;
	filter = dynamic_cast<SystemMulticastUpcallFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not an upcall filter

	// If filter found, remove it and delete it
	if (filter->ip_protocol() == ip_protocol) {
	    // Remove the filter
	    io_ip_comm->remove_filter(filter);

	    // Remove the filter from the group associated with this receiver
	    filters.erase(fi);

	    // Destruct the filter
	    delete filter;

	    //
	    // Reference counting: if there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    //
	    if (io_ip_comm->no_input_filters()) {
		comm_table.erase(ip_protocol);
		delete io_ip_comm;
	    }

	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot find registration for upcall receiver "
			 "family %d and protocol %d",
			 family,
			 ip_protocol);
    return (XORP_ERROR);
}
