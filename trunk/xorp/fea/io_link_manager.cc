// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "iftree.hh"
#include "io_link_manager.hh"

//
// Filter class for checking incoming raw link-level packets and checking
// whether to forward them.
//
class VifInputFilter : public IoLinkComm::InputFilter {
public:
    VifInputFilter(IoLinkManager&	io_link_manager,
		   IoLinkComm&		io_link_comm,
		   const string&	receiver_name,
		   const string&	if_name,
		   const string&	vif_name,
		   uint16_t		ether_type,
		   const string&	filter_program)
	: IoLinkComm::InputFilter(io_link_manager, receiver_name, if_name,
				  vif_name, ether_type, filter_program),
	  _io_link_comm(io_link_comm),
	  _enable_multicast_loopback(false)
    {}

    virtual ~VifInputFilter() {
	leave_all_multicast_groups();
    }

    void set_enable_multicast_loopback(bool v) {
	_enable_multicast_loopback = v;
    }

    void recv(const struct MacHeaderInfo& header,
	      const vector<uint8_t>& payload)
    {
	// Check the EtherType protocol
	if ((ether_type() != 0) && (ether_type() != header.ether_type)) {
	    debug_msg("Ignore packet with EtherType protocol %u (watching for %u)\n",
		      XORP_UINT_CAST(header.ether_type),
		      XORP_UINT_CAST(ether_type()));
	    return;
	}

	// Check if multicast loopback is enabled
	if (header.dst_address.is_multicast()
	    && is_my_address(header.src_address)
	    && (! _enable_multicast_loopback)) {
	    debug_msg("Ignore raw link-level packet with src %s dst %s: "
		      "multicast loopback is disabled\n",
		      header.src_address.str().c_str(),
		      header.dst_address.str().c_str());
	    return;
	}

	// Forward the packet
	io_link_manager().send_to_receiver(receiver_name(), header, payload);
    }

    void bye() {}

