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

// $XORP: xorp/libxorp/transaction.hh,v 1.2 2003/02/26 00:14:14 pavlin Exp $

#ifndef __LIBXORP_TRANSACTION_HH__
#define __LIBXORP_TRANSACTION_HH__

#include "config.h"

#include <map>
#include <list>
#include <string>

#include "libxorp/eventloop.hh"
#include "libxorp/ref_ptr.hh"

/**
 * @short Base class for operations within a Transaction.
 *
 * TransactionOperations when realized through derived classes are
 * operations that can be held and dispatched at a later time.  The
 * @ref TransactionManager class is provided as a container for
 * TransactionOperations.
 *
 * NB TransactionOperation is analogous to the Command pattern in the
 * BoF: Design Patterns , Erich Gamma, Richard Helm, Ralph Johnson,
 * John Vlissides, Addison Wesley, 1995, ISBN 0-201-63361-2.
 */
class TransactionOperation {
public:
    /**
     * Destructor
     */
    virtual ~TransactionOperation() {}

    /**
     * Dispatch operation
     * 
     * @return true on success, false on error.
     */
    virtual bool dispatch() = 0;

    /**
     * @return string representation of operation.
     */
    virtual string str() const = 0;
};

/**
 * @short A class for managing transactions.
 *
 * The TransactionManager creates, manages, and dispatches
 * transactions.  A Transaction is comprised of a sequence of @ref
 * TransactionOperation s.  Each transaction is uniquely identified by
 * a transaction id.
 */
class TransactionManager
{
public:
    typedef ref_ptr<TransactionOperation> Operation;

    /**
     * Constuctor with a given event loop, timeout, and max pending commits.
     *
     * @param e the EventLoop instance.
     * 
     * @param timeout_ms the inter-operation addition timeout.  If zero,
     *        timeouts are not used, otherwise a timeout will occur
     *	      and the transaction aborted if the transaction is not
     *	      updated for timeout_ms.
     *
     * @param max_pending the maximum number of uncommitted transactions
     *        pending commit.
     */
    TransactionManager(EventLoop& e,
		       uint32_t timeout_ms = 0,
		       uint32_t max_pending = 10) : 
	_e(e), _timeout_ms(timeout_ms), _max_pending(max_pending) 
    {
    }

    /**
     * Destructor
     */
    virtual ~TransactionManager() {}

    /** 
     * Start transaction
     *
     * @param new_tid variable to assigned new transaction id.
     * 
     * @return true on success, false if maximum number of pending 
     * 		    transactions is reached.
     */
    bool start(uint32_t& new_tid);

    /**
     * Commit transaction
     * 
     * @param tid the transaction ID.
     * @return true on success, false on error.
     */
    bool commit(uint32_t tid);

    /**
     * Abort transaction
     * 
     * @param tid the transaction ID.
     * @return true on success, false on error.
     */
    bool abort(uint32_t tid);

    /** 
     * Add operation to transaction.
     *
     * @param tid the transaction ID.
     * @param operation to be added.
     * @return true on success, false if tid is invalid.
     */
    virtual bool add(uint32_t tid, const Operation& op);

    /** 
     * Retrieve number of operations in pending transaction.
     *
     * @param tid the transaction ID.
     *
     * @param count variable to be assigned number of operations in 
     * transaction.
     *
     * @return true if tid is valid, false otherwise.
     */
    bool size(uint32_t tid, uint32_t& count) const;

    /**
     * Get the inter-operation additional timeout.
     * 
     * If the inter-operation addition timeout is zero,
     * timeouts are not used, otherwise a timeout will occur
     * and the transaction aborted if the transaction is not
     * updated for timeout_ms.
     * 
     * @return the inter-operation additional timeout.
     */
    inline uint32_t timeout_ms() const 	{ return _timeout_ms; }

    /**
     * Get the maximum number of uncommited pending transactions.
     * 
     * @return the maximum number of uncommitted transactions pending commit.
     */
    inline uint32_t max_pending() const { return _max_pending; }

    /**
     * Get the current number of uncommited pending transactions.
     * 
     * @return the current number of uncommitted transactions pending commit.
     */
    inline uint32_t pending() const 	{ return _transactions.size(); }

protected:

    /** 
     * Overrideable function that can be called before the first
     * operation in a commit is dispatched.
     *
     * Default implementation is a no-op.
     */
    virtual void pre_commit(uint32_t tid);

    /** 
     * Overrideable function that can be called after commit occurs 
     *
     * Default implementation is a no-op.
     */
    virtual void post_commit(uint32_t tid);

    /**
     * Overrideable function that is called immediately after an individual
     * operation is dispatched.
     *
     * Default implementation is a no-op.
     *
     * @param success whether the operation succeed.
     *
     * @param op the operation.
     */
    virtual void operation_result(bool success, 
				  const TransactionOperation& op);

    /**
     * Flush operations in transaction list.  May be use by @ref
     * operation_result methods to safely prevent further operations
     * being dispatched when errors are detected.  flush() always
     * succeeds if transaction exists.
     *
     * @param tid transaction id of transaction to be flushed.
     *
     * @return true if transaction exists, false otherwise.
     */
    bool flush(uint32_t tid);
    
protected:
    /** 
     * Transaction class, just a list of operations to be dispatched.
     * 
     * It is defined here so classes derived from TransactionManager
     * can operate, eg sort operations in list, before committing transaction.
     */
    struct Transaction {
	typedef list<Operation> OperationList;
	
	Transaction(TransactionManager& mgr, const XorpTimer& timeout_timer)
	    : _mgr(mgr), _timeout_timer(timeout_timer), _op_count(0)
	{}

	Transaction(TransactionManager& mgr) : _mgr(mgr), _op_count(0)
	{}

	/** Add an operation to list */
	inline void add(const Operation& op);

	/** Dispatch all operations on list */
	inline void commit();

	/** Flush all operations on list */
	inline void flush();
	
	/** Defer timeout by TransactionManagers timeout interval */
	inline void defer_timeout();

	/** Cancel timeout timer */
	inline void cancel_timeout();
	
	/** Get the list of operations */
	inline OperationList& operations() { return _ops; }

	/** Get the length of the operations list. */
	inline uint32_t size() const { return _op_count; }
	
    private:
	TransactionManager& _mgr;
	OperationList	    _ops;
	XorpTimer	    _timeout_timer;
	uint32_t	    _op_count;
    };

private:
    /** Called when timeout timer expires */
    void timeout(uint32_t tid);

    /** Increment next transaction id by a randomized amount. */
    void crank_tid();

private:
    typedef map<uint32_t, Transaction> TransactionDB;

    EventLoop& _e;
    TransactionDB _transactions;
    uint32_t _timeout_ms;
    uint32_t _max_pending;
    uint32_t _next_tid;
    
    friend class Transaction; // for Transaction to call operation_result()
};

#endif // __LIBXORP_TRANSACTION_HH__
