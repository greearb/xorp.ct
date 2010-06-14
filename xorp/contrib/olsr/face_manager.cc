// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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
#include "face_manager.hh"
#include "face.hh"

// #define DEBUG_LOGGING
// #define DEBUG_FUNCTION_NAME
// #define DETAILED_DEBUG

FaceManager::FaceManager(Olsr& olsr, EventLoop& ev)
    : _olsr(olsr),
      _eventloop(ev),
      _nh(0),
      _next_faceid(1),
      _enabled_face_count(0),
      _next_msg_seqno(1),
      _hello_interval(TimeVal(OlsrTypes::DEFAULT_HELLO_INTERVAL, 0)),
      _mid_interval(TimeVal(OlsrTypes::DEFAULT_MID_INTERVAL, 0)),
      _dup_hold_time(TimeVal(OlsrTypes::DEFAULT_DUP_HOLD_TIME, 0)),
      _is_early_mid_enabled(false)
{
    initialize_message_decoder(_md);
    add_message_cb(callback(this, &FaceManager::event_receive_unknown));
}

FaceManager::~FaceManager()
{
    // Timers may be scheduled even if we never got a slice.
    stop_all_timers();

    clear_dupetuples();
    clear_faces();

    XLOG_ASSERT(_faces.empty());
    XLOG_ASSERT(_duplicate_set.empty());

    delete_message_cb(callback(this, &FaceManager::event_receive_unknown));

    // TODO: Fix this assertion (minor leak)
    //XLOG_ASSERT(_handlers.empty());
}

const Face*
FaceManager::get_face_by_id(const OlsrTypes::FaceID faceid) const
    throw(BadFace)
{
    map<OlsrTypes::FaceID, Face*>::const_iterator ii = _faces.find(faceid);

    if (ii == _faces.end()) {
	xorp_throw(BadFace,
		   c_format("Mapping for %u does not exist",
			    XORP_UINT_CAST(faceid)));
    }

    return (*ii).second;
}

void
FaceManager::get_face_list(list<OlsrTypes::FaceID>& face_list) const
{
    map<OlsrTypes::FaceID, Face*>::const_iterator ii;

    for (ii = _faces.begin(); ii != _faces.end(); ii++)
	face_list.push_back((*ii).first);
}

void
FaceManager::set_hello_interval(const TimeVal& interval)
{
    if (interval == _hello_interval)
	return;

    debug_msg("%s: setting HELLO_INTERVAL to %s\n",
	      cstring(get_main_addr()),
	      cstring(interval));

    _hello_interval = interval;

    if (_hello_timer.scheduled()) {
	reschedule_hello_timer();
    }
}

void
FaceManager::set_mid_interval(const TimeVal& interval)
{
    if (interval == _mid_interval)
	return;

    debug_msg("%s: setting MID_INTERVAL to %s\n",
	      cstring(get_main_addr()),
	      cstring(interval));

    _mid_interval = interval;

    if (_mid_timer.scheduled()) {
	reschedule_mid_timer();
    }
}

void
FaceManager::clear_dupetuples()
{
    DupeTupleMap::iterator ii, jj;
    for (ii = _duplicate_set.begin(); ii != _duplicate_set.end(); ) {
	jj = ii++;
	delete (*jj).second;
	_duplicate_set.erase(jj);
    }
}

void
FaceManager::clear_faces()
{
    map<OlsrTypes::FaceID, Face*>::iterator ii, jj;
    for (ii = _faces.begin(); ii != _faces.end(); ) {
	jj = ii++;
	delete (*jj).second;
	_faces.erase(jj);
    }
}

