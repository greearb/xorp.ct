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

#ident "$XORP: xorp/libxorp/transaction.cc,v 1.2 2003/03/10 23:20:37 hodson Exp $"

#include <assert.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/transaction.hh"

/* ------------------------------------------------------------------------- */
/* Inline TransactionManager::Transaction methods */

inline void
TransactionManager::Transaction::add(const Operation& op)
{
    _ops.push_back(op);
    _op_count++;
    defer_timeout();
}

inline void
TransactionManager::Transaction::commit()
{
    //
    // Unschedule timeout timer, defense against a calls to 
    // EventLoop.run() in whacky places.
    //
    _timeout_timer.unschedule();

    while (_ops.empty() == false) {
	// Copy front of list, not a biggie data is refcnt'ed
	Operation op = _ops.front();

	// Erase item from list so if error occurs we don't have to
	// reference anything on list again; i.e., error handler could	
	// abort this transaction...
	_ops.erase(_ops.begin());
	_op_count--;

	//
	// Deref ref_ptr to get reference to operation
	//
	TransactionOperation& top = *(op.get());

	bool success = top.dispatch();

	//
	// Give manager a chance to deal with success / error
	//
	_mgr.operation_result(success, top);
    }
}

inline void
TransactionManager::Transaction::flush()
{
    while (_ops.empty() == false) {
	_ops.erase(_ops.begin());
	_op_count--;
    }
}

inline void
TransactionManager::Transaction::defer_timeout()
{
    uint32_t timeout_ms = _mgr.timeout_ms();
    if (timeout_ms)
	_timeout_timer.schedule_after_ms(timeout_ms);
}

inline void 
TransactionManager::Transaction::cancel_timeout()
{
    _timeout_timer.clear(); 
}

/* ------------------------------------------------------------------------- */
/* Transaction Manager methods */

void
TransactionManager::crank_tid()
{
    //
    // This would be very bad if number of pending transactions is large.
    // In practice the bad case should be well outside bounds of this code.
    //
    _next_tid++;
    do {
	_next_tid += (random() & 0xfffff);
    } while (_transactions.find(_next_tid) != _transactions.end());
}

bool
TransactionManager::start(uint32_t& new_tid)
{
    if (pending() == max_pending())
	return false;

    crank_tid();

    if (timeout_ms()) {
	XorpTimer t = _e.new_oneoff_after_ms(
	    timeout_ms(), 
	    callback(this,&TransactionManager::timeout, _next_tid)
	    );
	_transactions.insert(TransactionDB::value_type(
	    _next_tid, Transaction(*this, t))
	    );
    } else {
	_transactions.insert(TransactionDB::value_type(_next_tid, 
						       Transaction(*this)));
    }
    new_tid = _next_tid;

    return true;
}

bool
TransactionManager::abort(uint32_t tid)
{
    TransactionDB::iterator i = _transactions.find(tid);
    if (i == _transactions.end()) 
	return false;
    _transactions.erase(i);
    return true;
}

void
TransactionManager::pre_commit(uint32_t /* tid */)
{}

void
TransactionManager::post_commit(uint32_t /* tid */)
{}

void
TransactionManager::operation_result(bool /* success */, 
				     const TransactionOperation& /* op */)
{}

bool
TransactionManager::flush(uint32_t tid)
{
    TransactionDB::iterator i = _transactions.find(tid);
    if (i ==  _transactions.end()) {
	return false;
    }
    Transaction& t = i->second;
    t.flush();

    return true;
}

bool
TransactionManager::commit(uint32_t tid)
{
    if (_transactions.find(tid) == _transactions.end()) {
	return false;
    }

    pre_commit(tid);

    //
    // Check user did not do abort transaction in pre_commit().  This
    // comes at a cost since we've already done find(), but is better than 
    // dereferencing an invalid iterator.  We could disallow/prevent
    // abort by swapping transaction operations onto a temporary list
    // and deleting the transaction, but there are cases where abort
    // might be desirable in pre_commit and the double lookup is not
    // that expensive.
    //
    TransactionDB::iterator i = _transactions.find(tid);
    if (i == _transactions.end()) {
	return false;
    }

    Transaction& t = i->second;

    //
    // Sanity check, perhaps not the most appropriate place for this
    //
    assert(t.operations().size() == t.size());

    //
    // Commit all operations in queue
    //
    t.commit();

    //
    // Sanity check, perhaps not the most appropriate place for this
    //
    assert(t.operations().size() == t.size());

    //
    // Erase transaction
    // 
    _transactions.erase(i);

    post_commit(tid);
    return true;
}

bool
TransactionManager::add(uint32_t tid, const Operation& op)
{
    TransactionDB::iterator i = _transactions.find(tid);
    if (i == _transactions.end())
	return false;
    i->second.add(op);
    return true;
}

bool
TransactionManager::retrieve_size(uint32_t tid, uint32_t& count) const
{
    TransactionDB::const_iterator i = _transactions.find(tid);
    if (i == _transactions.end())
	return false;
    count = i->second.size();
    return true;
}

void
TransactionManager::timeout(uint32_t tid)
{
    TransactionDB::iterator i = _transactions.find(tid);
    if (i == _transactions.end())
	return;
    debug_msg("Timing out transaction id %d\n", tid);
    _transactions.erase(i);
}

