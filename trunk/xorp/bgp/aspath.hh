// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/bgp/aspath.hh,v 1.9 2003/01/29 00:38:56 rizzo Exp $

#ifndef __BGP_ASPATH_HH__
#define __BGP_ASPATH_HH__

#include "config.h"
#include <sys/types.h>
#include <inttypes.h>
#include <string>
#include <list>
#include <vector>

#include "libxorp/debug.h"
#include "libxorp/asnum.hh"
#include "libxorp/exceptions.hh"
#include "exceptions.hh"

/**
 * This file contains the classes to manipulate AS segments/lists/paths
 *
 * An AS_PATH is made of a list of segments, each segment being
 * an AS_SET or an AS_SEQUENCE.
 * Logically, you can think an AS_SEQUENCE as a list of AsNum,
 * and an AS_SET as an unordered set of AsNum; the path would then be
 * made of alternate AS_SET and AS_SEQUENCEs. However, the max number
 * of elements in an AS_SEQUENCE is 255, so you might need to split a
 * sequence into multiple ones.
 *
 * The external representation of AS_SET or AS_SEQUENCE is:
 *   1 byte:	segment type
 *   1 byte:	number of element in the segment
 *   n entries:	the ASnumbers in the segment, 2 or 4 bytes each
 *
 * In terms of internal representation, it might be more efficient
 * useful to store segments as either an AS_SET, or a sequence of size 1,
 * and then do the necessary grouping on output.
 *
 * The current implementation allows both forms.
 *
 * Note that the external representation (provided by encode()) returns
 * a malloc'ed chunk of memory which must be freed by the caller.
 */

// AS Path Values
enum ASPathSegType {
    AS_NONE = 0,	// not initialized
    AS_SET = 1,
    AS_SEQUENCE = 2
};

/**
 * Parent class for AsPath elements, which can be either AsSet or AsSequence.
 */
class AsSegment {
public:
    typedef list <AsNum>::iterator iterator;
    typedef list <AsNum>::const_iterator const_iterator;
    typedef list <AsNum>::const_reverse_iterator const_reverse_iterator;

    /**
     * Constructor of an empty AsSegment
     */
    AsSegment(ASPathSegType t = AS_NONE) : _type(t), _entries(0) {}

    /**
     * constructor from external representation will just decode
     * the chunk of data received, and convert to internal representation.
     * In fact, the external representation is quite simple and effective.
     *
     * _type is d[0], l is d[1], entries follow.
     */
    AsSegment(const uint8_t* d)				{ decode(d); }

    /**
     * Copy constructor
     */
    AsSegment(const AsSegment& a) :
	    _type(a._type), _entries(a._entries), _aslist(a._aslist)	{}

    /**
     * The destructor has nothing to do, the underlying container will
     * take care of the thing.
     */
    ~AsSegment()					{}

    /**
     * reset whatever is currently contained in the object.
     */
    void clear()					{
	_type = AS_NONE;
	_aslist.clear();
	_entries = 0;
    }

    /**
     * @return the path length, which is 1 for an AS_SET,
     * and the length of the sequence for an AS_SEQUENCE
     */
    size_t path_length() const				{
	if (_type == AS_SET)
	    return 1;
	else if (_type == AS_SEQUENCE)
	    return _entries;
	else
	    return 0; // XXX should not be called!
    }

    size_t as_size() const				{ return _entries; }

    /**
     * Add AsNum at the end of the segment (order is irrelevant
     * for AS_SET but important for AS_SEQUENCE)
     * This is used when initializing from a string.
     */
    void add_as(const AsNum& n)				{
	debug_msg("Number of As entries %u\n", (uint32_t)_entries);
	_aslist.push_back(n);
	_entries++;
    }

    /**
     * Add AsNum at the beginning of the segment (order is irrelevant
     * for AS_SET but important for AS_SEQUENCE).
     * This is used e.g. when a node is adding its AsNum to the sequence.
     */
    void prepend_as(const AsNum& n)			{
	debug_msg("Number of As entries %u\n", (uint32_t)_entries);
	_aslist.push_front(n);
	_entries++;
    }

