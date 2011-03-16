// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/mrt/mrib_table.hh,v 1.17 2008/10/02 21:57:45 bms Exp $

#ifndef __MRT_MRIB_TABLE_HH__
#define __MRT_MRIB_TABLE_HH__


//
// Multicast Routing Information Base Table header file.
//





#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"


//
// Constants definitions
//
enum {
    MRIB_DONT_CREATE = false,
    MRIB_DO_CREATE = true
};

//
// Structures/classes, typedefs and macros
//

class MribTable;
class MribTableIterator;
class MribLookup;
class Mrib;


/**
 * @short The Multicast Routing Information Base payload entry.
 */
class Mrib {
public:
    /**
     * Constructor for a given address family
     * 
     * @param family the address family.
     */
    Mrib(int family);

    /**
     * Constructor for a given network address prefix.
     * 
     * @param dest_prefix the network address prefix.
     */
    Mrib(const IPvXNet& dest_prefix);

    /**
     * Copy constructor for a given @ref Mrib entry.
     * 
     * @param mrib the @ref Mrib entry to copy.
     */
    Mrib(const Mrib& mrib);
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const Mrib& other) const;
    
    /**
     * Get the network prefix address.
     * 
     * @return the network prefix address.
     */
    const IPvXNet& dest_prefix() const { return (_dest_prefix); }

    /**
     * Set the network prefix address.
     * 
     * @param v the value of the network prefix address to set.
     */
    void	set_dest_prefix(const IPvXNet& v) { _dest_prefix = v; }
    
    /**
     * Get the next-hop router address.
     * 
     * @return the next-hop router address.
     */
    const IPvX&	next_hop_router_addr() const { return (_next_hop_router_addr); }
    
    /**
     * Set the next-hop router address.
     * 
     * @param v the value of the next-hop router address to set.
     */
    void	set_next_hop_router_addr(const IPvX& v) { _next_hop_router_addr = v; }
    
    /**
     * Get the vif index of the interface toward the next-hop router.
     * 
     * @return the vif index of the interface toward the next-hop router.
     */
    uint32_t	next_hop_vif_index() const { return (_next_hop_vif_index); }
    
    /**
     * Set the vif index of the interface toward the next-hop router.
     * 
     * @param v the value of the vif index to set.
     */
    void	set_next_hop_vif_index(uint32_t v) { _next_hop_vif_index = v; }
    
    /**
     * Get the metric preference value.
     * 
     * @return the metric preference value.
     */
    uint32_t	metric_preference() const { return (_metric_preference); }
    
    /**
     * Set the metric preference value.
     * 
     * @param v the value of the metric preference to set.
     */
    void	set_metric_preference(uint32_t v) { _metric_preference = v; }

    /**
     * Get the metric value.
     * 
     * @return the metric value.
     */
    uint32_t	metric() const { return (_metric); }
    
    /**
     * Set the metric value.
     * 
     * @param v the value of the metric to set.
     */
    void	set_metric(uint32_t v) { _metric = v; }
    
    /**
     * Convert this entry from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the entry.
     */
    string str() const;
    
private:
    IPvXNet	_dest_prefix;		// The destination prefix address
    IPvX	_next_hop_router_addr;	// The address of the next-hop router
    uint32_t	_next_hop_vif_index;	// The vif index to the next-hop router
    uint32_t	_metric_preference;	// The metric preference to the
					// destination
    uint32_t	_metric;		// The metric to the destination
};

/**
 * @short The Multicast Routing Information Base Table iterator
 */
class MribTableIterator {
public:
    /**
     * Constructor for a given @ref MribLookup entry.
     * 
     * @param mrib_lookup the basic @ref MribLookup entry.
     */
    MribTableIterator(MribLookup *mrib_lookup) : _mrib_lookup(mrib_lookup) {}
    
    /**
     * Destructor
     */
    MribTableIterator() {}
    
    /**
     * Increment Operator (prefix).
     * 
     * Increment the iterator to point to the next @ref MribLookup entry.
     * 
     * @return a reference to the iterator after it was incremented.
     * @see MribLookup::get_next()
     */
    MribTableIterator& operator++();

    /**
     * Increment Operator (postfix).
     * 
     * Increment the iterator to point to the next @ref MribLookup entry.
     * 
     * @return the value of the iterator before it was incremented.
     * @see MribLookup::get_next()
     */
    MribTableIterator operator++(int);
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is same as the
     * right-hand operand.
     */
    bool operator==(const MribTableIterator& other) const {
	return (_mrib_lookup == other._mrib_lookup);
    }
    
