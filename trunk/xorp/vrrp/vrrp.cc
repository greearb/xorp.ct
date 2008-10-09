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

#ident "$XORP: xorp/vrrp/vrrp.cc,v 1.1 2008/10/09 17:40:58 abittau Exp $"

#include <sstream>

#include "vrrp_module.h"
#include "libxorp/xlog.h"
#include "vrrp.hh"
#include "vrrp_exception.hh"
#include "vrrp_vif.hh"
#include "vrrp_target.hh"

namespace {

template <class T>
void
out_of_range(const string& msg, const T& x)
{
    ostringstream oss;

    oss << msg << " (" << x << ")";

    xorp_throw(VRRPException, oss.str());
}

} // anonymous namespace

VRRP::VRRP(VRRPVif& vif, uint32_t vrid)
		: _vif(vif),
		  _vrid(vrid),
		  _priority(100),
		  _interval(1),
		  _skew_time(0),
		  _master_down_interval(0),
		  _preempt(true),
		  _state(INITIALIZE),
		  _own(false),
		  _disable(true)
{
    if (_vrid < 1 || _vrid > 255)
	out_of_range("VRID out of range", _vrid);

    EventLoop& e = VRRPTarget::eventloop();

    _master_down_timer = e.new_periodic_ms(0x29a,
			    callback(this, &VRRP::master_down_expiry));

    _adver_timer = e.new_periodic_ms(0x29a,
			    callback(this, &VRRP::adver_expiry));
    cancel_timers();

    setup_intervals();
}

VRRP::~VRRP()
{
    stop();
}

void
VRRP::set_priority(uint32_t priority)
{
    if (priority == PRIORITY_LEAVE || priority >= PRIORITY_OWN)
	out_of_range("priority out of range", priority);

    _priority = priority;

    setup_intervals();
}

void
VRRP::set_interval(uint32_t interval)
{
    _interval = interval;

    setup_intervals();
}

void
VRRP::set_preempt(bool preempt)
{
    _preempt = preempt;
}

void
VRRP::set_disable(bool disable)
{
    _disable = disable;

    if (_disable)
	stop();
    else
	start();
}

void
VRRP::add_ip(const IPv4& ip)
{
    _ips.insert(ip);

    check_ownership();
}

void
VRRP::delete_ip(const IPv4& ip)
{
    _ips.erase(ip);

    check_ownership();
}

void
VRRP::setup_intervals()
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
VRRP::setup_timers(bool skew)
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
VRRP::check_ownership()
{
    bool own = true;

    for (IPS::iterator i = _ips.begin(); i != _ips.end(); ++i) {
	own &= _vif.own(*i);

	if (!own)
	    break;
    }

    _own = own;
    setup_intervals();
}

uint32_t
VRRP::priority() const
{
    if (_own)
	return PRIORITY_OWN;

    return _priority;
}

void
VRRP::start()
{
    if (running())
	return;

    if (!_vif.ready())
	return;

    if (priority() == PRIORITY_OWN)
	become_master();
    else
	become_backup();
}

void
VRRP::become_master()
{
    _state = MASTER;
    send_advertisement();
    send_arps();
    setup_timers();
}

void
VRRP::become_backup()
{
    _state = BACKUP;
    setup_timers();
}

void
VRRP::stop()
{
    if (!running())
	return;

    cancel_timers();

    if (_state == MASTER)
	send_advertisement(PRIORITY_LEAVE);

    _state = INITIALIZE;
}

bool
VRRP::running() const
{
    return _state != INITIALIZE;
}

void
VRRP::cancel_timers()
{
    _master_down_timer.unschedule();
    _adver_timer.unschedule();
}

void
VRRP::send_advertisement()
{
    send_advertisement(priority());
}

void
VRRP::send_advertisement(uint32_t priority)
{
    XLOG_ASSERT(priority <= PRIORITY_OWN);
    XLOG_ASSERT(_state == MASTER);
}

void
VRRP::send_arps()
{
    XLOG_ASSERT(_state == MASTER);
}

bool
VRRP::master_down_expiry()
{
    XLOG_ASSERT(_state == BACKUP);

    become_master();

    return false;
}

bool
VRRP::adver_expiry()
{
    XLOG_ASSERT(_state == MASTER);

    send_advertisement();
    setup_timers();

    return false;
}

void
VRRP::recv_advertisement(const IPv4& from, uint32_t priority)
{
    XLOG_ASSERT(priority <= PRIORITY_OWN);

    switch (_state) {
    case INITIALIZE:
	XLOG_ASSERT(false);
	break;

    case BACKUP:
	recv_adver_backup(priority);
	break;

    case MASTER:
	recv_adver_master(from, priority);
	break;
    }
}

void
VRRP::recv_adver_backup(uint32_t pri)
{
    if (pri == PRIORITY_LEAVE)
	setup_timers(true);
    else if (!_preempt || pri >= priority())
	setup_timers();
}

void
VRRP::recv_adver_master(const IPv4& from, uint32_t pri)
{
    if (pri == PRIORITY_LEAVE) {
	send_advertisement();
	setup_timers();
    } else if (pri >= priority() || (pri == priority() && from > _vif.addr()))
	become_backup();
}
