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

#include "libxorp/c_format.hh"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/safe_callback_obj.hh"

#include "libxipc/xrl_router.hh"

#include "constants.hh"
#include "route_db.hh"
#include "system.hh"

#include "xrl_config.hh"
#include "xrl_redist_manager.hh"

#include "xrl/interfaces/rib_xif.hh"

// ----------------------------------------------------------------------------
// Predicate for finding RouteRedistributor objects with a given protocol name

template <typename A>
struct is_redistributor_of {
    is_redistributor_of(const string& name) : _n(name) {}
    inline bool operator() (const RouteRedistributor<A>* rr) const
    {
	return rr->protocol() == _n;
    }
    const string _n;
};


// ----------------------------------------------------------------------------
// RedistJob

template <typename A>
class RedistJob : public CallbackSafeObject {
public:
    RedistJob(XrlRedistManager<A>& xrm) : _xrm(xrm), _attempts(0) {}
    virtual ~RedistJob() {}

    /**
     * Dispatch.
     *
     * @return true if dispatch succeeded, false if dispatch failed
     * and should be retried.
     */
    virtual bool dispatch() = 0;

    /**
     * Get a textual description of job.
     */
    virtual string str() const = 0;

    /**
     * Signal to parent XrlRedistManager that dispatch is completed.
     */
    inline void signal_complete()		{ _xrm.job_completed(this); }

    /**
     * Return number of times dispatch invoked.
     */
    inline uint32_t dispatch_attempts() const		{ return _attempts; }

    /**
     * Increment number of dispatch attempts.
     */
    inline void incr_dispatch_attempts()		{ _attempts++; }

    inline XrlRedistManager<A>& manager()		{ return _xrm; }
    inline const XrlRedistManager<A>& manager() const	{ return _xrm; }


protected:
    XrlRedistManager<A>& _xrm;
    bool		 _attempts;
};


/**
 * Class to send Xrl requesting addition of a redistribution.
 */
template <typename A>
class XrlRedistEnable : public RedistJob<A> {
public:
    typedef typename XrlRedistManager<A>::RedistList RedistList;

public:
    XrlRedistEnable(XrlRedistManager<A>&	xrm,
		    RedistList&			redists,
		    const string& 		protocol,
		    uint16_t			cost,
		    uint16_t			tag)
	: RedistJob<A>(xrm), _redists(redists), _protocol(protocol),
	  _cost(cost), _tag(tag)
    {}
    bool dispatch();
    void dispatch_complete(const XrlError& xe);
    inline const string& protocol() const		{ return _protocol; }
    inline const uint16_t& cost() const			{ return _cost; }
    inline const uint16_t& tag() const			{ return _tag; }
    string str() const;
protected:
    RedistList  _redists;
    string	_protocol;
    uint16_t	_cost;
    uint16_t	_tag;
};

#ifdef INSTANTIATE_IPV4
template <>
bool
XrlRedistEnable<IPv4>::dispatch()
{
    this->incr_dispatch_attempts();

    XrlRouter& 		xr = this->manager().xrl_router();
    XrlRibV0p1Client	cl(&xr);

    return cl.send_redist_enable4(
		xrl_rib_name(), xr.instance_name(), _protocol, "rip",
		true /* unicast */, false /* multicast */, _protocol,
		callback(this, &XrlRedistEnable<IPv4>::dispatch_complete)
		);
}
#endif /* INSTANTIATE_IPV4 */

#ifdef INSTANTIATE_IPV6
template <>
bool
XrlRedistEnable<IPv6>::dispatch()
{
    this->incr_dispatch_attempts();

    XrlRouter& 		xr = this->manager().xrl_router();
    XrlRibV0p1Client	cl(&xr);

    return cl.send_redist_enable6(
		xrl_rib_name(), xr.instance_name(), _protocol, "rip",
		true /* unicast */, false /* multicast */, _protocol,
		callback(this, &XrlRedistEnable<IPv6>::dispatch_complete)
		);
}
#endif /* INSTANTIATE_IPV6 */

template <typename A>
string
XrlRedistEnable<A>::str() const
{
    return c_format("Redistribution enable protocol \"%s\"",
		    protocol().c_str());
}

template <typename A>
void
XrlRedistEnable<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	RouteRedistributor<A>* rr =
	    new RouteRedistributor<A>(this->manager().route_db(),
				      this->protocol(),
				      this->cost(),
				      this->tag());
	_redists.push_back(rr);
    } else {
	XLOG_INFO("Failed %s - XRL failed: \"%s\"",
		  this->str().c_str(), xe.str().c_str());
    }
    this->signal_complete();
}


