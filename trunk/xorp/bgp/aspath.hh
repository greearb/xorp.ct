// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/aspath.hh,v 1.32 2008/07/23 05:09:31 pavlin Exp $

#ifndef __BGP_ASPATH_HH__
#define __BGP_ASPATH_HH__

#include <sys/types.h>
#include <inttypes.h>
#include <string>
#include <list>
#include <vector>

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/asnum.hh"
#include "libxorp/exceptions.hh"
#include "exceptions.hh"

class AS4Path;

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
 *
 * RFC 3065 (Autonomous System Confederations for BGP) introduced two
 * additional segment types:
 *
 * AS_CONFED_SEQUENCE: ordered set of Member AS Numbers
 * in the local confederation that the UPDATE message has traversed
 *
 * AS_CONFED_SET: unordered set of Member AS Numbers in
 * the local confederation that the UPDATE message has traversed
 *  
 *
 * 4-byte AS numbers need to be handled carefully.  There are two main
 * cases to consider: when we are configured to do 4-byte AS numbers,
 * and when we are not.
 *
 * When we are not configured to do 4-byte AS numbers all AS path
 * information received from peers will be 2-byte AS numbers.  If
 * they're configured to do 4-byte AS numbers, they will send us an
 * AS_PATH and an AS4_PATH, but if we're not configured to do 4-byte
 * AS numbers, we shouldn't care.  We don't merge the two; we
 * propagate AS4_PATH unchanged, and do the normal stuff with the
 * regular AS_PATH.  The AS4_PATH is actually stored internally in an
 * UnknownAttribute, as we don't need to know anything about it.
 *
 * When we are configured to do 4-byte AS numbers, we can tell from
 * the negotiated capabilities whether a peer is sending 4-byte AS
 * numbers or 2-byte AS numbers.  If a peer is sending 4-byte AS
 * numbers, these arrive in the AS_PATH attribute, and we store them
 * internally in an AS4Path, but stored in the ASPathAttribute, NOT in
 * the AS4PathAttribute.  If a peer is sending 2-byte AS numbers in
 * AS_PATH, he may also propagate an AS4_PATH.  We merge these on
 * input, cross-validate the two, and store the results just like we
 * would if we'd received a 4-byte AS_PATH.  So, whatever our peer
 * does, the only place we have AS path information that we use for
 * the decision process is in the ASPathAttribute.
 *
 * If we're doing 4-byte AS numbers and our peer is doing 4-byte AS
 * numbers, then all routes we send him will contain 4-byte AS_PATH
 * attributes.  We just need to make sure we use the right encode()
 * method.  If our peer is not doing 4-byte AS numbers, then we send a
 * 2-byte AS_PATH, substituting AS_TRANS for any AS numbers that can't
 * be represented in 2 bytes.  We also construct an AS4_PATH from our
 * ASPathAttribute, and send that too.
 */

// AS Path Values
enum ASPathSegType {
    AS_NONE = 0,	// not initialized
    AS_SET = 1,
    AS_SEQUENCE = 2,
    AS_CONFED_SEQUENCE = 3,
    AS_CONFED_SET = 4
};

/**
 * Parent class for ASPath elements, which can be either ASSet or ASSequence.
 */
class ASSegment {
public:
    typedef list<AsNum> ASLIST;
    typedef ASLIST::iterator iterator;
    typedef ASLIST::const_iterator const_iterator;
    typedef ASLIST::const_reverse_iterator const_reverse_iterator;

    /**
     * Constructor of an empty ASSegment
     */
    ASSegment(ASPathSegType t = AS_NONE) : _type(t)	{
    }

    /**
     * constructor from external representation will just decode
     * the chunk of data received, and convert to internal representation.
     * In fact, the external representation is quite simple and effective.
     *
     * _type is d[0], l is d[1], entries follow.
     */
    ASSegment(const uint8_t* d) throw(CorruptMessage)	{
	decode(d);
    }

    /**
     * Copy constructor
     */
    ASSegment(const ASSegment& a) :
	_type(a._type), _aslist(a._aslist)		{}

    /**
     * The destructor has nothing to do, the underlying container will
     * take care of the thing.
     */
    ~ASSegment()					{}

    /**
     * reset whatever is currently contained in the object.
     */
    void clear()					{
	_type = AS_NONE;
	_aslist.clear();
    }

    /**
     * @return the path length, which is 1 for an AS_SET,
     * and the length of the sequence for an AS_SEQUENCE
     */
    size_t path_length() const				{
	if (_type == AS_SET || _type == AS_CONFED_SET)
	    return 1;
	else if (_type == AS_SEQUENCE || _type == AS_CONFED_SEQUENCE)
	    return _aslist.size();
	else
	    return 0; // XXX should not be called!
    }

