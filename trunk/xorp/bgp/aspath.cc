// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/aspath.cc,v 1.29 2005/11/15 11:43:57 mjh Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <string.h>
#include <stdio.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "aspath.hh"
#include "path_attribute.hh"
#include "packet.hh"

extern void dump_bytes(const uint8_t *d, uint8_t l);

/* *************** AsSegment ********************** */

/**
 * Convert the external representation into the internal one. 
 * _type is d[0], _entries is d[1], entries follow.
 */
void
AsSegment::decode(const uint8_t *d) throw(CorruptMessage)
{
    size_t n = d[1];
    clear();
    _type = (ASPathSegType)d[0];
    switch(_type) {
    case AS_NONE:
    case AS_SET:
    case AS_SEQUENCE:
    case AS_CONFED_SET:
    case AS_CONFED_SEQUENCE:
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Bad AS Segment type: %u\n", _type),
		   UPDATEMSGERR, MALASPATH);
    }

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
    if (_type == AS_SET || _type == AS_CONFED_SET) {
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

/**
 * prints AsSegments as 
 *
 *  AS_SEQUENCE:         [comma-separated-asn-list]  
 *  AS_SET:              {comma-separated-asn-list}
 *  AS_CONFED_SEQUENCE:  (comma-separated-asn-list)
 *  AS_CONFED_SET:       <comma-separated-asn-list> 
 */
string
AsSegment::str() const
{
    string s;
    string sep; 
    switch(_type) {
    case AS_NONE: 
	break; 
    case AS_SET: 
	sep = "{"; 
	break; 
    case AS_SEQUENCE: 
	sep = "[";  
	break; 
    case AS_CONFED_SEQUENCE: 
	sep = "(";  
	break; 
    case AS_CONFED_SET: 
	sep = "<";  
	break; 
    }
    const_iterator iter = _aslist.begin();

    for (u_int i = 0; i<_entries; i++, ++iter) {
	s += sep ;
	s += iter->str();
	sep = ", ";
    }
    switch(_type) {
    case AS_NONE: 
	break; 
    case AS_SET: 
	sep = "}"; 
	break; 
    case AS_SEQUENCE: 
	sep = "]";  
	break; 
    case AS_CONFED_SEQUENCE: 
	sep = ")";  
	break; 
    case AS_CONFED_SET: 
	sep = ">";  
	break; 
    }
    s += sep; 
    return s;
}

/**
 * prints AsSegments as 
 *
 *  AS_SEQUENCE:         comma-separated-asn-list
 *  AS_SET:              {comma-separated-asn-list}
 *  AS_CONFED_SEQUENCE:  (comma-separated-asn-list)
 *  AS_CONFED_SET:       <comma-separated-asn-list> 
 */
string
AsSegment::short_str() const
{
    string s;
    string sep; 
    switch(_type) {
    case AS_NONE: 
	break; 
    case AS_SET: 
	sep = "{"; 
	break; 
    case AS_SEQUENCE: 
	sep = "";  
	break; 
    case AS_CONFED_SEQUENCE: 
	sep = "(";  
	break; 
    case AS_CONFED_SET: 
	sep = "<";  
	break; 
    }
    const_iterator iter = _aslist.begin();

    for (u_int i = 0; i<_entries; i++, ++iter) {
	s += sep ;
	s += iter->short_str();
	sep = " ";
    }
    switch(_type) {
    case AS_NONE: 
	break; 
    case AS_SET: 
	sep = "}"; 
	break; 
    case AS_SEQUENCE: 
	sep = "";  
	break; 
    case AS_CONFED_SEQUENCE: 
	sep = ")";  
	break; 
    case AS_CONFED_SET: 
	sep = ">";  
	break; 
    }
    s += sep; 

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

bool
AsSegment::two_byte_compatible() const
{
    const_iterator i;
    for (i = _aslist.begin(); i != _aslist.end(); i++) {
	if (i->extended())
	    return false;
    }
    return true;
}

/* *************** NewAsSegment ***************** */


/**
 * Convert the external representation into the internal one. This is
 * used when decoding a NEW_AS_PATH attribute.
 *
 * _type is d[0], _entries is d[1], entries follow.
 */
void
NewAsSegment::decode(const uint8_t *d) throw(CorruptMessage)
{
    size_t n = d[1];
    clear();
    _type = (ASPathSegType)d[0];

    switch(_type) {
    case AS_NONE:
    case AS_SET:
    case AS_SEQUENCE:
    case AS_CONFED_SET:
    case AS_CONFED_SEQUENCE:
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Bad AS Segment type: %u\n", _type),
		   UPDATEMSGERR, MALASPATH);
    }

    d += 2;	// skip header, d points to the raw data now.
    for (size_t i = 0; i < n; d += 4, i++) {
	/* copy to avoid alignment issues, and force use of 
	   correct constructor */
	uint32_t as_num;
        memcpy(&as_num, d, 4);
	as_num = htonl(as_num);
	add_as(AsNum(as_num));
    }
}

/**
 * Convert from internal to external NEW_AS_PATH representation.
 */
const uint8_t *
NewAsSegment::encode(size_t &len, uint8_t *data) const
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

    for (i = 2, as = _aslist.begin(); as != _aslist.end(); i += 4, ++as) {
	debug_msg("Encoding 32-bit As %d\n", as->as());
	uint32_t as_num = htonl(as->as32());
	memcpy(data + i, &as_num, 4);
    }

    return data;
}


