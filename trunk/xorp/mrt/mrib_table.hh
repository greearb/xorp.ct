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

// $XORP: xorp/mrt/mrib_table.hh,v 1.4 2003/03/10 23:20:45 hodson Exp $

#ifndef __MRT_MRIB_TABLE_HH__
#define __MRT_MRIB_TABLE_HH__


//
// Multicast Routing Information Base Table header file.
//


#include <list>
#include <utility>

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
    uint16_t	next_hop_vif_index() const { return (_next_hop_vif_index); }
    
    /**
     * Set the vif index of the interface toward the next-hop router.
     * 
     * @param v the value of the vif index to set.
     */
    void	set_next_hop_vif_index(uint16_t v) { _next_hop_vif_index = v; }
    
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
    // The private state
    IPvXNet	_dest_prefix;		// The destination prefix address
    IPvX	_next_hop_router_addr;	// The address of the next-hop router
    uint16_t	_next_hop_vif_index;	// The vif index to the next-hop router
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
     * Increment Operator
     * 
     * Increment the iterator to point to the next @ref MribLookup entry.
     * 
     * @return a reference to the iterator for the next @ref MribLookup entry
     * (see @ref MribLookup::get_next()).
     */
    MribTableIterator& operator++();
    
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
     * Remove all entries (make the container empty).
     */
    void	clear();
    
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
     * Commit pending transactions for adding or removing @ref Mrib
     * entries for a given transaction ID.
     * 
     * All pending transactions to add/remove @ref Mrib entries for a given
     * transaction ID are processes (see @ref MribTable::add_pending_insert()
     * and @ref MribTable::add_pending_remove()).
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
	PendingTransaction(uint32_t tid, const Mrib& mrib, bool is_insert)
	    : _tid(tid),
	      _mrib(mrib),
	      _is_insert(is_insert)
	    {}
	uint32_t	tid() const { return (_tid); }
	const Mrib&	mrib() const { return (_mrib); }
	bool		is_insert() const { return (_is_insert); }
	
    private:
	uint32_t	_tid;		// The transaction ID
	Mrib		_mrib;		// The MRIB to add or remove
	bool		_is_insert;	// If true, insert, otherwise remove
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
    MribLookup(MribLookup *parent) : _parent(parent) {
	_left_child = _right_child = NULL;
	_mrib = NULL;
    }
    
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
    // The private state
    friend class MribTable;
    
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
