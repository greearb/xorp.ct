// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/pa_transaction.hh,v 1.7 2007/09/15 19:52:40 pavlin Exp $

#ifndef __FEA_PA_TRANSACTION_HH__
#define __FEA_PA_TRANSACTION_HH__

#include <map>
#include <list>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"

/* ------------------------------------------------------------------------- */
/* Packet ACL Transactions */

/**
 * Base class for operations that can occur during a Packet ACL transaction.
 */
class PaTransactionOperation : public TransactionOperation {
public:
    PaTransactionOperation(PaTableManager& ptm) : _ptm(ptm) {}
    virtual ~PaTransactionOperation() {}

protected:
    PaTableManager& ptm() { return _ptm; }

private:
    PaTableManager& _ptm;
};

class PaAddEntry4 : public PaTransactionOperation {
public:
    PaAddEntry4(PaTableManager&	ptm,
		 const string&	ifname,
		 const string&	vifname,
		 const IPv4Net&	src,
		 const IPv4Net&	dst,
		 uint8_t	proto,
		 uint16_t	sport,
		 uint16_t	dport,
		 PaAction	action)
	: PaTransactionOperation(ptm),
	  _entry(ifname, vifname, src, dst, proto, sport, dport, action)
	  {}

    bool dispatch() {
	if (ptm().add_entry4(_entry) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("AddEntry4: ") + _entry.str(); }

private:
    PaEntry4 _entry;
};

class PaDeleteEntry4 : public PaTransactionOperation {
public:
    PaDeleteEntry4(PaTableManager&	ptm,
		 const string&	ifname,
		 const string&	vifname,
		 const IPv4Net&	src,
		 const IPv4Net&	dst,
		 uint8_t	proto,
		 uint16_t	sport,
		 uint16_t	dport)
	: PaTransactionOperation(ptm),
	  _entry(ifname, vifname, src, dst, proto, sport, dport,
		PaAction(ACTION_ANY))
	  {}

