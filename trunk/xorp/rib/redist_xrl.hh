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

// $XORP: xorp/devnotes/template.hh,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $

#ifndef __RIB_REDIST_XRL_HH__
#define __RIB_REDIST_XRL_HH__

#include "rt_tab_redist.hh"

class XrlRouter;

template <typename A> class RedistXrlTask;

/**
 * Route Redistributor output that sends route add and deletes to
 * remote redistribution target via the redist{4,6} xrl interfaces.
 */
template <typename A>
class RedistXrlOutput : public RedistOutput<A>
{
public:
    typedef RedistXrlTask<A>	Task;
    typedef list<Task*> TaskQueue;

public:
    /**
     * Constructor.
     *
     * @param redistributor originator of route add and delete requests.
     * @param xrl_router router to be used to send XRLs.
     * @param from_protocol name of protocol routes are redistributed from.
     * @param xrl_target_name name of XRL entity to send XRLs to.
     * @param cookie cookie passed in redist interface XRLs to identify
     *        source of updates.
     */
    RedistXrlOutput(Redistributor<A>*	redistributor,
		    XrlRouter& 		xrl_router,
		    const string& 	from_protocol,
		    const string& 	xrl_target_name,
		    const string&	cookie);
    ~RedistXrlOutput();

    void add_route(const IPRouteEntry<A>& ipr);
    void delete_route(const IPRouteEntry<A>& ipr);

    void task_completed(Task* task);
    void task_failed_fatally(Task* task);

    inline const string& xrl_target_name() const;
    inline const string& cookie() const;

public:
    static const uint32_t HI_WATER	 = 100;
    static const uint32_t LO_WATER	 =   5;
    static const uint32_t MAX_RETRIES	 =  20;
    static const uint32_t RETRY_PAUSE_MS = 100;

protected:
    void start_running_tasks();
    void start_next_task();

    inline void 	incr_task_count();
    inline void 	decr_task_count();
    inline uint32_t	task_count() const;

    inline void		enqueue_task(Task* task);
    inline void		dequeue_task(Task* task);

private:
    XrlRouter&	_xrl_router;
    string	_from_protocol;
    string	_target_name;
    string	_cookie;

    TaskQueue	_tasks;
    uint32_t	_n_tasks;
};


// ----------------------------------------------------------------------------
// Globally accessible RedistXrlOutput inline methods

template <typename A>
const string&
RedistXrlOutput<A>::xrl_target_name() const
{
    return _target_name;
}

template <typename A>
const string&
RedistXrlOutput<A>::cookie() const
{
    return _cookie;
}

#endif // __RIB_REDIST_XRL_HH__