void
FaceManager::receive(const string& interface, const string& vif,
    const IPv4 & dst, const uint16_t & dport,
    const IPv4 & src, const uint16_t & sport,
    uint8_t* data, const uint32_t & len)
{
    debug_msg("If %s Vif %s Dst %s Dport %u Src %s Sport %u Len %u\n",
	interface.c_str(), vif.c_str(),
	cstring(dst), dport, cstring(src), sport, len);

    // We must find a link which matches, to accept the packet.
    OlsrTypes::FaceID faceid;
    try {
	faceid = get_faceid(interface, vif);
    } catch(...) {
	// No Face listening on the given link.
	return;
    }
    XLOG_ASSERT(_faces.find(faceid) != _faces.end());

    Face* face = _faces[faceid];

    if (! face->enabled()) {
	debug_msg("dropping %p(%u) as face %s/%s is disabled.\n",
		  data, XORP_UINT_CAST(len),
		  interface.c_str(), vif.c_str());
	return;		// not enabled, therefore not listening.
    }

#if 0
    // These checks are normally stubbed out, as it isn't possible
    // to determine the destination address or port using the socket4 xif.

    // Check for a port match, to accept this packet.
    // Strictly this is only needed if we're running in the simulator.
    if (dport != face->local_port())
	return;

    // Check for a match with the configured broadcast address to
    // accept this packet.
    if (dst != face->all_nodes_addr())
	return;
#endif

    Packet* pkt = new Packet(_md, faceid);
    try {
	pkt->decode(data, len);
    } catch (InvalidPacket& e) {
	_faces[faceid]->counters().incr_bad_packets();
	debug_msg("Packet from %s:%u could not be decoded.\n",
		  cstring(src), XORP_UINT_CAST(sport));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Packet from %s:%u could not be decoded.",
		   cstring(src), XORP_UINT_CAST(sport));
	return;
    }

    // An OLSR control packet may contain several control messages.
    // Demultiplex them and process them accordigly.

    const vector<Message*>& messages = pkt->messages();
    vector<Message*>::const_iterator ii;
    for (ii = messages.begin(); ii != messages.end(); ii++) {
	Message* msg = (*ii);

	// 3.4, 2: Messages from myself must be silently dropped.
	if (msg->origin() == get_main_addr()) {
	    _faces[faceid]->counters().incr_messages_from_self();
	    // XXX refcounting
	    delete msg;
	    continue;
	}

	// 3.4, 3.1: If the message has been recorded in the duplicate
	// set, it should not be processed.
	if (is_duplicate_message(msg)) {
	    _faces[faceid]->counters().incr_duplicates();
	    // XXX refcounting
	    delete msg;
	    continue;
	}

	bool is_consumed = false;

	// Walk the list of message handler functions in reverse,
	// looking for one which is willing to consume this message.
	vector<MessageReceiveCB>::reverse_iterator jj;
	for (jj = _handlers.rbegin(); jj != _handlers.rend(); jj++) {
	    try {
		is_consumed = (*jj)->dispatch(msg, src, face->local_addr());
		if (is_consumed)
		    break;
	    } catch (InvalidMessage& im) {
		_faces[faceid]->counters().incr_bad_messages();
		XLOG_ERROR("%s", cstring(im));
	    }
	}

	// XXX This is here because we didn't implement
	// refcounting properly. We have to remember to delete the
	// message as we can't rely on Packet, or indeed forward_message(),
	// doing this for us.
	delete msg;

	// The message must be consumed at least by the UnknownMessage
	// handler, which floods messages according to the default
	// forwarding rules.
	if (! is_consumed) {
	    XLOG_UNREACHABLE();
	}
    }

    delete pkt;
}

bool
FaceManager::transmit(const string& interface, const string& vif,
    const IPv4 & dst, const uint16_t & dport,
    const IPv4 & src, const uint16_t & sport,
    uint8_t* data, const uint32_t & len)
{
    debug_msg("If %s Vif %s Dst %s Dport %u Src %s Sport %u Len %u\n",
	interface.c_str(), vif.c_str(),
	cstring(dst), dport, cstring(src), sport, len);

    return _olsr.transmit(interface, vif, dst, dport, src, sport, data, len);
}

OlsrTypes::FaceID
FaceManager::create_face(const string& interface, const string& vif)
    throw(BadFace)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());
    string concat = interface + "/" + vif;

    if (_faceid_map.find(concat) != _faceid_map.end()) {
	xorp_throw(BadFace,
		   c_format("Mapping for %s already exists", concat.c_str()));
    }

    OlsrTypes::FaceID faceid = _next_faceid++;
    _faceid_map[concat] = faceid;

    _faces[faceid] = new Face(_olsr, *this, _nh, _md, interface, vif, faceid);

    //
    // Register the interface/vif/address status callbacks.
    //
    // This only needs to be done once per invocation of xorp_olsr.
    // It is safe to do this once the interface manager service
    // has finished starting up in XrlIO.
    //
    // FaceManager::create_face() does not get called until this
    // happens, so OK to do this here.
    //
    _olsr.register_vif_status(callback(this, &FaceManager::
					     vif_status_change));
    _olsr.register_address_status(callback(this, &FaceManager::
						 address_status_change));

    return faceid;
}

