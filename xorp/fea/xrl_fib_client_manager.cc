// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "xrl_fib_client_manager.hh"


/**
 * Process a list of IPv4 FIB route changes.
 * 
 * The FIB route changes come from the underlying system.
 * 
 * @param fte_list the list of Fte entries to add or delete.
 */
void
XrlFibClientManager::process_fib_changes(const list<Fte4>& fte_list)
{
    map<string, FibClient4>::iterator iter;

    for (iter = _fib_clients4.begin(); iter != _fib_clients4.end(); ++iter) {
	FibClient4& fib_client = iter->second;
	fib_client.activate(fte_list);
    }
}


XrlCmdError
XrlFibClientManager::add_fib_client4(const string& client_target_name,
				     const bool send_updates,
				     const bool send_resolves)
{
    // Test if we have this client already
    if (_fib_clients4.find(client_target_name) != _fib_clients4.end()) {
	string error_msg = c_format("Target %s is already an IPv4 FIB client",
				    client_target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Add the client
    _fib_clients4.insert(make_pair(client_target_name,
				   FibClient4(client_target_name, *this)));
    FibClient4& fib_client = _fib_clients4.find(client_target_name)->second;

    fib_client.set_send_updates(send_updates);
    fib_client.set_send_resolves(send_resolves);

    // Activate the client
    list<Fte4> fte_list;
    if (_fibconfig.get_table4(fte_list) != XORP_OK) {
	string error_msg = "Cannot get the IPv4 FIB";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    fib_client.activate(fte_list);

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlFibClientManager::delete_fib_client4(const string& client_target_name)
{
    map<string, FibClient4>::iterator iter;

    iter = _fib_clients4.find(client_target_name);
    if (iter == _fib_clients4.end()) {
	string error_msg = c_format("Target %s is not an IPv4 FIB client",
				    client_target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    _fib_clients4.erase(iter);

    return XrlCmdError::OKAY();
}

int
XrlFibClientManager::send_fib_client_add_route(const string& target_name,
					       const Fte4& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_add_route4(
	target_name.c_str(),
	fte.net(),
	fte.nexthop(),
	fte.ifname(),
	fte.vifname(),
	fte.metric(),
	fte.admin_distance(),
	string("NOT_SUPPORTED"),
	fte.xorp_route(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_add_route4_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}

int
XrlFibClientManager::send_fib_client_delete_route(const string& target_name,
						  const Fte4& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_delete_route4(
	target_name.c_str(),
	fte.net(),
	fte.ifname(),
	fte.vifname(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_delete_route4_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}

int
XrlFibClientManager::send_fib_client_resolve_route(const string& target_name,
						   const Fte4& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_resolve_route4(
	target_name.c_str(),
	fte.net(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_resolve_route4_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}

void
XrlFibClientManager::send_fib_client_add_route4_cb(const XrlError& xrl_error,
						   string target_name)
{
    map<string, FibClient4>::iterator iter;

    iter = _fib_clients4.find(target_name);
    if (iter == _fib_clients4.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient4& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}

void
XrlFibClientManager::send_fib_client_delete_route4_cb(
    const XrlError& xrl_error,
    string target_name)
{
    map<string, FibClient4>::iterator iter;

    iter = _fib_clients4.find(target_name);
    if (iter == _fib_clients4.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient4& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}

void
XrlFibClientManager::send_fib_client_resolve_route4_cb(
    const XrlError& xrl_error,
    string target_name)
{
    map<string, FibClient4>::iterator iter;

    iter = _fib_clients4.find(target_name);
    if (iter == _fib_clients4.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient4& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}

template<class F>
void
XrlFibClientManager::FibClient<F>::activate(const list<F>& fte_list)
{
    bool queue_was_empty = _inform_fib_client_queue.empty();

    if (fte_list.empty())
	return;

    // Create the queue with the entries to add
    typename list<F>::const_iterator iter;
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const F& fte = *iter;
	_inform_fib_client_queue.push_back(fte);
    }

    // If the queue was empty before, start sending the routes
    if (queue_was_empty)
	send_fib_client_route_change();
}

template<class F>
void
XrlFibClientManager::FibClient<F>::send_fib_client_route_change()
{
    int success = XORP_ERROR;

    do {
	bool ignore_fte = true;

	if (_inform_fib_client_queue.empty())
	    return;		// No more route changes to send

	F& fte = _inform_fib_client_queue.front();

	//
	// If FIB route misses and resolution requests were requested to be
	// heard by the client, then send notifications of such events.
	//
	if (_send_resolves && fte.is_unresolved()) {
	    ignore_fte = false;
	    success = _xfcm->send_fib_client_resolve_route(_target_name, fte);
	}

	//
	// If FIB updates were requested by the client, then send notification
	// of a route being added or deleted.
	//
	if (_send_updates && !fte.is_unresolved()) {
	    ignore_fte = false;
	    if (!fte.is_deleted()) {
		// Send notification of a route being added
		success = _xfcm->send_fib_client_add_route(_target_name, fte);
	    } else {
		// Send notification of a route being deleted
		success = _xfcm->send_fib_client_delete_route(_target_name,
							     fte);
	    }
	}

	if (ignore_fte) {
	    //
	    // This entry is not needed hence silently drop it and process the
	    // next one.
	    //
	    _inform_fib_client_queue.pop_front();
	    continue;
	}

	break;
    } while (true);

    if (success != XORP_OK) {
	//
	// If an error, then start a timer to try again
	// TODO: XXX: the timer value is hardcoded here!!
	//
	_inform_fib_client_queue_timer = eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFibClientManager::FibClient<F>::send_fib_client_route_change));
    }
}

template<class F>
void
XrlFibClientManager::FibClient<F>::send_fib_client_route_change_cb(
    const XrlError& xrl_error)
{
    // If success, then send the next route change
    if (xrl_error == XrlError::OKAY()) {
	_inform_fib_client_queue.pop_front();
	send_fib_client_route_change();
	return;
    }

    //
    // If command failed because the other side rejected it,
    // then send the next route change.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Error sending route change to %s: %s",
		   _target_name.c_str(), xrl_error.str().c_str());
	_inform_fib_client_queue.pop_front();
	send_fib_client_route_change();
	return;
    }

    //
    // If a transport error, then start a timer to try again
    // (unless the timer is already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_inform_fib_client_queue_timer.scheduled())
	return;
    _inform_fib_client_queue_timer = eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlFibClientManager::FibClient<F>::send_fib_client_route_change));
}

template class XrlFibClientManager::FibClient<Fte4>;


#ifdef HAVE_IPV6

/**
 * Process a list of IPv6 FIB route changes.
 * 
 * The FIB route changes come from the underlying system.
 * 
 * @param fte_list the list of Fte entries to add or delete.
 */
void
XrlFibClientManager::process_fib_changes(const list<Fte6>& fte_list)
{
    map<string, FibClient6>::iterator iter;

    for (iter = _fib_clients6.begin(); iter != _fib_clients6.end(); ++iter) {
	FibClient6& fib_client = iter->second;
	fib_client.activate(fte_list);
    }
}

XrlCmdError
XrlFibClientManager::add_fib_client6(const string& client_target_name,
				     const bool send_updates,
				     const bool send_resolves)
{
    // Test if we have this client already
    if (_fib_clients6.find(client_target_name) != _fib_clients6.end()) {
	string error_msg = c_format("Target %s is already an IPv6 FIB client",
				    client_target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Add the client
    _fib_clients6.insert(make_pair(client_target_name,
				   FibClient6(client_target_name, *this)));
    FibClient6& fib_client = _fib_clients6.find(client_target_name)->second;

    fib_client.set_send_updates(send_updates);
    fib_client.set_send_resolves(send_resolves);

    // Activate the client
    list<Fte6> fte_list;
    if (_fibconfig.get_table6(fte_list) != XORP_OK) {
	string error_msg = "Cannot get the IPv6 FIB";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    fib_client.activate(fte_list);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFibClientManager::delete_fib_client6(const string& client_target_name)
{
    map<string, FibClient6>::iterator iter;

    iter = _fib_clients6.find(client_target_name);
    if (iter == _fib_clients6.end()) {
	string error_msg = c_format("Target %s is not an IPv6 FIB client",
				    client_target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    _fib_clients6.erase(iter);

    return XrlCmdError::OKAY();
}


int
XrlFibClientManager::send_fib_client_add_route(const string& target_name,
					       const Fte6& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_add_route6(
	target_name.c_str(),
	fte.net(),
	fte.nexthop(),
	fte.ifname(),
	fte.vifname(),
	fte.metric(),
	fte.admin_distance(),
	string("NOT_SUPPORTED"),
	fte.xorp_route(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_add_route6_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}


int
XrlFibClientManager::send_fib_client_delete_route(const string& target_name,
						  const Fte6& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_delete_route6(
	target_name.c_str(),
	fte.net(),
	fte.ifname(),
	fte.vifname(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_delete_route6_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}


int
XrlFibClientManager::send_fib_client_resolve_route(const string& target_name,
						   const Fte6& fte)
{
    bool success;

    success = _xrl_fea_fib_client.send_resolve_route6(
	target_name.c_str(),
	fte.net(),
	callback(this,
		 &XrlFibClientManager::send_fib_client_resolve_route6_cb,
		 target_name));

    if (success)
	return XORP_OK;
    else
	return XORP_ERROR;
}


void
XrlFibClientManager::send_fib_client_add_route6_cb(const XrlError& xrl_error,
						   string target_name)
{
    map<string, FibClient6>::iterator iter;

    iter = _fib_clients6.find(target_name);
    if (iter == _fib_clients6.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient6& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}


void
XrlFibClientManager::send_fib_client_delete_route6_cb(
    const XrlError& xrl_error,
    string target_name)
{
    map<string, FibClient6>::iterator iter;

    iter = _fib_clients6.find(target_name);
    if (iter == _fib_clients6.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient6& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}


void
XrlFibClientManager::send_fib_client_resolve_route6_cb(
    const XrlError& xrl_error,
    string target_name)
{
    map<string, FibClient6>::iterator iter;

    iter = _fib_clients6.find(target_name);
    if (iter == _fib_clients6.end()) {
	// The client has probably gone. Silently ignore.
	return;
    }

    FibClient6& fib_client = iter->second;
    fib_client.send_fib_client_route_change_cb(xrl_error);
}

template class XrlFibClientManager::FibClient<Fte6>;

#endif