    size_t as_size() const				{
	return _aslist.size();
    }

    /**
     * Add AsNum at the end of the segment (order is irrelevant
     * for AS_SET but important for AS_SEQUENCE)
     * This is used when initializing from a string.
     */
    void add_as(const AsNum& n)				{
	debug_msg("Number of As entries %u\n", XORP_UINT_CAST(_aslist.size()));
	_aslist.push_back(n);
    }

    /**
     * Add AsNum at the beginning of the segment (order is irrelevant
     * for AS_SET but important for AS_SEQUENCE).
     * This is used e.g. when a node is adding its AsNum to the sequence.
     */
    void prepend_as(const AsNum& n)			{
	debug_msg("Number of As entries %u\n", XORP_UINT_CAST(_aslist.size()));
	_aslist.push_front(n);
    }

    /**
     * Check if a given AsNum is contained in the segment.
     */
    bool contains(const AsNum& as_num) const		{
	const_iterator iter;

	for (iter = _aslist.begin(); iter != _aslist.end(); ++iter)
	    if (*iter == as_num)
		return true;
	return false;
    }

    const AsNum& first_asnum() const;

    /**
     * find the n'th AS number in the segment 
     */
    const AsNum& as_num(int n) const			{
	const_iterator i = _aslist.begin();

	while (n--)
	    i++;

	return *i;
    }

    /**
     * Convert the external representation into the internal one.
     * _type is d[0], _entries is d[1], entries follow.
     */
    void decode(const uint8_t *d) throw(CorruptMessage);

    /**
     * Convert from internal to external representation.
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf) const;

    /**
     * @return the size of the list on the wire.
     */
    size_t wire_size() const			{
	return 2 + 2 * _aslist.size();
    }

    /**
     * @return fancy string representation of the segment
     */
    string str() const;

    /**
     * @return compact string representation of the segment
     */
    string short_str() const;

    /**
     * compares internal representations for equality.
     */
    bool operator==(const ASSegment& him) const;

    /**
     * Compares internal representations for <.
     */
    bool operator<(const ASSegment& him) const;

    ASPathSegType type() const			{ return _type; }
    void set_type(ASPathSegType t)		{ _type = t; }

    size_t encode_for_mib(uint8_t* buf, size_t buf_size) const;

    /**
     * returns true if the AS segment does not lose information when
     * represented entirely as two-byte AS numbers 
     */
    bool two_byte_compatible() const;

protected:
    ASPathSegType	_type;
    ASLIST		_aslist;
};


/* subsclass of ASSegment to handle encoding and decoding of 4-byte AS
   numbers from a AS4_PATH attribute */
class AS4Segment : public ASSegment {
public:
    AS4Segment(const uint8_t* d) throw(CorruptMessage) { decode(d); }
    /**
     * Convert the external representation into the internal one.
     * _type is d[0], _entries is d[1], entries follow.
     */
    void decode(const uint8_t *d) throw(CorruptMessage);

    /**
     * Convert from internal to external representation.
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf) const;

    /**
     * @return the size of the list on the wire.
     */
    size_t wire_size() const			{
	return 2 + 4 * _aslist.size();
    }
private:
    /* no storage, as this is handled by the underlying ASSegment */
};

/**
 * An ASPath is a list of ASSegments, each of which can be an AS_SET,
 * AS_CONFED_SET, AS_SEQUENCE, or an AS_CONFED_SEQUENCE.
 */
class ASPath {
public:
    typedef list <ASSegment>::const_iterator const_iterator;
    typedef list <ASSegment>::iterator iterator;

    ASPath() : _num_segments(0), _path_len(0)		{}

    /**
     * Initialize from a string in the format
     *		1,2,(3,4,5),6,7,8,(9,10,11),12,13
     */
    ASPath(const char *as_path) throw(InvalidString);

    /**
     * construct from received data
     */
    ASPath(const uint8_t* d, size_t len) throw(CorruptMessage) {
	decode(d, len); 
    }

    /**
     * construct an aggregate from two ASPaths
     */
    ASPath(const ASPath &asp1, const ASPath &asp2);

    /**
     * Copy constructor
     */
    ASPath(const ASPath &a) : _segments(a._segments), 
	_num_segments(a._num_segments), _path_len(a._path_len) {}

    ~ASPath()						{}

    void add_segment(const ASSegment& s);
    void prepend_segment(const ASSegment& s);