bool
FaceManager::activate_face(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    OlsrTypes::FaceID faceid;
    try {
	faceid = get_faceid(interface, vif);
    } catch(...) {
	return false;
    }

    //recompute_addresses_face(faceid);
    //Face* face = _faces[faceid];

    return true;
}

bool
FaceManager::delete_face(OlsrTypes::FaceID faceid)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    set_face_enabled(faceid, false);

    delete _faces[faceid];
    _faces.erase(_faces.find(faceid));

    map<string, OlsrTypes::FaceID>::iterator ii;
    for (ii = _faceid_map.begin(); ii != _faceid_map.end(); ii++) {
	if ((*ii).second == faceid) {
	    _faceid_map.erase(ii);
	    break;
	}
    }

    return true;
}

// TODO: Support advertising the other protocol addresses which
// this OLSR node has been configured with on each link in MID.

bool
FaceManager::recompute_addresses_face(OlsrTypes::FaceID faceid)
{
#ifdef notyet
    debug_msg("FaceID %u\n", faceid);

    map<OlsrTypes::FaceID, Face*>::iterator ii = _faces.find(faceid);
    if (ii == _faces.end()) {
	XLOG_ERROR("Unable to find interface/vif associated with "
		   "FaceID %u", XORP_UINT_CAST(faceid));
	return false;
    }

    Face* face = (*ii).second;

    const string& interface = face->interface();
    const string& vif = face->vif();
    const IPv4& address = face->local_address();

    // Before trying to get the addresses, verify that this
    // interface/vif exists in the FEA, and that the address we have
    // been configured with is usable.
    if (! _olsr.is_address_enabled(interface, vif, address))
	return false;

    // Get all protocol addresses for this interface/vif.
    list<IPv4> addresses;
    if (! _olsr.get_addresses(interface, vif, addresses)) {
	XLOG_ERROR("Unable to find addresses on %s/%s ", interface.c_str(),
		   vif.c_str());
	return false;
    }

    list<IPv4>::iterator ii;
    for (ii = addresses.begin(); ii != addresses.end(); ii++) {
	// Skip linklocal addresses.
	if ((*i).is_linklocal_unicast())
	    continue;
	// Add the given address to MID.
    }

    // TODO: Force readvertisement of MID.

    return true;
#else
    return true;
    UNUSED(faceid);
#endif
}

void
FaceManager::vif_status_change(const string& interface,
			       const string& vif,
			       bool state)
{
    debug_msg("interface %s vif %s state %s\n",
	interface.c_str(), vif.c_str(), bool_c_str(state));

    OlsrTypes::FaceID faceid;

    try {
	faceid = get_faceid(interface, vif);
    } catch(...) {
	return;
    }

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return;
    }

    // TODO: Process link status in Face.

    return;
}

void
FaceManager::address_status_change(const string& interface,
				   const string& vif,
				   IPv4 addr,
				   bool state)
{
    debug_msg("interface %s vif %s addr %s state %s\n",
	interface.c_str(), vif.c_str(), cstring(addr),
	bool_c_str(state));

    OlsrTypes::FaceID faceid;

    try {
	faceid = get_faceid(interface, vif);
    } catch(...) {
	return;
    }

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return;
    }

    // TODO: Process link status in Face.

#ifdef notyet
    // XXX This may involve selecting a new main address for the OLSR node.
    if (false == recompute_addresses_face(faceid)) {
	XLOG_ERROR("Failed to recompute addresses for FaceID %u", faceid);
    }
#endif

    return;
}

OlsrTypes::FaceID
FaceManager::get_faceid(const string& interface, const string& vif)
    throw(BadFace)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());
    string concat = interface + "/" + vif;

    if (_faceid_map.find(concat) == _faceid_map.end()) {
	xorp_throw(BadFace,
		   c_format("No mapping for %s exists", concat.c_str()));
    }

    return _faceid_map[concat];
}

