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

#ident "$XORP: xorp/rip/xrl_rib_notifier.cc,v 1.2 2004/01/13 20:38:33 hodson Exp $"

#define DEBUG_LOGGING

#include "rip_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/rib_xif.hh"

#include "constants.hh"
#include "xrl_rib_notifier.hh"

static const char* RIBNAME = "rib";


// ----------------------------------------------------------------------------
// Type specific function pointers.
//
// The following type specific declarations and instantiations hold
// function pointers to methods in the XrlRibV0p1Client class.  This
// class contains similar methods for adding, updating, removing
// routes for IPv4 and IPv6.  The methods have the same forms but have
// different names and take different address argument types.  By
// specializing just the function pointers, we avoid any
// specialization of send related code in the XrlRibNotifier class
// later on in this file.

template <typename A>
struct Send {
    typedef bool (XrlRibV0p1Client::*AddIgpTable)
	(const char*, const string&, const string&, const string&,
	 const bool&, const bool&,
	 const XrlRibV0p1Client::AddIgpTable4CB&);

    typedef bool (XrlRibV0p1Client::*DeleteIgpTable)
	(const char*, const string&, const string&, const string&,
	 const bool&, const bool&,
	 const XrlRibV0p1Client::DeleteIgpTable4CB&);

    typedef bool (XrlRibV0p1Client::*AddRoute)
	(const char*, const string&, const bool&, const bool&,
	 const IPNet<A>&, const A&, const uint32_t&,
	 const XrlRibV0p1Client::AddRoute6CB&);

    typedef bool (XrlRibV0p1Client::*ReplaceRoute)
	(const char*, const string&, const bool&, const bool&,
	 const IPNet<A>&, const A&, const uint32_t&,
	 const XrlRibV0p1Client::ReplaceRoute6CB&);

    typedef bool (XrlRibV0p1Client::*DeleteRoute)
	(const char*, const string&, const bool&, const bool&,
	 const IPNet<A>&,
	 const XrlRibV0p1Client::DeleteRoute4CB&);

    static AddIgpTable		add_igp_table;
    static DeleteIgpTable	delete_igp_table;
    static AddRoute		add_route;
    static ReplaceRoute		replace_route;
    static DeleteRoute		delete_route;
};


//
// IPv4 Specialized Function pointers
//

#ifdef INSTANTIATE_IPV4
template <>
Send<IPv4>::AddIgpTable
Send<IPv4>::add_igp_table = &XrlRibV0p1Client::send_add_igp_table4;

template <>
Send<IPv4>::DeleteIgpTable
Send<IPv4>::delete_igp_table = &XrlRibV0p1Client::send_delete_igp_table4;

template <>
Send<IPv4>::AddRoute
Send<IPv4>::add_route = &XrlRibV0p1Client::send_add_route4;

template <>
Send<IPv4>::ReplaceRoute
Send<IPv4>::replace_route = &XrlRibV0p1Client::send_replace_route4;

template <>
Send<IPv4>::DeleteRoute
Send<IPv4>::delete_route = &XrlRibV0p1Client::send_delete_route4;
#endif // INSTANTIATE_IPV4

//
// IPv6 Specialized Function pointers
//
#ifdef INSTANTIATE_IPV6
template <>
Send<IPv6>::AddIgpTable
Send<IPv6>::add_igp_table = &XrlRibV0p1Client::send_add_igp_table6;

template <>
Send<IPv6>::DeleteIgpTable
Send<IPv6>::delete_igp_table = &XrlRibV0p1Client::send_delete_igp_table6;

template <>
Send<IPv6>::AddRoute
Send<IPv6>::add_route = &XrlRibV0p1Client::send_add_route6;

template <>
Send<IPv6>::ReplaceRoute
Send<IPv6>::replace_route = &XrlRibV0p1Client::send_replace_route6;

template <>
Send<IPv6>::DeleteRoute
Send<IPv6>::delete_route = &XrlRibV0p1Client::send_delete_route6;
#endif INSTANTIATE_IPV6


// ----------------------------------------------------------------------------
// XrlRibNotifier implementation

template <typename A>
XrlRibNotifier<A>::XrlRibNotifier(EventLoop&		e,
				  UpdateQueue<A>&	uq,
				  XrlRouter&		xr,
				  uint32_t		mf,
				  uint32_t		pms)
    : RibNotifierBase<A>(e, uq, pms), ServiceBase("RIB Updater"),
      _xs(xr), _cname(xr.class_name()), _iname(xr.instance_name()),
      _max_inflight(mf), _inflight(0)
{
    set_status(READY);
}

template <typename A>
XrlRibNotifier<A>::XrlRibNotifier(EventLoop&		e,
				  UpdateQueue<A>&	uq,
				  XrlSender&		xs,
				  const string&		class_name,
				  const string&		instance_name,
				  uint32_t		mf,
				  uint32_t		pms)
    : RibNotifierBase<A>(e, uq, pms),
      _xs(xs), _cname(class_name), _iname(instance_name),
      _max_inflight(mf), _inflight(0)
{
}

