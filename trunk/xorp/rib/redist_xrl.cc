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

#ident "$XORP: xorp/devnotes/template.cc,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $"

#include <list>
#include <string>

#include "rib_module.h"
#include "config.h"

#include "libxorp/safe_callback_obj.hh"
#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/redist4_xif.hh"
#include "xrl/interfaces/redist6_xif.hh"

#include "route.hh"
#include "redist_xrl.hh"

/**
 * Base class for RedistXrlOutput Tasks.  Classes derived from this
 * store enough state to dispatch XRL, or other task, at some
 * subsequent time in the future.
 */
template <typename A>
class RedistXrlTask : public CallbackSafeObject
{
public:
    RedistXrlTask(RedistXrlOutput<A>* parent)
	: _parent(parent), _attempts(0)
    {}
    virtual ~RedistXrlTask() {}

    /**
     * @return true on success, false if XrlRouter could not dispatch
     * request.
     */
    virtual bool dispatch(XrlRouter& xrl_router) = 0;

    /**
     * Get number of times dispatch() invoked on instance.
     */
    inline uint32_t dispatch_attempts() const		{ return _attempts; }

protected:
    inline void incr_dispatch_attempts()		{ _attempts++; }
    inline RedistXrlOutput<A>* parent()			{ return _parent; }
    inline const RedistXrlOutput<A>* parent() const	{ return _parent; }

    inline void signal_complete_ok()	{ _parent->task_completed(this); }
    inline void signal_fatal_failure()	{ _parent->task_failed_fatally(this); }

private:
    RedistXrlOutput<A>* _parent;
    uint32_t		_attempts;
};


// ----------------------------------------------------------------------------
// Task declarations

template <typename A>
class AddRoute : public RedistXrlTask<A>
{
public:
    AddRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr);
    virtual bool dispatch(XrlRouter& xrl_router);
    void dispatch_complete(const XrlError& xe);
protected:
    IPNet<A>	_net;
    A		_nh;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    uint32_t	_admin_distance;
};

template <typename A>
class DeleteRoute : public RedistXrlTask<A>
{
public:
    DeleteRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr);
    virtual bool dispatch(XrlRouter& xrl_router);
    void dispatch_complete(const XrlError& xe);
protected:
    IPNet<A>	_net;
};

template <typename A>
class Pause : public RedistXrlTask<A>
{
public:
    Pause(RedistXrlOutput<A>* parent, uint32_t ms);
    virtual bool dispatch(XrlRouter&  xrl_router);
    void expire();
private:
    XorpTimer _t;
    uint32_t  _p_ms;
};


// ----------------------------------------------------------------------------
// AddRoute implementation

template <typename A>
AddRoute<A>::AddRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr)
    : RedistXrlTask<A>(parent), _net(ipr.net()), _nh(ipr.nexthop_addr()),
      _ifname(ipr.vif()->ifname()), _vifname(ipr.vif()->name()),
      _metric(ipr.metric()), _admin_distance(ipr.admin_distance())
{
}

template <>
bool
AddRoute<IPv4>::dispatch(XrlRouter& xrl_router)
{
    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_add_route(this->parent()->xrl_target_name().c_str(),
			     _net, _nh, _ifname, _vifname, _metric,
			     _admin_distance, this->parent()->cookie(),
			     callback(this, &AddRoute<IPv4>::dispatch_complete)
			     );
}

template <>
bool
AddRoute<IPv6>::dispatch(XrlRouter& xrl_router)
{
    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_add_route(this->parent()->xrl_target_name().c_str(),
			     _net, _nh, _ifname, _vifname, _metric,
			     _admin_distance, this->parent()->cookie(),
			     callback(this, &AddRoute<IPv6>::dispatch_complete)
			     );

}

template <typename A>
void
AddRoute<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_INFO("Failed to redistribute route add for %s\n",
		  _net.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // For now all errors are signalled fatal
    XLOG_ERROR("Fatal error during route redistribution \"%s\"",
	       xe.str().c_str());

    this->signal_fatal_failure();
}

// ----------------------------------------------------------------------------
// DeleteRoute implementation

template <typename A>
DeleteRoute<A>::DeleteRoute(RedistXrlOutput<A>* parent,
			    const IPRouteEntry<A>& ipr)
    : RedistXrlTask<A>(parent), _net(ipr.net())
{
}

template <>
bool
DeleteRoute<IPv4>::dispatch(XrlRouter& xrl_router)
{
    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_delete_route(
		this->parent()->xrl_target_name().c_str(),
		_net, this->parent()->cookie(),
		callback(this, &DeleteRoute<IPv4>::dispatch_complete)
		);
}