/* *************** AsPath *********************** */

/**
 * AsPath constructor by parsing strings.  Input strings 
 * should have the form 
 * 
 *         "segment, segment, segment, ... ,segment"  
 * 
 * where segments are parsed as 
 *
 *  AS_SEQUENCE:         [comma-separated-asn-list]  or comma-separated-asn-list
 *  AS_SET:              {comma-separated-asn-list}
 *  AS_CONFED_SEQUENCE:  (comma-separated-asn-list)
 *  AS_CONFED_SET:       <comma-separated-asn-list> 
 * 
 *  blank spaces " " can appear at any point in the string. 
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

        debug_msg("check <%c> at pos %u\n", c, XORP_UINT_CAST(i));
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
	} else if (c == '[') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else if (seg.type() != AS_NONE) // nested, invalid
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	    seg.set_type(AS_SEQUENCE);
	} else if (c == ']') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	} else if (c == '{') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else if (seg.type() != AS_NONE) // nested, invalid
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	    seg.set_type(AS_SET);
	} else if (c == '}') {
	    if (seg.type() == AS_SET) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	} else if (c == '(') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else if (seg.type() != AS_NONE) // nested, invalid
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	    seg.set_type(AS_CONFED_SEQUENCE);
	} else if (c == ')') {
	    if (seg.type() == AS_CONFED_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	} else if (c == '<') {
	    if (seg.type() == AS_SEQUENCE) {
		// push previous thing and start a new one
		add_segment(seg);
		seg.clear();
	    } else if (seg.type() != AS_NONE) // nested, invalid
		xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				c, path.c_str()));
	    seg.set_type(AS_CONFED_SET);
	} else if (c == '>') {
	    if (seg.type() == AS_CONFED_SET) {
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
AsPath::decode(const uint8_t *d, size_t l) throw(CorruptMessage)
{
    _num_segments = 0;
    _path_len = 0;
    while (l > 0) {		// grab segments
	size_t len = 2 + d[1]*2;	// XXX length in bytes for 16bit AS's
	if (len > l)
	    xorp_throw(CorruptMessage,
		       c_format("Bad ASpath (len) %u > (l) %u\n", len, l),
		       UPDATEMSGERR, MALASPATH);

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
	      XORP_UINT_CAST(n), XORP_UINT_CAST(_path_len));
}

void
AsPath::prepend_segment(const AsSegment& s)
{
    debug_msg("Prepending As Segment\n");
    _segments.push_front(s);
    _num_segments++;

    size_t n = s.path_length();
    _path_len += n;
    debug_msg("End of add_segment, As Path length by %u to be %u\n",
	      XORP_UINT_CAST(n), XORP_UINT_CAST(_path_len));
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

void
AsPath::prepend_confed_as(const AsNum &asn)
{
    if (_segments.empty() || _segments.front().type() == AS_SET 
	|| _segments.front().type() == AS_SEQUENCE) {
	AsSegment seg = AsSegment(AS_CONFED_SEQUENCE);

	seg.add_as(asn);
	_segments.push_front(seg);
	_num_segments++;
    } else {
	XLOG_ASSERT(_segments.front().type() == AS_CONFED_SEQUENCE);
	_segments.front().prepend_as(asn);
    }
    _path_len++;	// in both cases the length increases by one.
}

void
AsPath::remove_confed_segments()
{
        debug_msg("Deleting all CONFED Segments\n");
	const_iterator iter = _segments.begin();
	while(iter != _segments.end()) {
	    if ((*iter).type() == AS_CONFED_SEQUENCE 
		|| (*iter).type() == AS_CONFED_SET) {
		_path_len--;
		_num_segments--;
		_segments.remove((*iter)); 
	    } 
	    ++iter;
	}
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

bool
AsPath::two_byte_compatible() const
{
    const_iterator i = _segments.begin();
    for(i = _segments.begin(); i != _segments.end(); i++) {
	if ( !(i->two_byte_compatible()) )
	    return false;
    }
    return true;
}


NewAsPath::NewAsPath(const uint8_t* d, size_t len, const AsPath& as_path)
     throw(CorruptMessage)
{
    decode(d, len);
    cross_validate(as_path);
}

/**
 * populate a AsPath from the received data representation from a
 * NEW_AS_PATH attribute.
 */
