// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/olsr.cc,v 1.2 2008/07/23 05:09:52 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "olsr.hh"

Olsr::Olsr(EventLoop& eventloop, IO* io)
    : _eventloop(eventloop), _io(io),
      _fm(*this, eventloop),
      _nh(*this, eventloop, _fm),
      _tm(*this, eventloop, _fm, _nh),
      _er(*this, eventloop, _fm, _nh),
      _rm(*this, eventloop, &_fm, &_nh, &_tm, &_er),
      _reason("Waiting for IO"), _process_status(PROC_STARTUP)
{
    _nh.set_topology_manager(&_tm);
    _fm.set_neighborhood(&_nh);

    _nh.set_route_manager(&_rm);
    _tm.set_route_manager(&_rm);
    _er.set_route_manager(&_rm);

    _io->register_receive(callback(this, &Olsr::receive));

    //initialize_profiling_variables(_profile);
}

ProcessStatus
Olsr::status(string& reason)
{
    if (PROC_STARTUP == _process_status) {
	if (SERVICE_RUNNING == _io->status()) {
	    _process_status = PROC_READY;
	    _reason = "Running";
	}
    }

    reason = _reason;
    return _process_status;
}

void
Olsr::shutdown()
{
    _io->shutdown();
    _reason = "shutting down";
    _process_status = PROC_SHUTDOWN;
}

// Methods typically called by code in FaceManager (coming out of us).

void
Olsr::receive(const string& interface, const string& vif,
    IPv4 dst, uint16_t dport, IPv4 src, uint16_t sport,
    uint8_t* data, uint32_t len)
{
    XLOG_TRACE(trace()._packets,
	       "interface %s vif %s dst %s:%u src %s:%u data %p len %u\n",
	       interface.c_str(), vif.c_str(),
	       cstring(dst), XORP_UINT_CAST(dport),
	       cstring(src), XORP_UINT_CAST(sport),
	       data, len);
    debug_msg("interface %s vif %s dst %s:%u src %s:%u data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      cstring(dst), XORP_UINT_CAST(dport),
	      cstring(src), XORP_UINT_CAST(sport),
	      data, len);

    // TODO: Decode packet in order to pretty-print it.

    _fm.receive(interface, vif, dst, dport, src, sport, data, len);
}

bool
Olsr::transmit(const string& interface, const string& vif,
    const IPv4& dst, const uint16_t& dport,
    const IPv4& src, const uint16_t& sport,
    uint8_t* data, const uint32_t& len)
{
    XLOG_TRACE(trace()._packets,
	       "interface %s vif %s dst %s:%u src %s:%u data %p len %u\n",
	       interface.c_str(), vif.c_str(),
	       cstring(dst), XORP_UINT_CAST(dport),
	       cstring(src), XORP_UINT_CAST(sport),
	       data, len);
    debug_msg("interface %s vif %s dst %s:%u src %s:%u data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      cstring(dst), XORP_UINT_CAST(dport),
	      cstring(src), XORP_UINT_CAST(sport),
	      data, len);

    // TODO: Decode packet in order to pretty-print it.

    return _io->send(interface, vif, src, sport, dst, dport, data, len);
}

uint32_t
Olsr::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    return _io->get_mtu(interface);
}

bool
Olsr::get_broadcast_address(const string& interface,
			    const string& vif,
			    const IPv4& address,
			    IPv4& bcast_address)
{
    return _io->get_broadcast_address(interface, vif, address, bcast_address);
}

bool
Olsr::add_route(IPv4Net net, IPv4 nexthop, uint32_t faceid,
		uint32_t metric, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d policy %s\n",
	      cstring(net), cstring(nexthop), metric, cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Add route "
	       "Net %s Nexthop %s metric %d policy %s\n",
	       cstring(net), cstring(nexthop), metric, cstring(policytags));

    return _io->add_route(net, nexthop, faceid, metric, policytags);
}

bool
Olsr::replace_route(IPv4Net net, IPv4 nexthop, uint32_t faceid,
		    uint32_t metric, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d policy %s\n",
	      cstring(net), cstring(nexthop), metric, cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Replace route "
	       "Net %s Nexthop %s metric %d policy %s\n",
	       cstring(net), cstring(nexthop), metric, cstring(policytags));

    return _io->replace_route(net, nexthop, faceid, metric, policytags);
}

