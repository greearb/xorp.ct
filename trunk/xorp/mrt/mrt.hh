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

// $XORP: xorp/mrt/mrt.hh,v 1.5 2004/04/01 19:54:11 mjh Exp $

#ifndef __MRT_MRT_HH__
#define __MRT_MRT_HH__


//
// Multicast Routing Table implementation.
//


#include <map>

#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"
#include "libxorp/ipvxnet.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short Class to store (S,G) (Source, Group) pair of addresses.
 */
class SourceGroup {
public:
    /**
     * Constructor for a source and a group address.
     * 
     * @param source_addr the source address.
     * @param group_addr the group address.
     */
    SourceGroup(const IPvX& source_addr, const IPvX& group_addr)
	: _source_addr(source_addr), _group_addr(group_addr) {}
    
    /**
     * Get the source address.
     * 
     * @return the source address.
     */
    const IPvX& source_addr() const { return (_source_addr); }
    
    /**
     * Get the group address.
     * 
     * @return the group address.
     */
    const IPvX& group_addr() const { return (_group_addr); }
    
private:
    IPvX _source_addr;		// The source address
    IPvX _group_addr;		// The group address
};

/**
 * @short Class for (S,G) lookup key (Source-first, Group-second priority).
 */
class MreSgKey {
public:
    /**
     * Constructor for a given @ref SourceGroup entry.
     * 
     * @param source_group a reference to the corresponding (S,G) entry.
     */
    MreSgKey(const SourceGroup& source_group) : _source_group(source_group) {}
    
    /**
     * Get the corresponding @ref SourceGroup entry.
     * 
     * @return a reference to the corresponding (S,G) entry.
     */
    const SourceGroup& source_group() const { return (_source_group); }
    
    /**
     * Less-Than Operator
     * 
     * @param other the right-hand operand to compare against.
     * 
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const MreSgKey& other) const {
	if (_source_group.source_addr() == other.source_group().source_addr())
	    return (_source_group.group_addr()
		    < other.source_group().group_addr());	
	return (_source_group.source_addr()
		< other.source_group().source_addr());
    }
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const MreSgKey& other) const {
	return ((_source_group.source_addr()
		 == other.source_group().source_addr())
		&& (_source_group.group_addr()
		    == other.source_group().group_addr()));
    }
    
private:
    const SourceGroup& _source_group;	// A reference to the corresponding
					// (S,G) used for comparison.
};

/**
 * @short Class for (S,G) lookup key (Group-first, Source-second priority).
 */
class MreGsKey {
public:
    /**
     * Constructor for a given @ref SourceGroup entry.
     *
     * @param source_group a reference to the corresponding (S,G) entry.
     */
    MreGsKey(const SourceGroup& source_group) : _source_group(source_group) {}

    /**
     * Get the corresponding @ref SourceGroup entry.
     * 
     * @return a reference to the corresponding (S,G) entry.
     */
    const SourceGroup& source_group() const { return (_source_group); }

    /**
     * Less-Than Operator
     * 
     * @param other the right-hand operand to compare against.
     * 
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const MreGsKey& other) const {
	if (_source_group.group_addr() == other.source_group().group_addr())
	    return (_source_group.source_addr()
		    < other.source_group().source_addr());
	return (_source_group.group_addr()
		< other.source_group().group_addr());
    }
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const MreGsKey& other) const {
	return ((_source_group.source_addr()
		 == other.source_group().source_addr())
		&& (_source_group.group_addr()
		    == other.source_group().group_addr()) );
    }
    
private:
    const SourceGroup& _source_group;	// A reference to the corresponding
					// (S,G) used for comparison.
};

/**
 * @short Template class for Multicast Routing Table.
 */
template <class E>
class Mrt {
public:
    /**
     * Default constructor
     */
    Mrt() {}
    
    /**
     * Destructor
     */
    virtual ~Mrt() { clear(); }
    
    //
    // Typedefs for lookup maps and iterators.
    //
    typedef map<MreSgKey, E*>		SgMap;
    typedef map<MreGsKey, E*>		GsMap;
    