void
NewAsPath::decode(const uint8_t *d, size_t l) throw(CorruptMessage)
{
    _num_segments = 0;
    _path_len = 0;
    while (l > 0) {		// grab segments
	size_t len = 2 + d[1]*4;	// XXX length in bytes for 32bit AS's
	XLOG_ASSERT(len <= l);

	NewAsSegment s(d);
	add_segment(s);
	d += len;
	l -= len;
    }
}

/**
 * the AS_PATH attribute may have had new ASes added since the
 * NEW_AS_PATH was last updated.  We need to copy across anything that
 * is missing 
 */

void NewAsPath::cross_validate(const AsPath& as_path)
{
    if (as_path.path_length() < path_length() ) {
	// This is illegal.  The spec says to ignore the NEW_AS_PATH
	// attribute and use the data from the AS_PATH attribute throw
	// away the data we had.
	while (!_segments.empty()) {
	    _segments.pop_front();
	}
	// copy in from the AS_PATH version 
	for (uint32_t i = 0; i < as_path.path_length(); i++) {
	    add_segment(as_path.segment(i));
	}
	return;
    }

    // The AS_PATH attribute may contain ASes that were added by an
    // old BGP speaker that are missing from the NEW_AS_PATH attribute.

    if (as_path.path_length() > path_length()) {
	if (as_path.num_segments() < num_segments()) {
	    // Now we're in trouble!  The AS_PATH has more ASes but
	    // fewer segments than the NEW_AS_PATH.
	    do_patchup(as_path);
	    return;
	}

	// The AS_PATH has at least as many segments as the
	// NEW_AS_PATH find where they differ, and copy across.
	for (uint32_t i = 1; i <= num_segments(); i++) {
	    const AsSegment *old_seg;
	    AsSegment *new_seg;
	    old_seg = &(as_path.segment(as_path.num_segments() - i));
	    new_seg = const_cast<AsSegment*>(&(segment(num_segments() - i)));
	    if (old_seg->path_length() == new_seg->path_length()) 
		continue;
	    if (old_seg->path_length() < new_seg->path_length()) {
		// ooops - WTF happened here
		do_patchup(as_path);
	    }
	    if (old_seg->path_length() > new_seg->path_length()) {
		// found a segment that needs data copying over
		pad_segment(*old_seg, *new_seg);
	    }
	}
	
	if (as_path.path_length() == path_length()) {
	    return;
	}
	
	XLOG_ASSERT(as_path.num_segments() > num_segments());
	// There are more segments, so copy across whole segments
	for (int i = as_path.num_segments() - num_segments() - 1; 
	     i >= 0; i--) {
	    prepend_segment(as_path.segment(i));
	}

	XLOG_ASSERT(as_path.path_length() == path_length());
	return;
    }
}

