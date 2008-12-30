// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2006-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/task.cc,v 1.11 2008/10/01 23:07:43 pavlin Exp $"

#include "libxorp_module.h"
#include "xorp.h"

#include "xlog.h"
#include "task.hh"


// ----------------------------------------------------------------------------
// TaskNode methods

TaskNode::TaskNode(TaskList* task_list, BasicTaskCallback cb)
    : _task_list(task_list), _cb(cb), _ref_cnt(0), _priority(0), _weight(0)
{
    debug_msg("TaskNode constructor %p\n", this);
}

TaskNode::~TaskNode()
{
    debug_msg("TaskNode destructor %p\n", this);

    unschedule();
}

void
TaskNode::add_ref()
{
    _ref_cnt++;
    debug_msg("add_ref on %p, now ref_cnt = %d\n", this, _ref_cnt);
}

void
TaskNode::release_ref()
{
    if (--_ref_cnt <= 0)
	delete this;
}

void
TaskNode::schedule(int priority, int weight)
{
    debug_msg("TaskNode schedule %p p = %d, w = %d\n", this, priority, weight);

    XLOG_ASSERT(_task_list != NULL);
    unschedule();
    _priority = priority;
    _weight = weight;
    _task_list->schedule_node(this);
}

void
TaskNode::reschedule()
{
    XLOG_ASSERT(_task_list != NULL);
    unschedule();
    _task_list->schedule_node(this);
}

void
TaskNode::unschedule()
{
    if (scheduled())
	_task_list->unschedule_node(this);
}


// ----------------------------------------------------------------------------
// Specialized Tasks.  These are what the XorpTask objects returned by
// the TaskList XorpTask creation methods (e.g. TaskList::new_oneoff_at(), etc)
// actually refer to.

class OneoffTaskNode2 : public TaskNode {
public:
    OneoffTaskNode2(TaskList* task_list, const OneoffTaskCallback& cb)
	: TaskNode(task_list, callback(this, &OneoffTaskNode2::run)),
	  _cb(cb) {}

private:
    void run(XorpTask& xorp_task) {
	debug_msg("OneoffTaskNode2::run() %p\n", this);
	//
	// XXX: we have to unschedule before the callback dispatch, in case
	// the callback decides to schedules again the task.
	//
	xorp_task.unschedule();
	_cb->dispatch();
    }

    OneoffTaskCallback _cb;
};

class RepeatedTaskNode2 : public TaskNode {
public:
    RepeatedTaskNode2(TaskList* task_list, const RepeatedTaskCallback& cb)
	: TaskNode(task_list, callback(this, &RepeatedTaskNode2::run)),
	  _cb(cb) {}

private:
    void run(XorpTask& xorp_task) {
	if (! _cb->dispatch()) {
	    xorp_task.unschedule();
	}
    }

    RepeatedTaskCallback _cb;
};


// ----------------------------------------------------------------------------
// XorpTask

void
XorpTask::unschedule()
{
    if (_task_node != NULL)
	_task_node->unschedule();
}

bool
XorpTask::scheduled() const
{
    if (_task_node != NULL)
	return _task_node->scheduled();
    else
	return false;
}


// ----------------------------------------------------------------------------
// TaskList

TaskList::~TaskList()
{
#ifdef notyet
    // Attempting to plug the leak causes problems elsewhere.
    while (! _rr_list.empty()) {
	map<int, RoundRobinQueue*>::iterator ii = _rr_list.begin();
	delete (*ii).second;
	_rr_list.erase(ii);
    }
#endif
}

XorpTask
TaskList::new_oneoff_task(const OneoffTaskCallback& cb,
			  int priority, int weight)
{
    debug_msg("new oneoff task %p p = %d, w = %d\n", this, priority, weight);

    TaskNode* task_node = new OneoffTaskNode2(this, cb);
    task_node->schedule(priority, weight);
    return XorpTask(task_node);
}

XorpTask
TaskList::new_task(const RepeatedTaskCallback& cb,
		   int priority, int weight)
{
    debug_msg("new task %p p = %d, w = %d\n", this, priority, weight);

    TaskNode* task_node = new RepeatedTaskNode2(this, cb);
    task_node->schedule(priority, weight);
    return XorpTask(task_node);
}

int
TaskList::get_runnable_priority() const
{
    map<int, RoundRobinQueue*>::const_iterator rri;

    for (rri = _rr_list.begin(); rri != _rr_list.end(); ++rri) {
	if (rri->second->size() != 0) {
	    return rri->first;
	}
    }

    return XorpTask::PRIORITY_INFINITY;
}

bool
TaskList::empty() const
{
    bool result = true;
    map<int, RoundRobinQueue*>::const_iterator rri;

    for (rri = _rr_list.begin(); rri != _rr_list.end(); ++rri) {
	if (rri->second->size() != 0) {
	    result = false;
	    break;
	}
    }

    return result;
}

void
TaskList::run()
{
    map<int, RoundRobinQueue*>::const_iterator rri;

    debug_msg("TaskList run()\n");

    for (rri = _rr_list.begin(); rri != _rr_list.end(); ++rri) {
	RoundRobinQueue* rr = rri->second;
	if (rr->size() != 0) {
	    TaskNode* task_node = static_cast<TaskNode*>(rr->get_next_entry());
	    debug_msg("node to run: %p\n", task_node);
	    XorpTask xorp_task(task_node);
	    task_node->run(xorp_task);
	    return;
	}
    }
}

RoundRobinQueue*
TaskList::find_round_robin(int priority)
{
    map<int, RoundRobinQueue*>::iterator rri = _rr_list.find(priority);

    if (rri == _rr_list.end()) {
	RoundRobinQueue* rr = new RoundRobinQueue();
	_rr_list[priority] = rr;
	return rr;
    } else {
	return rri->second;
    }
}

void 
TaskList::schedule_node(TaskNode* task_node)
{
    debug_msg("TaskList::schedule_node: n = %p\n", task_node);

    RoundRobinObjBase* obj = static_cast<RoundRobinObjBase*>(task_node);
    RoundRobinQueue* rr = find_round_robin(task_node->priority());
    rr->push(obj, task_node->weight());
}

void
TaskList::unschedule_node(TaskNode* task_node)
{
    debug_msg("TaskList::unschedule_node: n = %p\n", task_node);

    RoundRobinObjBase* obj = static_cast<RoundRobinObjBase*>(task_node);
    RoundRobinQueue* rr = find_round_robin(task_node->priority());
    rr->pop_obj(obj);
}