bool
FaceManager::get_interface_vif_by_faceid(OlsrTypes::FaceID faceid,
    string & interface, string & vif)
{
    debug_msg("FaceID %u\n", faceid);

    bool is_found = false;

    map<string, OlsrTypes::FaceID>::iterator ii;
    for (ii = _faceid_map.begin(); ii != _faceid_map.end(); ii++) {
	if ((*ii).second == faceid) {
	    const string* concat = &((*ii).first);
	    string::size_type n = concat->find_first_of("/");
	    interface = concat->substr(0, n);
	    vif = concat->substr(n + 1);
	    is_found = true;
	    break;
	}
    }

    return is_found;
}

bool
FaceManager::is_local_addr(const IPv4& addr)
{
    bool is_found = false;

    map<OlsrTypes::FaceID, Face*>::const_iterator ii;
    for (ii = _faces.begin(); ii != _faces.end(); ii++) {
	Face* f = (*ii).second;
	if (f->local_addr() == addr) {
	    is_found = true;
	    break;
	}
    }

    return is_found;
}

bool
FaceManager::get_face_stats(const string& ifname,
			    const string& vifname,
			    FaceCounters& stats)
{
    try {
	OlsrTypes::FaceID faceid = get_faceid(ifname, vifname);

	const Face* face = get_face_by_id(faceid);
	stats = face->counters();

	return true;
    } catch (...) {}

    return false;
}

bool
FaceManager::get_interface_cost(OlsrTypes::FaceID faceid, int& cost)
{
    debug_msg("FaceID %u\n", faceid);

    XLOG_ASSERT(_faces.find(faceid) != _faces.end());
    Face* face = _faces[faceid];
    bool is_found = true;

    cost = face->cost();

    return is_found;
}

bool
FaceManager::set_face_enabled(OlsrTypes::FaceID faceid, bool enabled)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    Face* face = _faces[faceid];

    if (enabled == face->enabled())
	return true;

    //
    // Check that the interface is capable of using the configured
    // means of addressing all OLSR nodes.
    //
    if (true == enabled) {
	bool try_multicast = face->all_nodes_addr().is_multicast();
	bool has_capability = false;

	if (try_multicast) {
	    has_capability =_olsr.is_vif_multicast_capable(
		face->interface(), face->vif());
	} else {
	    has_capability =_olsr.is_vif_broadcast_capable(
		face->interface(), face->vif());
	}

	if (! has_capability) {
	    XLOG_WARNING("%s/%s is not a %scast capable interface",
			 face->interface().c_str(), face->vif().c_str(),
			 try_multicast ? "multi" : "broad");
	}
    }

    //
    // Now actually request the binding to the underlying address.
    // FaceManager has no direct knowledge of IO, therefore we
    // request this through Olsr's facade.
    //
    // TODO: Join/leave multicast groups if required.
    //
    if (true == enabled) {
	//
	// Learn interface MTU from IO.
	// TODO: Size outgoing message queue as appropriate.
	//
	uint32_t mtu = _olsr.get_mtu(face->interface());
	face->set_mtu(mtu);

	bool success = _olsr.enable_address(face->interface(),
					    face->vif(),
					    face->local_addr(),
					    face->local_port(),
					    face->all_nodes_addr());
	if (! success) {
	    XLOG_ERROR("Failed to bring up I/O layer for %s/%s\n",
		       face->interface().c_str(), face->vif().c_str());
	    return false;
	}
    } else {
	bool success = _olsr.disable_address(face->interface(),
					     face->vif(),
					     face->local_addr(),
					     face->local_port());
	if (! success) {
	    XLOG_WARNING("Failed to shutdown I/O layer for %s/%s\n",
		         face->interface().c_str(), face->vif().c_str());
	}
    }

    // Mark the face status.
    face->set_enabled(enabled);

    XLOG_TRACE(_olsr.trace()._interface_events,
	       "Interface %s/%s is now %s.",
		face->interface().c_str(), face->vif().c_str(),
		enabled ? "up" : "down");

    if (enabled) {
	_enabled_face_count++;
	debug_msg("_enabled_face_count now %u\n", _enabled_face_count);

	if (_enabled_face_count == 1) {
	    start_hello_timer();
	} else if (_enabled_face_count > 1) {
	    if (_enabled_face_count == 2)
		start_mid_timer();

	    // If configured to trigger a MID as soon as each new
	    // interface is administratively up, do so.
	    if (_is_early_mid_enabled)
		reschedule_immediate_mid_timer();
	}
    } else {
	--_enabled_face_count;
	debug_msg("_enabled_face_count now %u\n", _enabled_face_count);

	if (_enabled_face_count == 1) {
	    stop_mid_timer();
	} else if (_enabled_face_count == 0) {
	    stop_hello_timer();
	}
    }

    return true;
}