    typedef typename SgMap::iterator	sg_iterator;
    typedef typename GsMap::iterator	gs_iterator;
    typedef typename SgMap::const_iterator const_sg_iterator;
    typedef typename GsMap::const_iterator const_gs_iterator;
    
    /**
     * Remove all multicast routing entries from the table.
     * 
     * Note that the entries themselves are also deleted.
     */
    void clear() {
	// Remove all multicast routing entries.
	for (sg_iterator iter = _sg_table.begin(); iter != _sg_table.end(); ) {
	    E *mre = iter->second;
	    ++iter;
	    delete mre;
	}
	// Clear the (S,G) and (G,S) lookup tables
	_sg_table.clear();
	_gs_table.clear();
    }
    
    /**
     * Insert a multicast routing entry that was already created.
     * 
     * Note that we insert a pointer to the @ref mre, hence @ref mre
     * should not be deleted without removing it first from the table.
     * 
     * @param mre the multicast routing entry to insert.
     * @return the multicast routing entry that was inserted on success,
     * otherwise NULL.
     */
    E *insert(E *mre) {
	pair<sg_iterator, bool> sg_pos
	    = _sg_table.insert(
		pair<MreSgKey, E*>(MreSgKey(mre->source_group()), mre));
	if (! sg_pos.second)
	    return (NULL);
	pair<gs_iterator, bool> gs_pos
	    = _gs_table.insert(
		pair<MreGsKey, E*>(MreGsKey(mre->source_group()), mre));
	if (! gs_pos.second) {
	    _sg_table.erase(sg_pos.first);
	    return (NULL);
	}
	mre->_sg_key = sg_pos.first;
	mre->_gs_key = gs_pos.first;
	
	return (mre);
    }
    
    /**
     * Remove a multicast routing entry from the table.
     * 
     * Note that the entry itself is not deleted.
     * 
     * @param mre the multicast routing entry to delete from the table.
     * @return XORP_OK if the entry was in the table and was successfully
     * removed, otherwise XORP_ERROR.
     */
    int remove(E *mre) {
	int ret_value = XORP_ERROR;
	
	if (mre->_sg_key != _sg_table.end()) {
	    _sg_table.erase(mre->_sg_key);
	    mre->_sg_key = _sg_table.end();
	    ret_value = XORP_OK;
	}
	if (mre->_gs_key != _gs_table.end()) {
	    _gs_table.erase(mre->_gs_key);
	    mre->_gs_key = _gs_table.end();
	    ret_value = XORP_OK;
	}
	
	return (ret_value);
    }
    
    /**
     * Find a multicast routing entry from the table.
     * 
     * @param source_addr the source address to search for.
     * @param group_addr the group address to search for.
     * @return the multicast routing entry for source @ref source_addr
     * and group @ref group_addr if found, otherwise NULL.
     */
    E *find(const IPvX& source_addr, const IPvX& group_addr) const {
	const_sg_iterator pos = _sg_table.find(
	    MreSgKey(SourceGroup(source_addr, group_addr)));
	if (pos != _sg_table.end())
	    return (pos->second);
	return (NULL);
    }
    
    /**
     * Find the first multicast routing entry for a source address.
     * 
     * @param source_addr the source address to search for.
     * @return the first multicast routing entry for source @ref source_addr
     * if found, otherwise NULL.
     */
    E *find_source(const IPvX& source_addr) const {
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(source_addr,
				     IPvX::ZERO(source_addr.af()) ) ) );
	if (pos != _sg_table.end()) {
	    if (pos->second->source_addr() == source_addr)
		return (pos->second);
	}
	return (NULL);
    }
    
