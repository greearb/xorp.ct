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

#ident "$XORP: xorp/mrt/mrib_table.cc,v 1.6 2003/09/30 00:22:25 pavlin Exp $"


//
// Multicast Routing Information Base Table implementation.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/vif.hh"
#include "mrib_table.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


//
// MribTable functions
//

/**
 * MribTable::MribTable:
 * @family: The address family (e.g., %AF_INET or %AF_INET6).
 * 
 * MribTable constructor.
 **/
MribTable::MribTable(int family)
    : _family(family)
{
    // TODO: add the default entry
    _mrib_lookup_root = NULL;
    _mrib_lookup_size = 0;
    _mrib_size = 0;
}

/**
 * MribTable::~MribTable:
 * @: 
 * 
 * MribTable destructor.
 * Remove and delete all MRIB entries and cleanup.
 **/
MribTable::~MribTable()
{
    clear();
}

//
// Remove all MRIB entries from the table.
// XXX: the entries themselves are deleted too.
//
void
MribTable::clear()
{
    //
    // Delete all MribLookup entries
    //
    remove_mrib_lookup(_mrib_lookup_root);
    _mrib_lookup_root = NULL;
    _mrib_lookup_size = 0;
    _mrib_size = 0;
    
    //
    // Delete all pending transactions
    //
    _mrib_pending_transactions.clear();
}

//
// Insert a copy of a MRIB routing entry that was already created.
// XXX: if there is an existing MRIB entry for the same prefix, the old entry
// is deleted.
//
Mrib *
MribTable::insert(const Mrib& mrib)
{
    const IPvX	 lookup_addr = mrib.dest_prefix().masked_addr();
    size_t	 prefix_len = mrib.dest_prefix().prefix_len();
    uint32_t	 mem_lookup_addr[sizeof(IPvX)];
    const size_t lookup_addr_size_words
	= lookup_addr.addr_size()/sizeof(mem_lookup_addr[0]);
    
    // Copy the destination address prefix to memory
    lookup_addr.copy_out((uint8_t *)mem_lookup_addr);
    
    MribLookup *mrib_lookup = _mrib_lookup_root;
    
    if (mrib_lookup == NULL) {
	// The root/default entry
	mrib_lookup = new MribLookup(NULL);
	_mrib_lookup_size++;
	_mrib_lookup_root = mrib_lookup;
    }
    
    if (prefix_len == 0) {
	// The default routing entry
	if (mrib_lookup->_mrib != NULL) {
	    delete mrib_lookup->_mrib;		// XXX: delete the old entry
	    _mrib_size--;
	}
	Mrib *mrib_copy = new Mrib(mrib);
	mrib_lookup->_mrib = mrib_copy;
	_mrib_size++;
	return (mrib_lookup->mrib());
    }
    
    //
    // The main lookup procedure
    //
    for (size_t i = 0; i < lookup_addr_size_words; i++) {
	uint32_t lookup_word = ntohl(mem_lookup_addr[i]);
	for (size_t j = 0; j < sizeof(lookup_word)*NBBY; j++) {
	    MribLookup *parent_mrib_lookup = mrib_lookup;
	    
#define MRIB_LOOKUP_BITTEST	(1 << (sizeof(lookup_word)*NBBY - 1))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->_right_child;
	    else
		mrib_lookup = mrib_lookup->_left_child;
	    
	    if (mrib_lookup == NULL) {
		// Create a new entry
		mrib_lookup = new MribLookup(parent_mrib_lookup);
		_mrib_lookup_size++;
		if (lookup_word & MRIB_LOOKUP_BITTEST)
		    parent_mrib_lookup->_right_child = mrib_lookup;
		else
		    parent_mrib_lookup->_left_child = mrib_lookup;
	    }
#undef MRIB_LOOKUP_BITTEST
	    
	    if (--prefix_len == 0) {
		// Found the place to install the entry
		if (mrib_lookup->_mrib != NULL) {
		    delete mrib_lookup->_mrib;	// XXX: delete the old entry
		    _mrib_size--;
		}
		Mrib *mrib_copy = new Mrib(mrib);
		mrib_lookup->_mrib = mrib_copy;
		_mrib_size++;
		return (mrib_lookup->mrib());
	    }
	    lookup_word <<= 1;
	}
    }
    
    assert(false);
    XLOG_FATAL("Unexpected internal error adding prefix %s to the "
	       "Multicast Routing Information Base Table",
	       mrib.str().c_str());
    
    return (NULL);
}