/**
 * Class to send Xrl requesting the stopping of a redistribution.
 */
template <typename A>
class XrlRedistDisable : public RedistJob<A> {
public:
    typedef typename XrlRedistManager<A>::RedistList RedistList;

public:
    XrlRedistDisable(XrlRedistManager<A>&	xm,
		     RedistList&		redists,
		     RedistList&		dead_redists,
		     const string& 		protocol)
	: RedistJob<A>(xm), _redists(redists), _dead_redists(dead_redists),
	  _protocol(protocol)
    {}
    bool dispatch();
    void dispatch_complete(const XrlError& xe);
    inline const string& protocol() const		{ return _protocol; }
    string str() const;

protected:
    RedistList& _redists;
    RedistList& _dead_redists;
    string	_protocol;
};

#ifdef INSTANTIATE_IPV4
template <>
bool
XrlRedistDisable<IPv4>::dispatch()
{
    this->incr_dispatch_attempts();

    XrlRouter& 		xr = this->manager().xrl_router();
    XrlRibV0p1Client	cl(&xr);

    return cl.send_redist_disable4(
		xrl_rib_name(), xr.instance_name(), _protocol, "rip",
		true /* unicast */, false /* multicast */,
		callback(this, &XrlRedistDisable<IPv4>::dispatch_complete)
		);
}
#endif /* INSTANTIATE_IPV4 */

#ifdef INSTANTIATE_IPV6
template <>
bool
XrlRedistDisable<IPv6>::dispatch()
{
    this->incr_dispatch_attempts();

    XrlRouter& 		xr = this->manager().xrl_router();
    XrlRibV0p1Client	cl(&xr);

    return cl.send_redist_disable6(
		xrl_rib_name(), xr.instance_name(), _protocol, "rip",
		true /* unicast */, false /* multicast */,
		callback(this, &XrlRedistDisable<IPv6>::dispatch_complete)
		);
}
#endif /* INSTANTIATE_IPV6 */

template <typename A>
string
XrlRedistDisable<A>::str() const
{
    return c_format("Redistribution disable protocol \"%s\"",
		    protocol().c_str());
}

template <typename A>
void
XrlRedistDisable<A>::dispatch_complete(const XrlError& xe)
{
    if (xe != XrlError::OKAY()) {
	XLOG_INFO("Failed %s - XRL failed: \"%s\"",
		  this->str().c_str(), xe.str().c_str());
    }

    typename RedistList::iterator i;
    i = find_if(_redists.begin(), _redists.end(),
		is_redistributor_of<A>(this->protocol()));
    if (i != _redists.end()) {
	_dead_redists.splice(_dead_redists.begin(), _redists, i);
	_dead_redists.front()->withdraw_routes();
    }

    this->signal_complete();
}


// ----------------------------------------------------------------------------
// Pause job

template <typename A>
class Pause : public RedistJob<A>
{
public:
    Pause(XrlRedistManager<A>& xrm, uint32_t pause_ms) :
	RedistJob<A>(xrm), _p_ms(pause_ms) {}
    bool dispatch();
    string str() const;
protected:
    void expire();

    uint32_t	_p_ms;
    XorpTimer	_t;
};

template <typename A>
bool
Pause<A>::dispatch()
{
    this->incr_dispatch_attempts();

    EventLoop& e = this->manager().eventloop();
    _t = e.new_oneoff_after_ms(_p_ms, callback(this, &Pause<A>::expire));

    return true;
}

template <typename A>
void
Pause<A>::expire()
{
    this->signal_complete();
}

template <typename A>
string
Pause<A>::str() const
{
    return c_format("Pause %d ms", this->_p_ms);
}

// ----------------------------------------------------------------------------
// XrlRedistManager Implementation

template <typename A>
XrlRedistManager<A>::XrlRedistManager(EventLoop& 	e,
				      RouteDB<A>& 	rdb,
				      XrlRouter& 	xr)
    : _e(e), _rdb(rdb), _xr(xr)
{
}

template <typename A>
XrlRedistManager<A>::XrlRedistManager(System<A>&	system,
				      XrlRouter&	xr)
    : _e(system.eventloop()), _rdb(system.route_db()), _xr(xr)
{
}

