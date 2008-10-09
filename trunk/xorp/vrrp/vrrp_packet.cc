// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#ident "$XORP: xorp/vrrp/vrrp_packet.cc,v 1.1 2008/10/09 17:46:27 abittau Exp $"

#include "vrrp_module.h"
#include "libxorp/xlog.h"
#include "libproto/checksum.h"
#include "vrrp_packet.hh"
#include "vrrp_exception.hh"

const IPv4 VRRPPacket::mcast_group = IPv4("224.0.0.18");

VRRPHeader&
VRRPHeader::assign(uint8_t* data)
{
    static_assert(sizeof(VRRPHeader) == 8);
    static_assert(sizeof(VRRPAuth) == 8);

    VRRPHeader* vh = reinterpret_cast<VRRPHeader*>(data);

    vh->vh_v	    = VRRP_VERSION;
    vh->vh_type	    = VRRP_TYPE_ADVERTISEMENT;
    vh->vh_vrid	    = 0;
    vh->vh_priority = 0;
    vh->vh_ipcount  = 0;
    vh->vh_auth	    = VRRP_AUTH_NONE;
    vh->vh_interval = 0;
    vh->vh_sum	    = 0;

    return *vh;
}

const VRRPHeader&
VRRPHeader::assign(const PAYLOAD& p)
{
    const VRRPHeader* vh = reinterpret_cast<const VRRPHeader*>(&p[0]);

    unsigned size = sizeof(*vh) + sizeof(VRRPAuth);

    if (p.size() < size)
	xorp_throw(VRRPException, "packet too small");

    if (vh->vh_v != VRRP_VERSION)
	xorp_throw(VRRPException, "unknown version");

    if (vh->vh_type != VRRP_TYPE_ADVERTISEMENT)
	xorp_throw(VRRPException, "unknown type");

    size += sizeof(*vh->vh_addr) * vh->vh_ipcount;
    if (size != p.size())
	xorp_throw(VRRPException, "bad size");

    // checksum
    VRRPHeader* tmp	= const_cast<VRRPHeader*>(vh);
    unsigned checksum	= vh->vh_sum;
    unsigned sz2	= tmp->finalize(); // XXX will overwrite auth

    XLOG_ASSERT(size == sz2);
    if (checksum != vh->vh_sum)
	xorp_throw(VRRPException, "bad checksum");

    return *vh;
}

void
VRRPHeader::add_ip(const IPv4& ip)
{
    XLOG_ASSERT(vh_ipcount < 255);

    ip.copy_out(vh_addr[vh_ipcount]);
    vh_ipcount++;
}

IPv4
VRRPHeader::ip(unsigned idx) const
{
    XLOG_ASSERT(idx < vh_ipcount);

    IPv4 ip;
    ip.copy_in(vh_addr[idx]);

    return ip;
}

uint32_t
VRRPHeader::finalize()
{
    uint32_t len = sizeof(*this);

    len += sizeof(*vh_addr) * vh_ipcount;

    VRRPAuth* auth = reinterpret_cast<VRRPAuth*>((unsigned long) this + len);
    memset(auth, 0, sizeof(*auth));

    len += sizeof(*auth);

    vh_sum = 0;
    vh_sum = inet_checksum(reinterpret_cast<uint8_t*>(this), len);

    return len;
}

VRRPPacket::VRRPPacket()
		: _data(VRRP_MAX_PACKET_SIZE, 0),
		  _ip(&_data[0]),
		  _vrrp(VRRPHeader::assign(&_data[IP_HEADER_MIN_SIZE]))
{
    _data.resize(VRRP_MAX_PACKET_SIZE);

    _ip.set_ip_vhl(0);
    _ip.set_ip_version(4);
    _ip.set_ip_header_len(IP_HEADER_MIN_SIZE);
    _ip.set_ip_tos(0);
    _ip.set_ip_len(0);
    _ip.set_ip_id(0);
    _ip.set_ip_off(0);
    _ip.set_ip_ttl(255);
    _ip.set_ip_p(IPPROTO_VRRP);
    _ip.set_ip_sum(0);
    _ip.set_ip_dst(mcast_group);
}

void
VRRPPacket::set_vrid(uint8_t vrid)
{
    _vrrp.vh_vrid = vrid;
}

void
VRRPPacket::set_priority(uint8_t priority)
{
    _vrrp.vh_priority = priority;
}

void
VRRPPacket::set_interval(uint8_t interval)
{
    _vrrp.vh_interval = interval;
}

void
VRRPPacket::finalize()
{
    uint32_t size = _vrrp.finalize();

    size += IP_HEADER_MIN_SIZE;
    _ip.set_ip_len(size);
    _ip.set_ip_sum(0);

    // XXX set_ip_sum does htons but inet_checksum returns in network order.  We
    // really need _ip.compute_checksum().
    _ip.set_ip_sum(ntohs(inet_checksum(_ip.data(), IP_HEADER_MIN_SIZE)));

    XLOG_ASSERT(size <= VRRP_MAX_PACKET_SIZE);
    XLOG_ASSERT(_data.size() == _data.capacity() 
                && _data.size() == VRRP_MAX_PACKET_SIZE);

    _data.resize(size);
}

uint32_t
VRRPPacket::size() const
{
    return _data.size();
}

const PAYLOAD&
VRRPPacket::data() const
{
    return _data;
}

void
VRRPPacket::clear_ips()
{
    _vrrp.vh_ipcount = 0;
}

void
VRRPPacket::add_ip(const IPv4& ip)
{
    _data.resize(VRRP_MAX_PACKET_SIZE);
    _vrrp.add_ip(ip);
}

void
VRRPPacket::set_source(const IPv4& ip)
{
    _ip.set_ip_src(ip);
}
