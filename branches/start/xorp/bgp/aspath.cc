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

#ident "$XORP: xorp/bgp/aspath.cc,v 1.57 2002/12/09 23:34:17 rizzo Exp $"

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

AsSegment::AsSegment()
{
    _num_as_entries = 0; // number of as numbers in this segment
    _num_bytes_in_aspath = 0; // number of bytes used to encode the segment
    _data = NULL;
}

AsSegment::AsSegment(const AsSegment& asseg)
{
    debug_msg("copy constructor AsSegment called\n");
    assert(asseg._num_as_entries == asseg._aslist.size());
    _num_as_entries = asseg._num_as_entries;
    _num_bytes_in_aspath = asseg._num_bytes_in_aspath;
    if (asseg._data != NULL) {
	uint8_t *data = new uint8_t[_num_bytes_in_aspath];
	memcpy(data, asseg._data, _num_bytes_in_aspath);
	_data = data;
    } else {
	_data = NULL;
    }

    list <AsNum>::const_iterator iter = asseg._aslist.begin();
    while (iter != asseg._aslist.end()) {
	debug_msg("pushing %s onto segment\n", iter->str().c_str());
	_aslist.push_back(*iter);
	++iter;
    }
    // _count = asseg._count;
}

AsSegment::~AsSegment()
{
    debug_msg("destructor AsSegment called\n");
    if (_data != NULL) {
	delete[] _data;
    }
}

void
AsSegment::set_size(size_t l)
{
    debug_msg("Number of As numbers in the path set as %d\n", l);
    _num_as_entries = l;
}

void
AsSegment::add_as(const AsNum& asnum)
{
    debug_msg("Number of As entries %d with a size in bytes of %d\n",
	      _num_as_entries, _num_bytes_in_aspath);
    _aslist.push_back(asnum);
//     _num_bytes_in_aspath += 2;
    _num_as_entries++;
    debug_msg("Number of As entries %d with a size in bytes of %d\n",
	      _num_as_entries, _num_bytes_in_aspath);
}

void
AsSegment::prepend_as(const AsNum& asnum)
{
    debug_msg("Number of As entries %d with a size in bytes of %d\n",
	      _num_as_entries, _num_bytes_in_aspath);
    _aslist.push_front(asnum);
//     _num_bytes_in_aspath += 2;
    _num_as_entries++;
    debug_msg("Number of As entries %d with a size in bytes of %d\n",
	      _num_as_entries, _num_bytes_in_aspath);
}

const char *
AsSegment::get_data() const
{
    debug_msg("AsSegment get_data() called\n");
    encode();

    return (const char *)_data;
}


void
AsSegment::encode(ASPathSegType type) const
{
    debug_msg("AsSegment encode\n");
    // XXXX we could just use the value of data we've already got,
    // because it's almost certainly unchanged from when we receive it,
    // but for now we'll enforce re-encoding so we test the encoding
    // code.
    if (_data != NULL)
	delete[] _data;

    size_t data_size = 2; // number of header bytes
    list <AsNum>::const_iterator iter = _aslist.begin();
    while(iter != _aslist.end()) {
	if (iter->is_extended())
	    data_size += 4;
	else
	    data_size += 2;
	++iter;
    }
    assert(_aslist.size() == _num_as_entries);
    _num_bytes_in_aspath = data_size;
    debug_msg("data size = %d\n", data_size);
    uint8_t *data = new uint8_t[_num_bytes_in_aspath];

    size_t pos = 0;
    data[pos] = type;
    pos++;
    data[pos] = _num_as_entries;
    pos++;

    iter = _aslist.begin();
    while(iter != _aslist.end()) {
	if (iter->is_extended()) {
	    debug_msg("Encoding 32-bit As %d\n", iter->as_extended());
	    assert("4 byte as's not supported yet\n");
	    abort();
	    uint16_t temp = htonl(iter->as_extended());
	    memcpy(&data[pos],&temp, 4);
	    pos = pos + 4;
	} else {
	    uint16_t temp = htons(iter->as());
	    debug_msg("Encoding 16-bit As %d\n", iter->as());
	    memcpy(&data[pos],&temp, 2);
	    pos = pos + 2;
	}
	++iter;
    }

    _data = data;
}

bool
AsSegment::contains(const AsNum& as_num) const
{
    list <AsNum>::const_iterator iter = _aslist.begin();
    while (iter != _aslist.end()) {
	if (*iter == as_num) {
	    return true;
	}
	++iter;
    }
    return false;
}

