// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "fea_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"

#include "xrl/interfaces/fea_rawpkt6_client_xif.hh"
#include "ifmanager.hh"
#include "rawsock6.hh"
#include "xrl_rawsock6.hh"

static const size_t MIN_IP6_PKT_BYTES = 4;
static const size_t MAX_IP6_PKT_BYTES = 131072;

class XrlRawSocket6Filter : public FilterRawSocket6::InputFilter {
public:
    XrlRawSocket6Filter(XrlRawSocket6Manager&	rsm6,
			const string&		tgt_name,
			const uint32_t&		proto)
	: _rsm6(rsm6), _tgt(tgt_name), _proto(proto) {}

    const string& target() const 	{ return _tgt; }

    XrlRawSocket6Manager& manager() 	{ return _rsm6; }

    const uint32_t& protocol() const	{ return _proto; }

protected:
    XrlRawSocket6Manager& _rsm6;
    const string	  _tgt;
    uint32_t		  _proto;
};

// Filter class for checking incoming raw packets and checking whether
// to forward them.

class XrlVifInputFilter6 : public XrlRawSocket6Filter {
public:
    XrlVifInputFilter6(XrlRawSocket6Manager&	rsm6,
		      const string&		tgt_name,
		      const string&		ifname,
		      const string&		vifname,
		      const uint32_t&		proto)
	: XrlRawSocket6Filter(rsm6, tgt_name, proto),
	  _if(ifname), _vif(vifname)
    {}

    void recv(const struct IPv6HeaderInfo& hdrinfo,
	      const vector<uint8_t>& hopopts,
	      const vector<uint8_t>& payload) {

	const IfTree& it = _rsm6.ifmgr().iftree();

	//
	// Find Interface
	//
	IfTree::IfMap::const_iterator ii = it.get_if(_if);
	if (ii == it.ifs().end()) {
	    debug_msg("Got packet on non-configured interface.");
	    return;
	}
	const IfTreeInterface& fi = ii->second;

	//
	// Find Vif
	//
	IfTreeInterface::VifMap::const_iterator vi = fi.get_vif(_vif);
	if (vi == fi.vifs().end()) {
	    debug_msg("Got packet on non-configured vif.");
	    return;
	}
	const IfTreeVif& fv = vi->second;

	//
	// Find Address
	//
	IfTreeVif::V6Map::const_iterator ai = fv.get_addr(hdrinfo.dst);
	if ( ai == fv.v6addrs().end() ) {
	    debug_msg("Got packet for address not associated with vif.");
	    return;
	}

	//
	// Instantiate client sending interface
	//
	XrlRawPacket6ClientV0p1Client cl(&_rsm6.router());

	// Send notification, note callback goes to owning
	// XrlRawSocket6Manager instance since send failure to xrl_target
	// is useful for reaping all filters to connected to target.
	cl.send_recv_raw(
	    _tgt.c_str(), _if, _vif, hdrinfo.src, hdrinfo.dst, _proto,
	    hdrinfo.tclass, hdrinfo.hoplimit, hopopts, payload,
	    callback(&_rsm6, &XrlRawSocket6Manager::xrl_vif_send_handler, _tgt)
	    );
    }

    void bye() {}

    const string& ifname() const { return _if; }
    const string& vifname() const { return _vif; }

protected:
    const string _if;
    const string _vif;
};

// ----------------------------------------------------------------------------
// XrlRawSocket6Manager code

XrlRawSocket6Manager::XrlRawSocket6Manager(EventLoop&	     eventloop,
					   InterfaceManager& ifmgr,
					   XrlRouter&	     xr)
    throw (RawSocket6Exception)
    : _eventloop(eventloop), _ifmgr(ifmgr), _xrlrouter(xr)
{
}

XrlRawSocket6Manager::~XrlRawSocket6Manager()
{
    erase_filters(_filters.begin(), _filters.end());
}

void
XrlRawSocket6Manager::erase_filters(const FilterBag6::iterator& begin,
				    const FilterBag6::iterator& end)
{
    FilterBag6::iterator fi(begin);
    while (fi != end) {
	XrlRawSocket6Filter* filter = fi->second;

	SocketTable6::iterator sti = _sockets.find(filter->protocol());
	assert(sti != _sockets.end());
	FilterRawSocket6* rs = sti->second;
	assert(rs != NULL);

	rs->remove_filter(filter);
	delete filter;

	_filters.erase(fi++);
    }
}