bool
FaceManager::get_face_enabled(OlsrTypes::FaceID faceid)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    return _faces[faceid]->enabled();
}

bool
FaceManager::set_interface_cost(OlsrTypes::FaceID faceid, int cost)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    _faces[faceid]->set_cost(cost);

    return true;
}

bool
FaceManager::set_local_addr(OlsrTypes::FaceID faceid, const IPv4& local_addr)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    _faces[faceid]->set_local_addr(local_addr);

    return true;
}

bool
FaceManager::get_local_addr(OlsrTypes::FaceID faceid, IPv4& local_addr)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    local_addr = _faces[faceid]->local_addr();

    return true;
}

bool
FaceManager::set_local_port(OlsrTypes::FaceID faceid,
			    const uint16_t local_port)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    _faces[faceid]->set_local_port(local_port);

    return true;
}

bool
FaceManager::get_local_port(OlsrTypes::FaceID faceid, uint16_t& local_port)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    local_port = _faces[faceid]->local_port();

    return true;
}

bool
FaceManager::set_all_nodes_addr(OlsrTypes::FaceID faceid,
				const IPv4& all_nodes_addr)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    Face* face = _faces[faceid];

    // Check if nothing need be done.
    if (face->all_nodes_addr() == all_nodes_addr)
	return true;

    // TODO: If we are listening for OLSR traffic on a multicast group
    // on the interface, ask Olsr to ask IO to leave the group.
    if (face->all_nodes_addr().is_multicast()) {
	XLOG_UNFINISHED();
    }

    const string& interface = face->interface();
    const string& vif = face->vif();
    const IPv4& local_addr = face->local_addr();

    if (all_nodes_addr.is_multicast()) {
	// Multicast address.
	if (! all_nodes_addr.is_linklocal_multicast()) {
	    XLOG_ERROR("Rejecting OLSR all-nodes address %s on %s/%s: "
		       "not a link-local group",
		       cstring(all_nodes_addr),
		       interface.c_str(), vif.c_str());
	    return false;
	}

	//XLOG_UNFINISHED();
	XLOG_ERROR("Rejecting OLSR all-nodes address %s on %s/%s: "
		   "multicast groups are not yet supported",
		   cstring(all_nodes_addr),
		   interface.c_str(), vif.c_str());
	return false;
    } else {
	// Broadcast address. 255.255.255.255 is always accepted.
	if (all_nodes_addr != IPv4::ALL_ONES()) {
	    bool matched_bcast = false;
	    do {
		// Attempt to get the configured broadcast address for
		// this interface, as seen by the interface manager.
		// If we cannot, or it does not match the all-nodes address
		// which the administrator has configured for this OLSR
		// binding, reject the address.
		IPv4 bcast_addr;

		if (! _olsr.get_broadcast_address(interface, vif,
						  local_addr, bcast_addr)) {
		    debug_msg("Could not obtain broadcast address for "
			      "%s/%s%s\n",
			      interface.c_str(), vif.c_str(),
			      cstring(local_addr));
		    break;
		}

		debug_msg("broadcast address for %s is %s\n",
			  cstring(local_addr), cstring(bcast_addr));

		if (all_nodes_addr == bcast_addr)
		    matched_bcast = true;
	    } while (false);

	    if (! matched_bcast) {
		XLOG_ERROR("Rejecting OLSR all-nodes address %s on %s/%s: "
			   "not a configured broadcast address",
			   cstring(all_nodes_addr),
			   interface.c_str(), vif.c_str());
		return false;
	    }
	}
    }

    face->set_all_nodes_addr(all_nodes_addr);

    return true;
}

bool
FaceManager::get_all_nodes_addr(OlsrTypes::FaceID faceid,
				IPv4& all_nodes_addr)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    all_nodes_addr = _faces[faceid]->all_nodes_addr();

    return true;
}

bool
FaceManager::set_all_nodes_port(OlsrTypes::FaceID faceid,
				const uint16_t all_nodes_port)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    Face* face = _faces[faceid];

    // Check if nothing need be done.
    if (face->all_nodes_port() == all_nodes_port)
	return true;

    // This is mostly a no-op; the outgoing port generally requires
    // no special setup.

    face->set_all_nodes_port(all_nodes_port);

    return true;
}