    size_t path_length() const				{ return _path_len; }

    bool contains(const AsNum& as_num) const		{
	const_iterator i = _segments.begin();
	for (; i != _segments.end(); ++i)
	    if ((*i).contains(as_num))
		return true;
	return false;
    }

    const AsNum& first_asnum() const			{
	XLOG_ASSERT(!_segments.empty());
	return _segments.front().first_asnum();
    }

    string str() const;
    string short_str() const;

    const ASSegment& segment(size_t n) const		{
	if (n < _num_segments) {
	    const_iterator iter = _segments.begin();
	    for (u_int i = 0; i<n; i++)
		++iter;
	    return (*iter);
        }
	XLOG_FATAL("Segment %u doesn't exist.", (uint32_t)n);
	xorp_throw(InvalidString, "segment invalid n\n");
    }

    size_t num_segments() const			{ return _num_segments; }

    /**
     * Convert from internal to external representation, with the
     * correct representation for the original AS_PATH attribute.  If
     * there are any 4-byte AS numbers, they will be encoded as
     * AS_TRAN.
     *
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf) const;

    /**
     * @return the size of the list on the wire.
     * XXX this should be made more efficient.
     */
    size_t wire_size() const;

    /**
     * Add the As number to the begining of the AS_SEQUENCE that starts
     * the As path, or if the ASPath starts with an AS_SET, then add a
     * new AS_SEQUENCE with the new AsNum to the start of the ASPath
     */
    void prepend_as(const AsNum &asn);

    /**
     * Add the As number to the begining of the AS_CONFED_SEQUENCE
     * that starts the As path, or if the ASPath does not start with
     * an AS_CONFED_SEQUENCE, then add a new AS_CONFED_SEQUENCE with
     * the new AsNum to the start of the ASPath
     */
    void prepend_confed_as(const AsNum &asn);

    /**
     * remove all confederation segments from aspath 
     */
    void remove_confed_segments();

    /**
     * @return true if the AS_PATH Contains confederation segments.
     */
    bool contains_confed_segments() const;

    ASPath& operator=(const ASPath& him);

    bool operator==(const ASPath& him) const;

    bool operator<(const ASPath& him) const;

    void encode_for_mib(vector<uint8_t>& aspath) const;

    /**
     * returns true if the AS path does not lose information when
     * represented entirely as two-byte AS numbers */
    bool two_byte_compatible() const;

    /**
     * Merge an AS4Path into a 2-byte AS Path.  Both paths will end up
     * containing the same data
     *
     * param as4path the AS4_PATH to be merged.  
     */
    void merge_as4_path(AS4Path& as4_path);

protected:
    /**
     * internal representation
     */
    list <ASSegment>	_segments;
    size_t		_num_segments;
    size_t		_path_len;

private:
    /**
     * populate an ASPath from received data. Only used in the constructor.
     */
    void decode(const uint8_t *d, size_t len) throw(CorruptMessage);
};

/* subclass to handle 4-byte AS encoding and decoding */
class AS4Path : public ASPath {
public:
    /**
     * Construct from received data from 4-byte peer.
     */
    AS4Path(const uint8_t* d, size_t len) throw(CorruptMessage);

    /**
     * Initialize from a string in the format
     *		3.1,2,(3,10.4,5),6,7,8,(9,10,11),12,13
     */
    AS4Path(const char *as_path) throw(InvalidString) 
	: ASPath(as_path)
    {};

#if 0
    /**
     * Construct from received data from 2-byte peer.  This needs to
     * take the regular ASPath in addition to the AS4_PATH data,
     * because it needs to cross-validate the two.
     */
    AS4Path(const uint8_t* d, size_t len, const ASPath& as_path)
	throw(CorruptMessage);

#endif

    /**
     * Convert from internal to external representation, with the
     * correct representation for the original AS4_PATH attribute.
     *
     * If we do not pass a buffer (buf = 0), then the routine will
     * allocate a new one; otherwise, len indicates the size of the
     * input buffer, which must be large enough to store the encoding.
     * @return the pointer to the buffer, len is the actual size.
     */
    const uint8_t *encode(size_t &len, uint8_t *buf) const;

    size_t wire_size() const;

    void cross_validate(const ASPath& as_path);

private:
    /**
     * populate an ASPath from received data. Only used in the constructor.
     */
    void decode(const uint8_t *d, size_t len) throw(CorruptMessage);
    void pad_segment(const ASSegment& old_seg, ASSegment& new_seg);
    void do_patchup(const ASPath& as_path);
};

#endif // __BGP_ASPATH_HH__