template <typename A>
XrlRibNotifier<A>::~XrlRibNotifier()
{
}

template <typename A>
inline void
XrlRibNotifier<A>::incr_inflight()
{
    _inflight++;
    XLOG_ASSERT(_inflight <= _max_inflight);
}

template <typename A>
inline void
XrlRibNotifier<A>::decr_inflight()
{
    _inflight--;
    XLOG_ASSERT(_inflight <= _max_inflight);
}


template<typename A>
void
XrlRibNotifier<A>::startup()
{
    XrlRibV0p1Client c(&_xs);
    bool ucast = true;
    bool mcast = false;

    if ((c.*Send<A>::add_igp_table)
	(RIBNAME, "rip", _cname, _iname, ucast, mcast,
	 callback(this, &XrlRibNotifier<A>::add_igp_cb)) == false) {
	XLOG_ERROR("Failed to send table creation request.");
	set_status(FAILED);
    }
    set_status(STARTING);
    incr_inflight();
}

template <typename A>
void
XrlRibNotifier<A>::add_igp_cb(const XrlError& xe)
{
    decr_inflight();
    if (xe != XrlError::OKAY()) {
	XLOG_ERROR("add_igp failed: %s\n", xe.str().c_str());
	set_status(FAILED);
	return;
    }
    start_polling();
    set_status(RUNNING);
}


template<typename A>
void
XrlRibNotifier<A>::shutdown()
{
    stop_polling();
    set_status(SHUTTING_DOWN);

    XrlRibV0p1Client c(&_xs);

    bool ucast = true;
    bool mcast = false;

    if ((c.*Send<A>::delete_igp_table)
	(RIBNAME, "rip", _cname, _iname, ucast, mcast,
	 callback(this, &XrlRibNotifier<A>::delete_igp_cb)) == false) {
	XLOG_ERROR("Failed to send table creation request.");
	set_status(FAILED);
    }
    incr_inflight();
}

template <typename A>
void
XrlRibNotifier<A>::delete_igp_cb(const XrlError& e)
{
    decr_inflight();
    if (e != XrlError::OKAY()) {
	set_status(FAILED);
	return;
    }
    set_status(SHUTDOWN);
}


template <typename A>
void
XrlRibNotifier<A>::send_add_route(const RouteEntry<A>& re)
{
    XrlRibV0p1Client c(&_xs);
    bool ok;

    if (_ribnets.find(re.net()) == _ribnets.end()) {
	_ribnets.insert(re.net());
	ok = (c.*Send<A>::add_route)
	      (RIBNAME, "rip", true, false,
	       re.net(), re.nexthop(), re.cost(),
	       callback(this, &XrlRibNotifier<A>::send_route_cb));
    } else {
	ok = (c.*Send<A>::replace_route)
	      (RIBNAME, "rip", true, false,
	       re.net(), re.nexthop(), re.cost(),
	       callback(this, &XrlRibNotifier<A>::send_route_cb));
    }

    if (ok == false) {
	shutdown();
	return;
    }
    incr_inflight();
}

template <typename A>
void
XrlRibNotifier<A>::send_delete_route(const RouteEntry<A>& re)
{
    typename set<IPNet<A> >::iterator i = _ribnets.find(re.net());
    if (i == _ribnets.end()) {
	debug_msg("Request to delete route to net %s that's not been passed"
		  "to rib\n", re.net().str().c_str());
	return;
    } else {
	_ribnets.erase(i);
    }

    XrlRibV0p1Client c(&_xs);
    if ((c.*Send<A>::delete_route)
	(RIBNAME, "rip", true, false, re.net(),
	 callback(this, &XrlRibNotifier<A>::send_route_cb)) == false) {
	shutdown();
	return;
    }
    incr_inflight();
}

template <typename A>
void
XrlRibNotifier<A>::send_route_cb(const XrlError& xe)
{
    decr_inflight();
    if (xe != XrlError::OKAY()) {
	XLOG_ERROR("Xrl error %s\n", xe.str().c_str());
    }
}


template <typename A>
void
XrlRibNotifier<A>::updates_available()
{
    XLOG_ASSERT(_inflight <= _max_inflight);
    for (const RouteEntry<A>* r = _uq.get(_ri); r != 0; r = _uq.next(_ri)) {
	if (_inflight == _max_inflight) {
	    break;
	}
	if (status() != RUNNING) {
	    // If we're not running just skip any available updates.
	    continue;
	}
	if (r->cost() < RIP_INFINITY) {
	    send_add_route(*r);
	} else {
	    send_delete_route(*r);
	}
    }
}

#ifdef INSTANTIATE_IPV4
template class XrlRibNotifier<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class XrlRibNotifier<IPv6>;
#endif