bool
FaceManager::get_all_nodes_port(OlsrTypes::FaceID faceid,
				uint16_t& all_nodes_port)
{
    debug_msg("FaceID %u\n", faceid);

    if (_faces.find(faceid) == _faces.end()) {
	XLOG_ERROR("Unknown FaceID %u", faceid);
	return false;
    }

    all_nodes_port = _faces[faceid]->all_nodes_port();

    return true;
}

bool
FaceManager::flood_message(Message* message)
{
    // TODO Use outgoing message queues per interface to
    // correctly deal with MTU and any fragmentation
    // which may be needed.

    // TODO Refcount messages as they flow up and down.

    map<OlsrTypes::FaceID, Face*>::iterator ii;
    for (ii = _faces.begin(); ii != _faces.end(); ii++) {
	Face* face = (*ii).second;

	// Skip transmission on disabled interfaces.
	if (! face->enabled())
	    continue;

	// XXX Deal with MTU by setting an upper limit on
	// the size of the encoded packet which is specific
	// to this interface; Packet will truncate to this size.

	Packet* pkt = new Packet(_md);
	pkt->set_mtu(face->mtu());

	pkt->add_message(message);

	vector<uint8_t> buf;
	bool result = pkt->encode(buf);

	if (result == false) {
	    XLOG_WARNING("Outgoing packet on %s/%s truncated by MTU.",
			 face->interface().c_str(),
			 face->vif().c_str());
	}

	pkt->set_seqno(face->get_pkt_seqno());
	pkt->update_encoded_seqno(buf);

	face->transmit(&buf[0], buf.size());

	// XXX message will be consumed by the deletion.
	delete pkt;
    }

    // consume message like we say we will in our API.
    // XXX We can't delete the message here as we have no way of
    // knowing if we were being called in response to forwarding
    // or originating a message.
    //delete message;

    // XXX we should do more meaningful error handling.
    return true;
}

void
FaceManager::stop_all_timers()
{
    stop_hello_timer();
    stop_mid_timer();
}

void
FaceManager::start_mid_timer()
{
    debug_msg("starting MID timer\n");
    _mid_timer = _olsr.get_eventloop().new_periodic(
	get_mid_interval(),
	callback(this, &FaceManager::event_send_mid));
}

void
FaceManager::stop_mid_timer()
{
    debug_msg("stopping MID timer\n");
    _mid_timer.clear();
}

void
FaceManager::restart_mid_timer()
{
    reschedule_mid_timer();
}

void
FaceManager::reschedule_mid_timer()
{
    _mid_timer.reschedule_after(get_mid_interval());
}

void
FaceManager::reschedule_immediate_mid_timer()
{
    _mid_timer.schedule_now();
}

bool
FaceManager::event_send_mid()
{
    debug_msg("MyMainAddr %s event_send_mid\n",
	      cstring(get_main_addr()));

    XLOG_ASSERT(_enabled_face_count > 1);

    MidMessage* mid = new MidMessage();

    mid->set_expiry_time(get_mid_hold_time());
    mid->set_origin(get_main_addr());
    mid->set_ttl(OlsrTypes::MAX_TTL);
    mid->set_hop_count(0);
    mid->set_seqno(this->get_msg_seqno());

    map<OlsrTypes::FaceID, Face*>::iterator ii;
    for (ii = _faces.begin(); ii != _faces.end(); ii++) {
	Face* face = (*ii).second;
	// Skip disabled interfaces, or interfaces with the main address,
	// which already appears in the message origin.
	if (false == face->enabled() ||
	    get_main_addr() == face->local_addr())
	    continue;
	mid->add_interface(face->local_addr());
    }

    mid->set_valid(true);

    // XXX Return meaningful errors.
    flood_message(mid);	// assumes that 'mid' is consumed

    // XXX no refcounting
    delete mid;

    return true;
}

void
FaceManager::start_hello_timer()
{
    _hello_timer = _olsr.get_eventloop().new_periodic(
	get_hello_interval(),
	callback(this, &FaceManager::event_send_hello));
}

void
FaceManager::stop_hello_timer()
{
    _hello_timer.clear();
}