    int join_multicast_group(const Mac& group_address, string& error_msg) {
	if (_io_link_comm.join_multicast_group(group_address, receiver_name(),
					       error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	_joined_multicast_groups.insert(group_address);
	return (XORP_OK);
    }

    int leave_multicast_group(const Mac& group_address, string& error_msg) {
	_joined_multicast_groups.erase(group_address);
	if (_io_link_comm.leave_multicast_group(group_address, receiver_name(),
						error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }

    void leave_all_multicast_groups() {
	string error_msg;
	while (! _joined_multicast_groups.empty()) {
	    const Mac& group_address = *(_joined_multicast_groups.begin());
	    leave_multicast_group(group_address, error_msg);
	}
    }

protected:
    bool is_my_address(const Mac& addr) const {
	const IfTreeInterface* ifp;

	ifp = io_link_manager().iftree().find_interface(if_name());
	if (ifp == NULL)
	    return (false);

	if (! ifp->enabled())
	    return (false);

	return (ifp->mac() == addr);
    }

    IoLinkComm&		_io_link_comm;
    set<Mac>		_joined_multicast_groups;
    bool		_enable_multicast_loopback;
};

/* ------------------------------------------------------------------------- */
/* IoLinkComm methods */

IoLinkComm::IoLinkComm(EventLoop& eventloop, const IfTree& iftree,
		       const string& if_name, const string& vif_name,
		       uint16_t ether_type, const string& filter_program)
    : IoLinkPcap(eventloop, iftree, if_name, vif_name, ether_type,
		 filter_program)
{
}

IoLinkComm::~IoLinkComm()
{
    if (_input_filters.empty() == false) {
	string dummy_error_msg;
	IoLinkPcap::stop(dummy_error_msg);

	do {
	    InputFilter* i = _input_filters.front();
	    _input_filters.erase(_input_filters.begin());
	    i->bye();
	} while (_input_filters.empty() == false);
    }
}

bool
IoLinkComm::add_filter(InputFilter* filter)
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
	if (IoLinkPcap::start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

bool
IoLinkComm::remove_filter(InputFilter* filter)
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
	if (IoLinkPcap::stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    return false;
	}
    }
    return true;
}

int
IoLinkComm::send_packet(const Mac&	src_address,
			const Mac&	dst_address,
			uint16_t	ether_type,
			const vector<uint8_t>& payload,
			string&		error_msg)
{
    return (IoLinkPcap::send_packet(src_address,
				    dst_address,
				    ether_type,
				    payload,
				    error_msg));
}

void
IoLinkComm::process_recv_data(const Mac&	src_address,
			      const Mac&	dst_address,
			      uint16_t		ether_type,
			      const vector<uint8_t>& payload)
{
    struct MacHeaderInfo header;

    header.if_name = if_name();
    header.vif_name = vif_name();
    header.src_address = src_address;
    header.dst_address = dst_address;
    header.ether_type = ether_type;

    for (list<InputFilter*>::iterator i = _input_filters.begin();
	 i != _input_filters.end(); ++i) {
	(*i)->recv(header, payload);
    }
}

int
IoLinkComm::join_multicast_group(const Mac&	group_address,
				 const string&	receiver_name,
				 string&	error_msg)
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
    if (receiver_name.empty()) {
	error_msg = c_format("Cannot join group %s on interface %s vif %s: "
			     "empty receiver name",
			     group_address.str().c_str(),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }

    JoinedMulticastGroup init_jmg(group_address);
    iter = _joined_groups_table.find(init_jmg);
    if (iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	if (IoLinkPcap::join_multicast_group(group_address, error_msg)
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
IoLinkComm::leave_multicast_group(const Mac&	group_address,
				  const string&	receiver_name,
				  string&	error_msg)
{
    JoinedGroupsTable::iterator iter;

    JoinedMulticastGroup init_jmg(group_address);
    iter = _joined_groups_table.find(init_jmg);
    if (iter == _joined_groups_table.end()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "the group was not joined",
			     group_address.str().c_str(),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
    JoinedMulticastGroup& jmg = iter->second;

    jmg.delete_receiver(receiver_name);
    if (jmg.empty()) {
	//
	// The last receiver, hence leave the group
	//
	_joined_groups_table.erase(iter);
	if (IoLinkPcap::leave_multicast_group(group_address, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

// ----------------------------------------------------------------------------
// IoLinkManager code

IoLinkManager::IoLinkManager(EventLoop&		eventloop,
			     const IfTree&	iftree)
    : _eventloop(eventloop),
      _iftree(iftree),
      _send_to_receiver_base(NULL)
{
}

IoLinkManager::~IoLinkManager()
{
    erase_filters(_comm_table, _filters, _filters.begin(), _filters.end());
}

void
IoLinkManager::send_to_receiver(const string&			receiver_name,
				const struct MacHeaderInfo&	header,
				const vector<uint8_t>&		payload)
{
    if (_send_to_receiver_base != NULL)
	_send_to_receiver_base->send_to_receiver(receiver_name, header,
						 payload);
}

void
IoLinkManager::erase_filters_by_name(const string& receiver_name)
{
    erase_filters(_comm_table, _filters,
		  _filters.lower_bound(receiver_name),
		  _filters.upper_bound(receiver_name));
}

void
IoLinkManager::erase_filters(CommTable& comm_table, FilterBag& filters,
			     const FilterBag::iterator& begin,
			     const FilterBag::iterator& end)
{
    FilterBag::iterator fi(begin);
    while (fi != end) {
	IoLinkComm::InputFilter* filter = fi->second;

	CommTableKey key(filter->if_name(), filter->vif_name(),
			 filter->ether_type(), filter->filter_program());

	CommTable::iterator cti = comm_table.find(key);
	XLOG_ASSERT(cti != comm_table.end());
	IoLinkComm* io_link_comm = cti->second;
	XLOG_ASSERT(io_link_comm != NULL);

	io_link_comm->remove_filter(filter);
	delete filter;

	filters.erase(fi++);

	//
	// Reference counting: if there are now no listeners on
	// this protocol socket (and hence no filters), remove it
	// from the table and delete it.
	//
	if (io_link_comm->no_input_filters()) {
	    comm_table.erase(key);
	    delete io_link_comm;
	}
    }
}

int
IoLinkManager::send(const string&	if_name,
		    const string&	vif_name,
		    const Mac&		src_address,
		    const Mac&		dst_address,
		    uint16_t		ether_type,
		    const vector<uint8_t>& payload,
		    string&		error_msg)
{
    CommTableKey key(if_name, vif_name, ether_type, "");

    // Find the IoLinkComm associated with this protocol
    CommTable::iterator cti = _comm_table.find(key);

    //
    // XXX: Search the whole table in case the protocol was registered
    // only with some non-empty filter program.
    //
    if (cti == _comm_table.end()) {
	for (cti = _comm_table.begin(); cti != _comm_table.end(); ++cti) {
	    IoLinkComm* c = cti->second;
	    if ((c->if_name() == if_name)
		&& (c->vif_name() == vif_name)
		&& (c->ether_type() == ether_type)) {
		break;
	    }
	}
    }

    if (cti == _comm_table.end()) {
	error_msg = c_format("EtherType protocol %u is not registered "
			     "on interface %s vif %s",
			     XORP_UINT_CAST(ether_type),
			     if_name.c_str(), vif_name.c_str());
	return (XORP_ERROR);
    }
    IoLinkComm* io_link_comm = cti->second;
    XLOG_ASSERT(io_link_comm != NULL);

    return (io_link_comm->send_packet(src_address,
				      dst_address,
				      ether_type,
				      payload,
				      error_msg));
}

int
IoLinkManager::register_receiver(const string&	receiver_name,
				 const string&	if_name,
				 const string&	vif_name,
				 uint16_t	ether_type,
				 const string&	filter_program,
				 bool		enable_multicast_loopback,
				 string&	error_msg)
{
    CommTableKey key(if_name, vif_name, ether_type, filter_program);
    VifInputFilter* filter;

    error_msg = "";

    //
    // Look in the CommTable for an entry matching this protocol.
    // If an entry does not yet exist, create one.
    //
    CommTable::iterator cti = _comm_table.find(key);
    IoLinkComm* io_link_comm = NULL;
    if (cti == _comm_table.end()) {
	io_link_comm = new IoLinkComm(_eventloop, iftree(), if_name, vif_name,
				      ether_type, filter_program);
	_comm_table[key] = io_link_comm;
    } else {
	io_link_comm = cti->second;
    }
    XLOG_ASSERT(io_link_comm != NULL);

    //
    // Walk through list of filters looking for matching filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	//
	// Search if we have already the filter
	//
	if ((filter->ether_type() == ether_type) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name) &&
	    (filter->filter_program() == filter_program)) {
	    // Already have this filter
	    filter->set_enable_multicast_loopback(enable_multicast_loopback);
	    return (XORP_OK);
	}
    }

    //
    // Create the filter
    //
    filter = new VifInputFilter(*this, *io_link_comm, receiver_name, if_name,
				vif_name, ether_type, filter_program);
    filter->set_enable_multicast_loopback(enable_multicast_loopback);

    // Add the filter to the appropriate IoLinkComm entry
    io_link_comm->add_filter(filter);

    // Add the filter to those associated with receiver_name
    _filters.insert(FilterBag::value_type(receiver_name, filter));

    return (XORP_OK);
}

int
IoLinkManager::unregister_receiver(const string&	receiver_name,
				   const string&	if_name,
				   const string&	vif_name,
				   uint16_t		ether_type,
				   const string&	filter_program,
				   string&		error_msg)
{
    CommTableKey key(if_name, vif_name, ether_type, filter_program);

    //
    // Find the IoLinkComm entry associated with this protocol
    //
    CommTable::iterator cti = _comm_table.find(key);
    if (cti == _comm_table.end()) {
	error_msg = c_format("EtherType protocol %u filter program %s "
			     "is not registered on interface %s vif %s",
			     XORP_UINT_CAST(ether_type),
			     filter_program.c_str(), if_name.c_str(),
			     vif_name.c_str());
	return (XORP_ERROR);
    }
    IoLinkComm* io_link_comm = cti->second;
    XLOG_ASSERT(io_link_comm != NULL);

    //
    // Walk through list of filters looking for matching interface and vif
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	// If filter found, remove it and delete it
	if ((filter->ether_type() == ether_type) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name) &&
	    (filter->filter_program() == filter_program)) {

	    // Remove the filter
	    io_link_comm->remove_filter(filter);

	    // Remove the filter from the group associated with this receiver
	    _filters.erase(fi);

	    // Destruct the filter
	    delete filter;

	    //
	    // Reference counting: if there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    //
	    if (io_link_comm->no_input_filters()) {
		_comm_table.erase(key);
		delete io_link_comm;
	    }

	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot find registration for receiver %s "
			 "EtherType protocol %u filter program %s "
			 "interface %s and vif %s",
			 receiver_name.c_str(),
			 XORP_UINT_CAST(ether_type),
			 filter_program.c_str(),
			 if_name.c_str(),
			 vif_name.c_str());
    return (XORP_ERROR);
}

int
IoLinkManager::join_multicast_group(const string&	receiver_name,
				    const string&	if_name,
				    const string&	vif_name,
				    uint16_t		ether_type,
				    const string&	filter_program,
				    const Mac&		group_address,
				    string&		error_msg)
{
    //
    // Search if we have already the filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ether_type() == ether_type) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name) &&
	    (filter->filter_program() == filter_program)) {
	    // Filter found
	    if (filter->join_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot join group %s on interface %s vif %s "
			 "protocol %u filter program %s receiver %s: "
			 "not registered",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ether_type),
			 filter_program.c_str(),
			 receiver_name.c_str());
    return (XORP_ERROR);
}

int
IoLinkManager::leave_multicast_group(const string&	receiver_name,
				     const string&	if_name,
				     const string&	vif_name,
				     uint16_t		ether_type,
				     const string&	filter_program,
				     const Mac&		group_address,
				     string&		error_msg)
{
    //
    // Search if we have already the filter
    //
    FilterBag::iterator fi;
    FilterBag::iterator fi_end = _filters.upper_bound(receiver_name);
    for (fi = _filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	VifInputFilter* filter;
	filter = dynamic_cast<VifInputFilter*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ether_type() == ether_type) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name) &&
	    (filter->filter_program() == filter_program)) {
	    // Filter found
	    if (filter->leave_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot leave group %s on interface %s vif %s "
			 "protocol %u filter program %s receiver %s: "
			 "not registered",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ether_type),
			 filter_program.c_str(),
			 receiver_name.c_str());
    return (XORP_ERROR);
}
