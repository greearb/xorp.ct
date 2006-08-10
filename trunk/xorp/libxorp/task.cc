// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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
//

#ident "$XORP$"

#include "xorp.h"
#include "task.hh"

// ----------------------------------------------------------------------------
// TaskNode methods

TaskNode::TaskNode(TaskList* l, BasicTaskCallback cb)
    : _ref_cnt(0), _cb(cb), _list(l)
{
    debug_msg("TaskNode constructor %p\n", this);
}

TaskNode::~TaskNode()
{
    unschedule();
    debug_msg("TaskNode destructor %p\n", this);
}

void
TaskNode::add_ref()
{
    _ref_cnt++;
    debug_msg("add_ref on %p, now %d\n", this, _ref_cnt);
}

void
TaskNode::release_ref()
{
    if (--_ref_cnt <= 0)
	delete this;
}

void
TaskNode::unschedule()
{
    if (scheduled())
	_list->unschedule_node(this);
}

void
TaskNode::schedule(int priority, int weight)
{
    debug_msg("TaskNode schedule %p p=%d, w=%d\n", this, priority, weight);
    assert(_list);
    unschedule();
    _priority = priority;
    _weight = weight;
    _list->schedule_node(this);
}

void
TaskNode::reschedule()
{
    assert(_list);
    unschedule();
    _list->schedule_node(this);
}


// ----------------------------------------------------------------------------
// Specialized Tasks.  These are what the XorpTask objects returned by
// the TaskList XorpTask creation methods (e.g. TaskList::new_oneoff_at(), etc)
// actually refer to/

#if 0
class OneoffTaskNode2 : public TaskNode {
public:
    OneoffTaskNode2(TaskList *l, const OneoffTaskCallback& cb)
	: TaskNode (l, callback(this, &OneoffTaskNode2::run)),
	_cb(cb) {}
private:
    OneoffTaskCallback _cb;

    void run(XorpTask&) {
	debug_msg("TaskNode::run() %p\n", this);
	_cb->dispatch();
    }
};
#endif

class RepeatedTaskNode2 : public TaskNode {
public:
    RepeatedTaskNode2(TaskList *l, const RepeatedTaskCallback& cb)
	: TaskNode(l, callback(this, &RepeatedTaskNode2::run)),
		    _cb(cb) { }

private:
    RepeatedTaskCallback _cb;

    void run(XorpTask& t) {
	if (!_cb->dispatch()) {
	    t.unschedule();
	}
    }
};


// ----------------------------------------------------------------------------
// XorpTask

bool
XorpTask::scheduled() const
{
    if (_node)
	return _node->scheduled();
    else
	return false;
}

void
XorpTask::unschedule()
{
    if (_node)
	_node->unschedule();
}


// ----------------------------------------------------------------------------
// TaskList


XorpTask
TaskList::new_task(const RepeatedTaskCallback& cb,
		   int priority, int weight)
{
    debug_msg("new task %p p=%d, w=%d\n", this, priority, weight);
    TaskNode* n = new RepeatedTaskNode2(this, cb);
    n->schedule(priority, weight);
    return XorpTask(n);
}

int 
TaskList::get_runnable_priority() const
{
    map<int,RoundRobin*>::const_iterator rri;
    for (rri = _rr_list.begin(); rri != _rr_list.end(); rri++) {
	if (rri->second->size() != 0) {
	    return rri->first;
	}
    }
    return INFINITY_PRIORITY;
}

bool
TaskList::empty() const
{
    bool result = true;
    map<int,RoundRobin*>::const_iterator rri;
    for (rri = _rr_list.begin(); rri != _rr_list.end(); rri++) {
	if (rri->second->size() != 0)
	    result = false;
    }
    return result;
}

void
TaskList::run()
{
    debug_msg("TaskList run()\n");
    map<int,RoundRobin*>::const_iterator rri;
    for (rri = _rr_list.begin(); rri != _rr_list.end(); rri++) {
	RoundRobin *rr = rri->second;
	if (rr->size() != 0) {
	    TaskNode *n = static_cast<TaskNode*>(rr->get_next_entry());
	    debug_msg("node to run: %p\n", n);
	    XorpTask task(n);
	    n->run(task);
	    return;
	}
    }
}

RoundRobin* 
TaskList::find_roundrobin(int priority)
{
    map<int,RoundRobin*>::iterator rri = _rr_list.find(priority);
    if (rri == _rr_list.end()) {
	RoundRobin *rr = new RoundRobin();
	_rr_list[priority] = rr;
	return rr;
    } else {
	return rri->second;
    }
}

void 
TaskList::schedule_node(TaskNode *n)
{
    debug_msg("TaskList::schedule_node: n=%p\n", n);
    RoundRobinBase *b = static_cast<RoundRobinBase*>(n);
    RoundRobin *rr = find_roundrobin(n->priority());
    rr->push(b, n->weight());
}

void 
TaskList::unschedule_node(TaskNode *n)
{
    debug_msg("TaskList::unschedule_node: n=%p\n", n);
    RoundRobinBase *b = static_cast<RoundRobinBase*>(n);
    RoundRobin *rr = find_roundrobin(n->priority());
    rr->pop_obj(b);
}


