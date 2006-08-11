// Copyright (c) 2006 International Computer Science Institute
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

// $XORP: xorp/libxorp/task.hh,v 1.4 2006/08/11 05:59:07 pavlin Exp $

#ifndef __LIBXORP_TASK_HH__
#define __LIBXORP_TASK_HH__

#include <map>

#include "debug.h"
#include "round_robin.hh"
#include "callback.hh"


class XorpTask;
class TaskNode;
class TaskList;

//typedef XorpCallback0<void>::RefPtr OneoffTaskCallback;
typedef XorpCallback0<bool>::RefPtr RepeatedTaskCallback;
typedef XorpCallback1<void, XorpTask&>::RefPtr BasicTaskCallback;


class TaskNode : public RoundRobinObjBase {
protected:
    TaskNode(TaskList*, BasicTaskCallback);
    virtual ~TaskNode();
    void add_ref();
    void release_ref();

    // we want this even if it is never called, to override the
    // default supplied by the compiler.
    TaskNode(const TaskNode&);	// never called
    TaskNode& operator=(const TaskNode&);

    void schedule(int priority, int weight);
    void reschedule();
    void unschedule();

    int priority()              const   { return _priority; }
    int weight()                const   { return _weight; }

    int		_ref_cnt;	// Number of referring XorpTask objects

    virtual void run(XorpTask &) {};     // Implemented by children
    BasicTaskCallback _cb;

    int _priority;              // Scheduling priority
    int _weight;

    TaskList*	_list;		// TaskList this node is associated w.

    friend class XorpTask;
    friend class TaskList;
};


class XorpTask {
public:
    //
    // Task/Timer priorities. Those are are suggested values.  
    //
    static const int PRIORITY_HIGHEST		= 0;
    static const int PRIORITY_XRL_KEEPALIVE	= 1;
    static const int PRIORITY_HIGH		= 2;
    static const int PRIORITY_DEFAULT		= 4;
    static const int PRIORITY_BACKGROUND	= 7;
    static const int PRIORITY_LOWEST		= 9;
    static const int PRIORITY_INFINITY		= 255;

    //
    // Task/Timer weights.
    //
    static const int WEIGHT_DEFAULT		= 1;

    XorpTask() : _node() {}
    XorpTask(TaskList* tlist, BasicTaskCallback cb);
    XorpTask(const XorpTask&);
    ~XorpTask();
    bool scheduled() const;
    void schedule(int priority, int weight);
    void reschedule();
    void unschedule();
    XorpTask& operator=(const XorpTask&);
private:
    TaskNode* _node;

    XorpTask(TaskNode* n);

    friend class TaskList;
};


class TaskList {
public:
    /**
     * Expire all pending @ref XorpTask objects associated with @ref
     * TaskList.
     */
    void run();

    /**
     * Create a XorpTask that will be scheduled once.
     *
     * @param ocb callback object that is invoked when task is run.
     *
     * @return the @ref XorpTask created.
     */
    XorpTask new_task(const RepeatedTaskCallback& ocb,
		      int priority = XorpTask::PRIORITY_DEFAULT,
		      int weight = XorpTask::WEIGHT_DEFAULT);

    /**
     * Get the priority of the highest priority runnable task.
     *
     * @return the priority (lowest is best)
     */
    int get_runnable_priority() const;

    bool empty() const;

private:
    void schedule_node(TaskNode *node);
    void unschedule_node(TaskNode *node);
    RoundRobinQueue* find_round_robin(int priority);

    map<int, RoundRobinQueue*> _rr_list;

    friend class TaskNode;
};


// ----------------------------------------------------------------------------
// inline Task methods

inline XorpTask::XorpTask(TaskList* tlist, BasicTaskCallback cb)
    : _node(new TaskNode(tlist, cb))
{
    if (_node)
	_node->add_ref();
}

inline XorpTask::XorpTask(const XorpTask& t)
    : _node(t._node)
{
  debug_msg("XorpTask copy constructor %p, n=%p\n", this, _node);
    if (_node)
	_node->add_ref();
}

inline XorpTask::~XorpTask()
{
  debug_msg("XorpTask destructor %p, n=%p\n", this, _node);
    if (_node)
	_node->release_ref();
}

inline XorpTask::XorpTask(TaskNode* n)
    : _node(n)
{
  debug_msg("XorpTask constructor %p, n=%p\n", this, _node);
    if (_node)
	_node->add_ref();
}

inline XorpTask&
XorpTask::operator=(const XorpTask& t)
{
    if (t._node)
	t._node->add_ref();
    if (_node)
	_node->release_ref();
    _node = t._node;
    return *this;
}

inline void
XorpTask::schedule(int priority, int weight)
{
    _node->schedule(priority, weight);
}

inline void
XorpTask::reschedule()
{
    assert(_node);
    _node->reschedule();
}

#endif
