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

#ident "$XORP: xorp/bgp/aspath.cc,v 1.20 2003/11/06 03:00:30 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "aspath.hh"
#include "path_attribute.hh"

extern void dump_bytes(const uint8_t *d, uint8_t l);

/* *************** AsSegment ********************** */

/**
 * Convert the external representation into the internal one. 
 * _type is d[0], _entries is d[1], entries follow.
 */
void
AsSegment::decode(const uint8_t *d)
{
    size_t n = d[1];
    clear();
    _type = (ASPathSegType)d[0];
    XLOG_ASSERT(_type == AS_NONE || _type == AS_SET || _type == AS_SEQUENCE);
    // XXX more error checking ?

    d += 2;	// skip header, d points to the raw data now.
    for (size_t i = 0; i < n; d += 2, i++)
	add_as(AsNum(d));
}

/**
 * Convert from internal to external representation.
 */
const uint8_t *
AsSegment::encode(size_t &len, uint8_t *data) const
{
    debug_msg("AsSegment encode\n");
    XLOG_ASSERT(_entries <= 255);
    XLOG_ASSERT(_aslist.size() == _entries);	// XXX this is expensive

    size_t i = wire_size();
    const_iterator as;

    if (data == 0)
	data = new uint8_t[i];
    else
	XLOG_ASSERT(len >= i);
    len = i;

    data[0] = _type;
    data[1] = _entries;

    for (i = 2, as = _aslist.begin(); as != _aslist.end(); i += 2, ++as) {
	debug_msg("Encoding 16-bit As %d\n", as->as());
	as->copy_out(data + i);
    }

    return data;
}

const AsNum&
AsSegment::first_asnum() const
{
    if (_type == AS_SET) {
	// This shouldn't be possible.  The spec doesn't explicitly
	// prohibit passing someone an AS_PATH starting with an AS_SET,
	// but it doesn't make sense, and doesn't seem to be allowed by
	// the aggregation rules.
	XLOG_ERROR("Attempting to extract first AS Number "
		    "from an AS Path that starts with an AS_SET "
		    "not an AS_SEQUENCE\n"); 
    }
    XLOG_ASSERT(!_aslist.empty());
    return _aslist.front();
}

string
AsSegment::str() const
{
    string s;
    string sep = (_type == AS_SET) ? "{": "[";	// separator
    const_iterator iter = _aslist.begin();

    for (u_int i = 0; i<_entries; i++, ++iter) {
	s += sep ;
	s += iter->str();
	sep = ", ";
    }
    s += (_type == AS_SET) ? "}": "]";
    return s;
}

string
AsSegment::short_str() const
{
    string s;
    string sep = (_type == AS_SET) ? "{": "";	// separator
    const_iterator iter = _aslist.begin();

    for (u_int i = 0; i<_entries; i++, ++iter) {
	s += sep ;
	s += iter->short_str();
	sep = " ";
    }
    s += (_type == AS_SET) ? "}": "";
    return s;
}

/**
 * compares internal representations for equality.
 */
bool
AsSegment::operator==(const AsSegment& him) const
{
    if (_entries != him._entries)
	return false;
    //We use reverse iterators because the aspaths we receive have
    //more in common in the ASs near us than in the ASs further away.
    //Much of the time we can detect disimilarity in the first AS if
    //we start at the end.
    const_reverse_iterator my_i = _aslist.rbegin();
    const_reverse_iterator his_i = him._aslist.rbegin();
    for (;my_i != _aslist.rend(); my_i++, his_i++)
	if (*my_i != *his_i)
	    return false;
    return true;
}

/**
 * Compares internal representations for <.
 */
bool
AsSegment::operator<(const AsSegment& him) const
{
    int mysize = _entries;
    int hissize = him._entries;
    if (mysize < hissize)
	return true;
    if (mysize > hissize)
	return false;
    const_reverse_iterator my_i = _aslist.rbegin();
    const_reverse_iterator his_i = him._aslist.rbegin();
    for (; my_i != _aslist.rend(); my_i++, his_i++) {
	if (*my_i < *his_i)
	    return true;
	if (*his_i < *my_i)
	    return false;
    }
    return false;
}

// XXX isn't this the same format as on the wire ???

size_t
AsSegment::encode_for_mib(uint8_t* buf, size_t buf_size) const
{
    //See RFC 1657, Page 15 for the encoding
    XLOG_ASSERT(buf_size >= (2 + _entries * 2));
    uint8_t *p = buf;
    *p = (uint8_t)_type;  p++;
    *p = (uint8_t)_entries; p++;
    const_iterator i;
    for (i = _aslist.begin(); i!= _aslist.end(); i++, p += 2)
	i->copy_out(p);

    return (2 + _entries * 2);
}


/* *************** AsPath *********************** */

/**
 * constructor (parsing strings of the form below)
 *
 * "1, 2,(3, 4, 5), 6,(7, 8), 9"
 */