void
FaceManager::restart_hello_timer()
{
    reschedule_hello_timer();
}

void
FaceManager::reschedule_hello_timer()
{
    _hello_timer.reschedule_after(get_hello_interval());
}

void
FaceManager::reschedule_immediate_hello_timer()
{
    _hello_timer.schedule_now();
}

bool
FaceManager::event_send_hello()
{
    map<OlsrTypes::FaceID, Face*>::iterator ii;
    for (ii = _faces.begin(); ii != _faces.end(); ii++) {
	Face* face = (*ii).second;
	if (false == face->enabled())
	    continue;
	face->originate_hello();
    }

    return true;
}

bool
FaceManager::set_main_addr(const IPv4& addr)
{
    bool is_allowed = false;

    if (get_enabled_face_count() == 0) {
	// If no interfaces are enabled, the main address may be changed
	// to any address during initial configuration.
	is_allowed = true;
    } else {
	// If any interfaces are enabled for OLSR, you may only
	// change the main address to one which is currently enabled.

	map<OlsrTypes::FaceID, Face*>::const_iterator ii;
	for (ii = _faces.begin(); ii != _faces.end(); ii++) {
	    Face* face = (*ii).second;
	    if (false == face->enabled())
		continue;
	    if (face->local_addr() == addr) {
		is_allowed = true;
		break;
	    }
	}
    }

    // TODO: Propagate any protocol state changes required by the
    // change of main address.
    if (is_allowed)
	_main_addr = addr;

    return is_allowed;
}

void
FaceManager::add_message_cb(MessageReceiveCB cb)
{
    _handlers.push_back(cb);
}