    /**
     * Not-Equal Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is not same as the
     * right-hand operand.
     */
    bool operator!=(const MribTableIterator& other) const {
	return (_mrib_lookup != other._mrib_lookup);
    }
    
    /**
     * Indirection Operator
     * 
     * @return a pointer to the @ref Mrib entry that corresponds to this
     * iterator.
     */
    Mrib* operator*() const;
    
private:
    MribLookup *_mrib_lookup;	// The MribLookup entry for this iterator
};

/**
 * @short Base class for the Multicast Routing Information Base Table
 */
class MribTable {
public:
    /**
     * Constructor for table of a given address family.
     * 
     * @param family the address family.
     */
    MribTable(int family);
    
    /**
     * Destructor
     */
    ~MribTable();
    
    typedef MribTableIterator	iterator;

    /**
     * Get the address family.
     *
     * @return the address family ((e.g., AF_INET or AF_INET6 for IPv4 and
     * IPv6 respectively).
     */
    int		family() const { return (_family); }

    /**
     * Remove all entries and pending transactions (make the container empty).
     */
    void	clear();

    /**
     * Remove all entries.
     */
    void	remove_all_entries();

    /**
     * Get a reference to the list with removed @ref Mrib entries.
     * 
     * @return a reference to the list with removed @ref Mrib entries.
     */
    list<Mrib *>& removed_mrib_entries() {
	return (_removed_mrib_entries);
    }

    /**
     * Test if the removed @ref Mrib entries are preserved or deleted.
     * 
     * @return true if the removed @ref Mrib entries are preserved, otherwise
     * false.
     */
    bool is_preserving_removed_mrib_entries() const {
	return (_is_preserving_removed_mrib_entries);
    }

    /**
     * Enable or disable the preserving of the removed @ref Mrib entries.
     * 
     * @param v if true, then the removed @ref Mrib entries are preserved
     * otherwise they are deleted.
     */
    void set_is_preserving_removed_mrib_entries(bool v) {
	_is_preserving_removed_mrib_entries = v;
    }

    /**
     * Insert a copy of a @ref Mrib entry.
     * 
     * Note: if there is an existing @ref Mrib entry for the same prefix,
     * the old entry is deleted.
     * 
     * @param mrib the entry to insert.
     * @return a pointer to the inserted entry on success, otherwise NULL.
     */
    Mrib	*insert(const Mrib& mrib);
    
    /**
     * Remove from the table a @ref Mrib entry for a given destination prefix.
     * 
     * @param dest_prefix the destination prefix of the entry to remove.
     */
    void	remove(const IPvXNet& dest_prefix);
    
    /**
     * Remove a @ref Mrib entry from the table.
     * 
     * @param mrib a @ref Mrib with information about the entry to remove.
     */
    void	remove(const Mrib& mrib);
    
    /**
     * Find the longest prefix match for an address.
     * 
     * @param address the lookup address.
     * @return a pointer to the longest prefix @ref Mrib match
     * for @ref address if exists, otherwise NULL.
     */
    Mrib	*find(const IPvX& address) const;
    
    /**
     * Find an exact match for a network address prefix.
     * 
     * @param dest_prefix the lookup network address prefix.
     * @return a pointer to the exact @ref Mrib match for @ref dest_prefix if
     * exists, otherwise NULL.
     */
    Mrib	*find_exact(const IPvXNet& dest_prefix) const;
    
    /**
     * Get an iterator for the first element.
     * 
     * @return the iterator for the first element.
     */
    iterator	begin() const { return (_mrib_lookup_root); }
    
    /**
     * Get an iterator for the last element.
     * 
     * @return the iterator for the last element.
     */
    iterator	end() const { return (NULL); }

    /**
     * Update the vif index of a @ref Mrib entry.
     * 
     * @param dest_prefix the destination prefix of the @ref Mrib entry
     * to update.
     * @param vif_index the new vif index of the @ref Mrib entry.
     */
    void update_entry_vif_index(const IPvXNet& dest_prefix,
				uint32_t vif_index);