    /**
     * Find the first multicast routing entry for a group address.
     * 
     * @param group_addr the group address to search for.
     * @return the first multicast routing entry for group @ref group_addr
     * if found, otherwise NULL.
     */
    E *find_group(const IPvX& group_addr) const {
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(IPvX::ZERO(group_addr.af()),
				     group_addr)));
	if (pos != _gs_table.end()) {
	    if (pos->second->group_addr() == group_addr)
		return (pos->second);
	}
	return (NULL);
    }
    
    /**
     * Find the first multicast routing entry for a source address prefix.
     * 
     * @param prefix_s the source address prefix to search for.
     * @return the first multicast routing entry for source address
     * prefix @ref prefix_s if found, otherwise NULL.
     */
    E *find_source_by_prefix(const IPvXNet& prefix_s) const {
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(prefix_s.masked_addr(),
				     IPvX::ZERO(prefix_s.af()) ) ) );
	if (pos == _sg_table.end())
	    return (NULL);
	E *mre = pos->second;
	if (mre->is_same_prefix_s(prefix_s))
	    return (mre);
	return (NULL);
    }
    
    /**
     * Find the first multicast routing entry for a group address prefix.
     * 
     * @param prefix_g the group address prefix to search for.
     * @return the first multicast routing entry for group address
     * prefix @ref prefix_g if found, otherwise NULL.
     */
    E *find_group_by_prefix(const IPvXNet& prefix_g) const {
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(prefix_g.masked_addr(),
				     IPvX::ZERO(prefix_g.af()) ) ) );
	if (pos == _gs_table.end())
	    return (NULL);
	E *mre = pos->second;
	if (mre->is_same_prefix_g(prefix_g))
	    return (mre);
	return (NULL);
    }
    
    /**
     * Get the number of multicast routing entries in the table.
     * 
     * @return the number of multicast routing entries.
     */
    size_t	size()			const { return (_sg_table.size());  }
    
    /**
     * Get an iterator for the first element in the source-group table.
     * 
     * @return the iterator for the first element in the source-group table.
     */
    const_sg_iterator sg_begin()	const { return (_sg_table.begin()); }

    /**
     * Get an iterator for the first element in the group-source table.
     * 
     * @return the iterator for the first element in the group-source table.
     */
    const_gs_iterator gs_begin()	const { return (_gs_table.begin()); }
    
    /**
     * Get an iterator for the last element in the source-group table.
     * 
     * @return the iterator for the last element in the source-group table.
     */
    const_sg_iterator sg_end()		const { return (_sg_table.end());   }
    
    /**
     * Get an iterator for the last element in the group-source table.
     * 
     * @return the iterator for the last element in the group-source table.
     */
    const_gs_iterator gs_end()		const { return (_gs_table.end());   }
    
    /**
     * Find the source iterator for the first multicast routing entry
     * for a source address prefix.
     * 
     * @param prefix_s the source address prefix to search for.
     * @return the iterator for first entry with source address that
     * matches @ref prefix_s. If no matching entry is found, the
     * return value is @ref source_by_prefix_end().
     */
    const_sg_iterator source_by_prefix_begin(const IPvXNet& prefix_s) const {
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(prefix_s.masked_addr(),
				     IPvX::ZERO(prefix_s.af()) ) ) );
	return (pos);
    }
    
    /**
     * Find the source iterator for the one-after-the-last multicast routing
     * entry for a source address prefix.
     * 
     * @param prefix_s the source address prefix to search for.
     * @return the iterator for one-after-the-last entry with source address
     * that matches @ref prefix_s. If no such entry exists, the
     * return value is @ref sg_end().
     */
    const_sg_iterator source_by_prefix_end(const IPvXNet& prefix_s) const {
	if (prefix_s.prefix_len() == 0)
	    return (_sg_table.end());		// XXX: special case
	
	IPvXNet next_prefix_s(prefix_s);
	++next_prefix_s;
	if (next_prefix_s.masked_addr().is_zero())
	    return (_sg_table.end());		// XXX: special case
	
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(next_prefix_s.masked_addr(),
				     IPvX::ZERO(next_prefix_s.af()) ) ) );
	return (pos);
    }
    
    /**
     * Find the group iterator for the first multicast routing entry
     * for a group address prefix.
     * 
     * @param prefix_g the group address prefix to search for.
     * @return the iterator for first entry with group address that
     * matches @ref prefix_g. If no matching entry is found, the
     * return value is @ref group_by_prefix_end().
     */
    const_gs_iterator group_by_prefix_begin(const IPvXNet& prefix_g) const {
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(IPvX::ZERO(prefix_g.af()),
				     prefix_g.masked_addr() ) ) );
	return (pos);
    }
    
    /**
     * Find the group iterator for the one-after-the-last multicast routing
     * entry for a group address prefix.
     * 
     * @param prefix_g the group address prefix to search for.
     * @return the iterator for one-after-the-last entry with group address
     * that matches @ref prefix_g. If no such entry exists, the
     * return value is @ref gs_end().
     */
    const_gs_iterator group_by_prefix_end(const IPvXNet& prefix_g) const {
	if (prefix_g.prefix_len() == 0)
	    return (_gs_table.end());		// XXX: special case
	
	IPvXNet next_prefix_g(prefix_g);
	++next_prefix_g;
	if (next_prefix_g.masked_addr().is_zero())
	    return (_gs_table.end());		// XXX: special case
	
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(IPvX::ZERO(next_prefix_g.af()),
				     next_prefix_g.masked_addr() ) ) );
	return (pos);
    }
    
    /**
     * Find the source iterator for the first multicast routing entry
     * for a source address.
     * 
     * @param source_addr the source address to search for.
     * @return the iterator for first entry with source address that
     * matches @ref source_addr. If no matching entry is found, the
     * return value is @ref source_by_addr_end().
     */
    const_sg_iterator source_by_addr_begin(const IPvX& source_addr) const {
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(source_addr,
				     IPvX::ZERO(source_addr.af()) ) ) );
	return (pos);
    }
    
    /**
     * Find the source iterator for the one-after-the-last multicast routing
     * entry for a source address.
     * 
     * @param source_addr the source address to search for.
     * @return the iterator for one-after-the-last entry with source address
     * that matches @ref source_addr. If no such entry exists, the
     * return value is @ref sg_end().
     */
    const_sg_iterator source_by_addr_end(const IPvX& source_addr) const {
	if (source_addr == IPvX::ALL_ONES(source_addr.af()))
	    return (_sg_table.end());		// XXX: special case
	
	IPvX next_source_addr(source_addr);
	++next_source_addr;
	
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(next_source_addr,
				     IPvX::ZERO(next_source_addr.af()) ) ) );
	return (pos);
    }
    
    /**
     * Find the group iterator for the first multicast routing entry
     * for a group address.
     * 
     * @param group_addr the group address to search for.
     * @return the iterator for first entry with group address that
     * matches @ref group_addr. If no matching entry is found, the
     * return value is @ref group_by_addr_end().
     */
    const_gs_iterator group_by_addr_begin(const IPvX& group_addr) const {
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(IPvX::ZERO(group_addr.af()),
				     group_addr ) ) );
	return (pos);
    }
    
    /**
     * Find the group iterator for the one-after-the-last multicast routing
     * entry for a group address.
     * 
     * @param group_addr the group address prefix to search for.
     * @return the iterator for one-after-the-last entry with group address
     * that matches @ref group_addr. If no such entry exists, the
     * return value is @ref gs_end().
     */
    const_gs_iterator group_by_addr_end(const IPvX& group_addr) const {
	if (group_addr == IPvX::ALL_ONES(group_addr.af()))
	    return (_gs_table.end());		// XXX: special case
	
	IPvX next_group_addr(group_addr);
	++next_group_addr;
	
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(IPvX::ZERO(next_group_addr.af()),
				     next_group_addr ) ) );
	return (pos);
    }
    
    /**
     * Find the group iterator for the multicast routing entry
     * for a source and a group address.
     * 
     * @param source_addr the source address to search for.
     * @param group_addr the group address to search for.
     * @return the iterator for entry with source and group address that
     * matches @ref source_addr and @ref group_addr. If no matching
     * entry is found, the return value is the next entry.
     */
    const_gs_iterator group_source_by_addr_begin(
	const IPvX& source_addr,
	const IPvX& group_addr) const {
	const_gs_iterator pos
	    = _gs_table.lower_bound(
		MreGsKey(SourceGroup(source_addr,
				     group_addr ) ) );
	return (pos);
    }
    
    /**
     * Find the source iterator for the multicast routing entry
     * for a source and a group address.
     * 
     * @param source_addr the source address to search for.
     * @param group_addr the group address to search for.
     * @return the iterator for entry with source and group address that
     * matches @ref source_addr and @ref group_addr. If no matching
     * entry is found, the return value is the next entry.
     */
    const_sg_iterator source_group_by_addr_begin(
	const IPvX& source_addr,
	const IPvX& group_addr) const {
	const_sg_iterator pos
	    = _sg_table.lower_bound(
		MreSgKey(SourceGroup(source_addr,
				     group_addr ) ) );
	return (pos);
    }
    
