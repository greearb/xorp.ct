// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/fea/fibconfig_transaction.hh,v 1.8 2008/07/23 05:10:08 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TRANSACTION_HH__
#define __FEA_FIBCONFIG_TRANSACTION_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/transaction.hh"

#include "fibconfig.hh"


/**
 * Class to store and execute FibConfig transactions.
 *
 * An FibConfig transaction is a sequence of commands that should
 * executed atomically.
 */
class FibConfigTransactionManager : public TransactionManager {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     * @param fibconfig the FibConfig to use.
     * @see FibConfig.
     */
    FibConfigTransactionManager(EventLoop& eventloop, FibConfig& fibconfig)
	: TransactionManager(eventloop, TIMEOUT_MS, MAX_PENDING),
	  _fibconfig(fibconfig)
    {}

    /**
     * Get a reference to the FibConfig.
     *
     * @return a reference to the FibConfig.
     * @see FibConfig.
     */
    FibConfig& fibconfig() { return _fibconfig; }

    /**
     * Get the string with the first error during commit.
     *
     * @return the string with the first error during commit or an empty
     * string if no error.
     */
    const string& error() const 	{ return _first_error; }

    /**
     * Get the maximum number of operations.
     *
     * @return the maximum number of operations.
     */
    size_t max_ops() const { return MAX_OPS; }

protected:
    /**
     * Pre-commit method that is called before the first operation
     * in a commit.
     *
     * This is an overriding method.
     *
     * @param tid the transaction ID.
     */
    virtual void pre_commit(uint32_t tid);

    /**
     * Post-commit method that is called after the last operation
     * in a commit.
     *
     * This is an overriding method.
     *
     * @param tid the transaction ID.
     */
    virtual void post_commit(uint32_t tid);

    /**
     * Method that is called after each operation.
     *
     * This is an overriding method.
     *
     * @param success set to true if the operation succeeded, otherwise false.
     * @param op the operation that has been just called.
     */
    virtual void operation_result(bool success,
				  const TransactionOperation& op);

private:
    /**
     * Set the string with the error.
     *
     * Only the string for the first error is recorded.
     *
     * @param error the string with the error.
     * @return XORP_OK if this was the first error, otherwise XORP_ERROR.
     */
    int set_error(const string& error);

    /**
     * Reset the string with the error.
     */
    void reset_error()			{ _first_error.erase(); }

    FibConfig&	_fibconfig;		// The FibConfig to use
    string	_first_error;		// The string with the first error

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
 * Class to store request to add forwarding entry to FibConfig and
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

    string str() const {
	return c_format("AddEntry4: %s",  _fte.str().c_str());
    }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete forwarding entry to FibConfig and
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

    string str() const {
	return c_format("DeleteEntry4: %s", _fte.str().c_str());
    }

private:
    Fte4 _fte;
};

/**
 * Class to store request to delete all forwarding entries to FibConfig and
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

    string str() const { return c_format("DeleteAllEntries4"); }
};

/**
 * Class to store request to add forwarding entry to FibConfig and
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

    string str() const { 
	return c_format("AddEntry6: %s", _fte.str().c_str());
    }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete forwarding entry to FibConfig
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

    string str() const {
	return c_format("DeleteEntry6: %s", _fte.str().c_str());
    }

private:
    Fte6 _fte;
};

/**
 * Class to store request to delete all forwarding entries to FibConfig
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

    string str() const { return c_format("DeleteAllEntries6"); }
};

#endif // __FEA_FIBCONFIG_TRANSACTION_HH__
