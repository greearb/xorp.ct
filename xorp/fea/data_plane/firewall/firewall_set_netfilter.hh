// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/firewall/firewall_set_netfilter.hh,v 1.5 2008/10/02 21:57:03 bms Exp $

#ifndef __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_NETFILTER_HH__
#define __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_NETFILTER_HH__



#include "fea/firewall_set.hh"


class FirewallSetNetfilter : public FirewallSet {
public:
    // Firewall entries trie indexed by rule number
    typedef map<uint32_t, FirewallEntry> FirewallTrie;

    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FirewallSetNetfilter(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FirewallSetNetfilter();

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

    /**
     * Encode a single IPv4 firewall chain.
     *
     * @param chain_name the name of the chain to encode.
     * @param buffer the buffer to store the encoded chain.
     * @param next_data_index the return-by-reference index into the buffer
     * with the location to store the encoded chain.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int encode_chain4(const string& chain_name,
		      vector<uint8_t>& buffer,
		      size_t& next_data_index,
		      string& error_msg);

    /**
     * Encode a single IPv6 firewall chain.
     *
     * @param chain_name the name of the chain to encode.
     * @param buffer the buffer to store the encoded chain.
     * @param next_data_index the return-by-reference index into the buffer
     * with the location to store the encoded chain.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int encode_chain6(const string& chain_name,
		      vector<uint8_t>& buffer,
		      size_t& next_data_index,
		      string& error_msg);

    /**
     * Encode a single IPv4 firewall entry.
     *
     * @param firewall_entry the entry to encode.
     * @param buffer the buffer to store the encoded entry.
     * @param next_data_index the return-by-reference index into the buffer
     * to store the encoded entry.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int encode_entry4(const FirewallEntry& firewall_entry,
		      vector<uint8_t>& buffer,
		      size_t& next_data_index,
		      string& error_msg);

    /**
     * Encode a single IPv6 firewall entry.
     *
     * @param firewall_entry the entry to encode.
     * @param buffer the buffer to store the encoded entry.
     * @param next_data_index the return-by-reference index into the buffer
     * to store the encoded entry.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int encode_entry6(const FirewallEntry& firewall_entry,
		      vector<uint8_t>& buffer,
		      size_t& next_data_index,
		      string& error_msg);

    /**
     * Push all locally stored IPv4 firewall entries.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int push_entries4(string& error_msg);

    /**
     * Push all locally stored IPv6 firewall entries.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int push_entries6(string& error_msg);

    int		_s4;		// The socket for IPv4 firewall access
    int		_s6;		// The socket for IPv6 firewall access

    // The locally saved firewall entries
    FirewallTrie	_firewall_entries4;
    FirewallTrie	_firewall_entries6;

    // Misc. local state
    size_t		_num_entries;
    size_t		_head_offset;
    size_t		_foot_offset;

    static const string _netfilter_table_name;	// The NETFILTER table name
    static const string _netfilter_match_tcp;	// The TCP match name
    static const string _netfilter_match_udp;	// The UDP match name
    static const string _netfilter_chain_input;	// The INPUT chain name
    static const string _netfilter_chain_forward; // The FORWARD chain name
    static const string _netfilter_chain_output; // The OUTPUT chain name
};

#endif // __FEA_DATA_PLANE_FIREWALL_FIREWALL_SET_NETFILTER_HH__
