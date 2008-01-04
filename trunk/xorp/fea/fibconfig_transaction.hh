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

// $XORP: xorp/fea/fibconfig_transaction.hh,v 1.4 2007/09/15 19:52:38 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TRANSACTION_HH__
#define __FEA_FIBCONFIG_TRANSACTION_HH__

#include <map>
#include <list>
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "fibconfig.hh"


/**
 * Class to store and execute FibConfig transactions.
 * An FibConfig transaction is a sequence of commands that should
 * executed atomically.
 */
class FibConfigTransactionManager : public TransactionManager
{
public:
    typedef TransactionManager::Operation Operation;

    FibConfigTransactionManager(EventLoop& eventloop, FibConfig& fibconfig)
	: TransactionManager(eventloop, TIMEOUT_MS, MAX_PENDING),
	  _fibconfig(fibconfig)
    {}

    FibConfig& fibconfig() { return _fibconfig; }

    /**
     * @return string representing first error during commit.  If string is
     * empty(), then no error occurred.
     */
    const string& error() const { return _error; }

    size_t max_ops() const { return MAX_OPS; }

protected:
    void unset_error();
    int set_unset_error(const string& error);

    // Overriding methods
    void pre_commit(uint32_t tid);
    void post_commit(uint32_t tid);
    void operation_result(bool success, const TransactionOperation& op);

protected:
    FibConfig&	_fibconfig;
    string	_error;

private:
    enum { TIMEOUT_MS = 5000, MAX_PENDING = 10, MAX_OPS = 200 };
};

/**
 * Base class for operations that can occur during an FibConfig transaction.
 */
class FibConfigTransactionOperation : public TransactionOperation {
public:
    FibConfigTransactionOperation(FibConfig& fibconfig)
	: _fibconfig(fibconfig) {}
    virtual ~FibConfigTransactionOperation() {}

protected:
    FibConfig& fibconfig() { return _fibconfig; }

private:
    FibConfig& _fibconfig;
};

/**
 * Class to store request to add routing entry to FibConfig and
 * dispatch it later.
 */
class FibAddEntry4 : public FibConfigTransactionOperation {
public:
    FibAddEntry4(FibConfig&	fibconfig,
		 const IPv4Net&	net,
		 const IPv4&	nexthop,
		 const string&	ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance,
		 bool		xorp_route,
		 bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() {
	if (fibconfig().add_entry4(_fte) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("AddEntry4: ") + _fte.str(); }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete routing entry to FibConfig and
 * dispatch it later.
 */
class FibDeleteEntry4 : public FibConfigTransactionOperation {
public:
    FibDeleteEntry4(FibConfig&		fibconfig,
		    const IPv4Net&	net,
		    const IPv4&		nexthop,
		    const string&	ifname,
		    const string&	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    bool		xorp_route,
		    bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() {
	if (fibconfig().delete_entry4(_fte) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteEntry4: ") + _fte.str();  }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete all routing entries to FibConfig and
 * dispatch it later.
 */
class FibDeleteAllEntries4 : public FibConfigTransactionOperation {
public:
    FibDeleteAllEntries4(FibConfig& fibconfig)
	: FibConfigTransactionOperation(fibconfig) {}

    bool dispatch() {
	if (fibconfig().delete_all_entries4() != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteAllEntries4");  }
};

/**
 * Class to store request to add routing entry to FibConfig and
 * dispatch it later.
 */
class FibAddEntry6 : public FibConfigTransactionOperation {
public:
    FibAddEntry6(FibConfig&	fibconfig,
		 const IPv6Net&	net,
		 const IPv6&	nexthop,
		 const string&  ifname,
		 const string&	vifname,
		 uint32_t	metric,
		 uint32_t	admin_distance,
		 bool		xorp_route,
		 bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() {
	if (fibconfig().add_entry6(_fte) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("AddEntry6: ") + _fte.str(); }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete routing entry to FibConfig
 * and dispatch it later.
 */
class FibDeleteEntry6 : public FibConfigTransactionOperation {
public:
    FibDeleteEntry6(FibConfig&		fibconfig,
		    const IPv6Net&	net,
		    const IPv6&		nexthop,
		    const string&	ifname,
		    const string&	vifname,
		    uint32_t		metric,
		    uint32_t		admin_distance,
		    bool		xorp_route,
		    bool		is_connected_route)
	: FibConfigTransactionOperation(fibconfig),
	  _fte(net, nexthop, ifname, vifname, metric, admin_distance,
	       xorp_route) {
	if (is_connected_route)
	    _fte.mark_connected_route();
    }

    bool dispatch() {
	if (fibconfig().delete_entry6(_fte) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteEntry6: ") + _fte.str();  }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete all routing entries to FibConfig
 * and dispatch it later.
 */
class FibDeleteAllEntries6 : public FibConfigTransactionOperation {
public:
    FibDeleteAllEntries6(FibConfig& fibconfig)
	: FibConfigTransactionOperation(fibconfig) {}

    bool dispatch() {
	if (fibconfig().delete_all_entries6() != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("DeleteAllEntries6");  }
};

#endif // __FEA_FIBCONFIG_TRANSACTION_HH__