    bool dispatch() {
	if (ptm().delete_entry4(_entry) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteEntry4: ") + _entry.str();  }

private:
    PaEntry4 _entry;
};

class PaDeleteAllEntries4 : public PaTransactionOperation {
public:
    PaDeleteAllEntries4(PaTableManager& ptm) : PaTransactionOperation(ptm) {}

    bool dispatch() {
	if (ptm().delete_all_entries4() != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteAllEntries4");  }
};

class PaAddEntry6 : public PaTransactionOperation {
public:
    PaAddEntry6(PaTableManager&	ptm,
		 const string&	ifname,
		 const string&	vifname,
		 const IPv6Net&	src,
		 const IPv6Net&	dst,
		 uint8_t	proto,
		 uint16_t	sport,
		 uint16_t	dport,
		 PaAction	action)
	: PaTransactionOperation(ptm),
	  _entry(ifname, vifname, src, dst, proto, sport, dport, action)
	  {}

    bool dispatch() {
	if (ptm().add_entry6(_entry) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("AddEntry6: ") + _entry.str(); }

private:
    PaEntry6 _entry;
};

class PaDeleteEntry6 : public PaTransactionOperation {
public:
    PaDeleteEntry6(PaTableManager&	ptm,
		 const string&	ifname,
		 const string&	vifname,
		 const IPv6Net&	src,
		 const IPv6Net&	dst,
		 uint8_t	proto,
		 uint16_t	sport,
		 uint16_t	dport)
	: PaTransactionOperation(ptm),
	  _entry(ifname, vifname, src, dst, proto, sport, dport,
		PaAction(ACTION_ANY))
	  {}

    bool dispatch() {
	if (ptm().delete_entry6(_entry) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteEntry6: ") + _entry.str();  }

private:
    PaEntry6 _entry;
};

class PaDeleteAllEntries6 : public PaTransactionOperation {
public:
    PaDeleteAllEntries6(PaTableManager& ptm) : PaTransactionOperation(ptm) {}

    bool dispatch() {
	if (ptm().delete_all_entries6() != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteAllEntries6");  }
};

/* ------------------------------------------------------------------------- */
/* Packet ACL Manager */

/**
 * Class to store and execute Packet ACL transactions. Such a transaction
 * is a sequence of Packet ACL commands that should executed atomically.
 *
 * This is also used as the container class for all access to the ACLs
 * maintained by the FEA.
 *
 * A map of pending transactions is maintained to facilitate rollback.
 */
class PaTransactionManager : public TransactionManager
{
public:
    typedef TransactionManager::Operation Operation;

    enum {
	MAX_TRANS_PENDING = 16,
	MAX_OPS_PENDING = 200,
	TIMEOUT_MS = 10000
    };

    PaTransactionManager(EventLoop& e, PaTableManager& ptm,
			 uint32_t max_pending = MAX_TRANS_PENDING)
	: TransactionManager(e, TIMEOUT_MS, max_pending),
	  _ptm(ptm), _pbp(0)
	{}

    /**
     * @return string representing first error during commit.  If string is
     * empty(), then no error occurred.
     */
    const string& error() const { return _error; }

    /**
     * @return maximum number of pending operations in a transaction allowed
     * by this class.
     */
    size_t retrieve_max_ops() const { return MAX_OPS_PENDING; }

    /**
     * @return the PaTableManager associated with this PaTransactionManager.
     */
    PaTableManager& ptm() const { return _ptm; }

    /**
     * @return pointer to the current PaBackend in use.
     */
    PaBackend* get_backend() const { return _pbp; }

    /**
     * @return true if the named PaBackend was successfully constructed.
     * The currently active PaBackend, if any, will be freed, only if
     * construction was successful.
     */
    int set_backend(const char* name);

protected:
    /**
     * PaTransaction is used to hold the Mementoes we request from underlying
     * subsystems to support rollback.
     */
    struct PaTransaction {
    public:
	PaTransaction(const PaSnapshot4* sn4,
		      const PaBackend::Snapshot4Base* bsn4);
	~PaTransaction();

	const PaSnapshot4* snap4() const { return _snap4; }

	void set_snap4(const PaSnapshot4* sn4) { _snap4 = sn4; }

	const PaBackend::Snapshot4Base* bsnap4() const { return _bsnap4; }

	void set_bsnap4(const PaBackend::Snapshot4Base* bsn4) {
	    _bsnap4 = bsn4;
	}

#ifdef notyet
	const PaSnapshot6* snap6() const { return _snap6; }

	void set_snap6(const PaSnapshot6* sn6) { _snap6 = sn6; }

	const PaBackend::Snapshot6Base* bsnap6() const { return _bsnap6; }

	void set_bsnap6(PaBackend::Snapshot6Base* bsn6) { _bsnap6 = bsn6; }
#endif
    private:
	const PaSnapshot4*		_snap4;
	const PaBackend::Snapshot4Base*	_bsnap4;
#ifdef notyet
	const PaSnapshot6*		_snap6;
	const PaBackend::Snapshot6Base*	_bsnap6;
#endif
    };

    void unset_error();
    int set_unset_error(const string& error);

    /* Methods overridden from TransactionManager */
    void pre_commit(uint32_t tid);
    void post_commit(uint32_t tid);
    void operation_result(bool success, const TransactionOperation& op);

private:
    typedef map<uint32_t, PaTransaction> PaTransactionDB;

    PaTransactionDB	_pa_transactions;
    string		_error;
    PaTableManager&	_ptm;
    PaBackend*		_pbp;
};

/* ------------------------------------------------------------------------- */
/* Inline methods for PaTransactionManager */

inline void
PaTransactionManager::unset_error()
{
    _error.erase();
}

inline int
PaTransactionManager::set_unset_error(const string& error)
{
    if (_error.empty()) {
	_error = error;
	return (XORP_OK);
    }
    return (XORP_ERROR);
}

/* ------------------------------------------------------------------------- */
#endif // __FEA_PA_TRANSACTION_HH__