    //
    // Pending transactions related methods
    //
    /**
     * Add a pending transaction to insert a @ref Mrib entry from the table.
     * 
     * The operation is added to the list of pending transactions, but
     * the entry itself is not added to the table
     * (until @ref MribTable::commit_pending_transactions() is called).
     * 
     * @param tid the transaction ID.
     * @param mrib the @ref Mrib entry that contains the information about
     * the entry to add.
     */
    void	add_pending_insert(uint32_t tid, const Mrib& mrib);

    /**
     * Add a pending transaction to remove a @ref Mrib entry from the table.
     * 
     * the operation is added to the list of pending transaction, but the
     * entry itself is not removed from the table
     * (until @ref MribTable::commit_pending_transactions() is called).
     * 
     * @param tid the transaction ID.
     * @param mrib the @ref Mrib entry that contains the information about
     * the entry to remove.
     */
    void	add_pending_remove(uint32_t tid, const Mrib& mrib);

    /**
     * Add a pending transaction to remove all @ref Mrib entries
     * from the table.
     * 
     * the operation is added to the list of pending transaction, but the
     * entries themselves is not removed from the table
     * (until @ref MribTable::commit_pending_transactions() is called).
     * 
     * @param tid the transaction ID.
     */
    void	add_pending_remove_all_entries(uint32_t tid);

    /**
     * Commit pending transactions for adding or removing @ref Mrib
     * entries for a given transaction ID.
     * 
     * All pending transactions to add/remove @ref Mrib entries for a given
     * transaction ID are processes (see @ref MribTable::add_pending_insert()
     * and @ref MribTable::add_pending_remove()
     * and @ref MribTable::add_pending_remove_all_entries()).
     * 
     * @param tid the transaction ID of the entries to commit.
     */
    void	commit_pending_transactions(uint32_t tid);
    
    /**
     * Abort pending transactions for adding or removing @ref Mrib
     * entries for a given transaction ID.
     * 
     * @param tid the transaction ID of the entries to abort.
     */
    void	abort_pending_transactions(uint32_t tid);
    
    /**
     * Abort all pending transactions for adding or remove @ref Mrib entries.
     */
    void	abort_all_pending_transactions() {
	_mrib_pending_transactions.clear();
    }
    
    /**
     * Get the number of @ref Mrib entries in the table.
     * 
     * @return the number of @ref Mrib entries in the table.
     */
    size_t	size()	 const	{ return (_mrib_size);	}
    
private:
    /**
     * Remove a @ref Mrib entry from the table.
     * 
     * The @ref Mrib entry itself is either deleted or added to the list
     * with removed entries.
     * 
     * @param mrib the @ref Mrib entry to remove.
     */
    void remove_mrib_entry(Mrib *mrib);

    /**
     * Find an exact @ref MribLookip match.
     * 
     * @param addr_prefix the lookup network address prefix.
     * @return a pointer to the exact @ref MribLookup match
     * for @ref addr_prefix if exists, otherwise NULL.
     */
    MribLookup	*find_prefix_mrib_lookup(const IPvXNet& addr_prefix) const;
    
    /**
     * Remove a subtree of entries in the table.
     * 
     * Remove recursively all @ref MribLookup below and including
     * the given @ref mrib_lookup entry.
     * 
     * @param mrib_lookup the @ref MribLookup entry that is the root of the
     * subtree to remove.
     */
    void	remove_mrib_lookup(MribLookup *mrib_lookup);
    
    //
    // Private class used to keep track of pending transactions
    //
    class PendingTransaction {
    public:
	/**
	 * Constructor to insert or remove a MRIB entry.
	 *
	 * @param tid the transaction ID.
	 * @param mrib the MRIB entry to insert or remove.
	 * @param is_insert if true, then insert the entry, otherwise
	 * remove it.
	 */
	PendingTransaction(uint32_t tid, const Mrib& mrib, bool is_insert)
	    : _tid(tid),
	      _mrib(mrib),
	      _is_insert(is_insert),
	      _is_remove_all(false)
	{}

#ifdef XORP_USE_USTL
	PendingTransaction(): _mrib(AF_INET) { }
#endif

	/**
	 * Constructor to remove all entries.
	 */
	PendingTransaction(const MribTable& mrib_table, uint32_t tid)
	    : _tid(tid),
	      _mrib(Mrib(IPvXNet(IPvX::ZERO(mrib_table.family()), 0))),
	      _is_insert(false),
	      _is_remove_all(true)
	{}
	uint32_t	tid() const { return (_tid); }
	const Mrib&	mrib() const { return (_mrib); }
	bool		is_insert() const { return (_is_insert); }
	bool		is_remove_all() const { return (_is_remove_all); }
	void		update_entry_vif_index(uint32_t vif_index) {
	    _mrib.set_next_hop_vif_index(vif_index);
	}
	