bool
FaceManager::delete_message_cb(MessageReceiveCB cb)
{
    bool is_deleted = false;

    vector<MessageReceiveCB>::iterator ii;
    for (ii = _handlers.begin(); ii != _handlers.end(); ii++) {
#ifdef XORP_USE_USTL
	if (ii->get() == cb.get()) {
#else
	if (*ii == cb) {
#endif
	    _handlers.erase(ii);
	    is_deleted = true;
	    break;
	}
    }

    return is_deleted;
}

bool
FaceManager::event_receive_unknown(
    Message* msg,
    const IPv4& remote_addr,
    const IPv4& local_addr)
{
    UnknownMessage* um = dynamic_cast<UnknownMessage *>(msg);
    if (um == 0) {
	debug_msg("Warning: couldn't cast to UnknownMessage; did you\n"
		  "forget to register a receive callback in addition to\n"
		  "registering a decoder?\n");
	XLOG_UNREACHABLE();
    }

    debug_msg("*** RECEIVED UNKNOWN MESSAGE %p ***\n", um);

    // Update statistics.
    _faces[msg->faceid()]->counters().incr_unknown_messages();

    // Forward the message.
    forward_message(remote_addr,  msg);

    return true;
    UNUSED(local_addr);
}

bool
FaceManager::is_duplicate_message(const Message* msg) const
{
    //debug_msg("Msg %p\n", msg);

    // 7.1.1: HELLO messages are excluded from duplicate detection.
    if (0 != dynamic_cast<const HelloMessage *>(msg))
	return false;

    return (0 != get_dupetuple(msg->origin(), msg->seqno()));
}

bool
FaceManager::forward_message(const IPv4& remote_addr, Message* msg)
{
    debug_msg("MyMainAddr %s Msg %p\n",
	      cstring(get_main_addr()),
	      msg);

    // Invariant: HELLO messages should never be forwarded.
    XLOG_ASSERT(0 == dynamic_cast<HelloMessage *>(msg));

    if (is_forwarded_message(msg)) {
	debug_msg("%p already forwarded.\n", msg);
	return false;
    }

    // 3.4, 4: Check if *sender interface address* of the packet
    // which originally contained this message belongs to a neighbor
    // which is an MPR selector of this node.
    // If so, the message MUST be forwarded.
    bool will_forward = _nh->is_mpr_selector_addr(remote_addr) &&
			msg->ttl() > 1;

    debug_msg("%sforwarding %p.\n", will_forward ? "" : "not ", msg);

    // 3.4, 5: Update the duplicate set tuple.
    update_dupetuple(msg, will_forward);

    if (will_forward) {
	// 3.4, 6: Increment hopcount.
	// 3.4, 7: Decrement ttl.
	// 3.4, 8: Forward message on all interfaces.
	msg->incr_hops();
	msg->decr_ttl();

	flood_message(msg);

	_faces[msg->faceid()]->counters().incr_forwarded();
    }

    return will_forward;
}

bool
FaceManager::is_forwarded_message(const Message* msg) const
{
    //debug_msg("Msg %p\n", msg);

    // 3.4, 2: If a duplicate tuple does not exist, it is OK to
    // forward the message.
    DupeTuple* dt = get_dupetuple(msg->origin(), msg->seqno());
    if (0 == dt)
	return false;

    // 3.4, 2: If a duplicate tuple exists, it is not OK to forward the
    // message if it has already been forwarded, OR it has already been
    // received on the interface where we originally received it.
    if (!dt->is_forwarded() && !dt->is_seen_by_face(msg->faceid()))
	return false;

    return true;
}

DupeTuple*
FaceManager::get_dupetuple(const IPv4& origin_addr,
			   const uint16_t seqno) const
{
    //debug_msg("OriginAddr %s Seqno %u\n", cstring(origin_addr),
    //		XORP_UINT_CAST(seqno));

    if (_duplicate_set.empty())
	return 0;

    // 3.4, 3.1: If address and sequence number are matched in
    // the duplicate set, the message is a duplicate.
    DupeTuple* found_dt = 0;

    pair<DupeTupleMap::const_iterator, DupeTupleMap::const_iterator> rd =
	_duplicate_set.equal_range(origin_addr);
    for (DupeTupleMap::const_iterator ii = rd.first; ii != rd.second; ii++) {
	DupeTuple* dt = (*ii).second;
	if (dt->seqno() == seqno) {
	    found_dt = dt;
	    break;
	}
    }

    return found_dt;
}

void
FaceManager::set_dup_hold_time(const TimeVal& dup_hold_time)
{
    _dup_hold_time = dup_hold_time;
}

void
FaceManager::update_dupetuple(const Message* msg, const bool is_forwarded)
{
    debug_msg("MyMainAddr %s OriginAddr %s Seqno %u\n",
	      cstring(get_main_addr()),
	      cstring(msg->origin()),
	      XORP_UINT_CAST(msg->seqno()));

    DupeTuple* dt = 0;

    // Check if we already have a duplicate set entry for the
    // sequence number and origin inside the message.
    pair<DupeTupleMap::iterator, DupeTupleMap::iterator> rd =
	_duplicate_set.equal_range(msg->origin());

    for (DupeTupleMap::iterator ii = rd.first; ii != rd.second; ii++) {
	DupeTuple* ndt = (*ii).second;
	if (ndt->seqno() == msg->seqno()) {
	    dt = ndt;
	    break;
	}
    }

    // If a duplicate set tuple was not found, record a new tuple with the
    // sequence number and origin inside the message.
    if (0 == dt) {
	dt = new DupeTuple(_eventloop, this, msg->origin(),
			   msg->seqno(), get_dup_hold_time());
	_duplicate_set.insert(make_pair(msg->origin(), dt));
    }

    XLOG_ASSERT(dt != 0);

    // Update the message expiry time and the set of interfaces where it
    // was received.
    dt->update_timer(get_dup_hold_time());
    dt->set_seen_by_face(msg->faceid());
    dt->set_is_forwarded(is_forwarded);
}

void
FaceManager::event_dupetuple_expired(const IPv4& origin, const uint16_t seqno)
{
    bool is_found = false;

    pair<DupeTupleMap::iterator, DupeTupleMap::iterator> rd =
	_duplicate_set.equal_range(origin);

    DupeTupleMap::iterator ii;
    for (ii = rd.first; ii != rd.second; ii++) {
	DupeTuple* dt = (*ii).second;
	if (dt->seqno() == seqno) {
	    is_found = true;
	    break;
	}
    }

    XLOG_ASSERT(is_found);

    delete (*ii).second;
    _duplicate_set.erase(ii);
}

void
DupeTuple::update_timer(const TimeVal& vtime)
{
    if (_expiry_timer.scheduled())
	_expiry_timer.clear();

    _expiry_timer = _ev.new_oneoff_after(vtime,
	callback(this, &DupeTuple::event_dead));
}

void
DupeTuple::event_dead()
{
    _parent->event_dupetuple_expired(origin(), seqno());
}
