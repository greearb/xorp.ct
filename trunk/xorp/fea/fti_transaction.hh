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

// $XORP: xorp/fea/fti_transaction.hh,v 1.3 2003/03/10 23:20:14 hodson Exp $

#ifndef __FEA_FTI_TRANSACTION_HH__
#define __FEA_FTI_TRANSACTION_HH__

#include <map>
#include <list>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "fticonfig.hh"

/**
 * Base class for operations that can occur during an FTI transaction.
 */
class FtiTransactionOperation : public TransactionOperation {
public:
    FtiTransactionOperation(FtiConfig& ftic) : _ftic(ftic) {}
    virtual ~FtiTransactionOperation() {}

protected:
    FtiConfig& ftic() { return _ftic; }

private:
    FtiConfig& _ftic;
};

/**
 * Class to store request to add routing entry to FtiConfig and
 * dispatch it later.
 */
class FtiAddEntry4 : public FtiTransactionOperation {
public:
    FtiAddEntry4(FtiConfig&	ftic,
		 const IPv4Net&	net,
		 const IPv4&	gateway,
		 const string&	ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance)
	: FtiTransactionOperation(ftic), _fte(net, gateway, ifname, vifname,
					      metric, admin_distance) {}

    bool dispatch() { return ftic().add_entry4(_fte); }

    string str() const { return string("AddEntry4: ") + _fte.str(); }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete routing entry to FtiConfig and
 * dispatch it later.
 */
class FtiDeleteEntry4 : public FtiTransactionOperation {
public:
    FtiDeleteEntry4(FtiConfig& ftic, const IPv4Net& net)
	: FtiTransactionOperation(ftic), _fte(net) {}

    bool dispatch() { return ftic().delete_entry4(_fte); }

    string str() const { return string("DeleteEntry4: ") + _fte.str();  }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete all routing entries to FtiConfig and
 * dispatch it later.
 */
class FtiDeleteAllEntries4 : public FtiTransactionOperation {
public:
    FtiDeleteAllEntries4(FtiConfig& ftic) : FtiTransactionOperation(ftic) {}

    bool dispatch() { return ftic().delete_all_entries4(); }

    string str() const { return string("DeleteAllEntries4");  }
};

/**
 * Class to store request to add routing entry to FtiConfig and
 * dispatch it later.
 */
class FtiAddEntry6 : public FtiTransactionOperation {
public:
    FtiAddEntry6(FtiConfig&	ftic,
		 const IPv6Net&	net,
		 const IPv6&	gateway,
		 const string&  ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance)
	: FtiTransactionOperation(ftic), _fte(net, gateway, ifname, vifname,
					      metric, admin_distance) {}

    bool dispatch() { return ftic().add_entry6(_fte); }

    string str() const { return string("AddEntry6: ") + _fte.str(); }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete routing entry to FtiConfig
 * and dispatch it later.
 */
class FtiDeleteEntry6 : public FtiTransactionOperation {
public:
    FtiDeleteEntry6(FtiConfig& ftic, const IPv6Net& net)
	: FtiTransactionOperation(ftic), _fte(net) {}

    bool dispatch() { return ftic().delete_entry6(_fte); }

    string str() const { return string("DeleteEntry6: ") + _fte.str();  }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete all routing entries to FtiConfig
 * and dispatch it later.
 */
class FtiDeleteAllEntries6 : public FtiTransactionOperation {
public:
    FtiDeleteAllEntries6(FtiConfig& ftic) : FtiTransactionOperation(ftic) {}

    bool dispatch() { return ftic().delete_all_entries6(); }

    string str() const { return string("DeleteAllEntries6");  }
};

/**
 * Class to store request to delete all routing entries to FtiConfig
 * and dispatch it later.
 */
class FtiDeleteAllEntries : public FtiTransactionOperation {
public:
    FtiDeleteAllEntries(FtiConfig& ftic) : FtiTransactionOperation(ftic) {}

    bool dispatch() {
	return ftic().delete_all_entries4() & ftic().delete_all_entries6();
    }

    string str() const { return string("DeleteAllEntries");  }
};


/**
 * Class to store and execute FTI transactions.  An FTI transaction is a
 * a sequence of FTI commands that should executed atomically.
 */
class FtiTransactionManager : public TransactionManager
{
public:
    typedef TransactionManager::Operation Operation;

    enum { TIMEOUT_MS = 5000 };

    FtiTransactionManager(EventLoop& e, FtiConfig& ftic,
			  uint32_t max_pending = 10)
	: TransactionManager(e, TIMEOUT_MS, max_pending), _ftic(ftic)
    {}

    FtiConfig& ftic() { return _ftic; }

    /**
     * @return string representing first error during commit.  If string is
     * empty(), then no error occurred.
     */
    const string& error() const { return _error; }

protected:
    inline void unset_error();
    inline bool set_unset_error(const string& error);

    /* Overriding methods */
    void pre_commit(uint32_t tid);
    void post_commit(uint32_t tid);
    void operation_result(bool success, const TransactionOperation& op);

protected:
    FtiConfig& _ftic;
    string _error;
};

/* ------------------------------------------------------------------------- */
/* Inline methods */

inline void
FtiTransactionManager::unset_error()
{
    _error.erase();
}

inline bool
FtiTransactionManager::set_unset_error(const string& error)
{
    if (_error.empty()) {
	_error = error;
	return true;
    }
    return false;
}

#endif // __FEA_FTI_TRANSACTION_HH__
