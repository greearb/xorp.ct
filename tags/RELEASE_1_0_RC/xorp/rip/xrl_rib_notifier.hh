// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rip/xrl_rib_notifier.hh,v 1.2 2004/04/22 01:11:52 pavlin Exp $

#ifndef __RIP_XRL_RIB_NOTIFIER_HH__
#define __RIP_XRL_RIB_NOTIFIER_HH__

#include <set>
#include "libxorp/ipnet.hh"
#include "libxorp/service.hh"

#include "rib_notifier_base.hh"

class XrlSender;
class XrlRouter;

/**
 * @short Class to send RIP updates to RIB process.
 *
 * This class periodically checks the UpdateQueue for updates and
 * sends them to the RIB.  Once an instance has been created, the
 * @ref startup() method should be called to register a routing table for
 * RIP and begin checking for updates.  Before being destructed,
 * @ref shutdown() should be called to remove the RIP routing table from
 * the RIB.
 *
 * The XrlRibNotifier may be in one of several states enumerated by
 * @ref RunStatus.  Before startup(), an instances state is READY.
 * Then when startup is called it transitions into
 * INSTALLING_RIP_TABLE and transitions into RUNNING.  When in RUNNING
 * state updates are sent to the RIB.  When shutdown() is called it
 * enters UNINSTALLING_RIP_TABLE before entering SHUTDOWN.  At any
 * time it may fall into state FAILED if communication with the RIB
 * fails.
 */
template <typename A>
class XrlRibNotifier : public RibNotifierBase<A>, public ServiceBase
{
public:
    typedef RibNotifierBase<A> Super;

    static const uint32_t DEFAULT_INFLIGHT = 20;

public:
    /**
     * Constructor.
     */
    XrlRibNotifier(EventLoop&		e,
		   UpdateQueue<A>&	uq,
		   XrlRouter&		xr,
		   uint32_t		max_inflight = DEFAULT_INFLIGHT,
		   uint32_t		poll_ms = Super::DEFAULT_POLL_MS);

    /**
     * Constructor taking an XrlSender, a class name, and an instance
     * name as arguments.  These arguments are broken out for
     * debugging instances, ie a fake XrlSender can be used to test
     * behaviour of this class.
     */
    XrlRibNotifier(EventLoop&		e,
		   UpdateQueue<A>&	uq,
		   XrlSender&		xs,
		   const string&	class_name,
		   const string&	intance_name,
		   uint32_t		max_inflight = DEFAULT_INFLIGHT,
		   uint32_t		poll_ms = Super::DEFAULT_POLL_MS);

    ~XrlRibNotifier();

    /**
     * Request RIB instantiates a RIP routing table and once instantiated
     * start passing route updates to RIB.
     *
     * @return true on success, false on failure.
     */
    bool startup();

    /**
     * Stop forwarding route updates to RIB and request RIB
     * unregisters RIP routing table.
     *
     * @return true on success, false on failure.
     */
    bool shutdown();

    /**
     * Accessor returning the current number of Xrls inflight.
     */
    inline uint32_t xrls_inflight() const;

    /**
     * Accessor returning the maximum number of Xrls inflight.
     */
    inline uint32_t max_xrls_inflight() const;

protected:
    void updates_available();

private:
    void add_igp_cb(const XrlError& e);
    void delete_igp_cb(const XrlError& e);

    void send_add_route(const RouteEntry<A>& re);
    void send_delete_route(const RouteEntry<A>& re);
    void send_route_cb(const XrlError& e);

    inline void incr_inflight();
    inline void decr_inflight();

protected:
    XrlSender&		_xs;
    string		_cname;
    string		_iname;
    const uint32_t	_max_inflight;
    uint32_t		_inflight;

    set<IPNet<A> >	_ribnets;	// XXX hack
};

// ----------------------------------------------------------------------------
// Public inline method definitions

template <typename A>
inline uint32_t
XrlRibNotifier<A>::xrls_inflight() const
{
    return _inflight;
}

template <typename A>
inline uint32_t
XrlRibNotifier<A>::max_xrls_inflight() const
{
    return _max_inflight;
}

#endif // __RIP_XRL_RIB_NOTIFIER_HH__