//
// Remove a MRIB entry from the table.
// The information about the entry to remove is in @mrib.
//
void
MribTable::remove(const Mrib& mrib)
{
    remove(mrib.dest_prefix());
}

//
// Remove a MRIB entry from the table.
// The destination prefix of the entry to remove is in @dest_prefix.
//
void
MribTable::remove(const IPvXNet& dest_prefix)
{
    MribLookup *mrib_lookup = find_prefix_mrib_lookup(dest_prefix);
    
    if (mrib_lookup == NULL)
	return;			// TODO: should we use assert() instead?
    
    if (mrib_lookup->_mrib != NULL) {
	delete mrib_lookup->_mrib;
	mrib_lookup->_mrib = NULL;
	_mrib_size--;
    }
    
    //
    // Remove recursively all parent nodes that are not in use anymore
    //
    do {
	if ((mrib_lookup->_left_child != NULL)
	    || (mrib_lookup->_right_child != NULL)
	    || (mrib_lookup->_mrib != NULL))
	    break;	// Node is still in use
	
	MribLookup *parent_mrib_lookup = mrib_lookup->_parent;
	
	if (parent_mrib_lookup != NULL) {
	    if (parent_mrib_lookup->_left_child == mrib_lookup)
		parent_mrib_lookup->_left_child = NULL;
	    else
		parent_mrib_lookup->_right_child = NULL;
	}
	delete mrib_lookup;
	_mrib_lookup_size--;
	mrib_lookup = parent_mrib_lookup;
    } while (mrib_lookup != NULL);
    
    if (_mrib_lookup_size == 0)
	_mrib_lookup_root = NULL;
}

//
// Remove recursively all MribLookup entries below (and including) mrib_lookup
// XXX: the Mrib entries are also deleted.
//
void
MribTable::remove_mrib_lookup(MribLookup *mrib_lookup)
{
    // Sanity check
    if (mrib_lookup == NULL)
	return;
    
    // Delete the Mrib entry itself
    if (mrib_lookup->_mrib != NULL) {
	delete mrib_lookup->_mrib;
	_mrib_size--;
	mrib_lookup->_mrib = NULL;
    }
    
    if (mrib_lookup->_parent != NULL) {
	// Chear the state in the parent
	if (mrib_lookup->_parent->_left_child == mrib_lookup) {
	    mrib_lookup->_parent->_left_child = NULL;
	} else {
	    assert(mrib_lookup->_parent->_right_child == mrib_lookup);
	    mrib_lookup->_parent->_right_child = NULL;
	}
    }
    
    // Remove recursively the left subtree
    if (mrib_lookup->_left_child != NULL) {
	mrib_lookup->_left_child->_parent = NULL;
	remove_mrib_lookup(mrib_lookup->_left_child);
    }
    // Remove recursively the right subtree
    if (mrib_lookup->_right_child != NULL) {
	mrib_lookup->_right_child->_parent = NULL;
	remove_mrib_lookup(mrib_lookup->_right_child);
    }
    
    // Delete myself
    delete mrib_lookup;
    _mrib_lookup_size--;
    if (_mrib_lookup_size == 0)
	_mrib_lookup_root = NULL;
    // Done
}

Mrib *
MribTable::find(const IPvX& lookup_addr) const
{
    uint32_t	 mem_lookup_addr[sizeof(IPvX)];
    const size_t lookup_addr_size_words
	= lookup_addr.addr_size()/sizeof(mem_lookup_addr[0]);
    
    // Copy the destination address prefix to memory
    lookup_addr.copy_out((uint8_t *)mem_lookup_addr);
    
    MribLookup *mrib_lookup = _mrib_lookup_root;
    Mrib *longest_match_mrib = NULL;
    
    if (mrib_lookup == NULL)
	return (longest_match_mrib);
    
    //
    // The main lookup procedure
    //
    for (size_t i = 0; i < lookup_addr_size_words; i++) {
	uint32_t lookup_word = ntohl(mem_lookup_addr[i]);
	for (size_t j = 0; j < sizeof(lookup_word)*NBBY; j++) {
	    MribLookup *parent_mrib_lookup = mrib_lookup;
	    if (parent_mrib_lookup->_mrib != NULL)
		longest_match_mrib = parent_mrib_lookup->_mrib;
	    
#define MRIB_LOOKUP_BITTEST	(1 << (sizeof(lookup_word)*NBBY - 1))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->_right_child;
	    else
		mrib_lookup = mrib_lookup->_left_child;
#undef MRIB_LOOKUP_BITTEST
	    
	    if (mrib_lookup == NULL) {
		// Longest match
		return (longest_match_mrib);
	    }
	    lookup_word <<= 1;
	}
    }
    
    assert(mrib_lookup->_mrib != NULL);
    return (mrib_lookup->_mrib);
}