const AsNum&
AsSegment::first_asnum() const
{
    if (dynamic_cast<const AsSet*>(this) != NULL) {
	// This shouldn't be possible.  The spec doesn't explicitly
	// prohibit passing someone an AS_PATH starting with an AS_SET,
	// but it doesn't make sense, and doesn't seem to be allowed by
	// the aggregation rules.
	XLOG_ERROR("Attempting to extract first AS Number from an AS Path \
that starts with an AS_SET not an AS_SEQUENCE\n");
    }
    assert(!_aslist.empty());
    return _aslist.front();
}

bool
AsSegment::operator==(const AsSegment& him) const
{
    if (_num_as_entries != him._num_as_entries)
	return false;
    list <AsNum>::const_reverse_iterator my_i = _aslist.rbegin();
    list <AsNum>::const_reverse_iterator his_i = him._aslist.rbegin();
    while (my_i != _aslist.rend()) {
	if (*my_i != *his_i) return false;
	my_i++;
	his_i++;
    }
    return true;
}

bool
AsSegment::operator<(const AsSegment& him) const
{
    int mysize = _num_as_entries;
    int hissize = him._num_as_entries;
    if (mysize < hissize)
	return true;
    if (mysize > hissize)
	return false;
    list <AsNum>::const_reverse_iterator my_i = _aslist.rbegin();
    list <AsNum>::const_reverse_iterator his_i = him._aslist.rbegin();
    while (my_i != _aslist.rend()) {
	if (*my_i < *his_i) return true;
	if (*his_i < *my_i) return false;
	my_i++;
	his_i++;
    }
    return false;
}

/* *************** AsSet ********************** */

AsSet::AsSet()
{
    _data = NULL;
}

AsSet::AsSet(const uint8_t* d, size_t l)
{
    debug_msg("constructor AsSet(uint8_t* d, size_t l) called\n");

    _num_bytes_in_aspath = l*sizeof(uint16_t) + 2; // data length in bytes
    set_size(l); // number of As entries

    uint8_t *data = new uint8_t[_num_bytes_in_aspath];
    memcpy(data, d, _num_bytes_in_aspath);
    _data = data;
    decode();
}

AsSet::AsSet(const AsSet& asset)
    : AsSegment(asset)
{
}

AsSet::~AsSet() {
    debug_msg("desstructor Asequence called\n");
}

void
AsSet::decode()
{
    const uint8_t* _temp = _data;
    _temp = _temp + 2*sizeof(uint8_t);
    size_t count = _num_as_entries;
    _num_as_entries = 0;

    for (u_int i = 0; i < count; i++) {
	add_as(ntohs((uint16_t &)(*_temp)));
	_temp += sizeof(uint16_t);
    }
}

void
AsSet::encode() const
{
    debug_msg("AsSet encode\n");
    ((const AsSegment*)this)->encode(AS_SET);
}

string
AsSet::str() const
{
    list <AsNum>::const_iterator iter = _aslist.begin();
    string s = "{";
    for (u_int i = 0; i<_num_as_entries; i++) {
	s = s.append(iter->str());
	if (i != _num_as_entries-1)
	    s = s.append(",");
	++iter;
    }
    s = s.append("}");
    return s;
}

/* *************** AsSequence ********************** */

AsSequence::AsSequence() {
    _data = NULL;
}

AsSequence::AsSequence(const uint8_t* d, size_t l)
{
    _num_as_entries = 0;
    debug_msg("constructor Asequence(uint8_t* d, uint8_t l (%d)) called\n", l);
    _num_bytes_in_aspath = l*sizeof(uint16_t) + 2; // data length in bits
    set_size(l); // number of As entries
    uint8_t *data = new uint8_t[_num_bytes_in_aspath];
    memcpy(data, d, _num_bytes_in_aspath);
    _data = data;
    decode();
}


AsSequence::AsSequence(const AsSequence& asseq)
    : AsSegment(asseq)
{
}

AsSequence::~AsSequence() {
    debug_msg("desstructor Asequence called\n");
}

void
AsSequence::decode()
{
    const uint8_t* temp = _data;
    temp = temp + 2*sizeof(uint8_t);
    debug_msg("decoding As Sequence size %d count %d \n",
	      _num_as_entries, _num_bytes_in_aspath);

    size_t count = _num_as_entries;
    _num_as_entries = 0;

    for (u_int i = 0; i < count; i++) {
	debug_msg("decoding As Sequence size %d count %d (%d)\n",
		  _num_as_entries, _num_bytes_in_aspath, i);
	add_as(ntohs((uint16_t &)(*temp)));
	temp += sizeof(uint16_t);
    }
}

