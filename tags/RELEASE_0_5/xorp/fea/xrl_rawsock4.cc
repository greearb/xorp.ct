// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/xrl_rawsock4.cc,v 1.4 2003/03/10 23:20:18 hodson Exp $"

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "fea_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"

#include "xrl/interfaces/fea_rawpkt_client_xif.hh"
#include "ifmanager.hh"
#include "rawsock4.hh"
#include "xrl_rawsock4.hh"

static const size_t MIN_IP_PKT_BYTES = 20;
static const size_t MAX_IP_PKT_BYTES = 65535;

class XrlRawSocketFilter : public FilterRawSocket4::InputFilter {
public:
    XrlRawSocketFilter(XrlRawSocket4Manager&	rsm,
		       const string&		tgt_name,
		       const uint32_t&		proto)
	: _rsm(rsm), _tgt(tgt_name), _proto(proto) {}

    const string& target() const 	{ return _tgt; }

    const uint32_t& protocol() const	{ return _proto; }

    XrlRawSocket4Manager& manager() 	{ return _rsm; }

protected:
    XrlRawSocket4Manager& _rsm;
    const string	  _tgt;
    const uint32_t	  _proto;
};

// Filter class for checking incoming raw packets and checking whether
// to forward them.

class XrlVifInputFilter : public XrlRawSocketFilter {
public:
    XrlVifInputFilter(XrlRawSocket4Manager&	rsm,
		      const string&		tgt_name,
		      const string&		ifname,
		      const string&		vifname,
		      uint32_t			proto)
	: XrlRawSocketFilter(rsm, tgt_name, proto), _if(ifname), _vif(vifname)
    {}

    void recv(const vector<uint8_t>& data)
    {

	assert(data.size() >= MIN_IP_PKT_BYTES);

	const ip* hdr = reinterpret_cast<const ip*>(&data[0]);
	if (hdr->ip_p != protocol()) {
	    debug_msg("Ignore packet with proto %d (watching for %d)\n",
		      hdr->ip_p, protocol());
	    return;
	}

	const IfTree& it = _rsm.ifmgr().iftree();

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
	IfTreeVif::V4Map::const_iterator ai =
	    fv.get_addr( IPv4(hdr->ip_dst.s_addr) );

	if ( ai == fv.v4addrs().end() ) {
	    debug_msg("Got packet for address not associated with vif.");
	    return;
	}

	//
	// Instantiate client sending interface
	//
	XrlRawPacketClientV0p1Client cl(&_rsm.router());

	// Send notification, note callback goes to owning
	// XrlRawSocket4Manager instance since send failure to xrl_target
	// is useful for reaping all filters to connected to target
	cl.send_recv_raw4(
	    _tgt.c_str(), _if, _vif, data,
	    callback(&_rsm, &XrlRawSocket4Manager::xrl_vif_send_handler, _tgt)
	    );
    }

    void bye()
    {}

    const string& ifname() const { return _if; }

    const string& vifname() const { return _vif; }

protected:
    const string _if;
    const string _vif;
};

// ----------------------------------------------------------------------------
// XrlRawSocket4Manager code

XrlRawSocket4Manager::XrlRawSocket4Manager(EventLoop&	     e,
					   InterfaceManager& ifmgr,
					   XrlRouter&	     xr)
    throw (RawSocketException)
    : _e(e), _ifmgr(ifmgr), _xrlrouter(xr), _rs(_e, 0)
{
}

XrlRawSocket4Manager::~XrlRawSocket4Manager()
{
    erase_filters(_filters.begin(), _filters.end());
}

void
XrlRawSocket4Manager::erase_filters(const FilterBag::iterator& begin,
				    const FilterBag::iterator& end)
{
    FilterBag::iterator fi(begin);
    while (fi != end) {
	XrlRawSocketFilter* filter = fi->second;

	_rs.remove_filter(filter);
	delete filter;

	_filters.erase(fi++);

	// Assert count of filters on rawsocket matches number we've added
	assert(_rs.empty() == _filters.empty());
    }
}

