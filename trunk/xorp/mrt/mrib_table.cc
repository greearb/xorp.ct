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




//
// Multicast Routing Information Base Table implementation.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/vif.hh"
#include "libxorp/utils.hh"

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
// MribTable methods
//

/**
 * MribTable::MribTable:
 * @family: The address family (e.g., %AF_INET or %AF_INET6).
 * 
 * MribTable constructor.
 **/
MribTable::MribTable(int family)
    : _family(family),
      _mrib_lookup_root(NULL),
      _mrib_lookup_size(0),
      _mrib_size(0),
      _is_preserving_removed_mrib_entries(false)
{
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
// Clear all entries and pending transactions.
// XXX: The entries themselves are deleted too.
//
void
MribTable::clear()
{
    remove_all_entries();

    //
    // Delete all pending transactions
    //
    _mrib_pending_transactions.clear();

    //
    // Delete the list with the removed Mrib entries
    //
    delete_pointers_list(_removed_mrib_entries);
}

//
// Remove all MRIB entries from the table.
// XXX: The MRIB entries themselves are either deleted or added to
// the list with removed entries.
//
void
MribTable::remove_all_entries()
{
    //
    // Delete all MribLookup entries
    //
    remove_mrib_lookup(_mrib_lookup_root);
    _mrib_lookup_root = NULL;
    _mrib_lookup_size = 0;
    _mrib_size = 0;
}

//
// Insert a copy of a MRIB routing entry that was already created.
// XXX: if there is an existing MRIB entry for the same prefix, the old entry
// is removed from the table and is either deleted or added to the list with
// removed entries.
//
Mrib *
MribTable::insert(const Mrib& mrib)
{
    const IPvX	 lookup_addr = mrib.dest_prefix().masked_addr();
    size_t	 prefix_len = mrib.dest_prefix().prefix_len();
    uint32_t	 mem_lookup_addr[sizeof(IPvX)];
    const size_t lookup_addr_size_words
	= lookup_addr.addr_bytelen()/sizeof(mem_lookup_addr[0]);
    
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
	if (mrib_lookup->mrib() != NULL) {
	    // XXX: remove the old entry
	    remove_mrib_entry(mrib_lookup->mrib());
	    _mrib_size--;
	}
	Mrib *mrib_copy = new Mrib(mrib);
	mrib_lookup->set_mrib(mrib_copy);
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
	    
#define MRIB_LOOKUP_BITTEST   ((uint32_t)(1 << (sizeof(lookup_word)*NBBY - 1)))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->right_child();
	    else
		mrib_lookup = mrib_lookup->left_child();
	    
	    if (mrib_lookup == NULL) {
		// Create a new entry
		mrib_lookup = new MribLookup(parent_mrib_lookup);
		_mrib_lookup_size++;
		if (lookup_word & MRIB_LOOKUP_BITTEST)
		    parent_mrib_lookup->set_right_child(mrib_lookup);
		else
		    parent_mrib_lookup->set_left_child(mrib_lookup);
	    }
#undef MRIB_LOOKUP_BITTEST
	    
	    if (--prefix_len == 0) {
		// Found the place to install the entry
		if (mrib_lookup->mrib() != NULL) {
		    // XXX: remove the old entry
		    remove_mrib_entry(mrib_lookup->mrib());
		    _mrib_size--;
		}
		Mrib *mrib_copy = new Mrib(mrib);
		mrib_lookup->set_mrib(mrib_copy);
		_mrib_size++;
		return (mrib_lookup->mrib());
	    }
	    lookup_word <<= 1;
	}
    }
    
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
	return;			// TODO: should we return an error instead?
    
    if (mrib_lookup->mrib() != NULL) {
	remove_mrib_entry(mrib_lookup->mrib());
	mrib_lookup->set_mrib(NULL);
	_mrib_size--;
    }
    
    //
    // Remove recursively all parent nodes that are not in use anymore
    //
    do {
	if ((mrib_lookup->left_child() != NULL)
	    || (mrib_lookup->right_child() != NULL)
	    || (mrib_lookup->mrib() != NULL))
	    break;	// Node is still in use
	
	MribLookup *parent_mrib_lookup = mrib_lookup->parent();
	
	if (parent_mrib_lookup != NULL) {
	    if (parent_mrib_lookup->left_child() == mrib_lookup)
		parent_mrib_lookup->set_left_child(NULL);
	    else
		parent_mrib_lookup->set_right_child(NULL);
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
// XXX: the Mrib entries are either deleted or added to the list with
// removed entries.
//
void
MribTable::remove_mrib_lookup(MribLookup *mrib_lookup)
{
    // Sanity check
    if (mrib_lookup == NULL)
	return;
    
    // Remove the Mrib entry itself
    if (mrib_lookup->mrib() != NULL) {
	remove_mrib_entry(mrib_lookup->mrib());
	_mrib_size--;
	mrib_lookup->set_mrib(NULL);
    }
    
    if (mrib_lookup->parent() != NULL) {
	// Chear the state in the parent
	if (mrib_lookup->parent()->left_child() == mrib_lookup) {
	    mrib_lookup->parent()->set_left_child(NULL);
	} else {
	    XLOG_ASSERT(mrib_lookup->parent()->right_child() == mrib_lookup);
	    mrib_lookup->parent()->set_right_child(NULL);
	}
    }
    
    // Remove recursively the left subtree
    if (mrib_lookup->left_child() != NULL) {
	mrib_lookup->left_child()->set_parent(NULL);
	remove_mrib_lookup(mrib_lookup->left_child());
    }
    // Remove recursively the right subtree
    if (mrib_lookup->right_child() != NULL) {
	mrib_lookup->right_child()->set_parent(NULL);
	remove_mrib_lookup(mrib_lookup->right_child());
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
	= lookup_addr.addr_bytelen()/sizeof(mem_lookup_addr[0]);
    
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
	    if (parent_mrib_lookup->mrib() != NULL)
		longest_match_mrib = parent_mrib_lookup->mrib();
	    
#define MRIB_LOOKUP_BITTEST   ((uint32_t)(1 << (sizeof(lookup_word)*NBBY - 1)))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->right_child();
	    else
		mrib_lookup = mrib_lookup->left_child();
#undef MRIB_LOOKUP_BITTEST
	    
	    if (mrib_lookup == NULL) {
		// Longest match
		return (longest_match_mrib);
	    }
	    lookup_word <<= 1;
	}
    }
    
    XLOG_ASSERT(mrib_lookup->mrib() != NULL);
    return (mrib_lookup->mrib());
}

Mrib *
MribTable::find_exact(const IPvXNet& dest_prefix) const
{
    MribLookup *mrib_lookup = find_prefix_mrib_lookup(dest_prefix);
    
    if (mrib_lookup == NULL)
	return (NULL);
    
    return (mrib_lookup->mrib());
}

/**
 * Remove a @ref Mrib entry from the table.
 * 
 * The @ref Mrib entry itself is either deleted or added to the list
 * with removed entries.
 * 
 * @param mrib the @ref Mrib entry to remove.
 */
void
MribTable::remove_mrib_entry(Mrib *mrib)
{
    if (is_preserving_removed_mrib_entries())
	_removed_mrib_entries.push_back(mrib);
    else
	delete mrib;
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
	= lookup_addr.addr_bytelen()/sizeof(mem_lookup_addr[0]);
    
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
	    
#define MRIB_LOOKUP_BITTEST   ((uint32_t)(1 << (sizeof(lookup_word)*NBBY - 1)))
	    if (lookup_word & MRIB_LOOKUP_BITTEST)
		mrib_lookup = mrib_lookup->right_child();
	    else
		mrib_lookup = mrib_lookup->left_child();
	    
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
				  uint32_t vif_index)
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
MribTable::add_pending_remove_all_entries(uint32_t tid)
{
    _mrib_pending_transactions.push_back(PendingTransaction(*this, tid));
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
	if (pending_transaction.is_remove_all()) {
	    remove_all_entries();
	} else {
	    if (pending_transaction.is_insert())
		insert(pending_transaction.mrib());
	    else
		remove(pending_transaction.mrib());
	}
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

MribTableIterator
MribTableIterator::operator++(int)
{
    MribTableIterator old_value = *this;
    _mrib_lookup = _mrib_lookup->get_next();
    return (old_value);
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
	XLOG_ASSERT(parent_mrib_lookup->_left_child == mrib_lookup);
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
// Mrib methods
//
Mrib::Mrib(int family)
    : _dest_prefix(family),
      _next_hop_router_addr(family),
      _next_hop_vif_index(Vif::VIF_INDEX_INVALID),
      _metric_preference(~0U),
      _metric(~0U)
{
}

Mrib::Mrib(const IPvXNet& dest_prefix)
    : _dest_prefix(dest_prefix),
      _next_hop_router_addr(dest_prefix.af()),
      _next_hop_vif_index(Vif::VIF_INDEX_INVALID),
      _metric_preference(~0U),
      _metric(~0U)
{
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
    s += " next_hop_vif_index: " +
	c_format("%u", XORP_UINT_CAST(_next_hop_vif_index));
    s += " metric_preference: " +
	c_format("%u", XORP_UINT_CAST(_metric_preference));
    s += " metric: " + c_format("%u", XORP_UINT_CAST(_metric));
    
    return s;
}

