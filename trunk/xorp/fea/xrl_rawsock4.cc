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

#ident "$XORP: xorp/fea/xrl_rawsock4.cc,v 1.22 2007/05/08 19:23:15 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "xrl/interfaces/fea_rawpkt4_client_xif.hh"

#include "iftree.hh"
#include "rawsock4.hh"
#include "xrl_rawsock4.hh"

class XrlFilterRawSocket4 : public FilterRawSocket4::InputFilter {
public:
    XrlFilterRawSocket4(XrlRawSocket4Manager&	rsm,
			const string&		xrl_target_name,
			uint8_t			ip_protocol)
	: _rsm(rsm),
	  _xrl_target_name(xrl_target_name),
	  _ip_protocol(ip_protocol)
    {}

    XrlRawSocket4Manager& manager()		{ return _rsm; }
    const string&	xrl_target_name() const { return _xrl_target_name; }
    uint8_t		ip_protocol() const	{ return _ip_protocol; }

protected:
    XrlRawSocket4Manager&	_rsm;
    const string		_xrl_target_name;
    const uint8_t		_ip_protocol;
};

//
// Filter class for checking incoming raw packets and checking whether
// to forward them.
//
class XrlVifInputFilter4 : public XrlFilterRawSocket4 {
public:
    XrlVifInputFilter4(XrlRawSocket4Manager&	rsm,
		       FilterRawSocket4&	rs,
		       const string&		xrl_target_name,
		       const string&		if_name,
		       const string&		vif_name,
		       uint8_t			ip_protocol)
	: XrlFilterRawSocket4(rsm, xrl_target_name, ip_protocol),
	  _rs(rs),
	  _if_name(if_name),
	  _vif_name(vif_name),
	  _enable_multicast_loopback(false)
    {}

    virtual ~XrlVifInputFilter4() {
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

	//
	// Instantiate client sending interface
	//
	XrlRawPacket4ClientV0p1Client cl(&_rsm.router());

	//
	// Send notification, note callback goes to owning
	// XrlRawSocket4Manager instance since send failure to xrl_target
	// is useful for reaping all filters to connected to target.
	//
	cl.send_recv(_xrl_target_name.c_str(),
		     header.if_name,
		     header.vif_name,
		     header.src_address,
		     header.dst_address,
		     header.ip_protocol,
		     header.ip_ttl,
		     header.ip_tos,
		     header.ip_router_alert,
		     header.ip_internet_control,
		     payload,
		     callback(&_rsm,
			      &XrlRawSocket4Manager::xrl_send_recv_cb,
			      _xrl_target_name));
    }

    void bye() {}
    const string& if_name() const { return _if_name; }
    const string& vif_name() const { return _vif_name; }

