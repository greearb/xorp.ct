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

// $XORP: xorp/rib/redist_xrl.hh,v 1.5 2004/05/18 21:35:47 hodson Exp $

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
    typedef list<Task*>		TaskQueue;

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
     * @param is_xrl_transaction_output if true, the add/delete route XRLs
     *        are grouped into transactions.
     */
    RedistXrlOutput(Redistributor<A>*	redistributor,
		    XrlRouter& 		xrl_router,
		    const string& 	from_protocol,
		    const string& 	xrl_target_name,
		    const string&	cookie);
    ~RedistXrlOutput();

    void add_route(const IPRouteEntry<A>& ipr);
    void delete_route(const IPRouteEntry<A>& ipr);

    void starting_route_dump();
    void finishing_route_dump();

    virtual void task_completed(Task* task);
    void task_failed_fatally(Task* task);

    inline const string& xrl_target_name() const;
    inline const string& cookie() const;

public:
    static const uint32_t HI_WATER	 = 100;
    static const uint32_t LO_WATER	 =   5;
    static const uint32_t MAX_RETRIES	 =  20;
    static const uint32_t RETRY_PAUSE_MS = 100;

protected:
    virtual void start_running_tasks();
    void start_next_task();

    inline void 	incr_task_count();
    inline void 	decr_task_count();
    inline uint32_t	task_count() const;

    inline void		enqueue_task(Task* task);
    inline void		dequeue_task(Task* task);

protected:
    XrlRouter&	_xrl_router;
    string	_from_protocol;
    string	_target_name;
    string	_cookie;

    TaskQueue	_tasks;
    uint32_t	_n_tasks;
};


/**
 * Route Redistributor output that sends route add and deletes to
 * remote redistribution target via the redist_transaction{4,6} xrl
 * interfaces.
 */
template <typename A>
class RedistTransactionXrlOutput : public RedistXrlOutput<A>
{
public:
    typedef typename RedistXrlOutput<A>::Task Task;

public:
    RedistTransactionXrlOutput(Redistributor<A>*	redistributor,
			       XrlRouter& 		xrl_router,
			       const string& 		from_protocol,
			       const string& 		xrl_target_name,
			       const string&		cookie);

    void add_route(const IPRouteEntry<A>& ipr);
    void delete_route(const IPRouteEntry<A>& ipr);

    void starting_route_dump();
    void finishing_route_dump();

    void task_completed(Task* task);

    inline uint32_t tid() const;
    inline void set_tid(uint32_t v);

    inline bool transaction_in_progress() const;
    inline void set_transaction_in_progress(bool v);

    inline bool transaction_in_error() const;
    inline void set_transaction_in_error(bool v);

    // The size of the transaction that is build-in-progress
    inline size_t transaction_size() const { return _transaction_size; }
    inline void reset_transaction_size() { _transaction_size = 0; }
    inline void incr_transaction_size() { _transaction_size++; }

    static const size_t MAX_TRANSACTION_SIZE	 = 100;

protected:
    void start_running_tasks();

protected:
    uint32_t	_tid;			// Send-in-progress transaction ID
    bool	_transaction_in_progress;
    bool	_transaction_in_error;
    size_t	_transaction_size;	// Build-in-progress transaction size
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


// ----------------------------------------------------------------------------
// Inline RedistrTransactionXrlOutput methods

template <typename A>
inline uint32_t
RedistTransactionXrlOutput<A>::tid() const
{
    return _tid;
}

template <typename A>
inline void
RedistTransactionXrlOutput<A>::set_tid(uint32_t v)
{
    _tid = v;
}

template <typename A>
inline bool
RedistTransactionXrlOutput<A>::transaction_in_progress() const
{
    return _transaction_in_progress;
}

template <typename A>
inline void
RedistTransactionXrlOutput<A>::set_transaction_in_progress(bool v)
{
    _transaction_in_progress = v;
}

template <typename A>
inline bool
RedistTransactionXrlOutput<A>::transaction_in_error() const
{
    return _transaction_in_error;
}

template <typename A>
inline void
RedistTransactionXrlOutput<A>::set_transaction_in_error(bool v)
{
    _transaction_in_error = v;
}

#endif // __RIB_REDIST_XRL_HH__
