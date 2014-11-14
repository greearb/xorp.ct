// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008-2009 XORP, Inc.
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

#include "fea_node.hh"
#include "iftree.hh"
#include "firewall_manager.hh"
#include "firewall_transaction.hh"

//
// Firewall configuration manager.
//

FirewallManager::FirewallManager(FeaNode&	fea_node,
				 const IfTree&	iftree)
    : _eventloop(fea_node.eventloop()),
      _iftree(iftree),
      _ftm(NULL),
      _next_token(0),
      _is_running(false)
{
    _ftm = new FirewallTransactionManager(_eventloop);
}

FirewallManager::~FirewallManager()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for manipulating "
		   "the firewall table information: %s",
		   error_msg.c_str());
    }

    while (! _browse_db.empty()) {
	uint32_t token = _browse_db.begin()->first;
	delete_browse_state(token);
    }

    if (_ftm != NULL) {
	delete _ftm;
	_ftm = NULL;
    }
}

ProcessStatus
FirewallManager::status(string& reason) const
{
    if (_ftm->pending() > 0) {
	reason = "There are transactions pending";
	return (PROC_NOT_READY);
    }
    return (PROC_READY);
}

int
FirewallManager::start_transaction(uint32_t& tid, string& error_msg)
{
    if (start(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot start FirewallManager: %s",
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    if (_ftm->start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending transactions hit");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::abort_transaction(uint32_t tid, string& error_msg)
{
    if (_ftm->abort(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    // Cleanup state
    _added_entries.clear();
    _replaced_entries.clear();
    _deleted_entries.clear();

    return (XORP_OK);
}

int
FirewallManager::add_transaction_operation(
    uint32_t tid,
    const TransactionManager::Operation& op,
    string& error_msg)
{
    uint32_t n_ops = 0;

    if (_ftm->retrieve_size(tid, n_ops) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    // XXX: If necessary, check whether n_ops is above a pre-defined limit.

    //
    // In theory, resource shortage is the only thing that could get us here
    //
    if (_ftm->add(tid, op) != true) {
	error_msg = c_format("Unknown resource shortage");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::commit_transaction(uint32_t tid, string& error_msg)
{
    int ret_value = XORP_OK;

    // Cleanup leftover state
    _added_entries.clear();
    _replaced_entries.clear();
    _deleted_entries.clear();

    if (_ftm->commit(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    if (_ftm->error().empty() != true) {
	error_msg = _ftm->error();
	return (XORP_ERROR);
    }

    ret_value = update_entries(error_msg);

    // Cleanup state
    _added_entries.clear();
    _replaced_entries.clear();
    _deleted_entries.clear();

    return (ret_value);
}

int
FirewallManager::register_firewall_get(FirewallGet* firewall_get,
				       bool is_exclusive)
{
    if (is_exclusive)
	_firewall_gets.clear();

    if ((firewall_get != NULL)
	&& (find(_firewall_gets.begin(), _firewall_gets.end(), firewall_get)
	    == _firewall_gets.end())) {
	_firewall_gets.push_back(firewall_get);
    }

    return (XORP_OK);
}

int
FirewallManager::unregister_firewall_get(FirewallGet* firewall_get)
{
    if (firewall_get == NULL)
	return (XORP_ERROR);

    list<FirewallGet*>::iterator iter;
    iter = find(_firewall_gets.begin(), _firewall_gets.end(), firewall_get);
    if (iter == _firewall_gets.end())
	return (XORP_ERROR);
    _firewall_gets.erase(iter);

    return (XORP_OK);
}

int
FirewallManager::register_firewall_set(FirewallSet* firewall_set,
				       bool is_exclusive)
{
    string error_msg;

    if (is_exclusive)
	_firewall_sets.clear();

    if ((firewall_set != NULL) 
	&& (find(_firewall_sets.begin(), _firewall_sets.end(), firewall_set)
	    == _firewall_sets.end())) {
	_firewall_sets.push_back(firewall_set);

	//
	// XXX: Push the current config into the new method
	//
	if (firewall_set->is_running()) {
	    list<FirewallEntry> firewall_entry_list;

	    if (get_table4(firewall_entry_list, error_msg) == XORP_OK) {
		if (firewall_set->set_table4(firewall_entry_list, error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv4 firewall table "
			       "into a new mechanism for setting the "
			       "firewall table: %s", error_msg.c_str());
		}
	    }

#ifdef HAVE_IPV6
	    firewall_entry_list.clear();

	    if (get_table6(firewall_entry_list, error_msg) == XORP_OK) {
		if (firewall_set->set_table6(firewall_entry_list, error_msg)
		    != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv6 firewall table "
			       "into a new mechanism for setting the "
			       "firewall table: %s", error_msg.c_str());
		}
	    }
#endif // HAVE_IPV6
	}
    }

    return (XORP_OK);
}

int
FirewallManager::unregister_firewall_set(FirewallSet* firewall_set)
{
    if (firewall_set == NULL)
	return (XORP_ERROR);

    list<FirewallSet*>::iterator iter;
    iter = find(_firewall_sets.begin(), _firewall_sets.end(), firewall_set);
    if (iter == _firewall_sets.end())
	return (XORP_ERROR);
    _firewall_sets.erase(iter);

    return (XORP_OK);
}

int
FirewallManager::start(string& error_msg)
{
    list<FirewallGet*>::iterator firewall_get_iter;
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Check whether all mechanisms are available
    //
    // XXX: The firewall mechanisms are optional hence we don't check
    // whether they are available.
    //
#if 0
    if (_firewall_gets.empty()) {
	error_msg = c_format("No mechanism to get firewall table entries");
	return (XORP_ERROR);
    }
    if (_firewall_sets.empty()) {
	error_msg = c_format("No mechanism to set firewall table entries");
	return (XORP_ERROR);
    }
#endif

    //
    // Start the FirewallGet methods
    //
    for (firewall_get_iter = _firewall_gets.begin();
	 firewall_get_iter != _firewall_gets.end();
	 ++firewall_get_iter) {
	FirewallGet* firewall_get = *firewall_get_iter;
	if (firewall_get->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FirewallSet methods
    //
    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FirewallManager::stop(string& error_msg)
{
    list<FirewallGet*>::iterator firewall_get_iter;
    list<FirewallSet*>::iterator firewall_set_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the FirewallSet methods
    //
    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FirewallGet methods
    //
    for (firewall_get_iter = _firewall_gets.begin();
	 firewall_get_iter != _firewall_gets.end();
	 ++firewall_get_iter) {
	FirewallGet* firewall_get = *firewall_get_iter;
	if (firewall_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    _is_running = false;

    return (ret_value);
}

int
FirewallManager::update_entries(string& error_msg)
{
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_firewall_sets.empty()) {
	error_msg = c_format("No firewall plugin to set the entries");
	return (XORP_ERROR);
    }

    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->update_entries(_added_entries, _replaced_entries,
					 _deleted_entries, error_msg)
	    != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::add_entry(const FirewallEntry& firewall_entry,
			   string& error_msg)
{
    UNUSED(error_msg);

    _added_entries.push_back(firewall_entry);

    return (XORP_OK);
}

int
FirewallManager::replace_entry(const FirewallEntry& firewall_entry,
			       string& error_msg)
{
    UNUSED(error_msg);

    _replaced_entries.push_back(firewall_entry);

    return (XORP_OK);
}

int
FirewallManager::delete_entry(const FirewallEntry& firewall_entry,
			      string& error_msg)
{
    UNUSED(error_msg);

    _deleted_entries.push_back(firewall_entry);

    return (XORP_OK);
}

int
FirewallManager::set_table4(const list<FirewallEntry>& firewall_entry_list,
			    string& error_msg)
{
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_firewall_sets.empty()) {
	error_msg = c_format("No firewall plugin to set the entries");
	return (XORP_ERROR);
    }

    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->set_table4(firewall_entry_list, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
FirewallManager::set_table6(const list<FirewallEntry>& firewall_entry_list,
			    string& error_msg)
{
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_firewall_sets.empty()) {
	error_msg = c_format("No firewall plugin to set the entries");
	return (XORP_ERROR);
    }

    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->set_table6(firewall_entry_list, error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
FirewallManager::delete_all_entries4(string& error_msg)
{
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_firewall_sets.empty()) {
	error_msg = c_format("No firewall plugin to set the entries");
	return (XORP_ERROR);
    }

    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->delete_all_entries4(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::delete_all_entries6(string& error_msg)
{
    list<FirewallSet*>::iterator firewall_set_iter;

    if (_firewall_sets.empty()) {
	error_msg = c_format("No firewall plugin to set the entries");
	return (XORP_ERROR);
    }

    for (firewall_set_iter = _firewall_sets.begin();
	 firewall_set_iter != _firewall_sets.end();
	 ++firewall_set_iter) {
	FirewallSet* firewall_set = *firewall_set_iter;
	if (firewall_set->delete_all_entries6(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::get_table4(list<FirewallEntry>& firewall_entry_list,
			    string& error_msg)
{
    if (_firewall_gets.empty()) {
	error_msg = c_format("No firewall plugin to get the entries");
	return (XORP_ERROR);
    }

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_firewall_gets.front()->get_table4(firewall_entry_list, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::get_table6(list<FirewallEntry>& firewall_entry_list,
			    string& error_msg)
{
    if (_firewall_gets.empty()) {
	error_msg = c_format("No firewall plugin to get the entries");
	return (XORP_ERROR);
    }

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_firewall_gets.front()->get_table6(firewall_entry_list, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallManager::get_entry_list_start4(uint32_t& token, bool& more,
				       string& error_msg)
{
    BrowseState* browse_state;

    generate_token();
    token = _next_token;

    // Create and insert the browse state
    browse_state = new BrowseState(*this, token);
    _browse_db.insert(make_pair(token, browse_state));

    if (browse_state->get_entry_list_start4(more, error_msg) != XORP_OK) {
	delete_browse_state(token);
	return (XORP_ERROR);
    }

    // If no entries to list, then cleanup the state
    if (! more)
	delete_browse_state(token);

    return (XORP_OK);
}

int
FirewallManager::get_entry_list_start6(uint32_t& token, bool& more,
				       string& error_msg)
{
    BrowseState* browse_state;

    generate_token();
    token = _next_token;

    // Create and insert the browse state
    browse_state = new BrowseState(*this, token);
    _browse_db.insert(make_pair(token, browse_state));

    if (browse_state->get_entry_list_start6(more, error_msg) != XORP_OK) {
	delete_browse_state(token);
	return (XORP_ERROR);
    }

    // If no entries to list, then cleanup the state
    if (! more)
	delete_browse_state(token);

    return (XORP_OK);
}

int
FirewallManager::get_entry_list_next4(uint32_t	token,
				      FirewallEntry& firewall_entry,
				      bool&	more,
				      string&	error_msg)
{
    BrowseState* browse_state;
    map<uint32_t, BrowseState *>::iterator iter;

    // Test whether the token is valid
    iter = _browse_db.find(token);
    if (iter == _browse_db.end()) {
	more = false;
	error_msg = c_format("No firewall state to browse for token %u",
			     token);
	return (XORP_ERROR);
    }

    browse_state = iter->second;
    if (browse_state->get_entry_list_next4(firewall_entry, more, error_msg)
	!= XORP_OK) {
	delete_browse_state(token);
	return (XORP_ERROR);
    }

    // If no entries to list, then cleanup the state
    if (! more)
	delete_browse_state(token);

    return (XORP_OK);
}

int
FirewallManager::get_entry_list_next6(uint32_t	token,
				      FirewallEntry& firewall_entry,
				      bool&	more,
				      string&	error_msg)
{
    BrowseState* browse_state;
    map<uint32_t, BrowseState *>::iterator iter;

    // Test whether the token is valid
    iter = _browse_db.find(token);
    if (iter == _browse_db.end()) {
	more = false;
	error_msg = c_format("No firewall state to browse for token %u",
			     token);
	return (XORP_ERROR);
    }

    browse_state = iter->second;
    if (browse_state->get_entry_list_next6(firewall_entry, more, error_msg)
	!= XORP_OK) {
	delete_browse_state(token);
	return (XORP_ERROR);
    }

    // If no entries to list, then cleanup the state
    if (! more)
	delete_browse_state(token);

    return (XORP_OK);
}

void
FirewallManager::generate_token()
{
    // Generate a new token that is available
    do {
	++_next_token;
	if (_browse_db.find(_next_token) == _browse_db.end())
	    break;
    } while (true);
}

void
FirewallManager::delete_browse_state(uint32_t token)
{
    map<uint32_t, BrowseState *>::iterator iter;

    iter = _browse_db.find(token);
    XLOG_ASSERT(iter != _browse_db.end());
    BrowseState* browse_state = iter->second;

    _browse_db.erase(iter);
    delete browse_state;
}

int
FirewallManager::BrowseState::get_entry_list_start4(bool& more,
						    string& error_msg)
{
    more = false;

    if (_firewall_manager.get_table4(_snapshot, error_msg) != XORP_OK)
	return (XORP_ERROR);

    _next_entry_iter = _snapshot.begin();
    if (! _snapshot.empty()) {
	more = true;
	schedule_timer();
    }

    return (XORP_OK);
}

int
FirewallManager::BrowseState::get_entry_list_start6(bool& more,
						    string& error_msg)
{
    more = false;

    if (_firewall_manager.get_table6(_snapshot, error_msg) != XORP_OK)
	return (XORP_ERROR);

    _next_entry_iter = _snapshot.begin();
    if (! _snapshot.empty()) {
	more = true;
	schedule_timer();
    }

    return (XORP_OK);
}

int
FirewallManager::BrowseState::get_entry_list_next4(FirewallEntry& firewall_entry,
						   bool&	more,
						   string&	error_msg)
{
    more = false;

    if (_next_entry_iter == _snapshot.end()) {
	error_msg = c_format("No more firewall entries for token %u", _token);
	return (XORP_ERROR);
    }

    // Get the state
    firewall_entry = *_next_entry_iter;

    // Prepare for the next request
    ++_next_entry_iter;
    if (_next_entry_iter != _snapshot.end()) {
	more = true;
	schedule_timer();
    }

    return (XORP_OK);
}

int
FirewallManager::BrowseState::get_entry_list_next6(FirewallEntry& firewall_entry,
						   bool&	more,
						   string&	error_msg)
{
    more = false;

    if (_next_entry_iter == _snapshot.end()) {
	error_msg = c_format("No more firewall entries for token %u", _token);
	return (XORP_ERROR);
    }

    // Get the state
    firewall_entry = *_next_entry_iter;

    // Prepare for the next request
    ++_next_entry_iter;
    if (_next_entry_iter != _snapshot.end()) {
	more = true;
	schedule_timer();
    }

    return (XORP_OK);
}

void
FirewallManager::BrowseState::schedule_timer()
{
    _timeout_timer = _firewall_manager.eventloop().new_oneoff_after_ms(
	BROWSE_TIMEOUT_MS,
	callback(this, &FirewallManager::BrowseState::timeout));
}

void
FirewallManager::BrowseState::timeout()
{
    _firewall_manager.delete_browse_state(_token);
}