    int join_multicast_group(const IPv4& group_address, string& error_msg) {
	if (_rs.join_multicast_group(if_name(), vif_name(), group_address,
				     xrl_target_name(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
	_joined_multicast_groups.insert(group_address);
	return (XORP_OK);
    }

    int leave_multicast_group(const IPv4& group_address, string& error_msg) {
	_joined_multicast_groups.erase(group_address);
	if (_rs.leave_multicast_group(if_name(), vif_name(), group_address,
				      xrl_target_name(), error_msg)
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
// XrlRawSocket4Manager code

XrlRawSocket4Manager::XrlRawSocket4Manager(EventLoop&		eventloop,
					   const IfTree&	iftree,
					   XrlRouter&		xr)
    : _eventloop(eventloop), _iftree(iftree), _xrlrouter(xr)
{
}

XrlRawSocket4Manager::~XrlRawSocket4Manager()
{
    erase_filters(_filters.begin(), _filters.end());
}

void
XrlRawSocket4Manager::erase_filters(const FilterBag4::iterator& begin,
				    const FilterBag4::iterator& end)
{
    FilterBag4::iterator fi(begin);
    while (fi != end) {
	XrlFilterRawSocket4* filter = fi->second;

	SocketTable4::iterator sti = _sockets.find(filter->ip_protocol());
	XLOG_ASSERT(sti != _sockets.end());
	FilterRawSocket4* rs = sti->second;
	XLOG_ASSERT(rs != NULL);

	rs->remove_filter(filter);
	delete filter;

	_filters.erase(fi++);
    }
}

XrlCmdError
XrlRawSocket4Manager::send(const string&	if_name,
			   const string&	vif_name,
			   const IPv4&		src_address,
			   const IPv4&		dst_address,
			   uint8_t		ip_protocol,
			   int32_t		ip_ttl,
			   int32_t		ip_tos,
			   bool			ip_router_alert,
			   bool			ip_internet_control,
			   const vector<uint8_t>& payload)
{
    string error_msg;

    // Find the socket associated with this protocol
    SocketTable4::iterator sti = _sockets.find(ip_protocol);
    if (sti == _sockets.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return XrlCmdError::COMMAND_FAILED(error_msg);
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
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRawSocket4Manager::register_receiver(const string&	xrl_target_name,
					const string&	if_name,
					const string&	vif_name,
					uint8_t		ip_protocol,
					bool		enable_multicast_loopback)
{
    XrlVifInputFilter4* filter;

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
    FilterBag4::iterator fi_end = _filters.upper_bound(xrl_target_name);
    for (fi = _filters.lower_bound(xrl_target_name); fi != fi_end; ++fi) {
	filter = dynamic_cast<XrlVifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Already have this filter
	    filter->set_enable_multicast_loopback(enable_multicast_loopback);
	    return XrlCmdError::OKAY();
	}
    }

    //
    // Create the filter
    //
    filter = new XrlVifInputFilter4(*this, *rs, xrl_target_name, if_name,
				    vif_name, ip_protocol);
    filter->set_enable_multicast_loopback(enable_multicast_loopback);

    // Add the filter to the appropriate raw socket
    rs->add_filter(filter);

    // Add the filter to those associated with xrl_target_name
    _filters.insert(FilterBag4::value_type(xrl_target_name, filter));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRawSocket4Manager::unregister_receiver(const string&	xrl_target_name,
					  const string&	if_name,
					  const string&	vif_name,
					  uint8_t	ip_protocol)
{
    string error_msg;

    //
    // Find the socket associated with this protocol
    //
    SocketTable4::iterator sti = _sockets.find(ip_protocol);
    if (sti == _sockets.end()) {
	error_msg = c_format("Protocol %u is not registered",
			     XORP_UINT_CAST(ip_protocol));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    FilterRawSocket4* rs = sti->second;
    XLOG_ASSERT(rs != NULL);

    //
    // Walk through list of filters looking for matching interface and vif
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(xrl_target_name);
    for (fi = _filters.lower_bound(xrl_target_name); fi != fi_end; ++fi) {
	XrlVifInputFilter4* filter;
	filter = dynamic_cast<XrlVifInputFilter4*>(fi->second);

	// If filter found, remove it and delete it
	if ((filter != NULL) &&
	    (filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    
	    // Remove the filter
	    rs->remove_filter(filter);

	    // Remove the filter from the group associated with this target
	    _filters.erase(fi);

	    // Destruct the filter
	    delete filter;

	    //
	    // Reference counting: if there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    //
	    if (rs->empty()) {
		_sockets.erase(ip_protocol);
		delete rs;
	    }

	    return XrlCmdError::OKAY();
	}
    }

    error_msg = c_format("Cannot find registration for target %s interface %s "
			 "and vif %s",
			 xrl_target_name.c_str(),
			 if_name.c_str(),
			 vif_name.c_str());
    return XrlCmdError::COMMAND_FAILED(error_msg);
}

XrlCmdError
XrlRawSocket4Manager::join_multicast_group(const string& xrl_target_name,
					   const string& if_name,
					   const string& vif_name,
					   uint8_t	 ip_protocol,
					   const IPv4&	 group_address)
{
    string error_msg;

    //
    // Search if we have already the filter
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(xrl_target_name);
    for (fi = _filters.lower_bound(xrl_target_name); fi != fi_end; ++fi) {
	XrlVifInputFilter4* filter;
	filter = dynamic_cast<XrlVifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Filter found
	    if (filter->join_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return XrlCmdError::COMMAND_FAILED(error_msg);
	    }
	    return XrlCmdError::OKAY();
	}
    }

    error_msg = c_format("Cannot join group %s on interface %s vif %s "
			 "protocol %u target %s: the target has not "
			 "registered as a receiver",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ip_protocol),
			 xrl_target_name.c_str());

    return XrlCmdError::COMMAND_FAILED(error_msg);
}

XrlCmdError
XrlRawSocket4Manager::leave_multicast_group(const string& xrl_target_name,
					    const string& if_name,
					    const string& vif_name,
					    uint8_t	  ip_protocol,
					    const IPv4&	  group_address)
{
    string error_msg;

    //
    // Search if we have already the filter
    //
    FilterBag4::iterator fi;
    FilterBag4::iterator fi_end = _filters.upper_bound(xrl_target_name);
    for (fi = _filters.lower_bound(xrl_target_name); fi != fi_end; ++fi) {
	XrlVifInputFilter4* filter;
	filter = dynamic_cast<XrlVifInputFilter4*>(fi->second);
	if (filter == NULL)
	    continue; // Not a vif filter

	if ((filter->ip_protocol() == ip_protocol) &&
	    (filter->if_name() == if_name) &&
	    (filter->vif_name() == vif_name)) {
	    // Filter found
	    if (filter->leave_multicast_group(group_address, error_msg)
		!= XORP_OK) {
		return XrlCmdError::COMMAND_FAILED(error_msg);
	    }
	    return XrlCmdError::OKAY();
	}
    }

    error_msg = c_format("Cannot leave group %s on interface %s vif %s "
			 "protocol %u target %s: the target has not "
			 "registered as a receiver",
			 group_address.str().c_str(),
			 if_name.c_str(),
			 vif_name.c_str(),
			 XORP_UINT_CAST(ip_protocol),
			 xrl_target_name.c_str());

    return XrlCmdError::COMMAND_FAILED(error_msg);
}

void
XrlRawSocket4Manager::xrl_send_recv_cb(const XrlError& e,
				       string xrl_target_name)
{
    if (e == XrlError::OKAY())
	return;

    debug_msg("xrl_send_recv_cb: error %s\n", e.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with Xrl Target that are tied to a raw
    // socket and then erase filters
    //
    erase_filters(_filters.lower_bound(xrl_target_name),
		  _filters.upper_bound(xrl_target_name));
}