private:
    SgMap _sg_table;		// The (S,G) source-first lookup table
    GsMap _gs_table;		// The (G,S) group-first lookup table
};

/**
 * @short Template class for the Multicast Routing Entry.
 */
template <class E>
class Mre {
public:
    /**
     * Constructor for a given source and group address.
     * 
     * @param source_addr the source address of the entry.
     * @param group_addr the group address of the entry.
     */
    Mre(const IPvX& source_addr, const IPvX& group_addr)
	: _source_group(source_addr, group_addr) {
	//
	// XXX: the iterators below should be set to
	// _sg_table.end() and _gs_table.end() in the Mrt, but here
	// we don't know those values. Sigh...
	//
	_sg_key = NULL;
	_gs_key = NULL;
    }
    
    /**
     * Destructor
     */
    virtual ~Mre() {};
    
    /**
     * Get the source-group entry.
     * 
     * @return a reference to the @ref SourceGroup source-group entry
     */
    const SourceGroup& source_group() const { return (_source_group); }
    
    /**
     * Get the source address.
     * 
     * @return the source address of the entry.
     */
    const IPvX&	source_addr() const { return (_source_group.source_addr()); }
    
    /**
     * Get the group address.
     * 
     * @return the group address of the entry.
     */
    const IPvX&	group_addr() const { return (_source_group.group_addr()); }
    
