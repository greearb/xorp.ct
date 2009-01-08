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

#ident "$XORP: xorp/bgp/aspath.cc,v 1.44 2008/10/02 21:56:14 bms Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

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

/* *************** ASSegment ********************** */

/**
 * Convert the external representation into the internal one. 
 * _type is d[0], _entries is d[1], entries follow.
 */
void
ASSegment::decode(const uint8_t *d) throw(CorruptMessage)
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
ASSegment::encode(size_t &len, uint8_t *data) const
{
    debug_msg("AsSegment encode\n");
    XLOG_ASSERT(_aslist.size() <= 255);

    size_t i = wire_size();
    const_iterator as;

    if (data == 0)
	data = new uint8_t[i];
    else
	XLOG_ASSERT(len >= i);
    len = i;

    data[0] = _type;
    data[1] = _aslist.size();

    for (i = 2, as = _aslist.begin(); as != _aslist.end(); i += 2, ++as) {
	debug_msg("Encoding 16-bit As %d\n", as->as());
	as->copy_out(data + i);
    }

    return data;
}

const AsNum&
ASSegment::first_asnum() const
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
 * prints ASSegments as 
 *
 *  AS_SEQUENCE:         [comma-separated-asn-list]  
 *  AS_SET:              {comma-separated-asn-list}
 *  AS_CONFED_SEQUENCE:  (comma-separated-asn-list)
 *  AS_CONFED_SET:       <comma-separated-asn-list> 
 */
