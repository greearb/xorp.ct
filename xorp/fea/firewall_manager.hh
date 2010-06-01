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


#ifndef __FEA_FIREWALL_MANAGER_HH__
#define __FEA_FIREWALL_MANAGER_HH__

#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"

#include <map>

#include "firewall_entry.hh"
#include "firewall_get.hh"
#include "firewall_set.hh"

class EventLoop;
class FeaNode;
class FirewallTransactionManager;


/**
 * @short Firewall configuration manager.
 */
class FirewallManager {
public:
    /**
     * Constructor.
     * 
     * @param fea_node the FEA node.
     * @param iftree the interface configuration tree to use.
     */
    FirewallManager(FeaNode& fea_node, const IfTree& iftree);

    /**
     * Virtual destructor.
     */
    virtual ~FirewallManager();

    /**
     * Get a reference to the @ref EventLoop instance.
     *
     * @return a reference to the @ref EventLoop instance.
     */
    EventLoop& eventloop() { return _eventloop; }

    /**
     * Get a reference to the interface configuration.
     *
     * @return a reference to the interface configuration.
     */
    const IfTree& iftree() const { return _iftree; }

    /**
     * Get the status code.
     *
     * @param reason the human-readable reason for any failure.
     * @return the status code.
     */
    ProcessStatus status(string& reason) const;

    /**
     * Start firewall-related transaction.
     *
     * @param tid the return-by-reference new transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_transaction(uint32_t& tid, string& error_msg);

    /**
     * Commit firewall-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int commit_transaction(uint32_t tid, string& error_msg);

    /**
     * Abort firewall-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int abort_transaction(uint32_t tid, string& error_msg);

    /**
     * Add operation to firewall-related transaction.
     *
     * @param tid the transaction ID.
     * @param op the operation to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_transaction_operation(uint32_t tid,
				  const TransactionManager::Operation& op,
				  string& error_msg);

    /**
     * Register @ref FirewallGet plugin.
     *
     * @param firewall_get the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_firewall_get(FirewallGet* firewall_get, bool is_exclusive);

    /**
     * Unregister @ref FirewallGet plugin.
     *
     * @param firewall_get the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_firewall_get(FirewallGet* firewall_get);

    /**
     * Register @ref FirewallSet plugin.
     *
     * @param firewall_set the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_firewall_set(FirewallSet* firewall_set, bool is_exclusive);

    /**
     * Unregister @ref FirewallSet plugin.
     *
     * @param firewall_set the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_firewall_set(FirewallSet* firewall_set);

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Add a single firewall entry that will be pushed into the underlying
     * system.
     *
     * @param firewall_entry the entry to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_entry(const FirewallEntry& firewall_entry, string& error_msg);

    /**
     * Replace a single firewall entry that will be pushed into the underlying
     * system.
     *
     * @param firewall_entry the entry to replace.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int replace_entry(const FirewallEntry& firewall_entry, string& error_msg);

    /**
     * Delete a single firewall entry that will be pushed into the underlying
     * system.
     *
     * @param firewall_entry the entry to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_entry(const FirewallEntry& firewall_entry, string& error_msg);

    /**
     * Set the IPv4 firewall table.
     *
     * @param firewall_entry_list the list with all entries to install into
     * the IPv4 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_table4(const list<FirewallEntry>& firewall_entry_list,
		   string& error_msg);

    /**
     * Set the IPv6 firewall table.
     *
     * @param firewall_entry_list the list with all entries to install into
     * the IPv6 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_table6(const list<FirewallEntry>& firewall_entry_list,
		   string& error_msg);

    /**
     * Delete all entries in the IPv4 firewall table.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_all_entries4(string& error_msg);

    /**
     * Delete all entries in the IPv6 firewall table.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_all_entries6(string& error_msg);

    /**
     * Obtain the IPv4 firewall table.
     *
     * @param firewall_entry_list the return-by-reference list with all
     * entries in the IPv4 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_table4(list<FirewallEntry>& firewall_entry_list,
		   string& error_msg);

    /**
     * Obtain the IPv6 firewall table.
     *
     * @param firewall_entry_list the return-by-reference list with all
     * entries in the IPv6 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_table6(list<FirewallEntry>& firewall_entry_list,
		   string& error_msg);

    /**
     * Get a token for a list of IPv4 firewall entries.
     *
     * @param token to be provided when calling get_entry_list_next4.
     * @param more true if the list is not empty.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_entry_list_start4(uint32_t& token, bool& more, string& error_msg);

    /**
     * Get a token for a list of IPv6 firewall entries.
     *
     * @param token to be provided when calling get_entry_list_next6.
     * @param more true if the list is not empty.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_entry_list_start6(uint32_t& token, bool& more, string& error_msg);

    /**
     * Get the next item in a list of IPv4 firewall entries.
     *
     * @param token returned by a previous call to get_entry_list_start4.
     * @param firewall_entry the firewall entry.
     * @param more true if the list has more items remaining.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_entry_list_next4(uint32_t	token,
			     FirewallEntry& firewall_entry,
			     bool&	more,
			     string&	error_msg);

    /**
     * Get the next item in a list of IPv6 firewall entries.
     *
     * @param token returned by a previous call to get_entry_list_start6.
     * @param firewall_entry the firewall entry.
     * @param more true if the list has more items remaining.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_entry_list_next6(uint32_t	token,
			     FirewallEntry& firewall_entry,
			     bool&	more,
			     string&	error_msg);

    /**
     * Delete browse state for a particular token.
     *
     * @param token the token for the state to delete.
     */
    void delete_browse_state(uint32_t token);

private:
    /**
     * Update the firewall entries by pushing them into the underlying system.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int update_entries(string& error_msg);

    class BrowseState {
    public:
	BrowseState(FirewallManager& firewall_manager, uint32_t token)
	    : _firewall_manager(firewall_manager), _token(token),
	      _next_entry_iter(_snapshot.begin()) {}

	int get_entry_list_start4(bool& more, string& error_msg);
	int get_entry_list_start6(bool& more, string& error_msg);
	int get_entry_list_next4(FirewallEntry& firewall_entry,
				 bool&		more,
				 string&	error_msg);
	int get_entry_list_next6(FirewallEntry& firewall_entry,
				 bool&		more,
				 string&	error_msg);

    private:
	enum {
	    BROWSE_TIMEOUT_MS = 15000		// XXX: 15 seconds
	};

	/**
	 * Schedule the timer to timeout the browse state.
	 */
	void schedule_timer();

	/**
	 * Timeout the browse state.
	 */
	void timeout();

	FirewallManager&	_firewall_manager;
	uint32_t		_token;
	list<FirewallEntry>	_snapshot;
	list<FirewallEntry>::iterator _next_entry_iter;
	XorpTimer	_timeout_timer;
    };

    /**
     * Generate a new token that is available.
     */
    void generate_token();

    FeaNode&				_fea_node;
    EventLoop&				_eventloop;
    const IfTree&			_iftree;

    //
    // The firewall transaction manager
    //
    FirewallTransactionManager*		_ftm;

    //
    // The registered plugins
    //
    list<FirewallGet*>			_firewall_gets;
    list<FirewallSet*>			_firewall_sets;

    //
    // State browsing information
    //
    uint32_t			_next_token;
    map<uint32_t, BrowseState *> _browse_db;

    //
    // State for collecting and updating the firewall entries
    //
    list<FirewallEntry>		_added_entries;
    list<FirewallEntry>		_replaced_entries;
    list<FirewallEntry>		_deleted_entries;

    //
    // Misc other state
    //
    bool	_is_running;
};

#endif // __FEA_FIREWALL_MANAGER_HH__
