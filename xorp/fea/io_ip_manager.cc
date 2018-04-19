// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea_node.hh"
#include "iftree.hh"
#include "io_ip_manager.hh"

//
// Filter class for checking incoming raw packets and checking whether
// to forward them.
//
class IpVifInputFilter : public IoIpComm::InputFilter {
public:
    IpVifInputFilter(IoIpManager&	io_ip_manager,
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

    virtual ~IpVifInputFilter() {
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
	io_ip_manager().recv_event(receiver_name(), header, payload);
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
	    IPvX group_address = *(_joined_multicast_groups.begin());
	    leave_multicast_group(group_address, error_msg);
	}
    }

protected:
    bool is_my_address(const IPvX& addr) const {
	const IfTreeInterface* ifp = NULL;
	const IfTreeVif* vifp = NULL;

	if (io_ip_manager().iftree().find_interface_vif_by_addr(IPvX(addr),
								ifp, vifp)
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
    set<IPvX>		_joined_multicast_groups;
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

IoIpComm::IoIpComm(IoIpManager& io_ip_manager, const IfTree& iftree,
		   int family, uint8_t ip_protocol)
    : IoIpReceiver(),
      _io_ip_manager(io_ip_manager),
      _iftree(iftree),
      _family(family),
      _ip_protocol(ip_protocol)
{
    XLOG_WARNING("Creating IoIpComm, family: %i  protocol: %i, tree: %s this: %p\n",
		 (int)(_family), (int)(_ip_protocol), _iftree.getName().c_str(), this);
}

IoIpComm::~IoIpComm()
{
    XLOG_WARNING("Deleting IoIpComm, family: %i  protocol: %i, iftree: %s this: %p\n",
		 (int)(_family), (int)(_ip_protocol), _iftree.getName().c_str(), this);
    deallocate_io_ip_plugins();

    while (_input_filters.empty() == false) {
	InputFilter* i = _input_filters.front();
	_input_filters.erase(_input_filters.begin());
	i->bye();
    }
}

int
IoIpComm::add_filter(InputFilter* filter)
{
    if (filter == NULL) {
	XLOG_FATAL("Adding a null filter");
	return (XORP_ERROR);
    }

    if (find(_input_filters.begin(), _input_filters.end(), filter)
	!= _input_filters.end()) {
	debug_msg("filter already exists\n");
	return (XORP_ERROR);
    }

    _input_filters.push_back(filter);

    //
    // Allocate and start the IoIp plugins: one per data plane manager.
    //
    if (_input_filters.front() == filter) {
	XLOG_ASSERT(_io_ip_plugins.empty());
	allocate_io_ip_plugins();
	start_io_ip_plugins();
    }
    return (XORP_OK);
}

int
IoIpComm::remove_filter(InputFilter* filter)
{
    list<InputFilter*>::iterator i;

    i = find(_input_filters.begin(), _input_filters.end(), filter);
    if (i == _input_filters.end()) {
	debug_msg("filter does not exist\n");
	return (XORP_ERROR);
    }

    _input_filters.erase(i);
    if (_input_filters.empty()) {
	deallocate_io_ip_plugins();
    }
    return (XORP_OK);
}

int
IoIpComm::send_packet(const string&	if_name,
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
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_ip_plugins.empty()) {
	error_msg = c_format("No I/O IP plugin to send a raw IP packet on "
			     "interface %s vif %s from %s to %s protocol %u",
			     if_name.c_str(), vif_name.c_str(),
			     src_address.str().c_str(),
			     dst_address.str().c_str(),
			     _ip_protocol);
	return (XORP_ERROR);
    }

    IoIpPlugins::iterator iter;
    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	IoIp* io_ip = iter->second;
	if (io_ip->send_packet(if_name,
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
			       error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += c_format("Error while sending to vif: %s:%s  src: %s  dest: %s:  ",
				  if_name.c_str(), vif_name.c_str(),
				  src_address.str().c_str(),
				  dst_address.str().c_str());
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

void
IoIpComm::recv_packet(const string&			if_name,
		      const string&			vif_name,
		      const IPvX&			src_address,
		      const IPvX&			dst_address,
		      int32_t				ip_ttl,
		      int32_t				ip_tos,
		      bool				ip_router_alert,
		      bool				ip_internet_control,
		      const vector<uint8_t>&		ext_headers_type,
		      const vector<vector<uint8_t> >&	ext_headers_payload,
		      const vector<uint8_t>&		payload)
{
    struct IPvXHeaderInfo header;

    header.if_name = if_name;
    header.vif_name = vif_name;
    header.src_address = src_address;
    header.dst_address = dst_address;
    header.ip_protocol = _ip_protocol;
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
IoIpComm::recv_system_multicast_upcall(const vector<uint8_t>& payload)
{
    for (list<InputFilter*>::iterator i = _input_filters.begin();
	 i != _input_filters.end(); ++i) {
	(*i)->recv_system_multicast_upcall(payload);
    }
}

void
IoIpComm::create_input_socket(const string& if_name,
				const string& vif_name)
{
    bool err = false;
    string error_msg;

    if (_io_ip_plugins.empty()) {
	error_msg = c_format("No I/O IP plugin to create input socket "
		"on interface %s vif %s protocol %u",
		if_name.c_str(), vif_name.c_str(),
		_ip_protocol);
	goto error;
    }

    //
    // Check the arguments
    //
    if (if_name.empty()) {
	error_msg = c_format("Cannot create input socket: empty interface name");
	goto error;
    }
    if (vif_name.empty()) {
	error_msg = c_format("Cannot create input socket on interface %s: "
		"empty vif name", if_name.c_str());
	goto error;
    }

    do {
	IoIpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_ip_plugins.begin(); plugin_iter != _io_ip_plugins.end(); ++plugin_iter) {
	    IoIp* io_ip = plugin_iter->second;
	    if (io_ip->create_input_socket(if_name, vif_name, error_msg) != XORP_OK) {
		err = true;
		if (!error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg;
	    }
	}
    } while (0); //dummy loop to avoid compiler warning

    if (err)
	goto error;

    return;
error:
    XLOG_WARNING("%s\n", error_msg.c_str());
    return;
}

int
IoIpComm::join_multicast_group(const string&	if_name,
			       const string&	vif_name,
			       const IPvX&	group_address,
			       const string&	receiver_name,
			       string&		error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_ip_plugins.empty()) {
	error_msg = c_format("No I/O IP plugin to join group %s "
			     "on interface %s vif %s protocol %u "
			     "receiver name %s",
			     group_address.str().c_str(),
			     if_name.c_str(), vif_name.c_str(),
			     _ip_protocol, receiver_name.c_str());
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

    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(if_name, vif_name, group_address);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	IoIpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_ip_plugins.begin();
	     plugin_iter != _io_ip_plugins.end();
	     ++plugin_iter) {
	    IoIp* io_ip = plugin_iter->second;
	    if (io_ip->join_multicast_group(if_name,
					    vif_name,
					    group_address,
					    error_msg2)
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
IoIpComm::leave_all_multicast_groups(const string&	if_name,
				const string&	vif_name,
				string&		error_msg)
{
    //XLOG_ERROR("IoIpComm::leave_all_multicast_groups, if: %s  vif: %s\n",
    //if_name.c_str(), vif_name.c_str());
  start:
    {
	JoinedGroupsTable::iterator joined_iter;
	for (joined_iter = _joined_groups_table.begin();
	     joined_iter != _joined_groups_table.end(); joined_iter++) {
	    //XLOG_ERROR("Found group, if: %s  vif: %s\n",
	    //	       joined_iter->second.if_name().c_str(),
	    //	       joined_iter->second.vif_name().c_str());
	    if ((joined_iter->second.if_name() == if_name) &&
		((vif_name.size() == 0) || (joined_iter->second.vif_name() == vif_name))) {

		string tmp_vifn(joined_iter->second.vif_name());

		//XLOG_ERROR("Matched, looking for receivers.\n");
		set<string>::iterator ri = joined_iter->second.get_receivers().begin();
		while (ri != joined_iter->second.get_receivers().end()) {
		    //XLOG_ERROR("Found receiver: %s\n", ri->c_str());
		    leave_multicast_group(if_name, tmp_vifn, joined_iter->second.group_address(),
					  *ri, error_msg);
		    // Maybe there are more...back out to top and search again.  This should keep our iterators
		    // from being corrupted by the leave_multicast_group call.
		    goto start;
		}//for all receivers
	    }//if this joined group matches our interface and vif (if specified)
	}//for all joined groups
    }//scoping brace
    // We delete all we can find...whatever we found must have been OK.
    return XORP_OK;
}


int
IoIpComm::leave_multicast_group(const string&	if_name,
				const string&	vif_name,
				const IPvX&	group_address,
				const string&	receiver_name,
				string&		error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_ip_plugins.empty()) {
	error_msg = c_format("No I/O IP plugin to leave group %s "
			     "on interface %s vif %s protocol %u "
			     "receiver name %s",
			     group_address.str().c_str(),
			     if_name.c_str(), vif_name.c_str(),
			     _ip_protocol, receiver_name.c_str());
	return (XORP_ERROR);
    }

    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(if_name, vif_name, group_address);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	error_msg = c_format("Cannot leave group %s on interface %s vif %s: "
			     "the group was not joined (will continue)",
			     group_address.str().c_str(),
			     if_name.c_str(),
			     vif_name.c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	// Don't fail this..would fail the whole commit, and the group isn't joined
	// anyway, so no loss of validity in the configuration.
	return XORP_OK;
    }
    JoinedMulticastGroup& jmg = joined_iter->second;

    jmg.delete_receiver(receiver_name);
    if (jmg.empty()) {
	//
	// The last receiver, hence leave the group
	//
	_joined_groups_table.erase(joined_iter);

	IoIpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_ip_plugins.begin();
	     plugin_iter != _io_ip_plugins.end();
	     ++plugin_iter) {
	    IoIp* io_ip = plugin_iter->second;
	    if (io_ip->leave_multicast_group(if_name,
					     vif_name,
					     group_address,
					     error_msg2)
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
IoIpComm::allocate_io_ip_plugins()
{
    list<FeaDataPlaneManager *>::iterator iter;

    for (iter = _io_ip_manager.fea_data_plane_managers().begin();
	 iter != _io_ip_manager.fea_data_plane_managers().end();
	 ++iter) {
	FeaDataPlaneManager* fea_data_plane_manager = *iter;
	allocate_io_ip_plugin(fea_data_plane_manager);
    }
}

void
IoIpComm::deallocate_io_ip_plugins()
{
    while (! _io_ip_plugins.empty()) {
	IoIpPlugins::iterator iter = _io_ip_plugins.begin();
	FeaDataPlaneManager* fea_data_plane_manager = iter->first;
	deallocate_io_ip_plugin(fea_data_plane_manager);
    }
}

void
IoIpComm::allocate_io_ip_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoIpPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter != _io_ip_plugins.end()) {
	return;	// XXX: the plugin was already allocated
    }

    IoIp* io_ip = fea_data_plane_manager->allocate_io_ip(_iftree, _family,
							 _ip_protocol);
    if (io_ip == NULL) {
	XLOG_ERROR("Couldn't allocate plugin for I/O IP raw "
		   "communications for data plane manager %s",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    _io_ip_plugins.push_back(make_pair(fea_data_plane_manager, io_ip));
}

void
IoIpComm::deallocate_io_ip_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoIpPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter == _io_ip_plugins.end()) {
	XLOG_ERROR("Couldn't deallocate plugin for I/O IP raw "
		   "communications for data plane manager %s: plugin not found",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    IoIp* io_ip = iter->second;
    fea_data_plane_manager->deallocate_io_ip(io_ip);
    _io_ip_plugins.erase(iter);
}


void
IoIpComm::start_io_ip_plugins()
{
    IoIpPlugins::iterator iter;
    string error_msg;

    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	IoIp* io_ip = iter->second;
	if (io_ip->is_running())
	    continue;
	io_ip->register_io_ip_receiver(this);
	if (io_ip->start(error_msg) != XORP_OK) {
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
	    if (io_ip->join_multicast_group(joined_multicast_group.if_name(),
					    joined_multicast_group.vif_name(),
					    joined_multicast_group.group_address(),
					    error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
	    }
	}
    }
}

void
IoIpComm::stop_io_ip_plugins()
{
    string error_msg;
    IoIpPlugins::iterator iter;

    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	IoIp* io_ip = iter->second;
	io_ip->unregister_io_ip_receiver();
	if (io_ip->stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	}
    }
}

XorpFd
IoIpComm::first_valid_mcast_protocol_fd_in(const string& local_dev)
{
    XorpFd xorp_fd;

    // Find the first valid file descriptor and return it
    IoIpPlugins::iterator iter;
    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	IoIp* io_ip = iter->second;
	XorpFd* xfd = io_ip->findExistingInputSocketMcast(local_dev, local_dev);
	if (xfd && xfd->is_valid()) {
	    xorp_fd = *xfd;
	    return xorp_fd;
	}
    }

    /* So, none are open yet, try to create one */
    for (iter = _io_ip_plugins.begin(); iter != _io_ip_plugins.end(); ++iter) {
	IoIp* io_ip = iter->second;
	string err_msg;
	XorpFd* xfd = io_ip->findOrCreateInputSocketMcast(local_dev, local_dev, err_msg);
	if (xfd && xfd->is_valid()) {
	    xorp_fd = *xfd;
	    return xorp_fd;
	}
	else {
	    XLOG_WARNING("Could not create mcast socket: %s\n", err_msg.c_str());
	}
    }

    // XXX: nothing found
    return xorp_fd;
}

// ----------------------------------------------------------------------------
// IoIpManager code

IoIpManager::IoIpManager(FeaNode&	fea_node,
			 const IfTree&	iftree)
    : IoIpManagerReceiver(),
      _fea_node(fea_node),
      _eventloop(fea_node.eventloop()),
      _iftree(iftree),
      _io_ip_manager_receiver(NULL)
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
IoIpManager::recv_event(const string&			receiver_name,
			const struct IPvXHeaderInfo&	header,
			const vector<uint8_t>&		payload)
{
    if (_io_ip_manager_receiver != NULL)
	_io_ip_manager_receiver->recv_event(receiver_name, header, payload);
}

void
IoIpManager::erase_filters_by_receiver_name(int family,
					    const string& receiver_name)
{
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    erase_filters(comm_table, filters,
		  filters.lower_bound(receiver_name),
		  filters.upper_bound(receiver_name));
}

bool
IoIpManager::has_filter_by_receiver_name(const string& receiver_name) const
{
    // There whether there is an entry for a given receiver name
    if (_filters4.find(receiver_name) != _filters4.end())
	return (true);
    if (_filters6.find(receiver_name) != _filters6.end())
	return (true);

    return (false);
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
	    XLOG_WARNING("Unregister receiver (erase_filters), protocol: %i\n",
			 (int)(io_ip_comm->ip_protocol()));
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
	error_msg = c_format("%s: Protocol %u is not registered",
			     __FUNCTION__, XORP_UINT_CAST(ip_protocol));
	return (XORP_ERROR);
    }
    IoIpComm* io_ip_comm = cti->second;
    XLOG_ASSERT(io_ip_comm != NULL);

    return (io_ip_comm->send_packet(if_name,
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
    IpVifInputFilter* filter;
    CommTable& comm_table = comm_table_by_family(family);
    FilterBag& filters = filters_by_family(family);

    error_msg = "";

    //XLOG_INFO("IoIpManager::register_receiver: %s on %s/%s\n",
    //          receiver_name.c_str(), if_name.c_str(), vif_name.c_str());

    //
    // Look in the CommTable for an entry matching this protocol.
    // If an entry does not yet exist, create one.
    //
    CommTable::iterator cti = comm_table.find(ip_protocol);
    IoIpComm* io_ip_comm = NULL;
    if (cti == comm_table.end()) {
	XLOG_INFO("Creating new receiver, name: %s iface: %s  protocol: %i family: %i\n",
		  receiver_name.c_str(), if_name.c_str(), (int)(ip_protocol),
		  family);
	io_ip_comm = new IoIpComm(*this, iftree(), family, ip_protocol);
	comm_table[ip_protocol] = io_ip_comm;
    } else {
	io_ip_comm = cti->second;
    }
    XLOG_ASSERT(io_ip_comm != NULL);

    //
    // Walk through list of filters looking for matching filter
    //
    FilterBag::iterator fi;
    const FilterBag::iterator fi_end = filters.upper_bound(receiver_name);
    for (fi = filters.lower_bound(receiver_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<IpVifInputFilter*>(fi->second);
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
	    //XLOG_INFO("filter exists, returning from register_receiver..\n");
	    return (XORP_OK);
	}
    }

    //
    // Create the filter
    //
    filter = new IpVifInputFilter(*this, *io_ip_comm, receiver_name, if_name,
				  vif_name, ip_protocol);
    filter->set_enable_multicast_loopback(enable_multicast_loopback);

    // Add the filter to the appropriate IoIpComm entry
    io_ip_comm->add_filter(filter);

    // Add the filter to those associated with receiver_name
    filters.insert(FilterBag::value_type(receiver_name, filter));

    //Create input socket for if vif pair
    io_ip_comm->create_input_socket(if_name, vif_name);

    // Register interest in watching the receiver
    if (_fea_node.fea_io().add_instance_watch(receiver_name, this, error_msg)
	!= XORP_OK) {
	string dummy_error_msg;
	unregister_receiver(family, receiver_name, if_name, vif_name,
			    ip_protocol, dummy_error_msg);
	return (XORP_ERROR);
    }

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
	error_msg = c_format("%s: Protocol %u is not registered",
			     __FUNCTION__, XORP_UINT_CAST(ip_protocol));
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
	IpVifInputFilter* filter;
	filter = dynamic_cast<IpVifInputFilter*>(fi->second);
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
		XLOG_WARNING("Unregister receiver, protocol: %i family: %i\n",
			     (int)(ip_protocol), family);
		comm_table.erase(ip_protocol);
		delete io_ip_comm;
	    }

	    // Deregister interest in watching the receiver
	    if (! has_filter_by_receiver_name(receiver_name)) {
		string dummy_error_msg;
		_fea_node.fea_io().delete_instance_watch(receiver_name, this,
							 dummy_error_msg);
	    }

	    return (XORP_OK);
	}
    }

    error_msg = c_format("Cannot find registration for receiver %s "
			 "protocol %u interface %s and vif %s",
			 receiver_name.c_str(),
			 XORP_UINT_CAST(ip_protocol),
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
	IpVifInputFilter* filter;
	filter = dynamic_cast<IpVifInputFilter*>(fi->second);
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


int IoIpManager::leave_all_multicast_groups(const string& if_name,
				       const string& vif_name,
				       string& error_msg) {
    CommTable::iterator cti;
    for (cti = _comm_table4.begin(); cti != _comm_table4.end(); cti++) {
	IoIpComm* c = cti->second;
	c->leave_all_multicast_groups(if_name, vif_name, error_msg);
    }
    return XORP_OK;
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
	IpVifInputFilter* filter;
	filter = dynamic_cast<IpVifInputFilter*>(fi->second);
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
    XorpFd&	mcast_receiver_fd,
    const string& local_dev,
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
	XLOG_WARNING("Creating new mcast protocol: %i(%s) family: %i\n",
		     (int)(ip_protocol), ip_proto_str(ip_protocol), family);
	io_ip_comm = new IoIpComm(*this, iftree(), family, ip_protocol);
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
	    mcast_receiver_fd = io_ip_comm->first_valid_mcast_protocol_fd_in(local_dev);
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

    mcast_receiver_fd = io_ip_comm->first_valid_mcast_protocol_fd_in(local_dev);

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
	error_msg = c_format("%s: Protocol %u is not registered",
			     __FUNCTION__, XORP_UINT_CAST(ip_protocol));
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
		XLOG_WARNING("Unregister mcast receiver, protocol: %i family: %i\n",
			     (int)(ip_protocol), family);
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

int
IoIpManager::register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
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
    // Allocate all I/O IP plugins for the new data plane manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table4.begin(); iter != _comm_table4.end(); ++iter) {
	IoIpComm* io_ip_comm = iter->second;
	io_ip_comm->allocate_io_ip_plugin(fea_data_plane_manager);
	io_ip_comm->start_io_ip_plugins();
    }
    for (iter = _comm_table6.begin(); iter != _comm_table6.end(); ++iter) {
	IoIpComm* io_ip_comm = iter->second;
	io_ip_comm->allocate_io_ip_plugin(fea_data_plane_manager);
	io_ip_comm->start_io_ip_plugins();
    }

    return (XORP_OK);
}

int
IoIpManager::unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager)
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
    // Dealocate all I/O IP plugins for the unregistered data plane manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table4.begin(); iter != _comm_table4.end(); ++iter) {
	IoIpComm* io_ip_comm = iter->second;
	io_ip_comm->deallocate_io_ip_plugin(fea_data_plane_manager);
    }
    for (iter = _comm_table6.begin(); iter != _comm_table6.end(); ++iter) {
	IoIpComm* io_ip_comm = iter->second;
	io_ip_comm->deallocate_io_ip_plugin(fea_data_plane_manager);
    }

    _fea_data_plane_managers.erase(data_plane_manager_iter);

    return (XORP_OK);
}

void
IoIpManager::instance_birth(const string& instance_name)
{
    // XXX: Nothing to do
    UNUSED(instance_name);
}

void
IoIpManager::instance_death(const string& instance_name)
{
    string dummy_error_msg;

    _fea_node.fea_io().delete_instance_watch(instance_name, this,
					     dummy_error_msg);

    erase_filters_by_receiver_name(AF_INET, instance_name);
#ifdef HAVE_IPV6
    erase_filters_by_receiver_name(AF_INET6, instance_name);
#endif
}