bool
Olsr::delete_route(IPv4Net net)
{
    debug_msg("Net %s\n", cstring(net));

    XLOG_TRACE(trace()._routes, "Delete route Net %s\n", cstring(net));

    return _io->delete_route(net);
}

bool
Olsr::is_vif_broadcast_capable(const string& interface, const string& vif)
{
    return _io->is_vif_broadcast_capable(interface, vif);
}

bool
Olsr::is_vif_multicast_capable(const string& interface, const string& vif)
{
    return _io->is_vif_multicast_capable(interface, vif);
}

bool
Olsr::enable_address(const string& interface, const string& vif,
		     const IPv4& address, const uint16_t& port,
		     const IPv4& all_nodes_address)
{
    return _io->enable_address(interface, vif, address, port,
			       all_nodes_address);
}

bool
Olsr::disable_address(const string& interface, const string& vif,
		      const IPv4& address, const uint16_t& port)
{
    return _io->disable_address(interface, vif, address, port);
}

// Methods typically called by XRL target (going into us).

bool
Olsr::bind_address(const string& interface,
		   const string& vif,
		   const IPv4& local_addr,
		   const uint32_t& local_port,
		   const IPv4& all_nodes_addr,
		   const uint32_t& all_nodes_port)
{
    try {
	OlsrTypes::FaceID faceid = face_manager().create_face(interface, vif);

	face_manager().set_local_addr(faceid, local_addr);
	face_manager().set_local_port(faceid, local_port);
	face_manager().set_all_nodes_addr(faceid, all_nodes_addr);
	face_manager().set_all_nodes_port(faceid, all_nodes_port);

	// XXX Do we need to do this yet? We aren't using link-local
	// addresses yet, and we still need to explicitly specify
	// a local interface address in this interface.
	//face_manager().activate_face(interface, vif);

	return true;
    } catch (...) {}

    return false;
}

bool
Olsr::unbind_address(const string& interface, const string& vif)
{
    try {
	OlsrTypes::FaceID faceid = face_manager().get_faceid(interface, vif);

	// TODO: xrlio teardown.
	return face_manager().delete_face(faceid);
    } catch (...) {}

    return false;
}

bool
Olsr::set_interface_enabled(const string& interface, const string& vif,
			    const bool enabled)
{
    try {
	OlsrTypes::FaceID faceid = face_manager().get_faceid(interface, vif);

	bool success = face_manager().set_face_enabled(faceid, enabled);
	debug_msg("%s/%s %senabled ok\n",
		  interface.c_str(), vif.c_str(), success ? "" : "not ");
	return success;
    } catch (...) {}

    return false;
}

bool
Olsr::get_interface_enabled(const string& interface, const string& vif,
			    bool& enabled)
{
    try {
	OlsrTypes::FaceID faceid = face_manager().get_faceid(interface, vif);

	enabled = face_manager().get_face_enabled(faceid);

	return true;
    } catch (...) {}

    return false;
}

bool
Olsr::get_interface_stats(const string& interface, const string& vif,
			  FaceCounters& stats)
{
    return face_manager().get_face_stats(interface, vif, stats);
}

bool
Olsr::clear_database()
{
    // Clear links first. This will nuke all the neighbors.
    _nh.clear_links();

    // Clear topology control and MID entries.
    _tm.clear_tc_entries();
    _tm.clear_mid_entries();

    // Clear learned external routes.
    // Do not clear routes redistributed from another protocol.
    _er.clear_hna_routes_in();

    // Finally clear the duplicate message set.
    _fm.clear_dupetuples();

    return true;
}

void
Olsr::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter, conf);
}

void
Olsr::reset_filter(const uint32_t& filter)
{
    _policy_filters.reset(filter);
}

void
Olsr::push_routes()
{
    _rm.push_routes();
}

bool
Olsr::originate_external_route(const IPv4Net& net,
			       const IPv4& nexthop,
			       const uint32_t& metric,
			       const PolicyTags& policytags)
{
    return _er.originate_hna_route_out(net);

    UNUSED(nexthop);
    UNUSED(metric);
    UNUSED(policytags);
}

bool
Olsr::withdraw_external_route(const IPv4Net& net)
{
    try {
	_er.withdraw_hna_route_out(net);
	return true;
    } catch (...) {}

    return false;
}
