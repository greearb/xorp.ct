// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2006-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/task.hh,v 1.13 2008/10/02 21:57:33 bms Exp $

#ifndef __LIBXORP_TASK_HH__
#define __LIBXORP_TASK_HH__

#include <map>

#include "debug.h"
#include "round_robin.hh"
#include "callback.hh"

#include <boost/noncopyable.hpp>

class TaskList;
class TaskNode;
class XorpTask;

typedef XorpCallback0<void>::RefPtr OneoffTaskCallback;
typedef XorpCallback0<bool>::RefPtr RepeatedTaskCallback;
typedef XorpCallback1<void, XorpTask&>::RefPtr BasicTaskCallback;


class TaskNode :
    public boost::noncopyable,
    public RoundRobinObjBase
{
public:
    TaskNode(TaskList* task_list, BasicTaskCallback cb);
    virtual ~TaskNode();

    void add_ref();
    void release_ref();

    void schedule(int priority, int weight);
    void reschedule();
    void unschedule();

    int priority()              const   { return _priority; }
    int weight()                const   { return _weight; }

    virtual void run(XorpTask &) {};     // Implemented by children

private:
    TaskList*	_task_list;	// TaskList this node is associated with
    BasicTaskCallback _cb;
    int		_ref_cnt;	// Number of referring XorpTask objects
    int		_priority;	// Scheduling priority
    int		_weight;	// Scheduling weight
};


class XorpTask {
public:
    XorpTask() : _task_node(NULL) {}
    XorpTask(const XorpTask& xorp_task);
    XorpTask(TaskNode* task_node);
    XorpTask(TaskList* task_list, BasicTaskCallback cb);
    ~XorpTask();

    //
    // Task/Timer priorities. Those are suggested values.
    //
    static const int PRIORITY_HIGHEST		= 0;
    static const int PRIORITY_XRL_KEEPALIVE	= 1;
    static const int PRIORITY_HIGH		= 2;
    static const int PRIORITY_DEFAULT		= 4;
    static const int PRIORITY_BACKGROUND	= 7;
    static const int PRIORITY_LOWEST		= 9;
    static const int PRIORITY_INFINITY		= 255;

    //
    // Task/Timer weights. Those are suggested values.
    //
    static const int WEIGHT_DEFAULT		= 1;

    XorpTask& operator=(const XorpTask& other);
    void schedule(int priority, int weight);
    void reschedule();
    void unschedule();
    bool scheduled() const;

private:
    TaskNode* _task_node;
};


class TaskList {
public:
    ~TaskList();

    /**
     * Expire all pending @ref XorpTask objects associated with @ref
     * TaskList.
     */
    void run();

    /**
     * Create a XorpTask that will be scheduled only once.
     *
     * @param cb callback object that is invoked when task is run.
     *
     * @return the @ref XorpTask created.
     */
    XorpTask new_oneoff_task(const OneoffTaskCallback& cb,
			     int priority = XorpTask::PRIORITY_DEFAULT,
			     int weight = XorpTask::WEIGHT_DEFAULT);

    /**
     * Create a repeated XorpTask.
     *
     * @param cb callback object that is invoked when task is run.
     * If the callback returns true, the task will continue to run,
     * otherwise it will be unscheduled.
     *
     * @return the @ref XorpTask created.
     */
    XorpTask new_task(const RepeatedTaskCallback& cb,
		      int priority = XorpTask::PRIORITY_DEFAULT,
		      int weight = XorpTask::WEIGHT_DEFAULT);

    /**
     * Get the priority of the highest priority runnable task.
     *
     * @return the priority (lowest value is best priority).
     */
    int get_runnable_priority() const;

    void schedule_node(TaskNode* node);
    void unschedule_node(TaskNode* node);

    bool empty() const;

private:
    RoundRobinQueue* find_round_robin(int priority);

    map<int, RoundRobinQueue*> _rr_list;
};


// ----------------------------------------------------------------------------
// inline Task methods

inline
XorpTask::XorpTask(TaskList* task_list, BasicTaskCallback cb)
    : _task_node(new TaskNode(task_list, cb))
{
    if (_task_node != NULL)
	_task_node->add_ref();
}

inline
XorpTask::XorpTask(const XorpTask& xorp_task)
    : _task_node(xorp_task._task_node)
{
    debug_msg("XorpTask copy constructor %p, n = %p\n", this, _task_node);

    if (_task_node != NULL)
	_task_node->add_ref();
}

inline
XorpTask::~XorpTask()
{
    debug_msg("XorpTask destructor %p, n = %p\n", this, _task_node);

    if (_task_node != NULL)
	_task_node->release_ref();
}

inline
XorpTask::XorpTask(TaskNode* task_node)
    : _task_node(task_node)
{
    debug_msg("XorpTask constructor %p, n = %p\n", this, _task_node);

    if (_task_node)
	_task_node->add_ref();
}

inline XorpTask&
XorpTask::operator=(const XorpTask& other)
{
    if (other._task_node != NULL)
	other._task_node->add_ref();
    if (_task_node != NULL)
	_task_node->release_ref();
    _task_node = other._task_node;
    return *this;
}

inline void
XorpTask::schedule(int priority, int weight)
{
    _task_node->schedule(priority, weight);
}

inline void
XorpTask::reschedule()
{
    _task_node->reschedule();
}

#endif // __LIBXORP_TASK_HH__