template <typename A>
XrlRedistManager<A>::~XrlRedistManager()
{
    while (_jobs.empty() == false) {
	delete _jobs.front();
	_jobs.pop_front();
    }

    _dead_redists.splice(_dead_redists.begin(), _redists);
    while (_dead_redists.empty() == false) {
	delete _dead_redists.front();
	_dead_redists.pop_front();
    }
}

template <typename A>
bool
XrlRedistManager<A>::startup()
{
    if (status() == READY) {
	set_status(RUNNING);
	return true;
    }
    return false;
}

template <typename A>
bool
XrlRedistManager<A>::shutdown()
{
    if (status() != RUNNING) {
	return false;
    }

    while (_redists.empty() == false) {
	RouteRedistributor<A>* rr = _redists.front();
	rr->withdraw_routes();
	_dead_redists.splice(_dead_redists.begin(),
			     _redists, _redists.begin());
    }
    set_status(SHUTDOWN);

    return true;
}

template <typename A>
void
XrlRedistManager<A>::add_route(const string&	protocol,
			       const Net&	net,
			       const Addr&	nh)
{
    typename RedistList::iterator i;

    i = find_if(_redists.begin(), _redists.end(),
		is_redistributor_of<A>(protocol));
    if (i != _redists.end()) {
	RouteRedistributor<A>* rr = *i;
	rr->add_route(net, nh);
	return;
    }

    i = find_if(_dead_redists.begin(), _dead_redists.end(),
		is_redistributor_of<A>(protocol));
    if (i == _dead_redists.end()) {
	XLOG_INFO("Received add route for redistribution from unknown "
		  "protocol \"%s\"", protocol.c_str());
    }
}

template <typename A>
void
XrlRedistManager<A>::delete_route(const string&	protocol,
				  const Net&	net)
{
    typename RedistList::iterator i;

    i = find_if(_redists.begin(), _redists.end(),
		is_redistributor_of<A>(protocol));
    if (i != _redists.end()) {
	RouteRedistributor<A>* rr = *i;
	rr->expire_route(net);
	return;
    }

    i = find_if(_dead_redists.begin(), _dead_redists.end(),
		is_redistributor_of<A>(protocol));
    if (i == _dead_redists.end()) {
	XLOG_INFO("Received route delete for redistribution from unknown "
		  "protocol \"%s\"", protocol.c_str());
    }
}

template <typename A>
void
XrlRedistManager<A>::request_redist_for(const string&	protocol,
					uint16_t	cost,
					uint16_t	tag)
{
    bool invoke_run = _jobs.empty();	// new job will be only one

    _jobs.push_back(
	new XrlRedistEnable<A>(*this, _redists, protocol, cost, tag)
	);

    if (invoke_run)
	run_next_job();
}

template <typename A>
void
XrlRedistManager<A>::request_no_redist_for(const string& protocol)
{
    bool invoke_run = _jobs.empty();	// new job will be only one

    _jobs.push_back(
	new XrlRedistDisable<A>(*this, this->_redists, this->_dead_redists,
				protocol)
	);

    if (invoke_run)
	run_next_job();
}

template <typename A>
void
XrlRedistManager<A>::job_completed(const RedistJob<A>* job)
{
    // Make sure job is next one we're expecting to complete
    XLOG_ASSERT(job == _jobs.front());

    // Delete front item and remove list entry
    delete _jobs.front();
    _jobs.pop_front();

    if (_jobs.empty() == false)
	run_next_job();
}

template <typename A>
void
XrlRedistManager<A>::run_next_job()
{
    XLOG_ASSERT(_jobs.empty() == false);

    RedistJob<A>* rj = _jobs.front();
    if (rj->dispatch() == true) {
	// Dispatch successful, no additional processing necessary.
	return;
    }

    // Dispatch failed.  Attempt a new dispatch a little if below threshold
    // retries or announce failure.
    static const uint32_t max_retries = 10;
    static const uint32_t retry_wait_ms = 100;

    if (rj->dispatch_attempts() < max_retries) {
	_jobs.push_front(new Pause<A>(*this, retry_wait_ms));
	_jobs.front()->dispatch();
	return;
    }

    XLOG_INFO("Failed to dispatch command %s after %d attempts - giving up.",
	      rj->str().c_str(), max_retries);
}

#ifdef INSTANTIATE_IPV4
template class XrlRedistManager<IPv4>;
#endif /* INSTANTIATE_IPV4 */

#ifdef INSTANTIATE_IPV6
template class XrlRedistManager<IPv6>;
#endif /* INSTANTIATE_IPV6 */