void
AsSequence::encode() const
{
    debug_msg("AsSequence encode\n");
    ((const AsSegment*)this)->encode(AS_SEQUENCE);
}

string
AsSequence::str() const
{
    list <AsNum>::const_iterator iter = _aslist.begin();
    string s = "[";
    for (u_int i = 0; i<_num_as_entries; i++) {
	s = s.append(iter->str());
	if (i != _num_as_entries - 1) s = s.append(",");
	++iter;
    }
    s += "]";
    return s;
}


/* *************** AsPath *********************** */

AsPath::AsPath()
{
    debug_msg("AsPath() constructor called\n");
    _num_segments = 0;
    _as_path_length = 0;
    _byte_length = 0;
    _data = NULL;
}

inline uint16_t
get_asnum(string path, size_t& index, bool& found)
{
    debug_msg("path: %s index: %d found: %d\n", path.c_str(), index, found);

    found = false;

    if (index >= path.length())
	return 0;

    /*
    ** Look for a digit.
    */
    if (!isdigit(path[index]))
	xorp_throw(InvalidString,
		   c_format("Digit expected: <%c> %s",
			    path[index], path.c_str()));

    size_t start = index;

    /*
    ** Find the end of the number.
    */
    while(index < path.length() && isdigit(path[index]))
	index++;

    uint16_t num = atoi(path.substr(start, index).c_str());

    debug_msg("asnum = %d\n", num);

    found = true;
    return num;
}

/*
** Strings of the form:
** "1, 2,(3, 4, 5), 6,(7, 8), 9"
*/
AsPath::AsPath(const char *as_path) throw(InvalidString)
{
    debug_msg("AsPath(%s) constructor called\n", as_path);
    _num_segments = 0;
    _as_path_length = 0;
    _byte_length = 0;
    _data = NULL;

    string path = as_path;
    /*
    ** Remove all spaces from the string.
    */
    size_t pos;
    while(string::npos != (pos = path.find(" "))) {
	path = path.substr(0, pos) + path.substr(pos + 1);
    }

    size_t i = 0;
    while(i < path.length()) {
	bool sequence;
	if (isdigit(path[i]))
	    sequence = true;
	else if ('(' == path[i]) {
	    sequence = false;
	    i++;
	} else
	    xorp_throw(InvalidString,
		       c_format("Illegal character: <%c> %s",
				path[i], path.c_str()));

	AsSequence seq;
	AsSet set;
	bool found;
	do {
	    uint16_t num = get_asnum(path, i, found);
	    if (found) {
		if (sequence)
		    seq.add_as(AsNum(num));
		else
		    set.add_as(AsNum(num));
		/*
		** Eat one "," if present.
		*/
		if (i < path.length() && ',' == path[i])
		    i++;
		if (i < path.length() && ('(' == path[i] || ')' == path[i]))
		    found = false;
	    }
	} while(found);
	if (sequence)
	    add_segment(seq);
	else {
	    add_segment(set);
	    if (i < path.length() && ')' == path[i]) {
		i++;
		if (i < path.length() && ',' == path[i])
		    i++;
	    } else
		xorp_throw(InvalidString,
			   c_format("Expected: ')' got <%c> %s",
				    path[i], path.c_str()));
	}
    }
}

AsPath::AsPath(const AsPath &as_path)
{
    debug_msg("AsPath() copy construtor called\n");
    _num_segments = as_path._num_segments;
    _as_path_length = as_path._as_path_length;
    list <AsSegment *>::const_iterator iter;
    iter = as_path._segments.begin();
    const AsSequence *asseq;
    const AsSet *asset;
    while (iter != as_path._segments.end()) {
	asseq = dynamic_cast<const AsSequence*>(*iter);
	if (asseq != NULL) {
	    _segments.push_back(new AsSequence(*asseq));
	    ++iter;
	    continue;
	}
	asset = dynamic_cast<const AsSet*>(*iter);
	if (asset != NULL) {
	    _segments.push_back(new AsSet(*asset));
	    ++iter;
	    continue;
	}
	XLOG_FATAL("Got an AsSegment that isn't a Set or a Sequence!");
    }
    // we shouldn't need to copy the buffer
    _data = NULL;
    _byte_length = 0;
}

AsPath::~AsPath()
{
    debug_msg("AsPath destructor called\n");
    list <AsSegment *>::iterator iter = _segments.begin();
    if (_data != NULL)
	delete[] _data;
    while (iter != _segments.end()) {
	delete *iter;
	++iter;
    }
}