void NewAsPath::pad_segment(const AsSegment& old_seg, AsSegment& new_seg) 
{
    if (new_seg.type() == AS_SET) {
	// find everything in the old seg that's not in the new seg,
	// and add it.
	for (int i = old_seg.as_size()-1; i >= 0; i--) {
	    const AsNum *asn = &(old_seg.as_num(i));
	    if (asn->as() != AsNum::AS_TRAN) {
		if (!new_seg.contains(*asn)) {
		    new_seg.prepend_as(*asn);
		}
	    }
	}
	// If that wasn't enough, do arbitrary padding to match size
	while (old_seg.as_size() < new_seg.as_size()) {
	    new_seg.prepend_as(new_seg.first_asnum());
	}
	return;
    }

    if (old_seg.type() == AS_SET) {
	// The old segment was an AS_SET, but the new isn't.  Looks
	// like some old BGP speaker did some form of aggregation.
	// Convert the new one to an AS_SET too.
	new_seg.set_type(AS_SET);
	pad_segment(old_seg, new_seg);
	return;
    }

    // They're both AS_SEQUENCES, so we need to preserve sequence.
    // First validate that new_seg is a sub-sequence of old_seg
    for (uint32_t i = 1; i <= new_seg.as_size(); i++) {
	const AsNum *old_asn = &(old_seg.as_num(old_seg.as_size()-i));
	const AsNum *new_asn = &(new_seg.as_num(new_seg.as_size()-i));
	if (*old_asn != *new_asn) {
	    // We've got a mismatch - make it into an AS_SET.  This is
	    // a pretty arbitrary decision, but this shouldn't happen
	    // in reality.  At least it should be safe.
	    new_seg.set_type(AS_SET);
	    pad_segment(old_seg, new_seg);
	    return;
	}
    }

    // now we can simply copy across the missing AS numbers
    for (int i = old_seg.as_size() - new_seg.as_size() - 1; i >= 0; i--) {
	new_seg.prepend_as(old_seg.as_num(i));
    }
    return;
}

void NewAsPath::do_patchup(const AsPath& as_path)
{
    // we get here when the cross validation fails, and we need to do
    // something that isn't actually illegal.

    // There's no good way to deal with this, but simplest is to pad
    // the NEW_AS_PATH with an AS_SET containing anything that wasn't
    // previously in the NEW_AS_PATH.  This at least should prevent
    // loops forming, but it's really ugly.

    AsSegment new_set(AS_SET);
    for (uint32_t i = 0; i < as_path.path_length(); i++) {
	const AsSegment *s = &(as_path.segment(i));
	for (uint32_t j = 0; j < s->path_length(); j++) {
	    const AsNum *asn = &(s->as_num(j));
	    if (asn->as() == AsNum::AS_TRAN)
		continue;
	    if (!contains(*asn)) {
		new_set.add_as(*asn);
		if (new_set.path_length() + path_length() 
		    == as_path.path_length()) {
		    // we've got enough now
		    i = as_path.path_length(); break;
		}
	    }
	}
    }
    // All this work, and we've still not got enough ASes in the AS
    // path. Artificially inflate it.
    if (_segments.front().type() == AS_SET) {
	// need to merge two sets
	for (uint32_t i = 0; i < new_set.path_length(); i++) {
	    _segments.front().add_as(new_set.as_num(i));
	}
    } else {
	prepend_segment(new_set);
    }

    // if we still haven't enough, then just pad
    while (as_path.path_length() > path_length()) {
	prepend_as(first_asnum());
    }
    return;
}

const uint8_t *
NewAsPath::encode(size_t &len, uint8_t *buf) const
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
	const NewAsSegment *new_seg = (const NewAsSegment*)(&(*i));
	l = new_seg->wire_size();
	new_seg->encode(l, buf + pos);
	pos += l;
    }
    return buf;
}


size_t
NewAsPath::wire_size() const
{
    size_t l = 0;
    const_iterator i;

    for (i = _segments.begin(); i != _segments.end(); ++i) {
	const NewAsSegment *new_seg = (const NewAsSegment*)(&(*i));
	l += new_seg->wire_size();
    }
    return l;
}
