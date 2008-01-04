// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/task.hh,v 1.9 2007/02/16 22:46:23 pavlin Exp $

#ifndef __LIBXORP_TASK_HH__
#define __LIBXORP_TASK_HH__

#include <map>

#include "debug.h"
#include "round_robin.hh"
#include "callback.hh"


class TaskList;
class TaskNode;
class XorpTask;

typedef XorpCallback0<void>::RefPtr OneoffTaskCallback;
typedef XorpCallback0<bool>::RefPtr RepeatedTaskCallback;
typedef XorpCallback1<void, XorpTask&>::RefPtr BasicTaskCallback;


class TaskNode : public RoundRobinObjBase {
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
    TaskNode(const TaskNode&);			// not implemented
    TaskNode& operator=(const TaskNode&);	// not implemented

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
