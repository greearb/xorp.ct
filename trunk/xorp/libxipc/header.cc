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

#ident "$XORP: xorp/libxipc/header.cc,v 1.3 2003/03/10 23:20:24 hodson Exp $"

#include "libxorp/xorp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xrl_module.h"
//#include "libxorp/debug.h"
#include "header.hh"

static const string HEADER_SEP(":\t");
static const string HEADER_EOL("\r\n");

bool
HeaderWriter::name_valid(const string &name)
{
    return (name.find(HEADER_SEP) == ~0U);
}

HeaderWriter&
HeaderWriter::add(const string& name,
		  const string& value) throw (InvalidName) {
    if (name_valid(name) == false) throw InvalidName();

    _list.push_back(Node(name, value));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name, int32_t value) throw (InvalidName)
{
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%d", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name, uint32_t value) throw (InvalidName)
{
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%u", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name,
		  const double& value) throw (InvalidName) {
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%f", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

string
HeaderWriter::str() const
{
    list<Node>::const_iterator ci;

    string r;
    for (ci = _list.begin(); ci != _list.end(); ci++) {
	r += ci->key + HEADER_SEP + ci->value + HEADER_EOL;
    }
    r += HEADER_EOL;
    return r;
}

#if 0
static string::size_type
skip(const string& buf, const string& skip, string::size_type pos)
{
    if (buf.size() - pos < skip.size())
	return string::npos;

    string::size_t i = 0;
    while (i != skip.size()) {
	if (buf[pos + i] != skip[i])
	    return string::npos;
	i++;
    }
    return i;
}
#endif /* 0 */

HeaderReader::HeaderReader(const string& serialized) throw (InvalidString)
    : _bytes_consumed(0)
{
    if (serialized.find(HEADER_EOL + HEADER_EOL) == ~0U)
	throw InvalidString();

    string::size_type start = 0;
    while (start < serialized.size()) {

	// Extract key
	string::size_type sep = serialized.find(HEADER_SEP, start);
	if (sep == string::npos) break;

	string key(serialized, start, sep - start);

	// Skip key:value separator
	start = sep + HEADER_SEP.size();

	// Find to end of value
	sep = serialized.find(HEADER_EOL, start);
	if (sep == string::npos) break;

	string value(serialized, start, sep - start);

	// Skip over end of line
	start = sep + HEADER_EOL.size();

	// Insert <key,value> into map
	_map[key] = value;

	if (string(serialized, start, HEADER_EOL.size()) == HEADER_EOL) {
	    _bytes_consumed = start + HEADER_EOL.size();
	    return;
	} // else start == start of new entry, we go around again
    }

    throw InvalidString();
}

HeaderReader&
HeaderReader::get(const string& name, string& value) throw (NotFound)
{
    CMI m = _map.find(name);
    if (m == _map.end())
	throw NotFound();
    value = m->second;
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, int32_t& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtol(tmp.c_str(), 0, 10);
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, uint32_t& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtoul(tmp.c_str(), 0, 10);
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, double& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtod(tmp.c_str(), 0);
    return *this;
}

