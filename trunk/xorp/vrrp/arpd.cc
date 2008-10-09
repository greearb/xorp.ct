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

#ident "$XORP: xorp/vrrp/arpd.cc,v 1.2 2008/10/09 17:55:51 abittau Exp $"

#include "vrrp_module.h"
#include "arpd.hh"
#include "vrrp_vif.hh"
#include "libxorp/xlog.h"

ARPd::ARPd(VRRPInterface& vif) 
		: _vif(vif),
		  _running(false),
		  _receiving(false)
{
}

ARPd::~ARPd()
{
    if (_running)
	stop();
}

void
ARPd::start()
{
    XLOG_ASSERT(!_running);

    _running = true;

    ips_updated();
}

void
ARPd::stop()
{
    XLOG_ASSERT(_running);

    stop_receiving();

    _running = false;
}

void
ARPd::clear()
{
    _ips.clear();
}

void
ARPd::insert(const IPv4& ip)
{
    XLOG_ASSERT(_ips.find(ip) == _ips.end());
    _ips.insert(ip);
}

void
ARPd::ips_updated()
{
    if (_ips.empty())
	stop_receiving();
    else
	start_receiving();
}

void
ARPd::start_receiving()
{
    if (!_running || _receiving)
	return;

    _vif.start_arps();

    _receiving = true;
}

void
ARPd::stop_receiving()
{
    if (!_running || !_receiving)
	return;

    _vif.stop_arps();

    _receiving = false;
}

void
ARPd::set_mac(const Mac& mac)
{
    _mac = mac;
}

void
ARPd::recv(const Mac& src, const PAYLOAD& payload)
{
    if (!_receiving)
	return;

    const ARPHeader& ah = ARPHeader::assign(payload);

    if (!ah.is_request())
	return;

    IPv4 ip = ah.get_request();

    if (_ips.find(ip) == _ips.end())
	return;

    PAYLOAD reply;
    ah.make_reply(reply, _mac);

    _vif.send(_mac, src, ETHERTYPE_ARP, reply);
}
