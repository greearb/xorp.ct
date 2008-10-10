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

#ident "$XORP: xorp/vrrp/vrrp.cc,v 1.13 2008/10/10 01:30:06 pavlin Exp $"

#include <sstream>

#include "vrrp_module.h"
#include "libxorp/xlog.h"
#include "vrrp.hh"
#include "vrrp_exception.hh"
#include "vrrp_vif.hh"
#include "vrrp_target.hh"
#include "vrrp_packet.hh"

namespace {

template <class T>
void
out_of_range(const string& msg, const T& x)
{
    ostringstream oss;

    oss << msg << " (" << x << ")";

    xorp_throw(VrrpException, oss.str());
}

} // anonymous namespace

// XXX init from VrrpPacket::mcast_group
const Mac Vrrp::mcast_mac = Mac("01:00:5E:00:00:12");

Vrrp::Vrrp(VrrpInterface& vif, EventLoop& e, uint32_t vrid)
    : _vif(vif),
      _vrid(vrid),
      _priority(100),
      _interval(1),
      _skew_time(0),
      _master_down_interval(0),
      _preempt(true),
      _state(INITIALIZE),
      _own(false),
      _disable(true),
      _arpd(_vif)
{
    if (_vrid < 1 || _vrid > 255)
	out_of_range("VRID out of range", _vrid);

    char tmp[sizeof "ff:ff:ff:ff:ff:ff"];
    snprintf(tmp, sizeof(tmp), "00:00:5E:00:01:%X", (uint8_t) vrid);
    _source_mac = Mac(tmp);
    _arpd.set_mac(_source_mac);

    _master_down_timer = e.new_periodic_ms(0x29a,
			    callback(this, &Vrrp::master_down_expiry));

    _adver_timer = e.new_periodic_ms(0x29a,
			    callback(this, &Vrrp::adver_expiry));
    cancel_timers();

    setup_intervals();
}

Vrrp::~Vrrp()
{
    stop();
}

void
Vrrp::set_priority(uint32_t priority)
{
    if (priority == PRIORITY_LEAVE || priority >= PRIORITY_OWN)
	out_of_range("priority out of range", priority);

    _priority = priority;

    setup_intervals();
}

void
Vrrp::set_interval(uint32_t interval)
{
    _interval = interval;

    setup_intervals();
}

void
Vrrp::set_preempt(bool preempt)
{
    _preempt = preempt;
}

void
Vrrp::set_disable(bool disable)
{
    _disable = disable;

    if (_disable)
	stop();
    else
	start();
}

void
Vrrp::add_ip(const IPv4& ip)
{
    _ips.insert(ip);

    check_ownership();
}

void
Vrrp::delete_ip(const IPv4& ip)
{
    _ips.erase(ip);

    check_ownership();
}

void
Vrrp::setup_intervals()
{
    double skew_time	        = (256.0 - (double) priority()) / 256.0;
    double master_down_interval = 3.0 * (double) _interval + _skew_time;

    if (_skew_time != skew_time 
	|| _master_down_interval != master_down_interval) {

	_skew_time	      = skew_time;
	_master_down_interval = master_down_interval;
	setup_timers();
    }
}

void
Vrrp::setup_timers(bool skew)
{
    if (!running())
	return;

    cancel_timers();

    switch (_state) {
    case INITIALIZE:
	XLOG_ASSERT(false);
	break;

    case MASTER:
	_adver_timer.schedule_after_ms(_interval * 1000);
	break;

    case BACKUP:
	_master_down_timer.schedule_after_ms(
		(skew ? _skew_time : _master_down_interval) * 1000.0);
	break;
    }
}

void
Vrrp::check_ownership()
{
    bool own = true;

    _arpd.clear();

    for (IPS::iterator i = _ips.begin(); i != _ips.end(); ++i) {
	own &= _vif.own(*i);

	if (!own)
	    _arpd.insert(*i);
    }

    _arpd.ips_updated();

    _own = own;
    setup_intervals();
}

uint32_t
Vrrp::priority() const
{
    if (_own)
	return PRIORITY_OWN;

    return _priority;
}

void
Vrrp::start()
{
    if (running())
	return;

    if (!_vif.ready())
	return;

    _vif.join_mcast();

    if (priority() == PRIORITY_OWN)
	become_master();
    else
	become_backup();
}

void
Vrrp::become_master()
{
    _state = MASTER;

    _vif.add_mac(_source_mac);
    send_advertisement();
    send_arps();
    setup_timers();
    _arpd.start();
}

void
Vrrp::become_backup()
{
    if (_state == MASTER) {
	_vif.delete_mac(_source_mac);
	_arpd.stop();
    }

    /* TODO
     *
     * -  MUST NOT respond to ARP requests for the IP address(s) associated
     *    with the virtual router.
     *
     * -  MUST NOT accept packets addressed to the IP address(es) associated
     *    with the virtual router.
     */
    for (IPS::iterator i = _ips.begin(); i != _ips.end(); ++i) {
	const IPv4& ip = *i;

	// We should change IP / "disable" it at this point, and restore it
	// later.
	if (_vif.own(ip))
	    XLOG_WARNING("XXX we will be responding to %s", ip.str().c_str());
    }

    _state = BACKUP;

    setup_timers();
}