XrlCmdError
XrlRawSocket6Manager::send(const IPv6& src,
			   const IPv6& dst,
			   const string& vifname,
			   const uint8_t proto,
			   const uint8_t tclass,
			   const uint8_t hoplimit,
			   const vector<uint8_t>& hopopts,
			   const vector<uint8_t>& payload)
{
    UNUSED(tclass);
    UNUSED(hoplimit);
    UNUSED(hopopts);

    // Find the socket associated with this protocol.
    SocketTable6::iterator sti = _sockets.find(proto);
    if (sti == _sockets.end()) {
	return XrlCmdError::COMMAND_FAILED("protocol not registered.");
    }
    FilterRawSocket6* rs = sti->second;
    assert(rs != NULL);

    // XXX Todo
    if (vifname.empty() == false) {
	return XrlCmdError::COMMAND_FAILED("vifname parameter not supported");
    }

    // Minimal size check
    if (payload.size() <= MIN_IP6_PKT_BYTES ||
	payload.size() > MAX_IP6_PKT_BYTES) {
	return XrlCmdError::COMMAND_FAILED(
	    c_format("Packet size, %u bytes, out of bounds %u-%u bytes)",
		     (uint32_t)payload.size(),
		     (uint32_t)MIN_IP6_PKT_BYTES,
		     (uint32_t)MAX_IP6_PKT_BYTES)
	    );
    }

    errno = 0;
    ssize_t bytes_out = rs->write(src, dst, &payload[0], payload.size());

    if (bytes_out > 0) {
	return XrlCmdError::OKAY();
    }
    if (errno != 0) {
	return XrlCmdError::COMMAND_FAILED(c_format("Send failed: %s",
						    strerror(errno)));
    }
    return XrlCmdError::COMMAND_FAILED("Send failed, consult FEA xlog to "
				       "determine cause");
}

XrlCmdError
XrlRawSocket6Manager::register_vif_receiver(const string&	tgt,
					    const string&	ifname,
					    const string&	vifname,
					    const uint32_t&	proto)
{

    // IPv6 raw sockets *must* have their protocol type specified at
    // creation time. Look in the SocketTable for a socket matching
    // this protocol. If a socket does not yet exist, create one.

    SocketTable6::iterator sti = _sockets.find(proto);
    FilterRawSocket6* rs = NULL;
    if (sti == _sockets.end()) {
	rs = new FilterRawSocket6(_eventloop, proto);
	_sockets[proto] = rs;
    } else {
	rs = sti->second;
    }
    assert(rs != NULL);

    FilterBag6::iterator end = _filters.upper_bound(tgt);
    for (FilterBag6::iterator fi = _filters.lower_bound(tgt); fi != end; ++fi) {
	XrlVifInputFilter6* filter =
	    dynamic_cast<XrlVifInputFilter6*>(fi->second);

	if (filter == NULL)
	    continue; // Not a vif filter

	if (filter->protocol() == proto &&
	    filter->ifname() == ifname &&
	    filter->vifname() == vifname) {
	    return XrlCmdError::OKAY();
	}
    }

    // Create filter for vif. The kernel filters protocols for us.
    XrlVifInputFilter6* new_filter =
	new XrlVifInputFilter6(*this, tgt, ifname, vifname, proto);

    // Add filter to appropriate raw_socket
    rs->add_filter(new_filter);

    // Add filter to those associated with xrl_target
    _filters.insert(FilterBag6::value_type(tgt, new_filter));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRawSocket6Manager::unregister_vif_receiver(const string&	tgt,
					      const string&	ifname,
					      const string&	vifname,
					      const uint32_t&	proto)
{

    // Find the socket associated with this protocol, if it exists.
    SocketTable6::iterator sti = _sockets.find(proto);
    if (sti == _sockets.end()) {
	return XrlCmdError::COMMAND_FAILED("protocol not registered.");
    }
    FilterRawSocket6* rs = sti->second;
    assert(rs != NULL);

    // Walk through list of filters looking for matching vif.
    FilterBag6::iterator end = _filters.upper_bound(tgt);
    for (FilterBag6::iterator fi = _filters.lower_bound(tgt); fi != end; ++fi) {

	XrlVifInputFilter6* filter =
	    dynamic_cast<XrlVifInputFilter6*>(fi->second);

	// If found, remove and delete the filter.
	if (filter && filter->protocol() == proto &&
	    filter->ifname() == ifname &&
	    filter->vifname() == vifname) {

	    // Remove the filter and delete it.
	    rs->remove_filter(filter);
	    _filters.erase(fi);
	    delete filter;

	    // Reference counting: If there are now no listeners on
	    // this protocol socket (and hence no filters), remove it
	    // from the table and delete it.
	    if (rs->empty()) {
		_sockets.erase(proto);
		delete rs;
	    }

	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED("target, interface, vif combination not"
				       " registered.");
}

void
XrlRawSocket6Manager::xrl_vif_send_handler(const XrlError& e, string tgt)
{
    if (e == XrlError::OKAY())
	return;

    debug_msg("xrl_vif_send_handler: error %s\n",
	      e.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with Xrl Target that are tied to a raw
    // socket and then erase filters.
    //
    erase_filters(_filters.lower_bound(tgt), _filters.upper_bound(tgt));
}
