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

#ident "$XORP: xorp/contrib/olsr/face.cc,v 1.3 2008/10/02 21:56:34 bms Exp $"

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

#include <set>
#include <algorithm>

#include "olsr.hh"
#include "face.hh"

Face::Face(Olsr& olsr, FaceManager& fm, Neighborhood* nh,
	   MessageDecoder& md, const string& interface, const string& vif,
	   OlsrTypes::FaceID id)
 : _olsr(olsr),
   _fm(fm),
   _nh(nh),
   _md(md),
   _id(id),
   _enabled(false),
   _interface(interface),
   _vif(vif),
   _mtu(0),			    // XXX obtained later
   _local_addr(IPv4::ZERO()),	    // XXX obtained later
   _local_port(OlsrTypes::DEFAULT_OLSR_PORT),
   _all_nodes_addr(IPv4::ALL_ONES()),
   _all_nodes_port(OlsrTypes::DEFAULT_OLSR_PORT),
   _cost(OlsrTypes::DEFAULT_STATIC_FACE_COST),
   _next_pkt_seqno(1)
{
}

void
Face::set_enabled(bool value)
{
    debug_msg("enable %s\n", bool_c_str(value));

    if (value == _enabled)
	return;

    _enabled = value;

    XLOG_ASSERT(0 != _nh);

    if (_enabled == false) {
	_nh->delete_face(id());
    } else {
	_nh->add_face(id());
    }
}

bool
Face::transmit(uint8_t* data, const uint32_t& len)
{
    debug_msg("tx data %p len %u\n", data, len);

    return _fm.transmit(_interface, _vif,
			_all_nodes_addr, _all_nodes_port,
			_local_addr, _local_port,
			data, len);
}

void
Face::originate_hello()
{
    Packet* pkt = new Packet(_md, id());
    HelloMessage* hello = new HelloMessage();

    // Set message attributes which are local to Face and FaceManager.
    hello->set_origin(_fm.get_main_addr());
    hello->set_ttl(1);
    hello->set_hop_count(0);
    hello->set_seqno(_fm.get_msg_seqno());
    hello->set_htime(_fm.get_hello_interval());

    // We must tell Neighborhood from which face this HELLO originates.
    hello->set_faceid(id());

    // Push link state into the message.
    _nh->populate_hello(hello);

    // We would typically segment the message here.
    pkt->set_mtu(mtu());

    pkt->add_message(hello);

    vector<uint8_t> buf;

    bool result = pkt->encode(buf);
    if (result == false) {
	XLOG_WARNING("Outgoing packet on %s/%s truncated by MTU.",
		     interface().c_str(), vif().c_str());
    }

    pkt->set_seqno(get_pkt_seqno());
    //pkt->update_encoded_seqno(buf);

    transmit(&buf[0], buf.size());

    delete hello;   // XXX no refcounting
    delete pkt;
}