Mrib *
MribTable::find_exact(const IPvXNet& dest_prefix) const
{
    MribLookup *mrib_lookup = find_prefix_mrib_lookup(dest_prefix);
    
    if (mrib_lookup == NULL)
	return (NULL);
    
    return (mrib_lookup->_mrib);
}

//
// Find an exact MribLookup match.
//
MribLookup *
MribTable::find_prefix_mrib_lookup(const IPvXNet& addr_prefix) const
{
    const IPvX	 lookup_addr = addr_prefix.masked_addr();
    size_t	 prefix_len = addr_prefix.prefix_len();
    uint32_t	 mem_lookup_addr[sizeof(IPvX)];
    const size_t lookup_addr_size_words
	= lookup_addr.addr_size()/sizeof(mem_lookup_addr[0]);
    
    // Copy the destination address prefix to memory
    lookup_addr.copy_out((uint8_t *)mem_lookup_addr);
    
    MribLookup *mrib_lookup = _mrib_lookup_root;

    if (mrib_lookup == NULL)
	return (mrib_lookup);

    if (prefix_len == 0) {
	// The default routing entry
	return (mrib_lookup);
    }

    //
    // The main lookup procedure
    //
    for (size_t i = 0; i < lookup_addr_size_words; i++) {
	uint32_t lookup_word = ntohl(mem_lookup_addr[i]);
	for (size_t j = 0; j < sizeof(lookup_word)*NBBY; j++) {
	    
#define MRIB_LOOKUP_BITTEST	(1 << (sizeof(lookup_word)*NBBY - 1))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->_right_child;
	    else
		mrib_lookup = mrib_lookup->_left_child;
	    
	    if (mrib_lookup == NULL) {
		// Not found
		return (mrib_lookup);
	    }
#undef MRIB_LOOKUP_BITTEST
	    
	    if (--prefix_len == 0) {
		// Found the entry
		return (mrib_lookup);
	    }
	    lookup_word <<= 1;
	}
    }
    
    assert(false);
    XLOG_FATAL("Unexpected internal error lookup prefix %s in the "
	       "Multicast Routing Information Base Table",
	       addr_prefix.str().c_str());
    
    return (NULL);
}

/**
 * Update the vif index of a @ref Mrib entry.
 * 
 * @param dest_prefix the destination prefix of the @ref Mrib entry
 * to update.
 * @param vif_index the new vif index of the @ref Mrib entry.
 */
void
MribTable::update_entry_vif_index(const IPvXNet& dest_prefix,
				  uint16_t vif_index)
{
    Mrib* mrib;

    //
    // Update the entry already installed in the table
    //
    mrib = find_exact(dest_prefix);
    if (mrib != NULL)
	mrib->set_next_hop_vif_index(vif_index);

    //
    // Update all entries in pending transactions
    //
    list<PendingTransaction>::iterator iter;
    for (iter = _mrib_pending_transactions.begin();
	 iter != _mrib_pending_transactions.end();
	 ++iter) {
	PendingTransaction& pending_transaction = *iter;
	if (pending_transaction.mrib().dest_prefix() == dest_prefix) {
	    pending_transaction.update_entry_vif_index(vif_index);
	}
    }
}

void
MribTable::add_pending_insert(uint32_t tid, const Mrib& mrib)
{
    _mrib_pending_transactions.push_back(PendingTransaction(tid, mrib, true));
}

void
MribTable::add_pending_remove(uint32_t tid, const Mrib& mrib)
{
    _mrib_pending_transactions.push_back(PendingTransaction(tid, mrib, false));
}