AsPath::AsPath(const char *as_path) throw(InvalidString)
{
    debug_msg("AsPath(%s) constructor called\n", as_path);
    _num_segments = 0;
    _path_len = 0;

    // make a copy removing all spaces from the string.

    string path = as_path;
    size_t pos;
    while(string::npos != (pos = path.find(" "))) {
	path = path.substr(0, pos) + path.substr(pos + 1);
    }

    AsSegment seg;
    for (size_t i = 0; i < path.length(); i++) {
	char c = path[i];

        debug_msg("check <%c> at pos %u\n", c, (uint32_t)i);
	if (xorp_isdigit(c)) {
	    size_t start = i;

	    if (seg.type() == AS_NONE)	// possibly start new segment
		seg.set_type(AS_SEQUENCE);

	    while (i < path.length() && xorp_isdigit(path[i]))
		i++;
        
	    uint16_t num = atoi(path.substr(start, i).c_str());
	    i--; // back to last valid character
	    debug_msg("asnum = %d\n", num);
	    seg.add_as(AsNum(num));
	} else if (c == '(') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else if (seg.type() == AS_SET) // nested, invalid
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	    seg.set_type(AS_SET);
	} else if (c == ')') {
	    if (seg.type() == AS_SET) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	} else if (c == ',') {
	} else if (c == ' ' || c == '\t') {
	} else
	    xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
    }
    if (seg.type() == AS_SEQUENCE)	// close existing seg.
	add_segment(seg);
    else if (seg.type() == AS_SET)
	xorp_throw(InvalidString,
		       c_format("Unterminated AsSet: %s", path.c_str()));
    debug_msg("end of AsPath()\n");
}

/**
 * populate an AsPath from the received data representation
 */
void
AsPath::decode(const uint8_t *d, size_t l)
{
    _num_segments = 0;
    _path_len = 0;
    while (l > 0) {		// grab segments
	size_t len = 2 + d[1]*2;	// XXX length in bytes for 16bit AS's
	XLOG_ASSERT(len <= l);

	AsSegment s(d);
	add_segment(s);
	d += len;
	l -= len;
    }
}

void
AsPath::add_segment(const AsSegment& s)
{
    debug_msg("Adding As Segment\n");
    _segments.push_back(s);
    _num_segments++;

    size_t n = s.path_length();
    _path_len += n;
    debug_msg("End of add_segment, As Path length by %u to be %u\n",
                (uint32_t)n, (uint32_t)_path_len);
}

string
AsPath::str() const
{
    string s = "AsPath:";
    const_iterator iter = _segments.begin();
    while(iter != _segments.end()) {
	s.append(" ");
	s.append((*iter).str());
	++iter;
    }
    return s;
}

string
AsPath::short_str() const
{
    string s;
    const_iterator iter = _segments.begin();
    while(iter != _segments.end()) {
	s.append(" ");
	s.append((*iter).short_str());
	++iter;
    }
    return s;
}

size_t
AsPath::wire_size() const
{
    size_t l = 0;
    const_iterator iter = _segments.begin();

    for (; iter != _segments.end(); ++iter)
	l += iter->wire_size();
    return l;
}

const uint8_t *
AsPath::encode(size_t &len, uint8_t *buf) const
{
    XLOG_ASSERT(_num_segments == _segments.size());	// XXX expensive
    const_iterator i;
    size_t pos, l = wire_size();

    // allocate or check the memory.
    if (buf == 0)		// no buffer, allocate one
	buf = new uint8_t[l];
    else			// we got a buffer, make sure is large enough.
	XLOG_ASSERT(len >= l);	// in fact, just abort if not so.
    len = l;			// set the correct value.

    // encode into the buffer
    for (pos = 0, i = _segments.begin(); i != _segments.end(); ++i) {
	l = i->wire_size();
	i->encode(l, buf + pos);
	pos += l;
    }
    return buf;
}

void
AsPath::prepend_as(const AsNum &asn)
{
    if (_segments.empty() || _segments.front().type() == AS_SET) {
	AsSegment seg = AsSegment(AS_SEQUENCE);

	seg.add_as(asn);
	_segments.push_front(seg);
	_num_segments++;
    } else {
	XLOG_ASSERT(_segments.front().type() == AS_SEQUENCE);
	_segments.front().prepend_as(asn);
    }
    _path_len++;	// in both cases the length increases by one.
}

bool
AsPath::operator==(const AsPath& him) const
{
    if (_num_segments != him._num_segments) 
	return false;
    const_iterator my_i = _segments.begin();
    const_iterator his_i = him._segments.begin();
    for (;my_i != _segments.end(); my_i++, his_i++)
	if (*my_i != *his_i)
	    return false;
    return true;
}

bool
AsPath::operator<(const AsPath& him) const
{
    if (_num_segments < him._num_segments)
	return true;
    if (_num_segments > him._num_segments)
	return false;
    const_iterator my_i = _segments.begin();
    const_iterator his_i = him._segments.begin();
    for (;my_i != _segments.end(); my_i++, his_i++) {
	if (*my_i < *his_i)
	    return true;
	if (*his_i < *my_i)
	    return false;
    }
    return false;
}
void
AsPath::encode_for_mib(vector<uint8_t>& encode_buf) const
{
    //See RFC 1657, Page 15 for the encoding.
    size_t buf_size = wire_size();
    if (buf_size > 2)
	encode_buf.resize(buf_size);
    else {
	//The AS path was empty - this can be the case if the route
	//originated in our AS.
	//XXX The MIB doesn't say how to encode this, but it says the
	//vector must be at least two bytes in size.
	encode_buf.resize(2);
	encode_buf[0]=0;
	encode_buf[1]=0;
	return;
    }

    int ctr = 0;
    const_iterator i = _segments.begin();
    for(i = _segments.begin(); i != _segments.end(); i++) {
	ctr += i->encode_for_mib(&encode_buf[ctr], buf_size - ctr);
    }
}