template <>
bool
DeleteRoute<IPv6>::dispatch(XrlRouter& xrl_router)
{
    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_delete_route(
		this->parent()->xrl_target_name().c_str(),
		_net, this->parent()->cookie(),
		callback(this, &DeleteRoute<IPv6>::dispatch_complete)
		);
}

template <typename A>
void
DeleteRoute<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_INFO("Failed to redistribute route add for %s\n",
		  _net.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // XXX For now all errors signalled as fatal
    XLOG_INFO("Fatal error during route redistribution \"%s\"",
	      xe.str().c_str());
    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// Pause implementation

template <typename A>
Pause<A>::Pause(RedistXrlOutput<A>* parent, uint32_t ms)
    : RedistXrlTask<A>(parent), _p_ms(ms)
{
}

template <typename A>
bool
Pause<A>::dispatch(XrlRouter& xrl_router)
{
    this->incr_dispatch_attempts();
    EventLoop& e = xrl_router.eventloop();
    _t = e.new_oneoff_after_ms(_p_ms, callback(this, &Pause<A>::expire));
    return true;
}

template <typename A>
void
Pause<A>::expire()
{
    this->signal_complete_ok();
}


// ----------------------------------------------------------------------------
// RedistXrlOutput implementation

template <typename A>
RedistXrlOutput<A>::RedistXrlOutput(Redistributor<A>*	redistributor,
				    XrlRouter&		xrl_router,
				    const string&	from_protocol,
				    const string&	xrl_target_name,
				    const string&	cookie)
    : RedistOutput<A>(redistributor), _xrl_router(xrl_router),
      _from_protocol(from_protocol), _target_name(xrl_target_name),
      _cookie(cookie)
{
}

template <typename A>
RedistXrlOutput<A>::~RedistXrlOutput()
{
    while (_tasks.empty() == false) {
	delete _tasks.front();
	_tasks.pop_front();
    }
    _n_tasks = 0;
}

template <typename A>
void
RedistXrlOutput<A>::incr_task_count()
{
    _n_tasks++;
    if (_n_tasks == HI_WATER) {
	this->announce_high_water();
    }
}

template <typename A>
void
RedistXrlOutput<A>::decr_task_count()
{
    _n_tasks--;
    if (_n_tasks == LO_WATER - 1) {
	this->announce_low_water();
    }
}

template <typename A>
uint32_t
RedistXrlOutput<A>::task_count() const
{
    return _n_tasks;
}

template <typename A>
void
RedistXrlOutput<A>::enqueue_task(Task* task)
{
    _tasks.push_back(task);
    incr_task_count();
}

template <typename A>
void
RedistXrlOutput<A>::dequeue_task(Task* task)
{
    XLOG_ASSERT(task == _tasks.front());
    delete _tasks.front();
    _tasks.pop_front();
    decr_task_count();
}

template <typename A>
void
RedistXrlOutput<A>::add_route(const IPRouteEntry<A>& ipr)
{
    enqueue_task(new AddRoute<A>(this, ipr));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::delete_route(const IPRouteEntry<A>& ipr)
{
    enqueue_task(new DeleteRoute<A>(this, ipr));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::start_running_tasks()
{
    XLOG_ASSERT(task_count() == 1);
    start_next_task();
}

template <typename A>
void
RedistXrlOutput<A>::start_next_task()
{
    XLOG_ASSERT(task_count() >= 1);
    RedistXrlTask<A>* t = _tasks.front();

    if (t->dispatch(_xrl_router) == false) {
	if (t->dispatch_attempts() > MAX_RETRIES) {
	    XLOG_ERROR("Failed to dispatch command after %u attempts.",
		       t->dispatch_attempts());
	    // XXX signal failure
	}
	// Dispatch of task failed.  XrlRouter is presumeably backlogged.
	// Insert a delay and dispatch that to cause later attempt at failing
	// task.
	t = new Pause<A>(this, RETRY_PAUSE_MS);
	_tasks.push_front(t);
	incr_task_count();
	t->dispatch(_xrl_router);
    }
}

template <typename A>
void
RedistXrlOutput<A>::task_completed(RedistXrlTask<A>* task)
{
    dequeue_task(task);
    if (task_count() != 0) {
	start_next_task();
    }
}

template <typename A>
void
RedistXrlOutput<A>::task_failed_fatally(RedistXrlTask<A>* task)
{
    XLOG_ASSERT(_tasks.front() == task);
    delete _tasks.front();
    _tasks.pop_front();
    decr_task_count();
    this->announce_fatal_error();
}

template class RedistXrlOutput<IPv4>;
template class RedistXrlOutput<IPv6>;
