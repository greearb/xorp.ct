// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#include "vrrp_module.h"
#include "arpd.hh"
#include "vrrp_vif.hh"
#include "libxorp/xlog.h"

ARPd::ARPd(VRRPVif& vif) 
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