string
ASSegment::str() const
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

    for (u_int i = 0; i<_aslist.size(); i++, ++iter) {
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
 * prints ASSegments as 
 *
 *  AS_SEQUENCE:         comma-separated-asn-list
 *  AS_SET:              {comma-separated-asn-list}
 *  AS_CONFED_SEQUENCE:  (comma-separated-asn-list)
 *  AS_CONFED_SET:       <comma-separated-asn-list> 
 */
string
ASSegment::short_str() const
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

    for (u_int i = 0; i<_aslist.size(); i++, ++iter) {
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
ASSegment::operator==(const ASSegment& him) const
{
    return (_aslist == him._aslist);
}

/**
 * Compares internal representations for <.
 */
bool
ASSegment::operator<(const ASSegment& him) const
{
    int mysize = _aslist.size();
    int hissize = him._aslist.size();
    if (mysize < hissize)
	return true;
    if (mysize > hissize)
	return false;
    return (_aslist < him._aslist);
}

// XXX isn't this the same format as on the wire ???

size_t
ASSegment::encode_for_mib(uint8_t* buf, size_t buf_size) const
{
    //See RFC 1657, Page 15 for the encoding
    XLOG_ASSERT(buf_size >= (2 + _aslist.size() * 2));
    uint8_t *p = buf;
    *p = (uint8_t)_type;  p++;
    *p = (uint8_t)_aslist.size(); p++;
    const_iterator i;
    for (i = _aslist.begin(); i!= _aslist.end(); i++, p += 2)
	i->copy_out(p);

    return (2 + _aslist.size() * 2);
}

bool
ASSegment::two_byte_compatible() const
{
    const_iterator i;
    for (i = _aslist.begin(); i != _aslist.end(); i++) {
	if (i->extended())
	    return false;
    }
    return true;
}

/* *************** AS4Segment ***************** */


/**
 * Convert the external representation into the internal one. This is
 * used when decoding a AS4_PATH attribute.
 *
 * _type is d[0], _entries is d[1], entries follow.
 */
void
AS4Segment::decode(const uint8_t *d) throw(CorruptMessage)
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
 * Convert from internal to external AS4_PATH representation.
 */
const uint8_t *
AS4Segment::encode(size_t &len, uint8_t *data) const
{
    debug_msg("AS4Segment encode\n");
    XLOG_ASSERT(_aslist.size() <= 255);

    size_t i = wire_size();
    const_iterator as;

    if (data == 0)
	data = new uint8_t[i];
    else
	XLOG_ASSERT(len >= i);
    len = i;

    data[0] = _type;
    data[1] = _aslist.size();

    for (i = 2, as = _aslist.begin(); as != _aslist.end(); i += 4, ++as) {
	debug_msg("Encoding 4-byte As %d\n", as->as());
	uint32_t as_num = htonl(as->as4());
	memcpy(data + i, &as_num, 4);
    }

    return data;
}


/* *************** ASPath *********************** */

/**
 * ASPath constructor by parsing strings.  Input strings 
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

ASPath::ASPath(const char *as_path) throw(InvalidString)
{
    debug_msg("ASPath(%s) constructor called\n", as_path);
    _num_segments = 0;
    _path_len = 0;

    // make a copy removing all spaces from the string.

    string path = as_path;
    size_t pos;
    while(string::npos != (pos = path.find(" "))) {
	path = path.substr(0, pos) + path.substr(pos + 1);
    }

    ASSegment seg;
    for (size_t i = 0; i < path.length(); i++) {
	char c = path[i];

        debug_msg("check <%c> at pos %u\n", c, XORP_UINT_CAST(i));
	if (xorp_isdigit(c)) {
	    size_t start = i;

	    if (seg.type() == AS_NONE)	// possibly start new segment
		seg.set_type(AS_SEQUENCE);

	    while (i < path.length() && (xorp_isdigit(path[i]) || path[i]=='.')) {
		i++;
	    }
        
	    string asnum = path.substr(start, i-start);
	    i--; // back to last valid character
	    debug_msg("asnum = %s\n", asnum.c_str());
	    seg.add_as(AsNum(asnum));
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
		       c_format("Unterminated ASSet: %s", path.c_str()));
    debug_msg("end of ASPath()\n");
}

/**
 * populate an ASPath from the received data representation
 */
void
ASPath::decode(const uint8_t *d, size_t l) throw(CorruptMessage)
{
    _num_segments = 0;
    _path_len = 0;
    while (l > 0) {		// grab segments
	size_t len = 2 + d[1]*2;	// XXX length in bytes for 16bit AS's
	if (len > l)
	    xorp_throw(CorruptMessage,
		       c_format("Bad ASpath (len) %u > (l) %u\n",
				XORP_UINT_CAST(len), XORP_UINT_CAST(l)),
		       UPDATEMSGERR, MALASPATH);

	ASSegment s(d);
	add_segment(s);
	d += len;
	l -= len;
    }
}

/**
 * construct a new aggregate ASPath from two ASPaths
 */
ASPath::ASPath(const ASPath &asp1, const ASPath &asp2)
{
    _num_segments = 0;
    _path_len = 0;

    size_t curseg;
    size_t matchelem = 0;
    bool fullmatch = true;

    for (curseg = 0;
	curseg < asp1.num_segments() && curseg < asp2.num_segments();
	curseg++) {
	if (asp1.segment(curseg).type() != asp2.segment(curseg).type())
	    break;

	size_t minseglen = min(asp1.segment(curseg).path_length(),
			       asp2.segment(curseg).path_length());

	for (matchelem = 0; matchelem < minseglen; matchelem++)
	    if (asp1.segment(curseg).as_num(matchelem) !=
		asp2.segment(curseg).as_num(matchelem))
		break;

	if (matchelem) {
	    ASSegment newseg(asp1.segment(curseg).type());
	    for (size_t elem = 0; elem < matchelem; elem++)
		newseg.add_as(asp1.segment(curseg).as_num(elem));
	    this->add_segment(newseg);
	}

	if (matchelem < asp1.segment(curseg).path_length() ||
	    matchelem < asp2.segment(curseg).path_length()) {
	    fullmatch = false;
	    break;
	}
    }

    if (!fullmatch) {
	ASSegment new_asset(AS_SET);
	size_t startelem = matchelem;
	for (size_t curseg1 = curseg; curseg1 < asp1.num_segments();
	     curseg1++) {
	    for (size_t elem = startelem;
		 elem < asp1.segment(curseg1).path_length(); elem++) {
		const class AsNum asn = asp1.segment(curseg).as_num(elem);
		if (!new_asset.contains(asn))
		    new_asset.add_as(asn);
		}
	    startelem = 0;
	}
	startelem = matchelem;
	for (size_t curseg2 = curseg; curseg2 < asp2.num_segments();
	     curseg2++) {
	    for (size_t elem = startelem;
		 elem < asp2.segment(curseg2).path_length(); elem++) {
		const class AsNum asn = asp2.segment(curseg).as_num(elem);
		if (!new_asset.contains(asn))
		    new_asset.add_as(asn);
		}
	    startelem = 0;
	}
	this->add_segment(new_asset);
    }
}

void
ASPath::add_segment(const ASSegment& s)
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
ASPath::prepend_segment(const ASSegment& s)
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
ASPath::str() const
{
    string s = "ASPath:";
    const_iterator iter = _segments.begin();
    while(iter != _segments.end()) {
	s.append(" ");
	s.append((*iter).str());
	++iter;
    }
    return s;
}

string
ASPath::short_str() const
{
    string s;
    const_iterator iter = _segments.begin();
    while(iter != _segments.end()) {
	if (iter != _segments.begin())
	    s.append(" ");
	s.append((*iter).short_str());
	++iter;
    }
    return s;
}

size_t
ASPath::wire_size() const
{
    size_t l = 0;
    const_iterator iter = _segments.begin();

    for (; iter != _segments.end(); ++iter)
	l += iter->wire_size();
    return l;
}

const uint8_t *
ASPath::encode(size_t &len, uint8_t *buf) const
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
ASPath::prepend_as(const AsNum &asn)
{
    if (_segments.empty() || _segments.front().type() == AS_SET) {
	ASSegment seg = ASSegment(AS_SEQUENCE);

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
ASPath::prepend_confed_as(const AsNum &asn)
{
    if (_segments.empty() || _segments.front().type() == AS_SET 
	|| _segments.front().type() == AS_SEQUENCE) {
	ASSegment seg = ASSegment(AS_CONFED_SEQUENCE);

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
ASPath::remove_confed_segments()
{
        debug_msg("Deleting all CONFED Segments\n");
	const_iterator iter = _segments.begin();
	const_iterator next_iter;
	while (iter != _segments.end()) {
	    next_iter = iter;
	    ++next_iter;
	    if ((*iter).type() == AS_CONFED_SEQUENCE 
		|| (*iter).type() == AS_CONFED_SET) {
		_path_len--;
		_num_segments--;
		_segments.remove((*iter)); 
	    } 
	    iter = next_iter;
	}
}

bool
ASPath::contains_confed_segments() const
{
    for (const_iterator i = _segments.begin(); i != _segments.end(); i++) {
	ASPathSegType type = (*i).type();
	if (AS_CONFED_SEQUENCE == type || AS_CONFED_SET == type)
	    return true;
    }

    return false;
}

ASPath&
ASPath::operator=(const ASPath& him)
{
    // clear out my list
    while (!_segments.empty()) {
	_segments.pop_front();
    }

    // copy in from his
    const_iterator i;
    for (i = him._segments.begin() ;i != him._segments.end(); i++) {
	_segments.push_back(*i);
    }
    return *this;
}

bool
ASPath::operator==(const ASPath& him) const
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
ASPath::operator<(const ASPath& him) const
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
ASPath::encode_for_mib(vector<uint8_t>& encode_buf) const
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
ASPath::two_byte_compatible() const
{
    const_iterator i = _segments.begin();
    for(i = _segments.begin(); i != _segments.end(); i++) {
	if ( !(i->two_byte_compatible()) )
	    return false;
    }
    return true;
}

void 
ASPath::merge_as4_path(AS4Path& as4_path)
{
    as4_path.cross_validate(*this);
    (*this) = as4_path;
}

#if 0
AS4Path::AS4Path(const uint8_t* d, size_t len, const ASPath& as_path)
     throw(CorruptMessage)
{
    decode(d, len);
    cross_validate(as_path);
}
#endif


AS4Path::AS4Path(const uint8_t* d, size_t len)
     throw(CorruptMessage)
{
    decode(d, len);
}

/**
 * populate a ASPath from the received data representation from a
 * AS4_PATH attribute.
 */
void
AS4Path::decode(const uint8_t *d, size_t l) throw(CorruptMessage)
{
    _num_segments = 0;
    _path_len = 0;
    while (l > 0) {		// grab segments
	size_t len = 2 + d[1]*4;	// XXX length in bytes for 32bit AS's
	XLOG_ASSERT(len <= l);

	AS4Segment s(d);
	add_segment(s);
	d += len;
	l -= len;
    }
}

/**
 * the AS_PATH attribute may have had new ASes added since the
 * AS4_PATH was last updated.  We need to copy across anything that
 * is missing 
 */

void AS4Path::cross_validate(const ASPath& as_path)
{
    debug_msg("cross validate\n%s\n%s\n", str().c_str(), as_path.str().c_str());
	      
    if (as_path.path_length() < path_length() ) {
	debug_msg("as_path.path_length() < path_length()\n");
	// This is illegal.  The spec says to ignore the AS4_PATH
	// attribute and use the data from the AS_PATH attribute throw
	// away the data we had.
	while (!_segments.empty()) {
	    _segments.pop_front();
	}
	// copy in from the AS_PATH version 
	for (uint32_t i = 0; i < as_path.num_segments(); i++) {
	    debug_msg("adding %u %s\n", i, as_path.segment(i).str().c_str()); 
	    add_segment(as_path.segment(i));
	}
	return;
    }

    // The AS_PATH attribute may contain ASes that were added by an
    // old BGP speaker that are missing from the AS4_PATH attribute.

    if (as_path.path_length() > path_length()) {
	if (as_path.num_segments() < num_segments()) {
	    // Now we're in trouble!  The AS_PATH has more ASes but
	    // fewer segments than the AS4_PATH.
	    do_patchup(as_path);
	    return;
	}

	// The AS_PATH has at least as many segments as the
	// AS4_PATH find where they differ, and copy across.
	for (uint32_t i = 1; i <= num_segments(); i++) {
	    const ASSegment *old_seg;
	    ASSegment *new_seg;
	    old_seg = &(as_path.segment(as_path.num_segments() - i));
	    new_seg = const_cast<ASSegment*>(&(segment(num_segments() - i)));
	    printf("old seg: %s\n", old_seg->str().c_str());
	    printf("new seg: %s\n", new_seg->str().c_str());
	    if (old_seg->path_length() == new_seg->path_length()) 
		continue;
	    if (old_seg->path_length() < new_seg->path_length()) {
		// ooops - WTF happened here
		debug_msg("do patchup\n");
		do_patchup(as_path);
	    }
	    if (old_seg->path_length() > new_seg->path_length()) {
		// found a segment that needs data copying over
		debug_msg("pad segment\n");
		printf("new_seg type: %u\n", new_seg->type());
		pad_segment(*old_seg, *new_seg);
	    }
	}

	debug_msg("after patching: \n");
	debug_msg("old as_path (len %u): %s\n", (uint32_t)as_path.path_length(), as_path.str().c_str());
	debug_msg("new as_path (len %u): %s\n", (uint32_t)path_length(), str().c_str());
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

void AS4Path::pad_segment(const ASSegment& old_seg, ASSegment& new_seg) 
{
    debug_msg("pad: new type: %u\n", new_seg.type());
    if (new_seg.type() == AS_SET) {
	debug_msg("new == AS_SET\n");
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
	debug_msg("old == AS_SET\n");
	// The old segment was an AS_SET, but the new isn't.  Looks
	// like some old BGP speaker did some form of aggregation.
	// Convert the new one to an AS_SET too.
	new_seg.set_type(AS_SET);
	pad_segment(old_seg, new_seg);
	return;
    }

    debug_msg("both are sequences\n");
    // They're both AS_SEQUENCES, so we need to preserve sequence.
    // First validate that new_seg is a sub-sequence of old_seg
    for (uint32_t i = 1; i <= new_seg.as_size(); i++) {
	const AsNum *old_asn = &(old_seg.as_num(old_seg.as_size()-i));
	const AsNum *new_asn = &(new_seg.as_num(new_seg.as_size()-i));
	debug_msg("old_asn: %s, new_asn: %s\n", old_asn->str().c_str(), new_asn->str().c_str());
	// do a compare on the 16-bit compat versions of the numbers
	if (old_asn->as() != new_asn->as()) {
	    debug_msg("Mismatch\n");
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
	_path_len++;
    }
    return;
}

void AS4Path::do_patchup(const ASPath& as_path)
{
    // we get here when the cross validation fails, and we need to do
    // something that isn't actually illegal.

    // There's no good way to deal with this, but simplest is to pad
    // the AS4_PATH with an AS_SET containing anything that wasn't
    // previously in the AS4_PATH.  This at least should prevent
    // loops forming, but it's really ugly.

    ASSegment new_set(AS_SET);
    for (uint32_t i = 0; i < as_path.path_length(); i++) {
	const ASSegment *s = &(as_path.segment(i));
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
AS4Path::encode(size_t &len, uint8_t *buf) const
{
    debug_msg("AS4Path::encode\n");
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
	const AS4Segment *new_seg = (const AS4Segment*)(&(*i));
	l = new_seg->wire_size();
	new_seg->encode(l, buf + pos);
	pos += l;
    }
    return buf;
}


size_t
AS4Path::wire_size() const
{
    size_t l = 0;
    const_iterator i;

    for (i = _segments.begin(); i != _segments.end(); ++i) {
	const AS4Segment *new_seg = (const AS4Segment*)(&(*i));
	l += new_seg->wire_size();
    }
    return l;
}