    /**
     * Check if a given AsNum is contained in the segment.
     */
    bool contains(const AsNum& as_num) const		{
	list <AsNum>::const_iterator iter;

	for (iter = _aslist.begin(); iter != _aslist.end(); ++iter)
	    if (*iter == as_num)
		return true;
	return false;
    }

    const AsNum& first_asnum() const;

    /**
     * Convert the external representation into the internal one.
     * _type is d[0], _entries is d[1], entries follow.
     */
    void decode(const uint8_t *d);

    /**
     * Convert from internal to external representation.
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf = 0) const;

    /**
     * @return the length of the list on the wire.
     */
    size_t wire_length() const			{ return 2 + 2*_entries; }

    /**
     * @return string representation of the segment
     */
    string str() const;

    /**
     * compares internal representations for equality.
     */
    bool operator==(const AsSegment& him) const;

    /**
     * Compares internal representations for <.
     */
    bool operator<(const AsSegment& him) const;

    ASPathSegType type() const			{ return _type; }
    void set_type(ASPathSegType t)		{ _type = t; }

    size_t encode_for_mib(uint8_t* buf, size_t buf_size) const;

private:
    ASPathSegType	_type;
    size_t		_entries;	// # of AS numbers in the as path
    list <AsNum>	_aslist;
};

/**
 * An AsPath is a list of AsSegments, each of which can be an AS_SET
 * or an AS_SEQUENCE.
 */
class AsPath {
public:
    typedef list <AsSegment>::const_iterator const_iterator;
    typedef list <AsSegment>::iterator iterator;

    AsPath() : _num_segments(0), _path_len(0)		{}

    /**
     * Initialize from a string in the format
     *		1,2,(3,4,5),6,7,8,(9,10,11),12,13
     */
    AsPath(const char *as_path) throw(InvalidString);

    /**
     * construct from received data
     */
    AsPath(const uint8_t* d, size_t len)		{ decode(d, len); }

    /**
     * Copy constructor
     */
    AsPath(const AsPath &a) : _segments(a._segments), 
	_num_segments(a._num_segments), _path_len(a._path_len) {}

    ~AsPath()						{}

    void add_segment(const AsSegment& s);

    size_t path_length() const				{ return _path_len; }

    bool contains(const AsNum& as_num) const		{
	const_iterator i = _segments.begin();
	for (; i != _segments.end(); ++i)
	    if ((*i).contains(as_num))
		return true;
	return false;
    }


    const AsNum& first_asnum() const			{
	assert(!_segments.empty());
	return _segments.front().first_asnum();
    }

    string str() const;

    const AsSegment& segment(size_t n) const		{
	if (n < _num_segments) {
	    const_iterator iter = _segments.begin();
	    for (u_int i = 0; i<n; i++)
		++iter;
	    return (*iter);
        }
	assert("Segment doesn't exist.\n"); // XXX eh ?
	xorp_throw(InvalidString, "segment invalid n\n");
    }

    size_t num_segments() const			{ return _num_segments; }

    /**
     * Convert from internal to external representation.
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf = 0) const;

    /**
     * @return the length of the list on the wire.
     * XXX this should be made more efficient.
     */
    size_t wire_length() const;

    /**
     * Add the As number to the begining of the AS_SEQUENCE that starts
     * the As path, or if the AsPath starts with an AS_SET, then add a
     * new AS_SEQUENCE with the new AsNum to the start of the AsPath
     */
    void prepend_as(const AsNum &asn);

    bool operator==(const AsPath& him) const;

    bool operator<(const AsPath& him) const;

    void encode_for_mib(vector<uint8_t>& aspath) const;

private:
    /**
     * populate an AsPath from received data. Only used in the constructor.
     */
    void decode(const uint8_t *d, size_t len);

    /**
     * internal representation
     */
    list <AsSegment>	_segments;
    size_t		_num_segments;
    size_t		_path_len;
};

#endif // __BGP_ASPATH_HH__