XrlCmdError
XrlRawSocket4Manager::send(const string& vifname, const vector<uint8_t>& pkt)
{
    // XXX Todo
    if (vifname.empty() == false) {
	return XrlCmdError::COMMAND_FAILED("vifname parameter not supported");
    }

    // Minimal size check
    if (pkt.size() < MIN_IP_PKT_BYTES || pkt.size() > MAX_IP_PKT_BYTES) {
	return XrlCmdError::COMMAND_FAILED(
	    c_format("Packet size, %u bytes, out of bounds %u-%u bytes)",
		     (uint32_t)pkt.size(), (uint32_t)MIN_IP_PKT_BYTES,
		     (uint32_t)MAX_IP_PKT_BYTES)
	    );
    }

    errno = 0;
    ssize_t bytes_out = _rs.write(&pkt[0], pkt.size());

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

static inline size_t round_up(size_t val, size_t step)
{
    //
    // Fast, but not entirely comprehensible.
    //
    // Not limited to powers of two for step in case thought occurs...
    //
    return (val & (step - 1)) ? (1 + (val | (step - 1))) : val;

    //
    // Slower but obvious
    //    return ((val + step - 1) / step) * step;
    //
    // Dog slow, nearly obvious
    //	  return ((step - (v % step)) % step) + v;
    //
}

XrlCmdError
XrlRawSocket4Manager::send(const IPv4&		  src,
			   const IPv4&		  dst,
			   const string&	  vifname,
			   const uint32_t&	  proto,
			   const uint32_t&	  ttl,
			   const uint32_t&	  tos,
			   const vector<uint8_t>& options,
			   const vector<uint8_t>& payload)
{
    // Sanity check
    static_assert(sizeof(struct ip) == MIN_IP_PKT_BYTES);

    // Calculate options size
    size_t osz = round_up(options.size(), 4);

    // Set aside zero'ed buffer to use as IP packet
    vector<uint8_t> pkt(0, sizeof(struct ip) + osz + payload.size());

    // Fill in header fields
    struct ip* hdr = reinterpret_cast<struct ip*>(&pkt[0]);
    hdr->ip_v = IPVERSION;
    hdr->ip_hl	= osz + sizeof(struct ip) / 4;
    hdr->ip_tos = (uint8_t)tos;
    hdr->ip_len = (uint16_t)pkt.size();
    hdr->ip_ttl = (uint8_t)ttl;
    hdr->ip_p = (uint8_t)proto;
    hdr->ip_sum = 0;
    hdr->ip_src.s_addr = src.addr();
    hdr->ip_dst.s_addr = dst.addr();

    // Copy options into packet
    if (osz > 0)
	memcpy(hdr + 1, &options[0], options.size());

    // Copy payload into packet
    if (payload.size() > 0)
	memcpy(&pkt[hdr->ip_hl * 4], &payload[0], payload.size());

    return send(vifname, pkt);
}

XrlCmdError
XrlRawSocket4Manager::register_vif_receiver(const string&	tgt,
					    const string&	ifname,
					    const string&	vifname,
					    const uint32_t&	proto)
{

    FilterBag::iterator end = _filters.upper_bound(tgt);
    for (FilterBag::iterator fi = _filters.lower_bound(tgt); fi != end; ++fi) {
	XrlVifInputFilter* filter =
	    dynamic_cast<XrlVifInputFilter*>(fi->second);

	if (filter == 0)
	    continue; // Not a vif filter

	if (filter->protocol() == proto &&
	    filter->ifname() == ifname &&
	    filter->vifname() == vifname) {
	    // Already have filter for protocol
	    return XrlCmdError::OKAY();
	}
    }

    // Create filter for vif / protocol
    XrlVifInputFilter* new_filter =
	new XrlVifInputFilter(*this, tgt, ifname, vifname, proto);

    // Add filter to appropriate raw_socket
    _rs.add_filter(new_filter);

    // Add filter to those associated with xrl_target
    _filters.insert(FilterBag::value_type(tgt, new_filter));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRawSocket4Manager::unregister_vif_receiver(const string&	tgt,
					      const string&	ifname,
					      const string&	vifname,
					      const uint32_t&	proto)
{
    XrlVifInputFilter* filter = 0;

    // Walk through list of iterators looking for matching vif / protocol
    // combination
    FilterBag::iterator end = _filters.upper_bound(tgt);
    for (FilterBag::iterator fi = _filters.lower_bound(tgt); fi != end; ++fi) {

	filter = dynamic_cast<XrlVifInputFilter*>(fi->second);
	if (filter && filter->protocol() == proto &&
	    filter->ifname() == ifname &&
	    filter->vifname() == vifname) {
	    // Remove filter from socket
	    _rs.remove_filter(filter);

	    // Remove filter from group associated with target
	    _filters.erase(fi);

	    // Destruct filter
	    delete filter;

	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED("target, interface, vif combination not"
				       " registered.");
}

void
XrlRawSocket4Manager::xrl_vif_send_handler(const XrlError& e, string tgt)
{
    if (e == XrlError::OKAY())
	return;

    debug_msg("xrl_vif_send_handler: error %s\n",
	      e.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with Xrl Target that are tied to a raw
    // socket and then erase filters
    //
    erase_filters(_filters.lower_bound(tgt), _filters.upper_bound(tgt));
}