void
MribTable::commit_pending_transactions(uint32_t tid)
{
    //
    // Process the pending Mrib transactions for the given transaction ID
    //
    // TODO: probably should allow commits in time slices of up to X ms?
    list<PendingTransaction>::iterator iter, old_iter;
    for (iter = _mrib_pending_transactions.begin();
	 iter != _mrib_pending_transactions.end();
	) {
	PendingTransaction& pending_transaction = *iter;
	old_iter = iter;
	++iter;
	if (pending_transaction.tid() != tid)
	    continue;
	if (pending_transaction.is_insert())
	    insert(pending_transaction.mrib());
	else
	    remove(pending_transaction.mrib());
	_mrib_pending_transactions.erase(old_iter);
    }
}

void
MribTable::abort_pending_transactions(uint32_t tid)
{
    //
    // Abort pending Mrib transactions for the given transaction ID
    //
    list<PendingTransaction>::iterator iter, old_iter;
    for (iter = _mrib_pending_transactions.begin();
	 iter != _mrib_pending_transactions.end();
	) {
	PendingTransaction& pending_transaction = *iter;
	old_iter = iter;
	++iter;
	if (pending_transaction.tid() != tid)
	    continue;
	_mrib_pending_transactions.erase(old_iter);
    }
}

MribTableIterator&
MribTableIterator::operator++()
{
    _mrib_lookup = _mrib_lookup->get_next();
    return (*this);
}

Mrib *
MribTableIterator::operator*() const
{
    return (_mrib_lookup->mrib());
}

MribLookup *
MribLookup::get_next() const
{
    if (_left_child != NULL)
	return (_left_child);
    if (_right_child != NULL)
	return (_right_child);
    
    // Go UP recursively and find the next branch to go DOWN
    const MribLookup *mrib_lookup = this;
    MribLookup *parent_mrib_lookup = mrib_lookup->_parent;
    while (parent_mrib_lookup != NULL) {
	if (parent_mrib_lookup->_right_child == mrib_lookup) {
	    mrib_lookup = parent_mrib_lookup;
	    parent_mrib_lookup = mrib_lookup->_parent;
	    continue;
	}
	assert(parent_mrib_lookup->_left_child == mrib_lookup);
	if (parent_mrib_lookup->_right_child != NULL)
	    return (parent_mrib_lookup->_right_child);	// The right child
	// Go UP
	mrib_lookup = parent_mrib_lookup;
	parent_mrib_lookup = mrib_lookup->_parent;
	continue;
    }
    
    return (NULL);
}

//
// Mrib functions
//
Mrib::Mrib(int family)
    : _dest_prefix(family),
      _next_hop_router_addr(family)
{
    _next_hop_vif_index = Vif::VIF_INDEX_INVALID;
    _metric_preference = ~0;
    _metric = ~0;
}

Mrib::Mrib(const IPvXNet& dest_prefix)
    : _dest_prefix(dest_prefix),
      _next_hop_router_addr(dest_prefix.af())
{
    _next_hop_vif_index = Vif::VIF_INDEX_INVALID;
    _metric_preference = ~0;
    _metric = ~0;
}
Mrib::Mrib(const Mrib& mrib)
    : _dest_prefix(mrib.dest_prefix()),
      _next_hop_router_addr(mrib.next_hop_router_addr()),
      _next_hop_vif_index(mrib.next_hop_vif_index()),
      _metric_preference(mrib.metric_preference()),
      _metric(mrib.metric())
{
    
}

bool
Mrib::operator==(const Mrib& other) const
{
    return ((_dest_prefix == other.dest_prefix())
	    && (_next_hop_router_addr == other.next_hop_router_addr())
	    && (_next_hop_vif_index == other.next_hop_vif_index())
	    && (_metric_preference == other.metric_preference())
	    && (_metric == other.metric()));
}

string
Mrib::str() const
{
    string s = "";
    
    s += "dest_prefix: " + _dest_prefix.str();
    s += " next_hop_router: " + _next_hop_router_addr.str();
    s += " next_hop_vif_index: " + c_format("%d", _next_hop_vif_index);
    s += " metric_preference: " + c_format("%d", _metric_preference);
    s += " metric: " + c_format("%d", _metric);
    
    return s;
}