    private:
	uint32_t	_tid;		// The transaction ID
	Mrib		_mrib;		// The MRIB to add or remove
	bool		_is_insert;	// If true, insert, otherwise remove
	bool		_is_remove_all;	// If true, remove all entries
    };
    
    //
    // The private state
    //
    int		_family;		// The address family of this table
    MribLookup	*_mrib_lookup_root;	// The root of the MRIB lookup tree
    size_t	_mrib_lookup_size;	// The number of MribLookup entries
    size_t	_mrib_size;		// The number of Mrib entries
    
    //
    // The list of pending transactions
    //
    list<PendingTransaction> _mrib_pending_transactions;

    //
    // A flag to indicate whether the removed Mrib entries are preserved
    // on the _removed_mrib_entries list, or are deleted.
    //
    bool		_is_preserving_removed_mrib_entries;

    //
    // The list of removed Mrib entries that may be still in use.
    //
    list<Mrib *>	_removed_mrib_entries;
};

/**
 * @short The basic entry in the @ref MribTable lookup tree
 */
class MribLookup {
public:
    /**
     * Constructor for a given parent entry.
     * 
     * @param parent the parent entry.
     */
    MribLookup(MribLookup *parent)
	: _parent(parent),
	  _left_child(NULL),
	  _right_child(NULL),
	  _mrib(NULL)
    {}
    
    /**
     * Destructor
     */
    ~MribLookup() { if (_mrib != NULL) delete _mrib; }
    
    /**
     * Get the corresponding @ref Mrib entry.
     * 
     * @return the corresponding @ref Mrib entry if exists, otherwise NULL.
     */
    Mrib *mrib() const { return (_mrib); }

    /**
     * Set the corresponding @ref Mrib entry.
     * 
     * Note that the previous value of the corresponding @ref Mrib entry
     * is overwritten.
     * 
     * @param v the value of the corresponding @ref Mrib entry.
     */
    void set_mrib(Mrib *v) { _mrib = v; }

    /**
     * Get the parent @ref MribLookup entry.
     * 
     * @return the parent @ref MribLookup entry.
     */
    MribLookup *parent() { return (_parent); }

    /**
     * Set the parent @ref MribLookup entry.
     * 
     * Note that the previous value of the parent is overwritten.
     * 
     * @param v the parent @ref MribLookup to assign to this entry.
     */
    void set_parent(MribLookup *v) { _parent = v; }

    /**
     * Get the left child @ref MribLookup entry.
     * 
     * Note that the previous value of the left child is overwritten.
     * 
     * @return the left child @ref MribLookup entry.
     */
    MribLookup *left_child() { return (_left_child); }

    /**
     * Set the left child @ref MribLookup entry.
     * 
     * @param v the left child @ref MribLookup to assign to this entry.
     */
    void set_left_child(MribLookup *v) { _left_child = v; }

    /**
     * Get the right child @ref MribLookup entry.
     * 
     * @return the right child @ref MribLookup entry.
     */
    MribLookup *right_child() { return (_right_child); }

    /**
     * Set the right child @ref MribLookup entry.
     * 
     * Note that the previous value of the right child is overwritten.
     * 
     * @param v the right child @ref MribLookup to assign to this entry.
     */
    void set_right_child(MribLookup *v) { _right_child = v; }

    /**
     * Get the next @ref MribLookup entry.
     * 
     * The ordering of the entries is "depth-first search", where
     * the nodes are returned in the following order:
     * (a) the parent node;
     * (b) the left-child node and all nodes within its subtree;
     * (c) the right-child node and all nodes within its subtree;
     * 
     * @return the next @ref MribLookup entry if exists, otherwise NULL.
     */
    MribLookup *get_next() const;
    
private:
    MribLookup	*_parent;	// The parent in the lookup tree
    MribLookup	*_left_child;	// The left child in the lookup tree
    MribLookup	*_right_child;	// The right child in the lookup tree
    Mrib	*_mrib;		// A pointer to the MRIB info
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MRT_MRIB_TABLE_HH__
