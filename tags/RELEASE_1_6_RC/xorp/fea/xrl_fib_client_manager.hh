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

// $XORP: xorp/fea/xrl_fib_client_manager.hh,v 1.6 2008/07/23 05:10:12 pavlin Exp $

#ifndef __FEA_XRL_FIB_CLIENT_MANAGER_HH__
#define __FEA_XRL_FIB_CLIENT_MANAGER_HH__

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_fib_client_xif.hh"

#include "fibconfig_transaction.hh"
#include "fte.hh"


/**
 * Class for managing clients interested in FIB changes notifications.
 */
class XrlFibClientManager : public FibTableObserverBase {
public:
    /**
     * Constructor
     *
     * @param fibconfig the FibConfig configuration object (@ref FibConfig).
     */
    XrlFibClientManager(FibConfig&	fibconfig,
			XrlRouter&	xrl_router)
	: _fibconfig(fibconfig),
	  _xrl_fea_fib_client(&xrl_router) {
	_fibconfig.add_fib_table_observer(this);
    }

    ~XrlFibClientManager() {
	_fibconfig.delete_fib_table_observer(this);
    }

    /**
     * Get a reference to the @ref EventLoop instance.
     *
     * @return a reference to the @ref EventLoop instance.
     */
    EventLoop& eventloop() { return _fibconfig.eventloop(); }

    /**
     * Process a list of IPv4 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    void process_fib_changes(const list<Fte4>& fte_list);

    /**
     * Process a list of IPv6 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    void process_fib_changes(const list<Fte6>& fte_list);

    /**
     * Add an IPv4 FIB client.
     *
     * @param client_target_name the target name of the client to add.
     * @param send_updates whether updates should be sent.
     * @param send_resolves whether resolve requests should be sent.
     * @return the XRL command error.
     */
    XrlCmdError add_fib_client4(const string& client_target_name,
				const bool send_updates,
				const bool send_resolves);

    /**
     * Add an IPv6 FIB client.
     * 
     * @param client_target_name the target name of the client to add.
     * @param send_updates whether updates should be sent.
     * @param send_resolves whether resolve requests should be sent.
     * @return the XRL command error.
     */
    XrlCmdError add_fib_client6(const string& client_target_name,
				const bool send_updates,
				const bool send_resolves);

    /**
     * Delete an IPv4 FIB client.
     * 
     * @param client_target_name the target name of the client to delete.
     * @return the XRL command error.
     */
    XrlCmdError delete_fib_client4(const string& client_target_name);

    /**
     * Delete an IPv6 FIB client.
     * 
     * @param client_target_name the target name of the client to delete.
     * @return the XRL command error.
     */
    XrlCmdError delete_fib_client6(const string& client_target_name);

    /**
     * Send an XRL to a FIB client to add an IPv4 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_add_route(const string& target_name,
				  const Fte4& fte);

    /**
     * Send an XRL to a FIB client to add an IPv6 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_add_route(const string& target_name,
				  const Fte6& fte);

    /**
     * Send an XRL to a FIB client to delete an IPv4 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_delete_route(const string& target_name,
				     const Fte4& fte);

    /**
     * Send an XRL to a FIB client to delete an IPv6 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_delete_route(const string& target_name,
				     const Fte6& fte);

    /**
     * Send an XRL to a FIB client to inform it of an IPv4 route miss.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the destination to resolve.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_resolve_route(const string& target_name,
				     const Fte4& fte);

    /**
     * Send an XRL to a FIB client to inform it of an IPv6 route miss.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the destination to resolve.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_resolve_route(const string& target_name,
				     const Fte6& fte);

protected:
    FibConfig&		_fibconfig;

private:

    void send_fib_client_add_route4_cb(const XrlError& xrl_error,
				       string target_name);
    void send_fib_client_add_route6_cb(const XrlError& xrl_error,
				       string target_name);
    void send_fib_client_delete_route4_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_delete_route6_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_resolve_route4_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_resolve_route6_cb(const XrlError& xrl_error,
					  string target_name);

    /**
     * A template class for storing FIB client information.
     */
    template<class F>
    class FibClient {
    public:
	FibClient(const string& target_name, XrlFibClientManager& xfcm)
	    : _target_name(target_name), _xfcm(xfcm),
	      _send_updates(false), _send_resolves(false) {}

	void	activate(const list<F>& fte_list);
	void	send_fib_client_route_change_cb(const XrlError& xrl_error);

	bool get_send_updates() const { return _send_updates; }
	bool get_send_resolves() const { return _send_resolves; }
	void set_send_updates(const bool sendit) { _send_updates = sendit; }
	void set_send_resolves(const bool sendit) { _send_resolves = sendit; }

    private:
	EventLoop& eventloop() { return _xfcm.eventloop(); }
	void	send_fib_client_route_change();

	list<F>			_inform_fib_client_queue;
	XorpTimer		_inform_fib_client_queue_timer;

	string			_target_name;	// Target name of the client
	XrlFibClientManager&	_xfcm;

	bool			_send_updates;	// Event filters
	bool			_send_resolves;
    };

    typedef FibClient<Fte4>	FibClient4;
    typedef FibClient<Fte6>	FibClient6;

    map<string, FibClient4>	_fib_clients4;
    map<string, FibClient6>	_fib_clients6;

    XrlFeaFibClientV0p1Client	_xrl_fea_fib_client;
};

#endif // __FEA_XRL_FIB_CLIENT_MANAGER_HH__