void
Vrrp::stop()
{
    if (!running())
	return;

    _vif.leave_mcast();

    cancel_timers();

    if (_state == MASTER) {
	send_advertisement(PRIORITY_LEAVE);
	_vif.delete_mac(_source_mac);
	_arpd.stop();
    }

    _state = INITIALIZE;
}

bool
Vrrp::running() const
{
    return _state != INITIALIZE;
}

void
Vrrp::cancel_timers()
{
    _master_down_timer.unschedule();
    _adver_timer.unschedule();
}

void
Vrrp::send_advertisement()
{
    send_advertisement(priority());
}

void
Vrrp::send_advertisement(uint32_t priority)
{
    XLOG_ASSERT(priority <= PRIORITY_OWN);
    XLOG_ASSERT(_state == MASTER);

    // XXX prepare only on change
    prepare_advertisement(priority);

    _vif.send(_source_mac, 
	      mcast_mac,
	      ETHERTYPE_IP,
	      _adv_packet.data());
}

void
Vrrp::prepare_advertisement(uint32_t priority)
{
    _adv_packet.set_source(_vif.addr());
    _adv_packet.set_vrid(_vrid);
    _adv_packet.set_priority(priority);
    _adv_packet.set_interval(_interval);
    _adv_packet.set_ips(_ips);
    _adv_packet.finalize();
}

void
Vrrp::send_arps()
{
    XLOG_ASSERT(_state == MASTER);

    for (IPS::iterator i = _ips.begin(); i != _ips.end(); ++i)
	send_arp(*i);
}

void
Vrrp::send_arp(const IPv4& ip)
{
    PAYLOAD data;

    ArpHeader::make_gratuitous(data, _source_mac, ip);

    _vif.send(_source_mac, Mac::BROADCAST(), ETHERTYPE_ARP, data);
}

bool
Vrrp::master_down_expiry()
{
    XLOG_ASSERT(_state == BACKUP);

    become_master();

    return false;
}

bool
Vrrp::adver_expiry()
{
    XLOG_ASSERT(_state == MASTER);

    send_advertisement();
    setup_timers();

    return false;
}

void
Vrrp::recv_advertisement(const IPv4& from, uint32_t priority)
{
    XLOG_ASSERT(priority <= PRIORITY_OWN);

    switch (_state) {
    case INITIALIZE:
	XLOG_ASSERT(false);
	break;

    case BACKUP:
	_last_adv = from;
	recv_adver_backup(priority);
	break;

    case MASTER:
	recv_adver_master(from, priority);
	break;
    }
}

void
Vrrp::recv_adver_backup(uint32_t pri)
{
    if (pri == PRIORITY_LEAVE)
	setup_timers(true);
    else if (!_preempt || pri >= priority())
	setup_timers();
}

void
Vrrp::recv_adver_master(const IPv4& from, uint32_t pri)
{
    if (pri == PRIORITY_LEAVE) {
	send_advertisement();
	setup_timers();
    } else if (pri >= priority() || (pri == priority() && from > _vif.addr()))
	become_backup();
}

void
Vrrp::recv(const IPv4& from, const VrrpHeader& vh)
{
    XLOG_ASSERT(vh.vh_vrid == _vrid);

    if (!running())
	xorp_throw(VrrpException, "VRRID not running");

    if (priority() == PRIORITY_OWN)
	xorp_throw(VrrpException, "I own VRRID but got advertisement");

    if (vh.vh_auth != VrrpHeader::VRRP_AUTH_NONE)
	xorp_throw(VrrpException, "Auth method not supported");

    if (!check_ips(vh) && vh.vh_priority != PRIORITY_OWN)
	xorp_throw(VrrpException, "Bad IPs");

    if (vh.vh_interval != _interval)
	xorp_throw(VrrpException, "Bad interval");

    recv_advertisement(from, vh.vh_priority);
}

bool
Vrrp::check_ips(const VrrpHeader& vh)
{
    if (vh.vh_ipcount != _ips.size()) {
	XLOG_WARNING("Mismatch in configured IPs (got %u have %u)",
		     vh.vh_ipcount, XORP_UINT_CAST(_ips.size()));

	return false;
    }

    for (unsigned i = 0; i < vh.vh_ipcount; i++) {
	IPv4 ip = vh.ip(i);

	if (_ips.find(ip) == _ips.end()) {
	    XLOG_WARNING("He's got %s configured but I don't",
			 ip.str().c_str());

	    return false;
	}
    }

    return true;
}

ARPd&
Vrrp::arpd()
{
    return _arpd;
}

void
Vrrp::get_info(string& state, IPv4& master) const
{
    typedef map<State, string> STATES;
    static STATES states;

    if (states.empty()) {
	states[INITIALIZE] = "initialize";
	states[MASTER]	   = "master";
	states[BACKUP]     = "backup";
    }

    state = states.find(_state)->second;

    if (_state == MASTER)
	master = _vif.addr();
    else
	master = _last_adv;
}
