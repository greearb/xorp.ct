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

#ident "$XORP: xorp/fea/io_link_manager.cc,v 1.1 2007/06/27 01:27:05 pavlin Exp $"

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

IoLinkComm::IoLinkComm(IoLinkManager& io_link_manager, const IfTree& iftree,
		       const string& if_name, const string& vif_name,
		       uint16_t ether_type, const string& filter_program)
    : IoLinkReceiver(),
      _io_link_manager(io_link_manager),
      _iftree(iftree),
      _if_name(if_name),
      _vif_name(vif_name),
      _ether_type(ether_type),
      _filter_program(filter_program)
{
}

IoLinkComm::~IoLinkComm()
{
    IoLinkPlugins::iterator iter;

    deallocate_io_link_plugins();

    while (_input_filters.empty() == false) {
	InputFilter* i = _input_filters.front();
	_input_filters.erase(_input_filters.begin());
	i->bye();
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

    //
    // Allocate and start the IoLink plugins: one per data plane manager.
    //
    if (_input_filters.front() == filter) {
	XLOG_ASSERT(_io_link_plugins.empty());
	allocate_io_link_plugins();
	start_io_link_plugins();
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

    XLOG_ASSERT(! _io_link_plugins.empty());

    _input_filters.erase(i);
    if (_input_filters.empty()) {
	deallocate_io_link_plugins();
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
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_link_plugins.empty()) {
	error_msg = c_format("No I/O Link plugin to send a link raw packet on "
			     "interface %s vif %s from %s to %s EtherType %u",
			     if_name().c_str(), vif_name().c_str(),
			     src_address.str().c_str(),
			     dst_address.str().c_str(),
			     ether_type);
	return (XORP_ERROR);
    }

    IoLinkPlugins::iterator iter;
    for (iter = _io_link_plugins.begin();
	 iter != _io_link_plugins.end();
	 ++iter) {
	IoLink* io_link = iter->second;
	if (io_link->send_packet(src_address,
				 dst_address,
				 ether_type,
				 payload,
				 error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

void
IoLinkComm::recv_packet(const Mac&		src_address,
			const Mac&		dst_address,
			uint16_t		ether_type,
			const vector<uint8_t>&	payload)
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
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_link_plugins.empty()) {
	error_msg = c_format("No I/O Link plugin to join group %s "
			     "on interface %s vif %s EtherType %u "
			     "receiver name %s",
			     group_address.str().c_str(),
			     if_name().c_str(), vif_name().c_str(),
			     ether_type(), receiver_name.c_str());
	return (XORP_ERROR);
    }

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

    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(group_address);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	IoLinkPlugins::iterator plugin_iter;
	for (plugin_iter = _io_link_plugins.begin();
	     plugin_iter != _io_link_plugins.end();
	     ++plugin_iter) {
	    IoLink* io_link = plugin_iter->second;
	    if (io_link->join_multicast_group(group_address, error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
	_joined_groups_table.insert(make_pair(init_jmg, init_jmg));
	joined_iter = _joined_groups_table.find(init_jmg);
    }
    XLOG_ASSERT(joined_iter != _joined_groups_table.end());
    JoinedMulticastGroup& jmg = joined_iter->second;

    jmg.add_receiver(receiver_name);

    return (ret_value);
}

int
IoLinkComm::leave_multicast_group(const Mac&	group_address,
				  const string&	receiver_name,
				  string&	error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_link_plugins.empty()) {
	error_msg = c_format("No I/O Link plugin to leave group %s "
			     "on interface %s vif %s EtherType %u "
			     "receiver name %s",
			     group_address.str().c_str(),
			     if_name().c_str(), vif_name().c_str(),
			     ether_type(), receiver_name.c_str());
	return (XORP_ERROR);
    }

    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(group_address);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "the group was not joined",
			     group_address.str().c_str(),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
    JoinedMulticastGroup& jmg = joined_iter->second;

    jmg.delete_receiver(receiver_name);
    if (jmg.empty()) {
	//
	// The last receiver, hence leave the group
	//
	_joined_groups_table.erase(joined_iter);

	IoLinkPlugins::iterator plugin_iter;
	for (plugin_iter = _io_link_plugins.begin();
	     plugin_iter != _io_link_plugins.end();
	     ++plugin_iter) {
	    IoLink* io_link = plugin_iter->second;
	    if (io_link->leave_multicast_group(group_address, error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
    }

    return (ret_value);
}

void
IoLinkComm::allocate_io_link_plugins()
{
    list<FeaDataPlaneManager *>::iterator iter;

    for (iter = _io_link_manager.fea_data_plane_managers().begin();
	 iter != _io_link_manager.fea_data_plane_managers().end();
	 ++iter) {
	FeaDataPlaneManager* fea_data_plane_manager = *iter;
	allocate_io_link_plugin(fea_data_plane_manager);
    }
}

void
IoLinkComm::deallocate_io_link_plugins()
{
    while (! _io_link_plugins.empty()) {
	IoLinkPlugins::iterator iter = _io_link_plugins.begin();
	FeaDataPlaneManager* fea_data_plane_manager = iter->first;
	deallocate_io_link_plugin(fea_data_plane_manager);
    }
}

void
IoLinkComm::allocate_io_link_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoLinkPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_link_plugins.begin();
	 iter != _io_link_plugins.end();
	 ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter != _io_link_plugins.end()) {
	return;	// XXX: the plugin was already allocated
    }

    IoLink* io_link = fea_data_plane_manager->allocate_io_link(_iftree,
							       _if_name,
							       _vif_name,
							       _ether_type,
							       _filter_program);
    if (io_link == NULL) {
	XLOG_ERROR("Couldn't allocate plugin for I/O Link raw "
		   "communications for data plane manager %s",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    _io_link_plugins.push_back(make_pair(fea_data_plane_manager, io_link));
}

void
IoLinkComm::deallocate_io_link_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoLinkPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_link_plugins.begin();
	 iter != _io_link_plugins.end();
	 ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter == _io_link_plugins.end()) {
	XLOG_ERROR("Couldn't deallocate plugin for I/O Link raw "
		   "communications for data plane manager %s: plugin not found",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    IoLink* io_link = iter->second;
    fea_data_plane_manager->deallocate_io_link(io_link);
    _io_link_plugins.erase(iter);
}


void
IoLinkComm::start_io_link_plugins()
{
    IoLinkPlugins::iterator iter;
    string error_msg;

    for (iter = _io_link_plugins.begin();
	 iter != _io_link_plugins.end();
	 ++iter) {
	IoLink* io_link = iter->second;
	if (io_link->is_running())
	    continue;
	io_link->register_io_link_receiver(this);
	if (io_link->start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    continue;
	}

	//
	// Push all multicast joins into the new plugin
	//
	JoinedGroupsTable::iterator join_iter;
	for (join_iter = _joined_groups_table.begin();
	     join_iter != _joined_groups_table.end();
	     ++join_iter) {
	    JoinedMulticastGroup& joined_multicast_group = join_iter->second;
	    if (io_link->join_multicast_group(joined_multicast_group.group_address(),
					      error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
	    }
	}
    }
}

void
IoLinkComm::stop_io_link_plugins()
{
    string error_msg;
    IoLinkPlugins::iterator iter;

    for (iter = _io_link_plugins.begin();
	 iter != _io_link_plugins.end();
	 ++iter) {
	IoLink* io_link = iter->second;
	io_link->unregister_io_link_receiver();
	if (io_link->stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	}
    }
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
	io_link_comm = new IoLinkComm(*this, iftree(), if_name, vif_name,
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

int
IoLinkManager::register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
					   bool is_exclusive)
{
    if (is_exclusive) {
	// Unregister all registered data plane managers
	while (! _fea_data_plane_managers.empty()) {
	    unregister_data_plane_manager(_fea_data_plane_managers.front());
	}
    }

    if (fea_data_plane_manager == NULL) {
	// XXX: exclusive NULL is used to unregister all data plane managers
	return (XORP_OK);
    }

    if (find(_fea_data_plane_managers.begin(),
	     _fea_data_plane_managers.end(),
	     fea_data_plane_manager)
	!= _fea_data_plane_managers.end()) {
	// XXX: already registered
	return (XORP_OK);
    }

    _fea_data_plane_managers.push_back(fea_data_plane_manager);

    //
    // Allocate all I/O Link plugins for the new data plane manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table.begin(); iter != _comm_table.end(); ++iter) {
	IoLinkComm* io_link_comm = iter->second;
	io_link_comm->allocate_io_link_plugin(fea_data_plane_manager);
	io_link_comm->start_io_link_plugins();
    }

    return (XORP_OK);
}

int
IoLinkManager::unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager)
{
    if (fea_data_plane_manager == NULL)
	return (XORP_ERROR);

    list<FeaDataPlaneManager*>::iterator data_plane_manager_iter;
    data_plane_manager_iter = find(_fea_data_plane_managers.begin(),
				   _fea_data_plane_managers.end(),
				   fea_data_plane_manager);
    if (data_plane_manager_iter == _fea_data_plane_managers.end())
	return (XORP_ERROR);

    //
    // Dealocate all I/O Link plugins for the unregistered data plane manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table.begin(); iter != _comm_table.end(); ++iter) {
	IoLinkComm* io_link_comm = iter->second;
	io_link_comm->deallocate_io_link_plugin(fea_data_plane_manager);
    }

    _fea_data_plane_managers.erase(data_plane_manager_iter);

    return (XORP_OK);
}
