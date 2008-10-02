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

// $XORP: xorp/fea/data_plane/firewall/firewall_set_dummy.hh,v 1.4 2008/07/23 05:10:25 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_DUMMY_HH__
#define __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_DUMMY_HH__

#include <map>

#include "fea/firewall_set.hh"


class FirewallSetDummy : public FirewallSet {
public:
    // Firewall entries trie indexed by rule number
    typedef map<uint32_t, FirewallEntry> FirewallTrie;

    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FirewallSetDummy(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FirewallSetDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Update the firewall entries by pushing them into the underlying system.
     *
     * @param added_entries the entries to add.
     * @param replaced_entries the entries to replace.
     * @param deleted_entries the deleted entries.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int update_entries(const list<FirewallEntry>& added_entries,
			       const list<FirewallEntry>& replaced_entries,
			       const list<FirewallEntry>& deleted_entries,
			       string& error_msg);

    /**
     * Set the IPv4 firewall table.
     *
     * @param firewall_entry_list the list with all entries to install into
     * the IPv4 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_table4(const list<FirewallEntry>& firewall_entry_list,
			   string& error_msg);

    /**
     * Delete all entries in the IPv4 firewall table.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_all_entries4(string& error_msg);

    /**
     * Set the IPv6 firewall table.
     *
     * @param firewall_entry_list the list with all entries to install into
     * the IPv6 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_table6(const list<FirewallEntry>& firewall_entry_list,
			   string& error_msg);

    /**
     * Delete all entries in the IPv6 firewall table.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_all_entries6(string& error_msg);

    /**
     * Get a reference to the trie with the IPv4 firewall entries.
     *
     * @return a reference to the trie with the IPv4 firewall entries.
     */
    FirewallTrie& firewall_entries4() { return _firewall_entries4; }

    /**
     * Get a reference to the trie with the IPv6 firewall entries.
     *
     * @return a reference to the trie with the IPv6 firewall entries.
     */
    FirewallTrie& firewall_entries6() { return _firewall_entries6; }

private:
    /**
     * Add a single firewall entry.
     *
     * @param firewall_entry the entry to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_entry(const FirewallEntry& firewall_entry,
			  string& error_msg);

    /**
     * Replace a single firewall entry.
     *
     * @param firewall_entry the replacement entry.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int replace_entry(const FirewallEntry& firewall_entry,
			      string& error_msg);

    /**
     * Delete a single firewall entry.
     *
     * @param firewall_entry the entry to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_entry(const FirewallEntry& firewall_entry,
			     string& error_msg);

    // The locally saved firewall entries
    FirewallTrie	_firewall_entries4;
    FirewallTrie	_firewall_entries6;
};

#endif // __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_DUMMY_HH__