void
AsPath::add_segment(const AsSegment& s)
{
    debug_msg("Adding As Segment\n");
    const AsSequence *asseq;
    const AsSet *asset;
    asseq = dynamic_cast<const AsSequence*>(&s);
    if (asseq != NULL) {
	_segments.push_back(new AsSequence(*asseq));
    } else {
	asset = dynamic_cast<const AsSet*>(&s);
	if (asset != NULL) {
	    _segments.push_back(new AsSet(*asset));
	} else {
	    XLOG_FATAL("Got AsSegment that isn't a Set or a Sequence!");
	}
    }
    _num_segments++;

    increase_as_path_length(s.get_as_path_length());
    debug_msg("End of add_segment\n");
}

void
AsPath::increase_as_path_length(size_t n)
{
    _as_path_length = _as_path_length + n;
    debug_msg("As Path length increased by %d to be %d\n", n, _as_path_length);
}

void
AsPath::decrease_as_path_length(size_t n)
{
    _as_path_length = _as_path_length - n;
    debug_msg("As Path length decreased by %d to be %d\n", n, _as_path_length);
}

string
AsPath::str() const
{
    string s;
    list <AsSegment*>::const_iterator iter = _segments.begin();
    s = "As Path :";
    while(iter != _segments.end()) {
	s = s.append(" ");
	s = s.append((*iter)->str());
	++iter;
    }
    return s;
}

bool
AsPath::contains(const AsNum& as_num) const
{
    list <AsSegment*>::const_iterator iter = _segments.begin();
    while(iter != _segments.end()) {
	if ((*iter)->contains(as_num))
	    return true;
	++iter;
    }
    return false;
}

const AsNum&
AsPath::first_asnum() const
{
    assert(!_segments.empty());
    return _segments.front()->first_asnum();
}

const AsSegment*
AsPath::get_segment(size_t n) const
{
    list <AsSegment*>::const_iterator iter = _segments.begin();
    if (n < _num_segments) {
	for (u_int i = 0; i<n; i++) {
	    ++iter;
	}
	return (*iter);
    }
    assert("Segment doesn't exist.\n");
    return NULL;
}

void
AsPath::encode() const
{
    if (_data != NULL)
	delete[] _data;

    size_t length = 0;
    size_t position = 0;
    const char *bufs[_segments.size()];
    int lengths[_segments.size()];

    assert(_num_segments == _segments.size());
    list <AsSegment*>::const_iterator iter = _segments.begin();
    int i = 0;
    while(iter != _segments.end()) {
	// Note: need to call get_data before get_as_byte_size
	bufs[i] = (*iter)->get_data();
	lengths[i] = (*iter)->get_as_byte_size();
	length = length + lengths[i];
	++iter;
	++i;
    }

    uint8_t* data = new uint8_t[length];

    iter = _segments.begin();
    i = 0;
    while(iter != _segments.end()) {
	memcpy(&data[position], bufs[i], lengths[i]);
	position = position + lengths[i];
	++iter;
	++i;
    }

    _byte_length = length;
    _data = data;
}

const char*
AsPath::get_data() const
{
    encode();

    return (const char*)_data;
}

/* add_As_in_sequence is what we call when we're As prepending.  We
   add the As number to the begining of the As sequence that starts
   the As path, or if the As path starts with an As set, then we add a
   new As sequence to the start of the As path */

void
AsPath::add_AS_in_sequence(const AsNum &asn)
{
    if (_segments.empty() || (_segments.front()->type() == AS_SET)) {
	AsSequence *seq = new AsSequence();
	seq->add_as(asn);
	_segments.push_front(seq);
	_num_segments++;
    } else {
	AsSequence *seq = dynamic_cast<AsSequence*>(_segments.front());
	assert(seq != NULL);
	seq->prepend_as(asn);
    }
    _as_path_length++;
}

bool
AsPath::operator==(const AsPath& him) const
{
    if (_num_segments != him._num_segments)
	return false;
    list <AsSegment*>::const_iterator my_i = _segments.begin();
    list <AsSegment*>::const_iterator his_i = him._segments.begin();
    while (my_i != _segments.end()) {
	if (**my_i != **his_i) return false;
	my_i++;
	his_i++;
    }
    return true;
}

bool
AsPath::operator<(const AsPath& him) const
{
    int mysize = _num_segments;
    int hissize = him._num_segments;
    if (mysize < hissize)
	return true;
    if (mysize > hissize)
	return false;
    list <AsSegment*>::const_iterator my_i = _segments.begin();
    list <AsSegment*>::const_iterator his_i = him._segments.begin();
    while (my_i != _segments.end()) {
	if (**my_i < **his_i) return true;
	if (**his_i < **my_i) return false;
	my_i++;
	his_i++;
    }
    return false;
}