    /**
     * Test if this entry matches a source address prefix.
     * 
     * @param prefix_s the source address prefix to match against.
     * @return true if the entry source address belongs to address
     * prefix @ref prefix_s, otherwise false.
     */
    bool	is_same_prefix_s(const IPvXNet& prefix_s) const {
	return (prefix_s.contains(source_addr()));
    }
    
    /**
     * Test if this entry matches a group address prefix.
     *
     * @param prefix_g the group address prefix to match against.
     * @return true if the entry group address belongs to address
     * prefix @ref prefix_g, otherwise false.
     */
    bool	is_same_prefix_g(const IPvXNet& prefix_g) const {
	return (prefix_g.contains(group_addr()));
    }
    
    /**
     * Get the source-group table iterator.
     * 
     * @return the source-group table iterator for this entry.
     */
    const typename Mrt<E>::sg_iterator& sg_key() const { return (_sg_key); }
    
    /**
     * Get the group-source table iterator.
     * 
     * @return the group-source table for this entry.
     */
    const typename Mrt<E>::gs_iterator& gs_key() const { return (_gs_key); }
    
    /**
     * Convert this entry from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the entry.
     */
    string	str() const {
	return (string("(") + source_addr().str()
		+ string(", ") + group_addr().str()
		+ string(")"));
    }
    
protected:
    friend class Mrt<E>;
    
private:
    // Mre state
    const SourceGroup _source_group;	// The source and group addresses
    typename Mrt<E>::sg_iterator _sg_key; // The source-group table iterator
    typename Mrt<E>::gs_iterator _gs_key; // The group-source table iterator
};

//
// Deferred definitions
//

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MRT_MRT_HH__
